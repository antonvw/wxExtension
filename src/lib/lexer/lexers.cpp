////////////////////////////////////////////////////////////////////////////////
// Name:      lexers.cpp
// Purpose:   Implementation of wex::lexers class
// Author:    Anton van Wezenbeek
// Copyright: (c) 2020 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <algorithm>
#include <functional>
#include <numeric>
#include <regex>
#include <wex/config.h>
#include <wex/lexers.h>
#include <wex/log.h>
#include <wex/stc.h>
#include <wex/tokenizer.h>
#include <wex/util.h>

// Constructor for lexers from specified filename.
// This must be an existing xml file containing all lexers.
// It does not do load_document, however if you use the static get,
// it both constructs and loads the lexers.
wex::lexers::lexers(const path& filename)
  : m_path(filename)
  , m_theme(config("theme").get())
{
}

void wex::lexers::apply(stc* stc) const
{
  m_default_style.apply(stc);

  for (const auto& i : m_indicators)
    i.apply(stc);
  for (const auto& p : m_global_properties)
    p.apply(stc);
  for (const auto& m : m_markers)
    m.apply(stc);

  if (stc->get_hexmode().is_active())
  {
    for (const auto& s : m_styles_hex)
      s.apply(stc);
  }
}

void wex::lexers::apply_default_style(
  std::function<void(const std::string&)> back,
  std::function<void(const std::string&)> fore) const
{
  if (std::vector<std::string> v;
      back != nullptr && match(",back:(.*),", m_default_style.value(), v) > 0)
  {
    back(v[0]);
  }

  if (std::vector<std::string> v;
      fore != nullptr && match(",fore:(.*)", m_default_style.value(), v) > 0)
  {
    fore(v[0]);
  }
}

// No longer const, as it updates m_default_colours.
void wex::lexers::apply_global_styles(stc* stc)
{
  if (m_default_colours.empty())
  {
    m_default_colours["caretforeground"] =
      stc->GetCaretForeground().GetAsString();
    m_default_colours["caretlinebackground"] =
      stc->GetCaretLineBackground().GetAsString();
    m_default_colours["edge"] = stc->GetEdgeColour().GetAsString();
  }

  m_default_style.apply(stc);

  stc->StyleClearAll();

  for (const auto& s : m_styles)
    s.apply(stc);

  stc->SetFoldMarginHiColour(
    true,
    !m_folding_background_colour.empty() ?
      wxColour(m_folding_background_colour.c_str()) :
      // See ViewStyle.cxx foldmarginColour
      wxColour(0xc0, 0xc0, 0xc0));

  stc->SetFoldMarginColour(
    true,
    !m_folding_foreground_colour.empty() ?
      wxColour(m_folding_foreground_colour.c_str()) :
      wxColour(0xff, 0, 0));

  if (const auto& colour_it = m_theme_colours.find(m_theme);
      colour_it != m_theme_colours.end())
  {
    for (const auto& it : colour_it->second)
    {
      if (it.first == "caretforeground")
        stc->SetCaretForeground(it.second.c_str());
      else if (it.first == "caretlinebackground")
        stc->SetCaretLineBackground(it.second.c_str());
      else if (it.first == "selbackground")
        stc->SetSelBackground(true, it.second.c_str());
      else if (it.first == "selforeground")
        stc->SetSelForeground(true, it.second.c_str());
      else if (it.first == "calltipbackground")
        stc->CallTipSetBackground(it.second.c_str());
      else if (it.first == "calltipforeground")
        stc->CallTipSetForeground(it.second.c_str());
      else if (it.first == "edge")
        stc->SetEdgeColour(it.second.c_str());
    }
  }
}

const std::string wex::lexers::apply_macro(
  const std::string& text,
  const std::string& lexer) const
{
  if (const auto& it = get_macros(lexer).find(text);
      it != get_macros(lexer).end())
    return it->second;
  else if (const auto& it = theme_macros().find(text);
           it != theme_macros().end())
    return it->second;
  else
    return text;
}

