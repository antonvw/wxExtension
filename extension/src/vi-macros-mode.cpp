////////////////////////////////////////////////////////////////////////////////
// Name:      vi-macros-mode.cpp
// Purpose:   Implementation of class wxExViMacrosMode
// Author:    Anton van Wezenbeek
// Copyright: (c) 2017 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/extension/vi-macros-mode.h>
#include <wx/extension/ex.h>
#include <wx/extension/frame.h>
#include <wx/extension/stc.h>
#include <wx/extension/util.h>
#include <wx/extension/vi-macros.h>
#include "vi-macros-fsm.h"

bool ShowDialog(wxWindow* parent, std::string& macro)
{
  const auto& v(wxExViMacros::Get());
  
  if (v.empty())
  {
    return false;
  }

  wxArrayString macros;
  macros.resize(v.size());
  copy(v.begin(), v.end(), macros.begin());
  
  wxSingleChoiceDialog dialog(parent,
    _("Input") + ":", 
    _("Select Macro"),
    macros);
    
  const int index = macros.Index(wxExViMacros::GetMacro());

  if (index != wxNOT_FOUND)
  {
    dialog.SetSelection(index);
  }

  if (dialog.ShowModal() != wxID_OK)
  {
    return false;
  }
  
  macro = dialog.GetStringSelection();

  return true;
}

wxExViMacrosMode::wxExViMacrosMode()
  : m_FSM(new wxExViMacrosFSM())
{
}

wxExViMacrosMode::~wxExViMacrosMode()
{
  delete m_FSM;
}

bool wxExViMacrosMode::Expand(
  wxExEx* ex, const wxExVariable& v, std::string& expanded)
{
  return m_FSM->Expand(ex, v, expanded);
}

bool wxExViMacrosMode::IsPlayback() const 
{
  return 
    m_FSM->Get() == States::PLAYINGBACK ||
    m_FSM->Get() == States::PLAYINGBACK_WHILE_RECORDING;
}
  
bool wxExViMacrosMode::IsRecording() const 
{
  return m_FSM->Get() == States::RECORDING;
}

const std::string wxExViMacrosMode::String() const
{
  return m_FSM->State();
}

int wxExViMacrosMode::Transition(
  const std::string& command, wxExEx* ex, bool complete, int repeat)
{
  if (command.empty() || repeat <= 0)
  {
    return 0;
  }

  Triggers trigger = Triggers::DONE;
  wxWindow* parent = (ex != nullptr ? ex->GetSTC(): wxTheApp->GetTopWindow());

  std::string macro(command);

  switch (macro[0])
  {
    case 'q': 
      trigger = Triggers::RECORD;

      macro = macro.substr(1);

      if (complete)
      {
        if (macro.empty())
        {
          wxTextEntryDialog dlg(parent,
            _("Input") + ":",
            _("Enter Macro"),
            wxExViMacros::GetMacro());
        
          wxTextValidator validator(wxFILTER_ALPHANUMERIC);
          dlg.SetTextValidator(validator);
        
          if (dlg.ShowModal() != wxID_OK)
          {
            return 0;
          }
          
          macro = dlg.GetValue();
        }
      }
      else if (m_FSM->Get() == States::IDLE && macro.empty())
      {
        return 0;
      }
    break;

    case '@': 
      if (macro == "@")
      {
        if (complete)
        {
          if (!ShowDialog(parent, macro))
          {
            return 1;
          }
        }
        else
        {
          return 0;
        }
      }
      else if (macro == "@@") 
      {
        if ((macro = wxExViMacros::GetMacro()) == std::string() &&
            !ShowDialog(parent, macro))
        {
          return 2;
        }
      }
      else if (wxExRegAfter("@", macro))
      {
        macro = std::string(1, macro.back());

        if (!wxExViMacros::IsRecorded(macro))
        {
          return 2;
        }
      }
      else
      {
        std::vector <std::string> v;

        if (wxExMatch("@([a-zA-Z].+)@", macro, v) > 0)
        {
          macro = v[0];
        }
        else if (wxExViMacros::StartsWith(macro.substr(1)))
        {
          std::string s;
          if (wxExAutoComplete(macro.substr(1), wxExViMacros::Get(), s))
          {
            wxExFrame::StatusText(s, "PaneMacro");
            macro = s;
          }
          else
          {
            wxExFrame::StatusText(macro.substr(1), "PaneMacro");
            return 0;
          }
        }
        else
        {
          wxExFrame::StatusText(wxExViMacros::GetMacro(), "PaneMacro");
          return macro.size();
        }
      }

      trigger = !wxExViMacros::IsRecordedMacro(macro) ? 
        Triggers::EXPAND_VARIABLE: Triggers::PLAYBACK; 
    break;

    default: return 0;
  }

  // The STC pointer might change because of playback.
  // Normally that is ok, but if playback fails, and
  // the m_STC was changed (because of e.g. :n), 
  // you would end up with an incorrect pointer.
  // Therefore set it back after the playback.
  wxExSTC* stc = (ex != nullptr ? ex->m_STC: nullptr);

  if (!m_FSM->Execute(trigger, macro, ex, repeat))
  {
    return 0;
  }

  if (stc != nullptr) ex->m_STC = stc;

  return command.size();
}
