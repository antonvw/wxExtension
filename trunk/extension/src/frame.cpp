////////////////////////////////////////////////////////////////////////////////
// Name:      frame.cpp
// Purpose:   Implementation of wxExFrame class
// Author:    Anton van Wezenbeek
// Created:   2010-03-26
// RCS-ID:    $Id$
// Copyright: (c) 2010 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/persist/toplevel.h>
#include <wx/extension/frame.h>
#include <wx/extension/frd.h>
#include <wx/extension/grid.h>
#include <wx/extension/lexers.h>
#include <wx/extension/listview.h>
#include <wx/extension/printing.h>
#include <wx/extension/stcfile.h>
#include <wx/extension/tool.h>
#include <wx/extension/util.h>

#if wxUSE_GUI

#if wxUSE_STATUSBAR
wxExStatusBar* wxExFrame::m_StatusBar = NULL;
#endif

#if wxUSE_DRAG_AND_DROP
class FileDropTarget : public wxFileDropTarget
{
public:
  FileDropTarget(wxExFrame* frame) 
    : m_Frame(frame){;};
private:
  virtual bool OnDropFiles(
    wxCoord x, 
    wxCoord y, 
    const wxArrayString& filenames) {
    wxExOpenFiles(m_Frame, filenames);
    return true;}

  wxExFrame* m_Frame;
};
#endif

BEGIN_EVENT_TABLE(wxExFrame, wxFrame)
  EVT_FIND(wxID_ANY, wxExFrame::OnFindDialog)
  EVT_FIND_CLOSE(wxID_ANY, wxExFrame::OnFindDialog)
  EVT_FIND_NEXT(wxID_ANY, wxExFrame::OnFindDialog)
  EVT_FIND_REPLACE(wxID_ANY, wxExFrame::OnFindDialog)
  EVT_FIND_REPLACE_ALL(wxID_ANY, wxExFrame::OnFindDialog)
  EVT_MENU(wxID_FIND, wxExFrame::OnCommand)
  EVT_MENU(wxID_REPLACE, wxExFrame::OnCommand)
  EVT_MENU(ID_EDIT_FIND_NEXT, wxExFrame::OnCommand)
  EVT_MENU(ID_EDIT_FIND_PREVIOUS, wxExFrame::OnCommand)
  EVT_MENU(ID_FOCUS_GRID, wxExFrame::OnCommand)
  EVT_MENU(ID_FOCUS_LISTVIEW, wxExFrame::OnCommand)
  EVT_MENU(ID_FOCUS_STC, wxExFrame::OnCommand)
#if wxUSE_STATUSBAR
  EVT_UPDATE_UI(ID_EDIT_STATUS_BAR, wxExFrame::OnUpdateUI)
#endif
END_EVENT_TABLE()

wxExFrame::wxExFrame(wxWindow* parent,
  wxWindowID id,
  const wxString& title,
  long style,
  const wxString& name)
  : wxFrame(parent, id, title, wxDefaultPosition, wxDefaultSize, style, name)
  , m_FindReplaceDialog(NULL)
  , m_FocusGrid(NULL)
  , m_FocusListView(NULL)
  , m_FocusSTC(NULL)
{
  Initialize();

  SetName("wxExFrame");
  wxPersistentRegisterAndRestore(this);

#if wxUSE_HTML & wxUSE_PRINTING_ARCHITECTURE
  wxExPrinting::Get()->GetHtmlPrinter()->SetParentWindow(this);
#endif
}

wxExFrame::~wxExFrame()
{
  if (m_FindReplaceDialog != NULL)
  {
    m_FindReplaceDialog->Destroy();
  }

#if wxUSE_STATUSBAR
  delete m_StatusBar;
#endif
}

void wxExFrame::FindIn(wxFindDialogEvent& event, wxExGrid* grid)
{
  if (
    event.GetEventType() == wxEVT_COMMAND_FIND ||
    event.GetEventType() == wxEVT_COMMAND_FIND_NEXT)
  {
    grid->FindNext(
      wxExFindReplaceData::Get()->GetFindString(), 
      wxExFindReplaceData::Get()->SearchDown());
  }
  else
  {
    wxFAIL;
  }
}

void wxExFrame::FindIn(wxFindDialogEvent& event, wxExListView* lv)
{
  if (
    event.GetEventType() == wxEVT_COMMAND_FIND ||
    event.GetEventType() == wxEVT_COMMAND_FIND_NEXT)
  {
    lv->FindNext(
      wxExFindReplaceData::Get()->GetFindString(), 
      wxExFindReplaceData::Get()->SearchDown());
  }
  else
  {
    wxFAIL;
  }
}