void wex::lexers::apply_margin_text_style(
  stc*               stc,
  int                line,
  margin_style_t     style,
  const std::string& text) const
{
  switch (style)
  {
    case margin_style_t::DAY:
      stc->MarginSetStyle(line, m_style_no_text_margin_day);
      break;

    case margin_style_t::MONTH:
      stc->MarginSetStyle(line, m_style_no_text_margin_month);
      break;

    case margin_style_t::OTHER:
      stc->MarginSetStyle(line, m_style_no_text_margin);
      break;

    case margin_style_t::WEEK:
      stc->MarginSetStyle(line, m_style_no_text_margin_week);
      break;

    case margin_style_t::YEAR:
      stc->MarginSetStyle(line, m_style_no_text_margin_year);
      break;

    case margin_style_t::UNKNOWN:
      break;
  }

  if (!text.empty())
  {
    stc->MarginSetText(line, text);
  }
}

void wex::lexers::clear_theme()
{
  if (!m_theme.empty())
  {
    m_theme_previous = m_theme;
    m_theme.clear();
  }
}

const wex::lexer& wex::lexers::find(const std::string& name) const
{
  const auto& it =
    std::find_if(m_lexers.begin(), m_lexers.end(), [name](auto const& e) {
      return e.display_lexer() == name;
    });

  return it != m_lexers.end() ? *it : m_lexers.front();
}

const wex::lexer&
wex::lexers::find_by_filename(const std::string& fullname) const
{
  const auto& it =
    std::find_if(m_lexers.begin(), m_lexers.end(), [fullname](auto const& e) {
      return !e.extensions().empty() &&
             matches_one_of(fullname, e.extensions());
    });

  return it != m_lexers.end() ? *it : m_lexers.front();
}

const wex::lexer& wex::lexers::find_by_text(const std::string& text) const
{
  if (text.empty())
  {
    return m_lexers.front();
  }

  try
  {
    const std::string filtered(std::regex_replace(
      text,
      std::regex("[ \t\n\v\f\r]+$"),
      "",
      std::regex_constants::format_sed));

    for (const auto& t : m_texts)
    {
      if (std::regex_search(filtered, std::regex(t.second)))
        return find(t.first);
    }
  }
  catch (std::exception& e)
  {
    log(e) << "find_by_text";
  }

  return m_lexers.front();
}

wex::lexers* wex::lexers::get(bool createOnDemand)
{
  if (m_self == nullptr && createOnDemand)
  {
    m_self = new lexers(path(config::dir(), "wex-lexers.xml"));
    m_self->load_document();
  }

  return m_self;
}

const wex::indicator&
wex::lexers::get_indicator(const indicator& indicator) const
{
  const auto& it = m_indicators.find(indicator);
  return (
    it != m_indicators.end() ? *it : *m_indicators.find(wex::indicator()));
}

const wex::lexers::name_values_t&
wex::lexers::get_macros(const std::string& lexer) const
{
  const auto& it = m_macros.find(lexer);
  return (
    it != m_macros.end() ? it->second : m_macros.find(std::string())->second);
}

const wex::marker& wex::lexers::get_marker(const marker& marker) const
{
  const auto& it = m_markers.find(marker);
  return (it != m_markers.end() ? *it : *m_markers.find(wex::marker()));
}

const std::string& wex::lexers::keywords(const std::string& set) const
{
  const auto& it = m_keywords.find(set);
  return (
    it != m_keywords.end() ? it->second :
                             m_keywords.find(std::string())->second);
}

