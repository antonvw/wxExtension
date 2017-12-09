////////////////////////////////////////////////////////////////////////////////
// Name:      vi-macros-fsm.cpp
// Purpose:   Implementation of class wxExViMacrosFSM
// Author:    Anton van Wezenbeek
// Copyright: (c) 2017 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <fstream>
#include <easylogging++.h>
#include <pugixml.hpp>
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/config.h>
#include <wx/extension/ex.h>
#include <wx/extension/managedframe.h>
#include <wx/extension/stc.h>
#include <wx/extension/util.h>
#include <wx/extension/variable.h>
#include <wx/extension/vi-macros.h>
#include "vi-macros-fsm.h"

std::string wxExViMacrosFSM::m_macro;

wxExViMacrosFSM::wxExViMacrosFSM()
{
  // from-state, to-state, trigger, guard, action
  m_fsm.add_transitions({
    // expanding
    {States::EXPANDING_TEMPLATE, States::IDLE, Triggers::DONE, 
      nullptr,
      nullptr},
    {States::EXPANDING_VARIABLE, States::IDLE, Triggers::DONE, 
      nullptr,
      nullptr},
    // idle
    {States::IDLE, States::EXPANDING_TEMPLATE, Triggers::EXPAND_TEMPLATE, 
      [&]{
        if (m_variable.GetType() != wxExVariable::VARIABLE_TEMPLATE)
        {
          LOG(ERROR) << "variable: " << m_variable.GetName() << " is no template";
          return false;
        }
        if (m_variable.GetValue().empty())
        {
          LOG(ERROR) << "template variable: " << m_variable.GetName() << " is empty";
          return false;
        }
        return true;},
      [&]{ExpandingTemplate();}},
    {States::IDLE, States::EXPANDING_VARIABLE, Triggers::EXPAND_VARIABLE, 
      nullptr,
      [&]{ExpandingVariable();}},
    {States::IDLE, States::PLAYINGBACK, Triggers::PLAYBACK, 
      [&]{return m_count > 0;}, 
      [&]{Playback();}},
    {States::IDLE, States::RECORDING, Triggers::RECORD, 
      nullptr,
      [&]{StartRecording();}},
    // playingback
    {States::PLAYINGBACK, States::IDLE, Triggers::DONE, 
      nullptr, 
      nullptr},
    {States::PLAYINGBACK, States::PLAYINGBACK, Triggers::PLAYBACK, 
      [&]{return m_count > 0 && m_macro != wxExViMacros::m_Macro;}, 
      [&]{Playback();}},
    // recording
    {States::PLAYINGBACK_WHILE_RECORDING, States::RECORDING, Triggers::DONE, 
      nullptr,
      nullptr},
    {States::RECORDING, States::IDLE, Triggers::RECORD, 
      nullptr, 
      [&]{StopRecording();}},
    {States::RECORDING, States::PLAYINGBACK_WHILE_RECORDING, Triggers::PLAYBACK, 
      [&]{return m_count > 0 && m_macro != wxExViMacros::m_Macro;}, 
      [&]{Playback();}}});

  m_fsm.add_debug_fn(Verbose);
}

void wxExViMacrosFSM::AskForInput() 
{
  for (auto& it : wxExViMacros::m_Variables)
  {
    it.second.AskForInput();
  }
}

bool wxExViMacrosFSM::Execute(
  Triggers trigger, const std::string& macro, wxExEx* ex, int repeat) 
{
  m_count = repeat;
  m_error = false;
  m_ex = ex;
  m_macro = macro;

  if (m_fsm.execute(trigger) != FSM::Fsm_Success)
  {
    return false;
  }

  if (m_fsm.state() == States::PLAYINGBACK || 
      m_fsm.state() == States::PLAYINGBACK_WHILE_RECORDING ||
      m_fsm.state() == States::EXPANDING_VARIABLE)
  {
    m_fsm.execute(Triggers::DONE);
  }

  wxExFrame::StatusText(wxExViMacros::m_Macro, "PaneMacro");
  wxExFrame::StatusText(State(), "PaneMode");

  if (m_ex != nullptr)
  {
    ((wxExStatusBar *)m_ex->GetFrame()->GetStatusBar())->ShowField(
      "PaneMode", 
      m_fsm.state() != States::IDLE && wxConfigBase::Get()->ReadBool(_("Show mode"), false));
  }

  return !m_error;
}

