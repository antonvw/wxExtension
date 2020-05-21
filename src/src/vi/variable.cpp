////////////////////////////////////////////////////////////////////////////////
// Name:      variable.cpp
// Purpose:   Implementation of class wex::variable
// Author:    Anton van Wezenbeek
// Copyright: (c) 2020 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wex/ex.h>
#include <wex/log.h>
#include <wex/macro-mode.h>
#include <wex/macros.h>
#include <wex/stc.h>
#include <wex/stcdlg.h>
#include <wex/util.h>
#include <wex/variable.h>

// Several types of variables are supported.
// See xml file.
enum class wex::variable::input_t
{
  BUILTIN,     // a builtin variable like "Created"
  ENVIRONMENT, // an environment variable like ENV
  INPUT,       // input from user
  INPUT_ONCE,  // input once from user, save value in xml file
  INPUT_SAVE,  // input from user, save value in xml file
  READ,        // read value from macros xml file
  TEMPLATE     // read value from a template file
};

wex::variable::variable(const std::string& name)
  : m_name(name)
  , m_type(input_t::INPUT_SAVE)
{
}

wex::variable::variable(const pugi::xml_node& node)
  : m_name(node.attribute("name").value())
  , m_type(input_t::INPUT_SAVE)
  , m_value(node.text().get())
  , m_prefix(node.attribute("prefix").value())
{
  if (const std::string type = node.attribute("type").value(); !type.empty())
  {
    if (type == "BUILTIN")
    {
      m_type = input_t::BUILTIN;
    }
    else if (type == "ENVIRONMENT")
    {
      m_type = input_t::ENVIRONMENT;
    }
    else if (type == "INPUT")
    {
      m_type = input_t::INPUT;
    }
    else if (type == "INPUT-SAVE")
    {
      m_type = input_t::INPUT_SAVE;
    }
    else if (type == "INPUT-ONCE")
    {
      m_type = input_t::INPUT_ONCE;
    }
    else if (type == "TEMPLATE")
    {
      m_type = input_t::TEMPLATE;
    }
    else
    {
      log::verbose("variable type is not supported") << type;
    }
  }
}

bool wex::variable::check_link(std::string& value) const
{
  if (std::vector<std::string> v; match("@([a-zA-Z].+)@", m_value, v) > 0)
  {
    if (const auto& it = ex::get_macros().get_variables().find(v[0]);
        it != ex::get_macros().get_variables().end())
    {
      if (!it->second.expand(value))
      {
        if (!is_input())
        {
          log() << "variable:" << m_name << "(" << v[0]
                << ") could not be expanded";
        }
      }
      else
      {
        if (!value.empty())
        {
          log::verbose("variable")
            << m_name << "(" << v[0] << ") expanded:" << value;
          return true;
        }
      }
    }
    else
    {
      log() << "variable:" << m_name << "(" << v[0] << ") is not found";
    }
  }

  return false;
}

bool wex::variable::expand(ex* ex)
{
  std::string value;

  if (check_link(value))
  {
    m_value = value;
    value.clear();
  }

  if (!expand(value, ex))
  {
    if (!is_input())
    // Now only show log status if this is no input variable,
    // as it was cancelled in that case.
    {
      log::status(_("Could not expand variable")) << m_name;
    }

    return false;
  }

  // If there is a prefix, make a comment out of it.
  auto commented(value);

  if (ex != nullptr)
  {
    if (ex->get_stc()->GetReadOnly() || ex->get_stc()->is_hexmode())
    {
      return false;
    }

    if (!m_prefix.empty())
    {
      commented = ex->get_stc()->get_lexer().make_comment(
        m_prefix == "WRAP" ? std::string() : m_prefix,
        value);
    }

    ex->get_stc()->add_text(commented);
  }

  if (m_type == input_t::INPUT_SAVE || m_type == input_t::INPUT_ONCE)
  {
    m_value = value;

    log::verbose("variable") << m_name << "expanded and saved:" << m_value;
  }
  else
  {
    log::verbose("variable") << m_name << "expanded to:" << value;
  }

  if (m_type == input_t::INPUT_ONCE && !m_value.empty())
  {
    m_ask_for_input = false;
  }

  return true;
}

bool wex::variable::expand(std::string& value, ex* ex) const
{
  check_link(value);

  switch (m_type)
  {
    case input_t::BUILTIN:
      if (!expand_builtin(ex, value))
      {
        return false;
      }
      break;

    case input_t::ENVIRONMENT:
      if (wxString val; !wxGetEnv(m_name, &val))
      {
        return false;
      }
      else
      {
        value = val;
      }
      break;

    case input_t::INPUT:
    case input_t::INPUT_ONCE:
    case input_t::INPUT_SAVE:
      if (!expand_input(value))
      {
        return false;
      }
      break;

    case input_t::READ:
      if (m_value.empty())
      {
        return false;
      }
      value = m_value;
      break;

    case input_t::TEMPLATE:
      if (!ex::get_macros().mode().expand(ex, *this, value))
      {
        return false;
      }
      break;

    default:
      assert(0);
      break;
  }

  return true;
}

