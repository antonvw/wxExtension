////////////////////////////////////////////////////////////////////////////////
// Name:      dir.cpp
// Purpose:   Implementation of class wex::dir
// Author:    Anton van Wezenbeek
// Copyright: (c) 2021 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <thread>
#include <wex/core.h>
#include <wex/dir.h>
#include <wex/log.h>
#include <wex/stream.h>
#include <wx/translation.h>

namespace fs = std::filesystem;

namespace wex
{
class string_dir : public dir
{
public:
  string_dir(
    const std::string&        path,
    const wex::data::dir&     data,
    std::vector<std::string>& v)
    : dir(wex::path(path), data)
    , m_container(v)
  {
    ;
  }

private:
  bool on_dir(const path& p) const final
  {
    m_container.emplace_back(
      data().type().test(data::dir::RECURSIVE) ? p.string() : p.filename());
    return true;
  };

  bool on_file(const path& p) const final
  {
    m_container.emplace_back(
      data().type().test(data::dir::RECURSIVE) ? p.string() : p.filename());
    return true;
  };

  std::vector<std::string>& m_container;
};

class path_dir : public dir
{
public:
  path_dir(const path& p, const wex::data::dir& data, std::vector<path>& v)
    : dir(p, data)
    , m_container(v)
  {
    ;
  }

private:
  bool on_dir(const path& p) const final
  {
    m_container.emplace_back(p);
    return true;
  }

  bool on_file(const path& p) const final
  {
    m_container.emplace_back(p);
    return true;
  }

  std::vector<path>& m_container;
};

bool allow_hidden(const fs::directory_entry& e, const data::dir& data)
{
  // If this is not a hidden file, return true.
  if (!e.path().filename().string().starts_with("."))
  {
    return true;
  }

  // This file is hidden, only allow if flag HIDDEN is set.
  return data.type().test(data::dir::HIDDEN);
}
}; // namespace wex

wex::dir::dir(
  const wex::path&      dir,
  const wex::data::dir& data,
  wxEvtHandler*         eh)
  : m_dir(dir)
  , m_data(data)
  , m_eh(eh)
  , m_tool(ID_TOOL_ADD)
{
}

int wex::dir::find_files()
{
  if (!m_dir.dir_exists())
  {
    log("traverse invalid path") << m_dir;
    return 0;
  }

  if (!start())
  {
    log::trace("traverse busy") << m_dir;
    log::status(_("Busy"));
    return 0;
  }

  m_statistics.clear();

  if (m_eh != nullptr)
  {
    std::thread t(
      [*this]
      {
        const auto id(std::hash<std::thread::id>{}(std::this_thread::get_id()));

        log::trace("thread") << id << "started" << m_dir.string();

        run();

        log::trace("thread") << id << "end";
      });

    t.detach();

    return 1;
  }
  else
  {
    return run();
  }
}

bool wex::dir::find_files(const tool& tool)
{
  if (m_eh == nullptr)
  {
    return false;
  }

  m_tool = tool;

  return find_files() > 0;
}

void wex::dir::find_files_end() const
{
  log::status(m_tool.info(&m_statistics.get_elements()));

  if (m_eh != nullptr)
  {
    wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED, ID_LIST_MATCH_FINISH);
    wxPostEvent(m_eh, event);
  }
}

int wex::dir::matches() const
{
  return m_statistics.get_elements().get(_("Files"));
}

bool wex::dir::on_dir(const path& p) const
{
  if (m_eh != nullptr)
  {
    if (!m_tool.is_find_type() && m_data.type().test(data::dir::DIRS))
    {
      m_statistics.get_elements().inc(_("Folders"));
      post_event(p);
    }
  }

  return true;
}

bool wex::dir::on_file(const path& p) const
{
  if (m_eh != nullptr)
  {
    if (m_tool.is_find_type())
    {
      stream s(m_data.find_replace_data(), p, m_tool, m_eh);

      if (!s.run_tool())
      {
        cancel();
        return false;
      }

      m_statistics += s.get_statistics();
    }
    else
    {
      post_event(p);
    }
  }

  return true;
}

void wex::dir::post_event(const path& p) const
{
  wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED, ID_LIST_MATCH);
  event.SetString(m_data.file_spec());
  event.SetClientData(new wex::path_match(p));
  wxPostEvent(m_eh, event);
}

int wex::dir::run() const
{
  try
  {
    if (m_data.type().test(data::dir::RECURSIVE))
    {
      if (fs::recursive_directory_iterator rdi(
            m_dir.data(),
#ifdef __WXMSW__
            fs::directory_options::none),
          end;
#else
            fs::directory_options::skip_permission_denied),
          end;
#endif
          !std::all_of(
            rdi,
            end,
            [&](const fs::directory_entry& p)
            {
              return traverse(p);
            }))
      {
        log::trace("iterating aborted");
      }
    }
    else
    {
      if (fs::directory_iterator di(m_dir.data()), end; !std::all_of(
            di,
            end,
            [&](const fs::directory_entry& p)
            {
              return traverse(p);
            }))
      {
        log::trace("iterating aborted");
      }
    }
  }
  catch (fs::filesystem_error& e)
  {
    log(e) << "filesystem";
  }
  catch (std::exception& e)
  {
    log(e) << "exception";
  }

  log::trace("iterated") << m_dir << "on files:" << m_data.file_spec()
                         << "on dirs:" << m_data.dir_spec()
                         << "flags:" << m_data.type()
                         << "matches:" << matches();

  stop();

  find_files_end();

  return matches();
}

bool wex::dir::traverse(const fs::directory_entry& e) const
{
  if (fs::is_regular_file(e.path()))
  {
    if (
      m_data.type().test(data::dir::FILES) && allow_hidden(e, m_data) &&
      matches_one_of(e.path().filename().string(), m_data.file_spec()))
    {
      if (on_file(e.path()))
      {
        dir::get_statistics().inc_actions();
      }
    }
  }
  else if (
    m_data.type().test(data::dir::DIRS) && fs::is_directory(e.path()) &&
    allow_hidden(e, m_data) &&
    (m_data.dir_spec().empty() ||
     matches_one_of(e.path().filename().string(), m_data.dir_spec())))
  {
    on_dir(e.path());

    if (!m_data.dir_spec().empty())
    {
      dir::get_statistics().inc_actions();
    }
  }

  if (m_data.max_matches() != -1 && matches() >= m_data.max_matches())
  {
    log::trace("traverse limit reached") << m_data.max_matches();
    return false;
  }

  return !interruptible::is_cancelled();
}

std::vector<std::string>
wex::get_all_files(const std::string& path, const wex::data::dir& data)
{
  std::vector<std::string> v;
  string_dir               dir(path, data, v);
  dir.find_files();
  return v;
}

std::vector<wex::path>
wex::get_all_files(const path& p, const wex::data::dir& data)
{
  std::vector<path> v;
  path_dir          dir(p, data, v);
  dir.find_files();
  return v;
}
