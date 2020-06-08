////////////////////////////////////////////////////////////////////////////////
// Name:      textctrl-imp.cpp
// Purpose:   Implementation of wex::textctrl_imp class
// Author:    Anton van Wezenbeek
// Copyright: (c) 2020 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <wex/config.h>
#include <wex/core.h>
#include <wex/ex.h>
#include <wex/frd.h>
#include <wex/log.h>
#include <wex/macros.h>
#include <wex/managedframe.h>
#include <wex/stc.h>
#include <wex/textctrl.h>
#include <wx/control.h>
#include <wx/settings.h>
#include <wx/timer.h>

#include "textctrl-imp.h"

const auto ID_REGISTER = wxWindow::NewControlId();

wex::textctrl_imp::textctrl_imp(
  textctrl*          tc,
  wxControl*         prefix,
  const data::window& data)
  : wxTextCtrl(
      data.parent(),
      data.id(),
      wxEmptyString,
      data.pos(),
      data.size(),
      data.style() | wxTE_PROCESS_ENTER | wxTE_MULTILINE)
  , m_prefix(prefix)
  , m_tc(tc)
  , m_timer(this)
{
  SetFont(config(_("stc.Text font"))
            .get(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT)));

  Bind(wxEVT_CHAR, [=](wxKeyEvent& event) {
    if (event.GetUnicodeKey() == WXK_NONE)
    {
      return;
    }

    switch (event.GetKeyCode())
    {
      case WXK_RETURN:
        event.Skip();

        if (m_input != 0)
        {
          m_command.handle(this, event.GetKeyCode());
        }
        break;

      case WXK_TAB:
        if (
          m_tc->ex() != nullptr &&
          m_tc->ex()->get_stc()->get_filename().file_exists())
        {
          path::current(m_tc->ex()->get_stc()->get_filename().get_path());
        }

        if ([[maybe_unused]] const auto& [r, e, v] =
              auto_complete_filename(m_command.command());
            r)
        {
          AppendText(e);
          m_command.append(e);
        }
        break;

      default:
        bool skip = true;

        if (m_control_r)
        {
          skip             = false;
          const char     c = event.GetUnicodeKey();
          wxCommandEvent event(wxEVT_MENU, ID_REGISTER);

          if (c == '%')
          {
            if (m_tc->ex() != nullptr)
            {
              event.SetString(m_tc->ex()->get_stc()->get_filename().fullname());
            }
          }
          else
          {
            event.SetString(ex::get_macros().get_register(c));
          }

          if (!event.GetString().empty())
          {
            wxPostEvent(this, event);
          }
        }

        m_user_input = true;
        m_control_r  = false;

        if (skip)
        {
          event.Skip();
          m_command.handle(this, event.GetKeyCode());
        }
    }
  });

  Bind(wxEVT_KEY_DOWN, [=](wxKeyEvent& event) {
    switch (event.GetKeyCode())
    {
      case 'r':
      case 'R':
        cut();

#ifdef __WXMAC__
        if (event.GetModifiers() & wxMOD_RAW_CONTROL)
#else
        if (event.GetModifiers() & wxMOD_CONTROL)
#endif
        {
          m_command.append(WXK_CONTROL_R);
          m_user_input = true;
          m_control_r  = true;
        }
        else
        {
          event.Skip();
        }
        break;

      case WXK_DOWN:
      case WXK_END:
      case WXK_HOME:
      case WXK_PAGEDOWN:
      case WXK_PAGEUP:
      case WXK_UP:
        if (
          (event.GetKeyCode() == WXK_HOME || event.GetKeyCode() == WXK_END) &&
          !event.ControlDown())
        {
          event.Skip();
        }
        else if (m_command.type() == ex_command::type_t::FIND)
        {
          find_replace_data::get()->m_find_strings.set(
            event.GetKeyCode(),
            m_tc);
        }
        else if (m_input == 0)
        {
          TCI().set(event.GetKeyCode(), m_tc);
        }
        else
        {
          event.Skip();
        }
        break;

      case WXK_BACK:
        m_command.handle(this, event.GetKeyCode());
        event.Skip();
        break;

      case WXK_ESCAPE:
        if (m_tc->ex() != nullptr)
        {
          m_tc->ex()->get_stc()->position_restore();
        }
        m_tc->frame()->hide_ex_bar(managed_frame::HIDE_BAR_FORCE_FOCUS_STC);
        m_control_r  = false;
        m_user_input = false;
        break;

      default:
        if (isascii(event.GetKeyCode()) && event.GetKeyCode() != WXK_RETURN)
        {
          cut();
        }
        event.Skip();
        break;
    }
  });

  Bind(
    wxEVT_MENU,
    [=](wxCommandEvent& event) {
      WriteText(event.GetString());
    },
    ID_REGISTER);

  Bind(wxEVT_SET_FOCUS, [=](wxFocusEvent& event) {
    event.Skip();

    if (m_tc->ex() != nullptr)
    {
      m_tc->ex()->get_stc()->position_save();
    }
  });

  Bind(wxEVT_TEXT, [=](wxCommandEvent& event) {
    event.Skip();

    if (GetValue().size() == 0 && m_input == 0)
    {
      m_command.reset();
    }

    if (
      m_user_input && m_tc->ex() != nullptr &&
      m_command.type() == ex_command::type_t::FIND)
    {
      m_tc->ex()->get_stc()->position_restore();
      m_tc->ex()->get_stc()->find_next(
        get_text(),
        m_tc->ex()->search_flags(),
        m_command.str() == "/");
    }
  });

  Bind(wxEVT_TEXT_ENTER, [=](wxCommandEvent& event) {
    if (m_tc->ex() == nullptr || get_text().empty())
    {
      m_tc->frame()->hide_ex_bar(managed_frame::HIDE_BAR_FORCE_FOCUS_STC);
      return;
    }

    if (
      m_user_input && m_command.type() == ex_command::type_t::FIND &&
      m_tc->ex() != nullptr)
    {
      m_tc->ex()->get_macros().record(m_command.str() + get_text());
    }

    if (input_mode_finish())
    {
      if (m_command.command() != ":.")
      {
        m_tc->ex()->command(
          ":" + std::string(1, m_input) + "|" +
          m_command.command().substr(1, m_command.size() - 3) +
          m_tc->ex()->get_stc()->eol());
      }

      m_tc->frame()->hide_ex_bar();
    }
    else if (m_input != 0)
    {
      event.Skip();
    }
    else if (
      (m_user_input && m_command.type() == ex_command::type_t::FIND) ||
      m_command.exec())
    {
      int focus =
        (m_command.type() == ex_command::type_t::FIND ?
           managed_frame::HIDE_BAR_FORCE_FOCUS_STC :
           managed_frame::HIDE_BAR_FOCUS_STC);

      if (m_command.type() == ex_command::type_t::FIND)
      {
        find_replace_data::get()->set_find_string(get_text());
      }
      else
      {
        TCI().set(m_tc);

        if (m_command.type() == ex_command::type_t::COMMAND)
        {
          if (
            get_text() == "gt" || get_text() == "n" || get_text() == "prev" ||
            get_text().find("ta") == 0)
          {
            focus = managed_frame::HIDE_BAR_FORCE;
          }
          else if (get_text().find("!") == 0)
          {
            focus = managed_frame::HIDE_BAR;
          }
        }
      }

      if (m_input == 0)
      {
        m_tc->frame()->hide_ex_bar(focus);
      }
    }
  });

  bind();
}