bool wex::lexers::load_document()
{
  // This test is to prevent showing an error if the lexers file does not exist,
  // as this is not required.
  if (!m_path.file_exists())
    return false;

  pugi::xml_document doc;

  if (const auto result = doc.load_file(
        m_path.string().c_str(),
        pugi::parse_default | pugi::parse_trim_pcdata);
      !result)
  {
    xml_error(m_path, &result);
    return false;
  }

  if (m_is_loaded)
  {
    m_default_style.clear();
    m_folding_background_colour.clear();
    m_folding_foreground_colour.clear();
    m_global_properties.clear();
    m_indicators.clear();
    m_keywords.clear();
    m_lexers.clear();
    m_macros.clear();
    m_markers.clear();
    m_styles.clear();
    m_styles_hex.clear();
    m_texts.clear();
    m_theme_colours.clear();
    m_theme_macros.clear();
  }

  m_indicators.insert(indicator());
  m_keywords[std::string()] = std::string();
  m_lexers.push_back(std::string());
  m_macros[std::string()] = name_values_t{};
  m_markers.insert(marker());
  m_theme_colours[std::string()] = m_default_colours;
  m_theme_macros[std::string()]  = name_values_t{};

  for (const auto& node : doc.document_element().children())
  {
    if (strcmp(node.name(), "macro") == 0)
      parse_node_macro(node);
    else if (strcmp(node.name(), "global") == 0)
      parse_node_global(node);
    else if (strcmp(node.name(), "keyword") == 0)
      parse_node_keyword(node);
    else if (strcmp(node.name(), "lexer") == 0)
    {
      if (const wex::lexer lexer(&node); lexer.is_ok())
        m_lexers.emplace_back(lexer);
    }
  }

  // Do some checking.
  if (!m_lexers.empty() && !m_theme.empty())
  {
    if (!m_default_style.is_ok())
      log() << "default style not ok";
    if (!m_default_style.contains_default_style())
      log() << "default style does not contain default style";
  }

  if (m_theme_macros.size() == 1)
  {
    log() << "themes are missing";
  }
  else
  {
    for (const auto& it : m_theme_macros)
    {
      if (!it.first.empty() && it.second.empty())
      {
        log("theme") << it.first << " is unknown";
      }
    }
  }

  log::verbose("lexers info")
    << "default colors:" << m_default_colours.size()
    << "global properties:" << m_global_properties.size()
    << "indicators:" << m_indicators.size() << "keywords:" << m_keywords.size()
    << "lexers:" << m_lexers.size() << "macros:" << m_macros.size()
    << "markers:" << m_markers.size() << "styles:" << m_styles.size()
    << "styles hex:" << m_styles_hex.size() << "texts:" << m_texts.size()
    << "theme colours:" << m_theme_colours.size()
    << "theme macros:" << m_theme_macros.size();

  m_is_loaded = true;

  return true;
}

void wex::lexers::parse_node_folding(const pugi::xml_node& node)
{
  tokenizer fields(node.text().get(), ",");

  m_folding_background_colour = apply_macro(fields.get_next_token());
  m_folding_foreground_colour = apply_macro(fields.get_next_token());
}

void wex::lexers::parse_node_global(const pugi::xml_node& node)
{
  for (const auto& child : node.children())
  {
    if (m_theme.empty())
    {
      // Do nothing.
    }
    else if (strcmp(child.name(), "colour") == 0)
    {
      wxTheColourDatabase->AddColour(
        child.attribute("no").value(),
        child.text().get());
    }
    else if (strcmp(child.name(), "foldmargin") == 0)
    {
      parse_node_folding(child);
    }
    else if (strcmp(child.name(), "hex") == 0)
    {
      m_styles_hex.emplace_back(child, "global");
    }
    else if (strcmp(child.name(), "indicator") == 0)
    {
      if (const wex::indicator indicator(child); indicator.is_ok())
      {
        m_indicators.insert(indicator);
      }
    }
    else if (strcmp(child.name(), "marker") == 0)
    {
      if (const wex::marker marker(child); marker.is_ok())
        m_markers.insert(marker);
    }
    else if (strcmp(child.name(), "properties") == 0)
    {
      node_properties(&child, m_global_properties);
    }
    else if (strcmp(child.name(), "style") == 0)
    {
      if (const wex::style style(child, "global");
          style.contains_default_style())
      {
        if (m_default_style.is_ok())
        {
          log("duplicate default style") << child.name() << child;
        }
        else
        {
          m_default_style = style;
        }
      }
      else
      {
        if (style.define() == "style_textmargin")
        {
          m_style_no_text_margin = std::stoi(style.number());
        }
        else if (style.define() == "style_textmargin_day")
        {
          m_style_no_text_margin_day = std::stoi(style.number());
        }
        else if (style.define() == "style_textmargin_week")
        {
          m_style_no_text_margin_week = std::stoi(style.number());
        }
        else if (style.define() == "style_textmargin_month")
        {
          m_style_no_text_margin_month = std::stoi(style.number());
        }
        else if (style.define() == "style_textmargin_year")
        {
          m_style_no_text_margin_year = std::stoi(style.number());
        }

        m_styles.emplace_back(style);
      }
    }
    else if (strcmp(child.name(), "text") == 0)
    {
      m_texts.push_back({child.attribute("no").value(), child.text().get()});
    }
  }
}

