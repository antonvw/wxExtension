////////////////////////////////////////////////////////////////////////////////
// Name:      ctags.cpp
// Purpose:   Implementation of class wex::ctags
//            https://github.com/universal-ctags/ctags
// Author:    Anton van Wezenbeek
// Copyright: (c) 2020 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <vector>

#include <ctags/libreadtags/readtags.h>
#include <wex/config.h>
#include <wex/core.h>
#include <wex/ctags.h>
#include <wex/ex.h>
#include <wex/frd.h>
#include <wex/log.h>
#include <wex/managed-frame.h>
#include <wex/path.h>
#include <wex/stc.h>
#include <wx/app.h>
#include <wx/artprov.h>
#include <wx/choicdlg.h>

namespace wex
{
  enum image_access_t
  {
    IMAGE_NONE,
    IMAGE_PUBLIC,
    IMAGE_PROTECTED,
    IMAGE_PRIVATE
  };

  bool equal(
    const tagEntry&    entry,
    const std::string& text,
    const std::string& field)
  {
    if (const char* valuep = tagsField(&entry, field.c_str());
        valuep != nullptr)
    {
      if (std::string value(valuep); value.find("::") != std::string::npos)
      {
        value = wex::after(value, ':', false);
        return text == value;
      }
    }

    return false;
  }

  const std::string
  filtered(const tagEntry& entry, const wex::ctags_entry& filter)
  {
    if (!filter.is_active())
      return entry.name;

    if (!filter.kind().empty())
    {
      if (
        entry.kind == nullptr || strcmp(filter.kind().c_str(), entry.kind) != 0)
      {
        return std::string();
      }
    }

    if (!filter.access().empty() && !equal(entry, filter.access(), "access"))
    {
      return std::string();
    }

    if (
      !filter.class_name().empty() &&
      !equal(entry, filter.class_name(), "class"))
    {
      return std::string();
    }

    if (
      !filter.signature().empty() &&
      !equal(entry, filter.signature(), "signature"))
    {
      return std::string();
    }

    return entry.name;
  }

  frame* get_frame()
  {
    return dynamic_cast<managed_frame*>(wxTheApp->GetTopWindow());
  }

  void set_image(const tagEntry& entry, wex::image_access_t& image)
  {
    if (const char* value = tagsField(&entry, "access"); value != nullptr)
    {
      if (strcmp(value, "public") == 0)
      {
        image = wex::IMAGE_PUBLIC;
      }
      else if (strcmp(value, "protected") == 0)
      {
        image = wex::IMAGE_PROTECTED;
      }
      else if (strcmp(value, "private") == 0)
      {
        image = wex::IMAGE_PRIVATE;
      }
    }
  }

  std::string skip_const(const std::string& text)
  {
    if (text.empty())
      return std::string();
    else if (std::vector<std::string> v;
             wex::match("(.*) *const$", text, v) == 1)
      return v[0];
    else
      return text;
  }

  const std::string tag_name(const path& p)
  {
    return config(_("stc.vi tag fullpath")).get(false) ? p.string() :
                                                         p.fullname();
  };

  // Support class.
  class ctags_info
  {
  public:
    // Constructor.
    explicit ctags_info(const tagEntry& entry)
      : m_line_number(entry.address.lineNumber)
      , m_path(entry.file)
      , m_pattern(
          entry.address.pattern != nullptr ?
            // prepend colon to force ex command
            ":" + std::string(entry.address.pattern) :
            std::string())
    {
      // the pattern generated by
      // ctags mixes regex with non regex....
      replace_all(m_pattern, "*", "\\*");
      replace_all(m_pattern, "(", "\\(");
      replace_all(m_pattern, ")", "\\)");
    };

    // Returns name, being fullpath or path name depending on
    // config settings.
    const std::string name() const { return tag_name(m_path); };

    // Opens file in specified frame.
    auto open_file(frame* frame) const
    {
      return frame->open_file(
        m_path,
        data::control().line(m_line_number).command(m_pattern));
    }

  private:
    const path  m_path;
    const int   m_line_number;
    std::string m_pattern;
  };
}; // namespace wex

std::map<std::string, wex::ctags_info>           wex::ctags::m_matches;
std::map<std::string, wex::ctags_info>::iterator wex::ctags::m_iterator;

wex::ctags::ctags(wex::ex* ex, bool open_file)
  : m_ex(ex)
{
  if (open_file)
  {
    open();
  }
}

const std::string
wex::ctags::auto_complete(const std::string& text, const ctags_entry& filter)
{
  if (m_file == nullptr)
  {
    return std::string();
  }

  tagEntry entry;

  if (text.empty())
  {
    if (tagsFirst(m_file, &entry) == TagFailure)
    {
      return std::string();
    }
  }
  else if (
    tagsFind(
      m_file,
      &entry,
      text.c_str(),
      TAG_PARTIALMATCH | TAG_OBSERVECASE) == TagFailure)
  {
    return std::string();
  }

  if (!m_is_prepared)
  {
    auto_complete_prepare();
  }

  std::string s, prev_tag;

  const int min_size{3}, max{100};
  int       count{0};
  tagResult result = TagSuccess;

  do
  {
    wex::image_access_t image = IMAGE_NONE;

    if (const auto tag(filtered(entry, filter));
        tag.size() > min_size && tag != prev_tag)
    {
      if (!s.empty())
      {
        s.push_back(m_separator);
      }

      s.append(tag);
      count++;

      set_image(entry, image);

      if (filter.kind() == "f")
      {
        const char* value = tagsField(&entry, "signature");

        if (value != nullptr)
        {
          s.append(skip_const(value));
        }
      }

      s.append(
        image != IMAGE_NONE ? "?" + std::to_string(image) : std::string());

      prev_tag = tag;
    }

    result =
      (text.empty() ? tagsNext(m_file, &entry) : tagsFindNext(m_file, &entry));
  } while (result == TagSuccess && count < max);

  log::trace("ctags::auto_complete count") << count;

  return s;
}