bool wxExViMacrosFSM::Expand(
  wxExEx* ex, const wxExVariable& v, std::string& expanded)
{
  m_error = false;
  m_ex = ex;
  m_expanded = &expanded;
  m_variable = v;

  FSM::Fsm_Errors r1 = m_fsm.execute(Triggers::EXPAND_TEMPLATE);
  FSM::Fsm_Errors r2 = m_fsm.execute(Triggers::DONE);

  return r1 == FSM::Fsm_Success && r2 == FSM::Fsm_Success && !m_error;
}

void wxExViMacrosFSM::ExpandingTemplate()
{
  // Read the file (file name is in variable value), expand
  // all macro variables in it, and set expanded to the result.
  const wxExPath filename(wxExConfigDir(), m_variable.GetValue());

  std::ifstream ifs(filename.Path());
  
  if (!ifs.is_open())
  {
    LOG(ERROR) << "could not open template file: " << filename.Path().string();
    m_error = true;
    return;
  }

  AskForInput();

  // Keep current macro, in case you cancel expanding,
  // this one is restored.
  std::string macro = wxExViMacros::m_Macro;
  char c;

  while (ifs.get(c) && !m_error)
  {
    if (c != '@')
    {
      *m_expanded += c;
    }
    else
    {
      std::string variable;
      bool completed = false;
      
      while (!completed && ifs.get(c)) 
      {
        if (c != '@')
        {
          variable += c;
        }
        else
        {
          completed = true;
        }
      }
      
      if (!completed)
      {
        m_error = true;
      }
      // Prevent recursion.
      else if (variable == m_variable.GetName())
      {
        m_error = true;
      }
      else
      {
        std::string value;
        
        if (!ExpandTo(variable, value))
        {
          m_error = true;
        }
        
        *m_expanded += value;
      }
    }
  }
  
  // Set back to normal value.  
  AskForInput();
    
  wxExViMacros::m_Macro = m_variable.GetName();
  wxExFrame::StatusText(wxExViMacros::m_Macro, "PaneMacro");
    
  wxLogStatus(_("Macro expanded"));
}

void wxExViMacrosFSM::ExpandingVariable()
{
  pugi::xml_node node;
  wxExVariable var;
  const auto& it = wxExViMacros::m_Variables.find(m_macro);
    
  if (it == wxExViMacros::m_Variables.end())
  {
    var = wxExVariable(var);
    wxExViMacros::m_Variables.insert({m_macro, var});
    wxLogStatus(_("Added variable") + ": "  +  m_macro);
    node = wxExViMacros::m_doc.document_element().append_child("variable");
  }
  else
  {
    try
    {
      const std::string query("//variable[@name='" + m_macro + "']");
      pugi::xpath_node xp = wxExViMacros::m_doc.document_element().select_node(query.c_str());

      if (xp && xp.node())
      {
        node = xp.node();
      }
    }
    catch (pugi::xpath_exception& e)
    {
      LOG(ERROR) << e.what();
    }

    var = it->second;
  }

  if (var.Expand(m_ex))
  {
    if (var.IsModified())
    {
      var.Save(node);
      wxExViMacros::m_IsModified = true;
    }
    
    wxLogStatus(_("Variable expanded"));
  
    if (Get() == States::PLAYINGBACK)
    {
      wxExViMacros::m_Macro = m_macro;
    }
  }
  else
  {
    // Now only show log status if this is no input variable,
    // as it was cancelled in that case.    
    if (!it->second.IsInput())
    {
      wxLogStatus(_("Could not expand variable") + ": "  +  m_macro);
      m_error = true;
    }
  }
}  