void wex::lexers::parse_node_keyword(const pugi::xml_node& node)
{
  for (const auto& child : node.children())
  {
    m_keywords[child.attribute("name").value()] = child.text().get();
  }
}

void wex::lexers::parse_node_macro(const pugi::xml_node& node)
{
  for (const auto& child : node.children())
  {
    if (strcmp(child.name(), "def") == 0)
    {
      const std::string name = child.attribute("name").value();
      name_values_t     macro_map;
      int               val = 0;

      for (const auto& macro : child.children())
      {
        if (const std::string no = macro.attribute("no").value(); !no.empty())
        {
          if (const auto& it = macro_map.find(no); it != macro_map.end())
          {
            log("macro") << no << macro << " already exists";
          }
          else
          {
            if (const std::string content = macro.text().get();
                !content.empty())
            {
              try
              {
                val           = std::stoi(content) + 1;
                macro_map[no] = content;
              }
              catch (std::exception& e)
              {
                log(e) << "macro";
              }
            }
            else
            {
              macro_map[no] = std::to_string(val);
              val++;
            }
          }
        }
      }

      m_macros[name] = macro_map;
    }
    else if (strcmp(child.name(), "themes") == 0)
    {
      parse_node_themes(child);
    }
  }
}

void wex::lexers::parse_node_theme(const pugi::xml_node& node)
{
  if (m_theme.empty() && !config().item("theme").exists())
  {
    m_theme = std::string(node.attribute("name").value());
  }

  name_values_t tmpColours, tmpMacros;

  for (const auto& child : node.children())
  {
    if (const std::string content = child.text().get();
        strcmp(child.name(), "def") == 0)
    {
      if (const std::string style = child.attribute("style").value();
          !style.empty())
      {
        if (const auto& it = tmpMacros.find(style); it != tmpMacros.end())
        {
          log("macro style") << style << child << " already exists";
        }
        else
        {
          tmpMacros[style] = content;
        }
      }
    }
    else if (strcmp(child.name(), "colour") == 0)
    {
      tmpColours[child.attribute("name").value()] = apply_macro(content);
    }
  }

  m_theme_colours[node.attribute("name").value()] = tmpColours;
  m_theme_macros[node.attribute("name").value()]  = tmpMacros;
}

void wex::lexers::parse_node_themes(const pugi::xml_node& node)
{
  for (const auto& child : node.children())
    parse_node_theme(child);
}

wex::lexers* wex::lexers::set(wex::lexers* lexers)
{
  wex::lexers* old = m_self;
  m_self           = lexers;
  return old;
}

bool wex::lexers::show_dialog(stc* stc) const
{
  wxArrayString s;
  for (const auto& it : m_lexers)
    s.Add(it.display_lexer());

  auto lexer = stc->get_lexer().display_lexer();
  if (!single_choice_dialog(stc, _("Enter Lexer"), s, lexer))
    return false;

  lexer.empty() ? stc->get_lexer().clear() :
                  (void)stc->get_lexer().set(lexer, true);

  return true;
}

bool wex::lexers::show_theme_dialog(wxWindow* parent)
{
  wxArrayString s;
  for (const auto& it : m_theme_macros)
    s.Add(it.first);

  if (!single_choice_dialog(parent, _("Enter Theme"), s, m_theme))
    return false;

  config("theme").set(m_theme);

  return load_document();
}

const wex::lexers::name_values_t& wex::lexers::theme_macros() const
{
  const auto& it = m_theme_macros.find(m_theme);
  return (
    it != m_theme_macros.end() ? it->second :
                                 m_theme_macros.find(std::string())->second);
}