void wex::ctags::auto_complete_prepare()
{
  m_ex->get_stc()->AutoCompSetIgnoreCase(false);
  m_ex->get_stc()->AutoCompSetAutoHide(false);

  m_ex->get_stc()->RegisterImage(
    IMAGE_PUBLIC,
    wxArtProvider::GetBitmap(wxART_PLUS));
  m_ex->get_stc()->RegisterImage(
    IMAGE_PROTECTED,
    wxArtProvider::GetBitmap(wxART_MINUS));
  m_ex->get_stc()->RegisterImage(
    IMAGE_PRIVATE,
    wxArtProvider::GetBitmap(wxART_TICK_MARK));

  m_is_prepared = true;
}

bool wex::ctags::close()
{
  if (m_file == nullptr || tagsClose(m_file) == TagFailure)
  {
    return false;
  }

  m_file = nullptr;

  return true;
}

bool wex::ctags::do_open(const std::string& path)
{
  if (tagFileInfo info; (m_file = tagsOpen(path.c_str(), &info)) != nullptr)
  {
    log::trace("ctags file") << path;
    return true;
  }

  return false;
}

bool wex::ctags::find(const std::string& tag, ex* ex)
{
  if (m_file == nullptr)
  {
    return false;
  }

  if (tag.empty())
  {
    log::trace("ctags::find empty tag") << m_matches.size();
    return next();
  }

  tagEntry entry;

  if (tagsFind(m_file, &entry, tag.c_str(), TAG_FULLMATCH) == TagFailure)
  {
    log::status("Tag not found") << tag;
    return false;
  }

  m_matches.clear();

  do
  {
    if (const ctags_info ct(entry);
        ex == nullptr || (ct.name() != tag_name(ex->get_stc()->get_filename())))
    {
      m_matches.insert({ct.name(), ct});
    }
  } while (tagsFindNext(m_file, &entry) == TagSuccess);

  m_iterator = m_matches.begin();

  log::trace("ctags::find matches") << m_matches.size();

  switch (m_matches.size())
  {
    case 0:
      if (ex != nullptr)
      {
        ex->get_stc()->find_next(tag);
      }
      break;

    case 1:
      m_matches.begin()->second.open_file(get_frame());
      break;

    default:
    {
      wxArrayString as;

      for (const auto& it : m_matches)
        as.Add(it.second.name());

      wxMultiChoiceDialog dialog(
        get_frame(),
        _("Input") + ":",
        _("Select File"),
        as);

      if (dialog.ShowModal() != wxID_OK)
        return false;

      for (const auto& sel : dialog.GetSelections())
      {
        m_iterator = m_matches.find(as[sel]);
        m_iterator->second.open_file(get_frame());
      }
    }
  }

  find_replace_data::get()->set_find_string(tag);

  return true;
}

bool master(const tagEntry& entry)
{
  return entry.kind != nullptr &&
         ((strcmp(entry.kind, "c") == 0) || (strcmp(entry.kind, "e") == 0) ||
          (strcmp(entry.kind, "m") == 0));
}

bool wex::ctags::find(const std::string& tag, wex::ctags_entry& filter)
{
  if (m_file == nullptr)
  {
    return false;
  }

  tagEntry entry;

  // Find first entry. This entry determines which kind of
  // filter will be set.
  if (tagsFind(m_file, &entry, tag.c_str(), TAG_FULLMATCH) == TagFailure)
  {
    return false;
  }

  filter.clear();

  do
  {
    // If this is not a master entry find next.
    if (master(entry))
    {
      // Set filter for member functions for this member or class.
      if (strcmp(entry.kind, "m") == 0)
      {
        if (const char* value = tagsField(&entry, "typeref"); value != nullptr)
        {
          filter.kind("f").class_name(wex::before(wex::after(value, ':'), ' '));
        }
      }
      else
      {
        filter.kind("f").class_name(entry.name);
      }

      return true;
    }
  } while (!master(entry) && tagsFindNext(m_file, &entry) == TagSuccess);

  return false;
}

bool wex::ctags::next()
{
  if (m_matches.size() <= 1)
  {
    log::trace("ctags::next no next match") << m_matches.size();
    return false;
  }

  if (++m_iterator == m_matches.end())
  {
    m_iterator = m_matches.begin();
  }

  m_iterator->second.open_file(get_frame());

  return true;
}

void wex::ctags::open(const std::string& filename)
{
  if (m_file != nullptr)
  {
    return;
  }

  m_iterator = m_matches.begin();

  if (filename != DEFAULT_TAGFILE)
  {
    do_open(filename);
  }
  else
  {
    for (const auto& it : std::vector<std::string>{"./", config::dir() + "/"})
    {
      if (do_open(it + filename))
      {
        return; // finish, we found a file
      }
    }
  }

  if (filename != DEFAULT_TAGFILE && m_file == nullptr)
  {
    log("could not locate ctags file") << filename;
  }
}

bool wex::ctags::previous()
{
  if (m_matches.size() <= 1)
  {
    log::trace("ctags::previous no previous match") << m_matches.size();
    return false;
  }

  if (m_iterator == m_matches.begin())
  {
    m_iterator = m_matches.end();
  }

  m_iterator--;
  m_iterator->second.open_file(get_frame());

  return true;
}