bool wxExViMacrosFSM::ExpandTo(const std::string& variable, std::string& value)
{
  const auto& it = wxExViMacros::m_Variables.find(variable);
    
  bool ok;
    
  if (it == wxExViMacros::m_Variables.end())
  {
    auto ret = wxExViMacros::m_Variables.insert({variable, wxExVariable(variable)});
      
    wxLogStatus(_("Added variable") + ": "  +  variable);
    
    ok = ret.first->second.Expand(m_ex, value);
    ret.first->second.SkipInput();
    
    if (ret.first->second.IsModified())
    {
      pugi::xml_node node = wxExViMacros::m_doc.document_element().child("variable");
      pugi::xml_node app = node.append_child(variable.c_str());
      ret.first->second.Save(app);
      wxExViMacros::m_IsModified = true;
    }
  }
  else
  {
    ok = it->second.Expand(m_ex, value);
    it->second.SkipInput();

    if (it->second.IsModified())
    {
      pugi::xml_node node = wxExViMacros::m_doc.document_element().child(variable.c_str());
      it->second.Save(node);
      wxExViMacros::m_IsModified = true;
    }
  }
  
  wxLogStatus(!ok ?
    _("Could not expand variable") + ": "  +  variable: _("Variable expanded"));
  
  return ok;
}

void wxExViMacrosFSM::Playback()
{
  m_ex->GetSTC()->BeginUndoAction();
  
  wxBusyCursor wait;
    
  AskForInput();
  
  const auto& macro_commands(wxExViMacros::m_Macros[m_macro]);
  
  for (int i = 0; i < m_count && !m_error; i++)
  {
    for (const auto& it : macro_commands)
    { 
      if (!m_ex->Command(it))
      {
        m_error = true;
        wxLogStatus(_("Macro aborted at '") + it + "'");
        break;
      }
    }
  }

  m_ex->GetSTC()->EndUndoAction();

  if (!m_error)
  {
    if (Get() == States::PLAYINGBACK)
    {
      wxExViMacros::m_Macro = m_macro;
    }

    wxLogStatus(_("Macro played back"));
  }
}

void wxExViMacrosFSM::StartRecording()
{
  wxExViMacros::m_IsModified = true;
  
  if (m_macro.size() == 1)
  {
    // We only use lower case macro's, to be able to
    // append to them using.
    wxExViMacros::m_Macro = m_macro;
    std::transform(wxExViMacros::m_Macro.begin(), wxExViMacros::m_Macro.end(), wxExViMacros::m_Macro.begin(), ::tolower);
  
    // Clear macro if it is lower case
    // (otherwise append to the macro).
    if (islower(m_macro[0]))
    {
      wxExViMacros::m_Macros[wxExViMacros::m_Macro].clear();
    }
  }
  else
  {
    wxExViMacros::m_Macro = m_macro;
    wxExViMacros::m_Macros[wxExViMacros::m_Macro].clear();
  }

  wxLogStatus(_("Macro recording"));
}

void wxExViMacrosFSM::StopRecording()
{
  if (!wxExViMacros::Get(wxExViMacros::m_Macro).empty())
  {
    wxExViMacros::SaveMacro(wxExViMacros::m_Macro);
    wxLogStatus(wxString::Format(_("Macro '%s' is recorded"), wxExViMacros::m_Macro.c_str()));
  }
  else
  {
    wxExViMacros::m_Macros.erase(wxExViMacros::m_Macro);
    wxExViMacros::m_Macro.clear();
    wxLogStatus(wxEmptyString);
  }
}

void wxExViMacrosFSM::Verbose(States from, States to, Triggers trigger)
{
  VLOG(2) << 
    "vi macro " << m_macro <<
    " trigger " << Trigger(trigger) <<
    " state from " << State(from) << 
    " to " << State(to);
}