wex::textctrl_imp::textctrl_imp(
  textctrl*          tc,
  const std::string& value,
  const data::window& data)
  : wxTextCtrl(
      data.parent(),
      data.id(),
      value,
      data.pos(),
      data.size(),
      data.style() | wxTE_PROCESS_ENTER)
  , m_tc(tc)
  , m_timer(this)
{
  m_command.no_type();
  m_command.set(value);

  bind();
}

void wex::textctrl_imp::bind()
{
  Bind(wxEVT_TIMER, [=](wxTimerEvent& event) {
    wxTextCtrl::SelectAll();
  });
}

void wex::textctrl_imp::cut()
{
  if (!GetStringSelection().empty())
  {
    m_command.handle(this, wxID_CUT);
    Cut();
  }
}

// A GetValue().ToStdString() should suffice, but that
// corrupts a " character.
const std::string wex::textctrl_imp::get_text() const
{
  return m_prefix == nullptr ?
           GetValue().ToStdString() :
           m_command.command().substr(m_command.str().size());
}

bool wex::textctrl_imp::handle(const std::string& command)
{
  const std::string range(command.substr(1));

  m_user_input  = false;
  m_command     = ex_command(m_tc->ex()->get_command()).set(command);
  m_input       = 0;
  m_mode_visual = !range.empty();
  m_control_r   = false;

  m_tc->frame()->pane_set(
    "VIBAR",
    wxAuiPaneInfo().BestSize(-1, GetFont().GetPixelSize().GetHeight() + 5));

  if (m_prefix != nullptr)
  {
    m_prefix->SetLabel(std::string(1, m_command.str().back()));
  }

  switch (m_command.type())
  {
    case ex_command::type_t::CALC:
    case ex_command::type_t::EXEC:
    case ex_command::type_t::FIND_MARGIN:
      set_text(TCI().get());
      SelectAll();
      break;

    case ex_command::type_t::COMMAND:
      if (command == ":!")
      {
        set_text("!");
        SetInsertionPointEnd();
      }
      else if (!TCI().get().empty())
      {
        set_text(
          m_mode_visual && TCI().get().find(range) != 0 ? range + TCI().get() :
                                                          TCI().get());
        SelectAll();
      }
      else
      {
        set_text(range);
        SelectAll();
      }
      break;

    case ex_command::type_t::FIND:
      set_text(
        !m_mode_visual ? m_tc->ex()->get_stc()->get_find_string() :
                         std::string());
      SelectAll();
      break;

    default:
      return false;
  }

  Show();
  SetFocus();

  return true;
}

bool wex::textctrl_imp::handle(char command)
{
  m_control_r = false;
  m_input     = command;

  Clear();

  m_command.reset();

  m_tc->frame()->pane_set(
    "VIBAR",
    wxAuiPaneInfo().BestSize(-1, 4 * GetFont().GetPixelSize().GetHeight() + 5));

  return true;
}

bool wex::textctrl_imp::input_mode_finish() const
{
  if (m_input == 0 || m_command.size() < 2)
  {
    return false;
  }

  const std::string last_two(
    m_command.command().substr(m_command.size() - 2, 2));

  return m_command.command() == ":." || last_two == ".\n" || last_two == ".\r";
}

void wex::textctrl_imp::set_text(const std::string& text)
{
  m_command.reset(text);
  ChangeValue(text);
}

void wex::textctrl_imp::SelectAll()
{
  if (
    m_command.command().find("\"") != std::string::npos ||
    m_command.command().find("\'") != std::string::npos)
  {
    m_timer.StartOnce(500);
  }
  else
  {
    wxTextCtrl::SelectAll();
  }
}

wex::textctrl_input& wex::textctrl_imp::TCI()
{
  switch (m_command.type())
  {
    case ex_command::type_t::CALC:
      return m_calcs;

    case ex_command::type_t::EXEC:
      return m_execs;

    case ex_command::type_t::FIND_MARGIN:
      return m_find_margins;

    default:
      return m_commands;
  }
};