void wxExFrame::FindIn(wxFindDialogEvent& event, wxExSTC* stc)
{
  auto* frd = wxExFindReplaceData::Get();

  // Match word and regular expression do not work together.
  if (frd->MatchWord())
  {
    frd->SetUseRegularExpression(false);
  }

  if (
    event.GetEventType() == wxEVT_COMMAND_FIND ||
    event.GetEventType() == wxEVT_COMMAND_FIND_NEXT)
  {
    stc->FindNext(frd->SearchDown());
  }
  else if (event.GetEventType() == wxEVT_COMMAND_FIND_REPLACE)
  {
    stc->ReplaceNext(frd->SearchDown());
  }
  else if (event.GetEventType() == wxEVT_COMMAND_FIND_REPLACE_ALL)
  {
    stc->ReplaceAll(
      frd->GetFindString(), 
      frd->GetReplaceString());
  }
  else
  {
    wxFAIL;
  }
}

wxExGrid* wxExFrame::GetFocusedGrid()
{
  wxWindow* win = wxWindow::FindFocus();

  if (win == NULL)
  {
    return NULL;
  }

  return wxDynamicCast(win, wxExGrid);
}

wxExListView* wxExFrame::GetFocusedListView()
{
  wxWindow* win = wxWindow::FindFocus();

  if (win == NULL)
  {
    return NULL;
  }

  return wxDynamicCast(win, wxExListView);
}

wxExSTCFile* wxExFrame::GetFocusedSTC()
{
  wxWindow* win = wxWindow::FindFocus();

  if (win == NULL)
  {
    return NULL;
  }

  return wxDynamicCast(win, wxExSTCFile);
}

void wxExFrame::GetSearchText()
{
  if (m_FocusSTC != NULL)
  {
    m_FocusSTC->GetSearchText();
  }
  else if (m_FocusGrid != NULL)
  {
    m_FocusGrid->GetSearchText();
  }
  else
  {
    auto* stc = GetSTC();

    if (stc != NULL && stc->IsShown())
    {
      stc->GetSearchText();
    }
    else
    {
      wxExGrid* grid = GetGrid();

      if (grid != NULL && grid->IsShown() )
      {
        grid->GetSearchText();
      }
    }
  }
}

void wxExFrame::Initialize()
{
#if wxUSE_DRAG_AND_DROP
  SetDropTarget(new FileDropTarget(this));
#endif

  wxAcceleratorEntry entries[4];
  entries[0].Set(wxACCEL_NORMAL, WXK_F3, ID_EDIT_FIND_NEXT);
  entries[1].Set(wxACCEL_NORMAL, WXK_F4, ID_EDIT_FIND_PREVIOUS);
  entries[2].Set(wxACCEL_NORMAL, WXK_F5, wxID_FIND);
  entries[3].Set(wxACCEL_NORMAL, WXK_F6, wxID_REPLACE);

  wxAcceleratorTable accel(WXSIZEOF(entries), entries);
  SetAcceleratorTable(accel);
}

void wxExFrame::OnCommand(wxCommandEvent& command)
{
  switch (command.GetId())
  {
  case wxID_FIND: 
    GetSearchText();
    
    if (m_FindReplaceDialog != NULL)
    {
      if (m_FindReplaceDialog->GetWindowStyle() & wxFR_REPLACEDIALOG)
      {
        m_FindReplaceDialog->Destroy();
        m_FindReplaceDialog = NULL;
      }
    }
    
    if (m_FindReplaceDialog == NULL)
    {
      m_FindReplaceDialog = new wxFindReplaceDialog(
        this, wxExFindReplaceData::Get(), _("Find")); 
    }
    
    m_FindReplaceDialog->Show();
    break;
    
  case wxID_REPLACE: 
    GetSearchText();
    
    if (m_FindReplaceDialog != NULL)
    {
      if (!(m_FindReplaceDialog->GetWindowStyle() & wxFR_REPLACEDIALOG))
      {
        m_FindReplaceDialog->Destroy();
        m_FindReplaceDialog = NULL;
      }
    }
    
    if (m_FindReplaceDialog == NULL)
    {
      m_FindReplaceDialog = new wxFindReplaceDialog(
        this, 
        wxExFindReplaceData::Get(),
        _("Replace"), 
        wxFR_REPLACEDIALOG); 
    }
      
    m_FindReplaceDialog->Show();
    break;
    
  case ID_EDIT_FIND_NEXT: 
  case ID_EDIT_FIND_PREVIOUS: 
    if (m_FocusSTC != NULL)
    {
      m_FocusSTC->GetSearchText();
      m_FocusSTC->FindNext(command.GetId() == ID_EDIT_FIND_NEXT); 
    }
    else if (m_FocusListView != NULL)
    {
      m_FocusListView->FindNext(
        wxExFindReplaceData::Get()->GetFindString(), 
        command.GetId() == ID_EDIT_FIND_NEXT); 
    }
    else if (m_FocusGrid != NULL)
    {
      m_FocusGrid->GetSearchText();
      m_FocusGrid->FindNext(
        wxExFindReplaceData::Get()->GetFindString(), 
        command.GetId() == ID_EDIT_FIND_NEXT); 
    }
    break;

  case ID_FOCUS_GRID:
    m_FocusGrid = (wxExGrid*)command.GetEventObject();
    break;

  case ID_FOCUS_LISTVIEW:
    m_FocusListView = (wxExListView*)command.GetEventObject();;
    break;

  case ID_FOCUS_STC: 
    m_FocusSTC = (wxExSTC*)command.GetEventObject();;
    break;

  default: wxFAIL; break;
  }
}

