////////////////////////////////////////////////////////////////////////////////
// Name:      statusbar.cpp
// Purpose:   Implementation of wxExStatusbar class
// Author:    Anton van Wezenbeek
// Copyright: (c) 2017 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/config.h>
#include <wx/extension/statusbar.h>
#include <wx/extension/frame.h>

#if wxUSE_GUI
#if wxUSE_STATUSBAR

const int FIELD_NOT_SHOWN = -1;

std::string ConfigName(wxExStatusBar* sb, const std::string item, int f)
{
  return "SB" + sb->GetField(f).GetName() + item;
}

void wxExStatusBarPane::Show(bool show)
{
  m_IsShown = show;
  
  if (show) 
  {
    m_HiddenText.clear();
  }
  else
  {
    m_HiddenText = GetText();
  }
}

wxExStatusBar::wxExStatusBar(
  wxExFrame* parent,
  wxWindowID id,
  long style,
  const wxString& name)
  : wxStatusBar(parent, id, style, name)
  , m_Frame(parent)
{
  // The statusbar is not managed by Aui, so show/hide it explicitly.    
  Show(wxConfigBase::Get()->ReadBool("ShowStatusBar", true));
}

wxExStatusBar::~wxExStatusBar()
{ 
  wxConfigBase::Get()->Write("ShowStatusBar", IsShown());

  for (int i = 0; i < GetFieldsCount(); i++)
  {
    wxConfigBase::Get()->Write(ConfigName(this, "Style", i), GetField(i).GetStyle());
    wxConfigBase::Get()->Write(ConfigName(this, "Width", i), GetField(i).GetWidth());
  }
}

const wxExStatusBarPane& wxExStatusBar::GetField(int n) const
{
  return m_Panes[n];
}

bool wxExStatusBar::GetFieldNo(
  const std::string& field, int& shown_pane_no, int& pane_no) const
{
  const std::string use_field = field.empty() ? "PaneText": field;
  shown_pane_no = 0;
  pane_no = 0;
  
  for (const auto& it : m_Panes)
  {
    if (it.IsShown())
    {
      if (it.GetName() == use_field)
      {
        return true;
      }
      
      shown_pane_no++;
    }
    else
    {
      if (it.GetName() == use_field)
      {
        shown_pane_no = FIELD_NOT_SHOWN;
        return true;
      }
    }
  
    pane_no++;
  }
  
  return false;
}
  
const std::string wxExStatusBar::GetStatusText(const std::string& field) const
{
  int shown_pane_no, dummy;
  return !GetFieldNo(field, shown_pane_no, dummy) || shown_pane_no == FIELD_NOT_SHOWN ?
    // Do not show error, as you might explicitly want to ignore messages.
    std::string(): wxStatusBar::GetStatusText(shown_pane_no).ToStdString();
}

void wxExStatusBar::Handle(wxMouseEvent& event, const wxExStatusBarPane& pane)
{
  if (event.LeftUp())
  {
    m_Frame->StatusBarClicked(pane.GetName());
  }
  else if (event.RightUp())
  {
    m_Frame->StatusBarClickedRight(pane.GetName());
  }
  // Show tooltip if tooltip is available, and not yet presented.
  else if (event.Moving())
  {
#if wxUSE_TOOLTIPS
    const wxString tooltip = GetToolTipText();
              
    if (pane.GetHelpText().empty())
    {
      if (!tooltip.empty())
      {
        UnsetToolTip();
      }
    }
    else if (tooltip != pane.GetHelpText())
    {
      SetToolTip(pane.GetHelpText());
    }
#endif    
  }
}

void wxExStatusBar::OnMouse(wxMouseEvent& event)
{
  event.Skip();

  int fieldno = 0;
  
  for (const auto& it : m_Panes)
  {
    if (it.IsShown())
    {
      wxRect rect;
      
      if (GetFieldRect(fieldno, rect))
      {
        if (rect.Contains(event.GetPosition()))
        {
          Handle(event, it);
          return;
        }
      }
      
      fieldno++;
    }
  }
}

