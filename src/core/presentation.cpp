////////////////////////////////////////////////////////////////////////////////
// Name:      presentation.cpp
// Purpose:   Implementation of class wex::presentation
// Author:    Anton van Wezenbeek
// Copyright: (c) 2021 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <boost/tokenizer.hpp>
#include <wex/lexers.h>
#include <wex/log.h>
#include <wex/presentation.h>
#include <wx/stc/stc.h>

wex::presentation::presentation(presentation_t type, const pugi::xml_node& node)
  : m_type(type)
{
  if (node.empty())
    return;

  const auto single = lexers::get()->apply_macro(node.attribute("no").value());

  try
  {
    boost::tokenizer<boost::char_separator<char>> tok(
      std::string(node.text().get()),
      boost::char_separator<char>(","));

    m_no = std::stoi(single);

    if (auto it = tok.begin(); it != tok.end())
    {
      m_style = std::stoi(lexers::get()->apply_macro(*it));

      if (++it != tok.end())
      {
        m_foreground_colour = lexers::get()->apply_macro(*it);

        if (m_type == INDICATOR && ++it != tok.end())
        {
          m_under = (*it == "true");
        }

        if (m_type == MARKER && ++it != tok.end())
        {
          m_background_colour = lexers::get()->apply_macro(*it);
        }
      }
    }

    if (!is_ok())
    {
      log("illegal " + name() + " number") << m_no << node;
    }
  }
  catch (std::exception& e)
  {
    log(e) << name() << single << node.text().get();
  }
}

wex::presentation::presentation(presentation_t type, int no, int style)
  : m_type(type)
  , m_no(no)
  , m_style(style)
{
}

bool wex::presentation::operator<(const wex::presentation& i) const
{
  return m_no < i.m_no;
}

bool wex::presentation::operator==(const wex::presentation& i) const
{
  return m_style == -1 ? m_no == i.m_no :
                         m_no == i.m_no && m_style == i.m_style;
}

void wex::presentation::apply(wxStyledTextCtrl* stc) const
{
  if (is_ok() && stc->GetParent() != nullptr)
  {
    switch (m_type)
    {
      case INDICATOR:
        stc->IndicatorSetStyle(m_no, m_style);

        if (!m_foreground_colour.empty())
        {
          stc->IndicatorSetForeground(m_no, wxString(m_foreground_colour));
        }

        stc->IndicatorSetUnder(m_no, m_under);
        break;

      case MARKER:
        stc->MarkerDefine(
          m_no,
          m_style,
          wxString(m_foreground_colour),
          wxString(m_background_colour));
        break;

      default:
        assert(0);
    }
  }
}

bool wex::presentation::is_ok() const
{
  switch (m_type)
  {
    case INDICATOR:
      return m_no >= 0 && m_no <= wxSTC_INDIC_MAX && m_style >= 0 &&
             m_style <= wxSTC_INDIC_ROUNDBOX;

    case MARKER:
      return m_no >= 0 && m_no <= wxSTC_MARKER_MAX &&
             ((m_style >= 0 && m_style <= wxSTC_MARKER_MAX) ||
              (m_style >= wxSTC_MARK_CHARACTER &&
               m_style <= wxSTC_MARK_CHARACTER + 255));
    default:
      assert(0);
      return false;
  }
}

const std::string wex::presentation::name() const
{
  switch (m_type)
  {
    case INDICATOR:
      return "indicator";

    case MARKER:
      return "marker";

    default:
      assert(0);
      return std::string();
  }
}