#if wxUSE_STATUSBAR
wxStatusBar* wxExFrame::OnCreateStatusBar(
  int number,
  long style,
  wxWindowID id,
  const wxString& name)
{
  m_StatusBar = new wxExStatusBar(this, id, style, name);
  m_StatusBar->SetFieldsCount(number);
  return m_StatusBar;
}
#endif

void wxExFrame::OnFindDialog(wxFindDialogEvent& event)
{
  wxASSERT(m_FindReplaceDialog != NULL);

  if (event.GetEventType() == wxEVT_COMMAND_FIND_CLOSE)
  {
    // Hiding instead of destroying, does not 
    // show the dialog next time.
    m_FindReplaceDialog->Destroy();
    m_FindReplaceDialog = NULL;
    return;
  }

  if (m_FocusSTC != NULL)
  {
    FindIn(event, m_FocusSTC);
  }
  else if (m_FocusListView != NULL)
  {
    FindIn(event, m_FocusListView);
  }
  else if (m_FocusGrid != NULL)
  {
    FindIn(event, m_FocusGrid);
  }
  else
  {
    auto* stc = GetSTC();
    auto* lv = GetListView();
    auto* grid = GetGrid();

    if (stc != NULL && stc->IsShown())
    {
      FindIn(event, stc);
    }
    else if (lv != NULL && lv->IsShown())
    {
      FindIn(event, lv);
    }
    else if (grid != NULL && grid->IsShown())
    {
      FindIn(event, grid);
    }
  }
}

void wxExFrame::OnUpdateUI(wxUpdateUIEvent& event)
{
  auto* stc = GetFocusedSTC();
  if (stc == NULL) return;

  switch (event.GetId())
  {
#if wxUSE_STATUSBAR
  case ID_EDIT_STATUS_BAR: stc->UpdateStatusBar("PaneLines"); break;
#endif
  default:
    wxFAIL;
    break;
  }
}

bool wxExFrame::OpenFile(
  const wxExFileName& filename,
  int line_number,
  const wxString& match,
  long flags)
{
  auto* stc = GetFocusedSTC();

  if (stc != NULL)
  {
    return stc->Open(filename, line_number, match, flags);
  }

  return false;
}

#if wxUSE_STATUSBAR
void wxExFrame::SetupStatusBar(
  const std::vector<wxExPane>& panes,
  long style,
  wxWindowID id,
  const wxString& name)
{
  wxFrame::CreateStatusBar(panes.size(), style, id, name);
  m_StatusBar->SetPanes(panes);
}
#endif // wxUSE_STATUSBAR

#if wxUSE_STATUSBAR
void wxExFrame::StatusBarDoubleClicked(
  const wxString& pane)
{
  if (pane == "PaneLines")
  {
    auto* stc = GetSTC();
    if (stc != NULL) stc->GotoDialog();
  }
  else if (pane == "PaneLexer")
  {
    auto* stc = GetSTC();

    if (stc != NULL && wxExLexers::Get()->Count() > 0)
    {
      wxString lexer = stc->GetLexer().GetScintillaLexer();

      if (wxExLexers::Get()->ShowDialog(this, lexer, _("Enter Lexer")))
      {
        stc->SetLexer(lexer);
      }
    }
  }
  else if (pane == "PaneFileType")
  {
    auto* stc = GetSTC();
    if (stc != NULL) stc->FileTypeMenu();
  }
  else if (pane == "PaneItems")
  {
    wxExListView* list = GetListView();
    if (list != NULL) list->GotoDialog();
  }
  else
  {
    // Clicking on another field, do nothing. 
  }
}

// This is a static method, so you cannot call wxFrame::SetStatusText.
void wxExFrame::StatusText(const wxString& text, const wxString& pane)
{
  if (m_StatusBar != NULL)
  {
    m_StatusBar->SetStatusText(text, pane);
  }
}

void wxExFrame::StatusText(const wxExFileName& filename, long flags)
{
  wxString text; // clear status bar for empty or not existing or not initialized file names

  if (filename.IsOk())
  {
    const wxString path = (flags & STAT_FULLPATH
      ? filename.GetFullPath(): filename.GetFullName());

    text += path;

    if (filename.GetStat().IsOk())
    {
      const wxString what = (flags & STAT_SYNC
        ? _("Synchronized"): _("Modified"));
      const wxString time = (flags & STAT_SYNC
        ? wxDateTime::Now().Format(): filename.GetStat().GetModificationTime());
      text += " " + what + " " + time;
    }
  }

  StatusText(text);
}

#endif // wxUSE_STATUSBAR
#endif // wxUSE_GUI