void wxExStatusBar::SetFields(const std::vector<wxExStatusBarPane>& fields)
{
  m_Panes = fields;
    
  int* styles = new int[fields.size()];
  int* widths = new int[fields.size()];

  for (size_t i = 0; i < fields.size(); i++)
  {
    styles[i] = wxConfigBase::Get()->ReadLong(ConfigName(this, "Style", i),
      fields[i].GetStyle());
    m_Panes[i].SetStyle(styles[i]);
    widths[i] = wxConfigBase::Get()->ReadLong(ConfigName(this, "Width", i),
      fields[i].GetWidth());
    m_Panes[i].SetWidth(widths[i]);
  }
  
  SetFieldsCount(fields.size(), widths);
  SetStatusStyles(fields.size(), styles);

  delete[] styles;
  delete[] widths;

  Bind(wxEVT_LEFT_UP, &wxExStatusBar::OnMouse, this);
  Bind(wxEVT_RIGHT_UP, &wxExStatusBar::OnMouse, this);
  Bind(wxEVT_MOTION, &wxExStatusBar::OnMouse, this);
}

bool wxExStatusBar::SetStatusText(
  const std::string& text, const std::string& field)
{
  int shown_pane_no, pane_no;

  if (!GetFieldNo(field, shown_pane_no, pane_no))
  {
    // Do not show error, as you might explicitly want to ignore messages.
    return false;
  }

  if (shown_pane_no == FIELD_NOT_SHOWN)
  {
    m_Panes[pane_no].SetHiddenText(text);
    return false;
  }
  else
  {
    m_Panes[pane_no].SetText(text);
    
    // wxStatusBar checks whether new text differs from current,
    // and does nothing if the same to avoid flicker.
    wxStatusBar::SetStatusText(text, shown_pane_no);
    return true;
  }
}

bool wxExStatusBar::ShowField(const std::string& field, bool show)
{
  wxASSERT(!m_Panes.empty());
  
  int* widths = new int[m_Panes.size()];
  int* styles = new int[m_Panes.size()];
  int panes_shown = 0;
  std::vector<std::string> changes;
  bool changed = false;

  for (auto& it : m_Panes)
  {
    if (it.GetName() == field)
    {
      if (show)
      {
        if (!it.IsShown())
        {
          changes.emplace_back(it.GetHiddenText());
          
          it.Show(true);
          
          for (int j = panes_shown; j < GetFieldsCount(); j++)
          {
            changes.emplace_back(wxStatusBar::GetStatusText(j));
          }
        }
        
        widths[panes_shown] = it.GetWidth();
        styles[panes_shown] = it.GetStyle();
        
        panes_shown++;
      }
      else
      {
        if (it.IsShown())
        {
          it.Show(false);
          changed = true;
          
          for (int j = panes_shown + 1; j < GetFieldsCount(); j++)
          {
            changes.emplace_back(wxStatusBar::GetStatusText(j));
          }
        }
      }
    }
    else
    {
      if (it.IsShown())
      {
        widths[panes_shown] = it.GetWidth();
        styles[panes_shown] = it.GetStyle();
        
        panes_shown++;
      }
    }
  }

  if (!changes.empty() || changed)
  {
    SetFieldsCount(panes_shown, widths);
    SetStatusStyles(panes_shown, styles);

    if (!changes.empty())
    {
      int z = 0;
      for (int j = changes.size() - 1; j >= 0; j--)
      {
        wxStatusBar::SetStatusText(changes[j], GetFieldsCount() - 1 - z);
        z++;
      }
    }
  }

  delete[] styles;
  delete[] widths;
  
  return !changes.empty() || changed;
}

#endif // wxUSE_STATUSBAR
#endif // wxUSE_GUI