bool wex::variable::expand_builtin(ex* ex, std::string& expanded) const
{
  if (m_name == "Date")
  {
    expanded = wxDateTime::Now().FormatISODate();
  }
  else if (m_name == "Datetime")
  {
    expanded = wxDateTime::Now().FormatISOCombined(' ');
  }
  else if (m_name == "Time")
  {
    expanded = wxDateTime::Now().FormatISOTime();
  }
  else if (m_name == "Year")
  {
    expanded = wxDateTime::Now().Format("%Y");
  }
  else if (ex != nullptr)
  {
    if (m_name == "Cb")
    {
      expanded = ex->get_stc()->get_lexer().comment_begin();
    }
    else if (m_name == "Cc")
    {
      const int line     = ex->get_stc()->GetCurrentLine();
      const int startPos = ex->get_stc()->PositionFromLine(line);
      const int endPos   = ex->get_stc()->GetLineEndPosition(line);
      expanded           = ex->get_stc()->get_lexer().comment_complete(
        ex->get_stc()->GetTextRange(startPos, endPos).ToStdString());
    }
    else if (m_name == "Ce")
    {
      expanded = ex->get_stc()->get_lexer().comment_end();
    }
    else if (m_name == "Cl")
    {
      expanded = ex->get_stc()->get_lexer().make_comment(std::string(), false);
    }
    else if (m_name == "Created")
    {
      if (path file(ex->get_stc()->get_filename());
          ex->get_stc()->get_filename().stat().is_ok())
      {
        expanded = wxDateTime(file.stat().st_ctime).FormatISODate();
      }
      else
      {
        expanded = wxDateTime::Now().FormatISODate();
      }
    }
    else if (m_name == "Filename")
    {
      expanded = ex->get_stc()->get_filename().name();
    }
    else if (m_name == "Fullname")
    {
      expanded = ex->get_stc()->get_filename().fullname();
    }
    else if (m_name == "Fullpath")
    {
      expanded = ex->get_stc()->get_filename().string();
    }
    else if (m_name == "Nl")
    {
      expanded = ex->get_stc()->eol();
    }
    else if (m_name == "Path")
    {
      expanded = ex->get_stc()->get_filename().get_path();
    }
    else
    {
      return false;
    }
  }

  return true;
}

bool wex::variable::expand_input(std::string& expanded) const
{
  if (m_ask_for_input)
  {
    const auto use(!expanded.empty() ? expanded : m_value);

    if (m_dialog == nullptr)
    {
      m_dialog = new stc_entry_dialog(
        use,
        std::string(),
        window_data().title(m_name + ":"));

      m_dialog->get_stc()->get_vi().use(false);
      m_dialog->get_stc()->SetWrapMode(wxSTC_WRAP_WORD);
    }

    m_dialog->SetTitle(m_name);
    m_dialog->get_stc()->set_text(use);
    m_dialog->get_stc()->SetFocus();

    bool ended = false;

    if (wxIsBusy())
    {
      ended = true;
      wxEndBusyCursor();
    }

    const int result = m_dialog->ShowModal();

    if (ended)
    {
      wxBeginBusyCursor();
    }

    if (result == wxID_CANCEL)
    {
      return false;
    }

    const std::string value(m_dialog->get_stc()->get_text());

    if (value.empty())
    {
      return false;
    }

    expanded = value;
  }
  else
  {
    expanded = m_value;
  }

  return true;
}

bool wex::variable::is_builtin() const
{
  return m_type == input_t::BUILTIN;
}

bool wex::variable::is_input() const
{
  return m_type == input_t::INPUT || m_type == input_t::INPUT_ONCE ||
         m_type == input_t::INPUT_SAVE;
}

bool wex::variable::is_template() const
{
  return m_type == input_t::TEMPLATE;
}

void wex::variable::save(pugi::xml_node& node, const std::string* value)
{
  assert(!m_name.empty());

  if (!node.attribute("name"))
  {
    node.append_attribute("name") = m_name.c_str();
  }

  if (!node.attribute("type"))
  {
    pugi::xml_attribute type = node.append_attribute("type");

    switch (m_type)
    {
      case input_t::BUILTIN:
        type.set_value("BUILTIN");
        break;
      case input_t::ENVIRONMENT:
        type.set_value("ENVIRONMENT");
        break;
      case input_t::INPUT:
        type.set_value("INPUT");
        break;
      case input_t::INPUT_ONCE:
        type.set_value("INPUT-ONCE");
        break;
      case input_t::INPUT_SAVE:
        type.set_value("INPUT-SAVE");
        break;
      case input_t::READ:
        break;
      case input_t::TEMPLATE:
        type.set_value("TEMPLATE");
        break;

      default:
        assert(0);
        break;
    }
  }

  if (!m_prefix.empty() && !node.attribute("prefix"))
  {
    node.append_attribute("prefix") = m_prefix.c_str();
  }

  if (value != nullptr)
  {
    m_value = *value;
  }

  if (!m_value.empty() && m_type != input_t::INPUT)
  {
    node.text().set(m_value.c_str());
  }
}

void wex::variable::set_ask_for_input(bool value)
{
  if (!value)
  {
    m_ask_for_input = value;
  }
  else if (is_input() && m_type != input_t::INPUT_ONCE)
  {
    m_ask_for_input = value;
  }
}