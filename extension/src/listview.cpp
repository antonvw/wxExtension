////////////////////////////////////////////////////////////////////////////////
// Name:      listview.cpp
// Purpose:   Implementation of wxExListView and related classes
// Author:    Anton van Wezenbeek
// Copyright: (c) 2017 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <cctype>
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/config.h>
#include <wx/dnd.h> 
#include <wx/fdrepdlg.h> // for wxFindDialogEvent
#include <wx/numdlg.h> // for wxGetNumberFromUser
#include <wx/generic/dirctrlg.h> // for wxTheFileIconsTable
#include <wx/imaglist.h>
#include <wx/extension/listview.h>
#include <wx/extension/defs.h>
#include <wx/extension/frame.h>
#include <wx/extension/frd.h>
#include <wx/extension/interruptable.h>
#include <wx/extension/item.h>
#include <wx/extension/itemdlg.h>
#include <wx/extension/lexer.h>
#include <wx/extension/listitem.h>
#include <wx/extension/menu.h>
#include <wx/extension/printing.h>
#include <wx/extension/tokenizer.h>
#include <wx/extension/util.h>

#if wxUSE_GUI

class ListViewDefaults : public wxExConfigDefaults
{
public:
  ListViewDefaults() 
  : wxExConfigDefaults({
    {_("Background colour"), ITEM_COLOURPICKERWIDGET, *wxWHITE},
    {_("Context size"), ITEM_SPINCTRL, 10l},
    {_("Foreground colour"), ITEM_COLOURPICKERWIDGET, *wxBLACK},
    {_("List font"), ITEM_FONTPICKERCTRL, wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT)},
    {_("List tab font"), ITEM_FONTPICKERCTRL, wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT)},
    {_("Readonly colour"), ITEM_COLOURPICKERWIDGET, *wxLIGHT_GREY},
    {_("Header"), ITEM_CHECKBOX, true}}) {;};
};
  
#if wxUSE_DRAG_AND_DROP
// FileDropTarget is already used by wxExFrame.
class DropTarget : public wxFileDropTarget
{
public:
  explicit DropTarget(wxExListView* lv) {m_ListView = lv;}
  virtual bool OnDropFiles(wxCoord x, wxCoord y, 
    const wxArrayString& filenames) override
  {
    // Only drop text if nothing is selected,
    // so dropping on your own selection is impossible.
    if (m_ListView->GetSelectedItemCount() == 0)
    {
      for (size_t i = 0; i < filenames.GetCount(); i++)
      {
        m_ListView->ItemFromText(filenames[i].ToStdString());
      }
    
      return true;
    }
    else
    {
      return false;
    }
  };
private:
  wxExListView* m_ListView;
};

// A text drop target, is not used, but could
// be used instead of the file drop target.
class TextDropTarget : public wxTextDropTarget
{
public:
  explicit TextDropTarget(wxExListView* lv) {m_ListView = lv;}
private:
  virtual bool OnDropText(
    wxCoord x, 
    wxCoord y, 
    const wxString& text) override {
      // Only drop text if nothing is selected,
      // so dropping on your own selection is impossible.
      return (m_ListView->GetSelectedItemCount() == 0 ?
        m_ListView->ItemFromText(text.ToStdString()): false);};
        
  wxExListView* m_ListView;
};
#endif

wxExColumn::wxExColumn()
{
  SetColumn(-1);
}

wxExColumn::wxExColumn(
  const std::string& name,
  wxExColumn::wxExColumnType type,
  int width)
  : m_Type(type)
{
  wxListColumnFormat align = wxLIST_FORMAT_RIGHT;

  switch (m_Type)
  {
    case wxExColumn::COL_FLOAT: 
      align = wxLIST_FORMAT_RIGHT; 
      if (width == 0) width = 80; 
      break;
      
    case wxExColumn::COL_INT: 
      align = wxLIST_FORMAT_RIGHT;
      if (width == 0) width = 60; 
      break;
      
    case wxExColumn::COL_STRING: 
      align = wxLIST_FORMAT_LEFT;  
      if (width == 0) width = 100; 
      break;
      
    case wxExColumn::COL_DATE: 
      align = wxLIST_FORMAT_LEFT;  
      if (width == 0) width = 150; 
      break;
      
    default: wxFAIL;
  }

  SetColumn(-1); // default value, is set when inserting the col
  SetText(name);
  SetAlign(align);
  SetWidth(width);
}

void wxExColumn::SetIsSortedAscending(wxExSortType type)
{
  switch (type)
  {
    case SORT_ASCENDING: m_IsSortedAscending = true; break;
    case SORT_DESCENDING: m_IsSortedAscending = false; break;
    case SORT_KEEP: break;
    case SORT_TOGGLE: m_IsSortedAscending = !m_IsSortedAscending; break;
    default: wxFAIL; break;
  }
}

// wxWindow::NewControlId() is negative...
const wxWindowID ID_COL_FIRST = 1000;

wxExItemDialog* wxExListView::m_ConfigDialog = nullptr;

wxExListView::wxExListView(const wxExListViewData& data)
  : wxListView(
      data.Window().Parent(), 
      data.Window().Id(), 
      data.Window().Pos(), 
      data.Window().Size(), 
      data.Window().Style() == DATA_NUMBER_NOT_SET ? wxLC_REPORT: data.Window().Style(), 
      data.Control().Validator() != nullptr ? *data.Control().Validator(): wxDefaultValidator, 
      data.Window().Name())
  , m_ImageHeight(16) // not used if IMAGE_FILE_ICON is used, then 16 is fixed
  , m_ImageWidth(16)
  , m_Data(this, wxExListViewData(data).Image(data.Type() == LIST_NONE ? 
      data.Image(): IMAGE_FILE_ICON))
{
  ConfigGet();

  m_Data.Inject();

#if wxUSE_DRAG_AND_DROP
  // We can only have one drop target, we use file drop target,
  // as list items can also be copied and pasted.
  SetDropTarget(new DropTarget(this));
#endif

  wxAcceleratorEntry entries[4];

  entries[0].Set(wxACCEL_NORMAL, WXK_DELETE, wxID_DELETE);
  entries[1].Set(wxACCEL_CTRL, WXK_INSERT, wxID_COPY);
  entries[2].Set(wxACCEL_SHIFT, WXK_INSERT, wxID_PASTE);
  entries[3].Set(wxACCEL_SHIFT, WXK_DELETE, wxID_CUT);

  wxAcceleratorTable accel(WXSIZEOF(entries), entries);
  SetAcceleratorTable(accel);
  
  switch (m_Data.Image())
  {
    case IMAGE_NONE: break;
    case IMAGE_ART:
    case IMAGE_OWN:
      AssignImageList(
        new wxImageList(m_ImageWidth, m_ImageHeight, true, 0), 
        wxIMAGE_LIST_SMALL);
      break;
    case IMAGE_FILE_ICON:
      SetImageList(
        wxTheFileIconsTable->GetSmallImageList(), wxIMAGE_LIST_SMALL);
      break;
    default:
      wxFAIL;
  }

#if wxUSE_STATUSBAR
  wxExFrame::UpdateStatusBar(this);
#endif  

  Bind(wxEVT_FIND, [=](wxFindDialogEvent& event) {
    FindNext(
      wxExFindReplaceData::Get()->GetFindString(), 
      wxExFindReplaceData::Get()->SearchDown());});
      
  Bind(wxEVT_FIND_NEXT, [=](wxFindDialogEvent& event) {
    FindNext(
      wxExFindReplaceData::Get()->GetFindString(), 
      wxExFindReplaceData::Get()->SearchDown());});
      
  if (m_Data.Type() != LIST_NONE)
  {
    Bind(wxEVT_IDLE, [=](wxIdleEvent& event) {
      event.Skip();
      if (
        !IsShown() ||
         wxExInterruptable::Running() ||
         GetItemCount() == 0 ||
        !wxConfigBase::Get()->ReadBool("AllowSync", true))
      {
        return;
      }
      if (m_ItemNumber < GetItemCount())
      {
        wxExListItem item(this, m_ItemNumber);
    
        if ( item.GetFileName().FileExists() &&
            (item.GetFileName().GetStat().GetModificationTime() != 
             GetItemText(m_ItemNumber, _("Modified").ToStdString()) ||
             item.GetFileName().GetStat().IsReadOnly() != item.IsReadOnly())
            )
        {
          item.Update();
          wxExLogStatus(item.GetFileName(), STAT_SYNC | STAT_FULLPATH);
          m_ItemUpdated = true;
        }
    
        m_ItemNumber++;
      }
      else
      {
        m_ItemNumber = 0;
    
        if (m_ItemUpdated)
        {
          if (m_Data.Type() == LIST_FILE)
          {
            if (
              wxConfigBase::Get()->ReadBool("List/SortSync", true) &&
              GetSortedColumnNo() == FindColumn(_("Modified").ToStdString()))
            {
              SortColumn(_("Modified").ToStdString(), SORT_KEEP);
            }
          }
    
          m_ItemUpdated = false;
        }
      }});
    }

#if wxUSE_DRAG_AND_DROP
  Bind(wxEVT_LIST_BEGIN_DRAG, [=](wxListEvent& event) {
    // Start drag operation.
    wxString text;
    for (long i = GetFirstSelected(); i != -1; i = GetNextSelected(i))
      text += ItemToText(i) + "\n";
    if (!text.empty())
    {
      wxTextDataObject textData(text);
      wxDropSource source(textData, this);
      source.DoDragDrop(wxDragCopy);
    }});
#endif

  Bind(wxEVT_LIST_ITEM_ACTIVATED, [=] (wxListEvent& event) {
    ItemActivated(event.GetIndex());});
  
#if wxUSE_STATUSBAR
  Bind(wxEVT_LIST_ITEM_DESELECTED, [=](wxListEvent& event) {
    wxExFrame::UpdateStatusBar(this);});
#endif  
  
  Bind(wxEVT_LIST_ITEM_SELECTED, [=](wxListEvent& event) {
    if (m_Data.Type() != LIST_NONE && GetSelectedItemCount() == 1)
    {
      const wxExPath fn(wxExListItem(this, event.GetIndex()).GetFileName());
      if (fn.GetStat().IsOk())
      {
        wxExLogStatus(fn, STAT_FULLPATH);
      }
      else
      {
        wxExLogStatus(GetItemText(GetFirstSelected()));
      }
    }
    wxExFrame::UpdateStatusBar(this);});
    
  Bind(wxEVT_LIST_COL_CLICK, [=](wxListEvent& event) {
    SortColumn(
      event.GetColumn(),
      (wxExSortType)wxConfigBase::Get()->ReadLong(_("Sort method"), 
         SORT_TOGGLE));});

  Bind(wxEVT_LIST_COL_RIGHT_CLICK, [=](wxListEvent& event) {
    m_ToBeSortedColumnNo = event.GetColumn();

    wxExMenu menu(GetSelectedItemCount() > 0 ? 
      wxExMenu::MENU_IS_SELECTED: 
      wxExMenu::MENU_DEFAULT);
      
    menu.Append(wxID_SORT_ASCENDING);
    menu.Append(wxID_SORT_DESCENDING);

    PopupMenu(&menu);});
    
  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {EditClearAll();}, wxID_CLEAR);
  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {CopySelectedItemsToClipboard();}, wxID_COPY);
  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {EditDelete();}, wxID_DELETE);
  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {ItemFromText(wxExClipboardGet());}, wxID_PASTE);
  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {SetItemState(-1, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);}, wxID_SELECTALL);
  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {SortColumn(m_ToBeSortedColumnNo, SORT_ASCENDING);}, wxID_SORT_ASCENDING);
  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {SortColumn(m_ToBeSortedColumnNo, SORT_DESCENDING);}, wxID_SORT_DESCENDING);
    
  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    CopySelectedItemsToClipboard();
    EditDelete();}, wxID_CUT);
    
  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    for (auto i = 0; i < GetItemCount(); i++)
    {
      Select(i, !IsSelected(i));
    }}, ID_EDIT_SELECT_INVERT);
    
  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    for (auto i = 0; i < GetItemCount(); i++)
    {
      Select(i, false);
    }}, ID_EDIT_SELECT_NONE);
  
  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    wxString defaultPath;
    if (GetSelectedItemCount() > 0)
    {
      defaultPath = wxExListItem(
        this, GetFirstSelected()).GetFileName().Path().string();
    }
    wxDirDialog dir_dlg(
      this,
      _(wxDirSelectorPromptStr),
      defaultPath,
      wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);
    if (dir_dlg.ShowModal() == wxID_OK)
    {
      const auto no = (GetSelectedItemCount() > 0 ? 
        GetFirstSelected(): GetItemCount());
       
      wxExListItem(this, dir_dlg.GetPath().ToStdString()).Insert(no);
    }}, wxID_ADD);

  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    for (int i = GetFirstSelected(); i != -1; i = GetNextSelected(i))
    {
      ItemActivated(i);
    }}, ID_EDIT_OPEN);

  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    if (!IsShown() || GetItemCount() == 0) return false;
    long val;
    if ((val = wxGetNumberFromUser(
      _("Input") + wxString::Format(" (1 - %d):", GetItemCount()),
      wxEmptyString,
      _("Enter Item Number"),
      (GetFirstSelected() == -1 ? 1: GetFirstSelected() + 1),
      1,
      GetItemCount())) > 0)
    {
      wxExListViewData(wxExControlData().Line(val), this).Inject();
    }
    return true;}, wxID_JUMP_TO);

  Bind(wxEVT_RIGHT_DOWN, [=](wxMouseEvent& event) {
    long style = 0; // otherwise CAN_PASTE already on
    if (GetSelectedItemCount() > 0) style |= wxExMenu::MENU_IS_SELECTED;
    if (GetItemCount() == 0) style |= wxExMenu::MENU_IS_EMPTY;
    if (m_Data.Type() != LIST_FIND) style |= wxExMenu::MENU_CAN_PASTE;
    if (GetSelectedItemCount() == 0 && GetItemCount() > 0) 
    {
      style |= wxExMenu::MENU_ALLOW_CLEAR;
    }
    wxExMenu menu(style);
    BuildPopupMenu(menu);
    if (menu.GetMenuItemCount() > 0)
    {
      PopupMenu(&menu);
    }});
    
  Bind(wxEVT_SET_FOCUS, [=](wxFocusEvent& event) {
    wxExFrame* frame = dynamic_cast<wxExFrame*>(wxTheApp->GetTopWindow());
    if (frame != nullptr)
    {
      frame->SetFindFocus(this);
    }
    event.Skip();});
  
#if wxUSE_STATUSBAR
  Bind(wxEVT_SHOW, [=](wxShowEvent& event) {
    event.Skip();
    wxExFrame::UpdateStatusBar(this);});
#endif  
}    

bool wxExListView::AppendColumns(const std::vector <wxExColumn>& cols)
{
  SetSingleStyle(wxLC_REPORT);

  for (const auto col : cols)
  {
    wxExColumn mycol(col);
    
    const long index = wxListView::AppendColumn(
      mycol.GetText(), mycol.GetAlign(), mycol.GetWidth());
    
    if (index == -1) return false;

    mycol.SetColumn(GetColumnCount() - 1);
    m_Columns.emplace_back(mycol);
      
    Bind(wxEVT_MENU,  [=](wxCommandEvent& event) {
      SortColumn(event.GetId() - ID_COL_FIRST, SORT_TOGGLE);},
      ID_COL_FIRST + GetColumnCount() - 1);
  }
  
  return true;
}

const std::string wxExListView::BuildPage()
{
  wxString text;
  text << "<TABLE "
       << (((GetWindowStyle() & wxLC_HRULES) || (GetWindowStyle() & wxLC_VRULES)) 
          ? "border=1": "border=0")
       << " cellpadding=4 cellspacing=0 >\n"
       << "<tr>\n";

  for (auto c = 0; c < GetColumnCount(); c++)
  {
    wxListItem col;
    GetColumn(c, col);
    text << "<td><i>" << col.GetText() << "</i>\n";
  }

  for (auto i = 0; i < GetItemCount(); i++)
  {
    text << "<tr>\n";

    for (auto col = 0; col < GetColumnCount(); col++)
    {
      text << "<td>" << wxListView::GetItemText(i, col) << "\n";
    }
  }

  text << "</TABLE>\n";

  return text.ToStdString();
}

void wxExListView::BuildPopupMenu(wxExMenu& menu)
{
  if (GetSelectedItemCount() >= 1 && 
    wxExListItem(this, GetFirstSelected()).GetFileName().GetStat().IsOk())
  {
    menu.Append(ID_EDIT_OPEN, _("&Open"), wxART_FILE_OPEN);
    menu.AppendSeparator();
  }

  menu.AppendSeparator();
  menu.AppendEdit(true);
  
  if (
    GetItemCount() > 0 && 
    GetSelectedItemCount() == 0 &&
    InReportView())
  {
    menu.AppendSeparator();

    wxMenu* menuSort = new wxMenu;

    for (const auto& it : m_Columns)
    {
      menuSort->Append(ID_COL_FIRST + it.GetColumn(), it.GetText());
    }

    menu.AppendSubMenu(menuSort, _("Sort On"));
  }
  
  if (m_Data.Type() == LIST_FOLDER && GetSelectedItemCount() <= 1)
  {
    menu.AppendSeparator();
    menu.Append(wxID_ADD);
  }
}

wxExColumn wxExListView::Column(const std::string& name) const
{
  for (auto& it : m_Columns)
  {
    if (it.GetText() == name)
    {
      return it;
    }
  }
  
  return wxExColumn();
}

int wxExListView::ConfigDialog(const wxExWindowData& par)
{
  const wxExWindowData data(wxExWindowData(par).
    Title(_("List Options").ToStdString()));

  if (m_ConfigDialog == nullptr)
  {
    ListViewDefaults use;

    m_ConfigDialog = new wxExItemDialog({{"notebook", {
      {_X("General"),
        {{_X("Header"), ITEM_CHECKBOX},
         {_X("Single selection"), ITEM_CHECKBOX},
         {_X("Comparator"), ITEM_FILEPICKERCTRL},
         {_X("Sort method"), {
           {SORT_ASCENDING, _X("Sort ascending")},
           {SORT_DESCENDING, _X("Sort descending")},
           {SORT_TOGGLE, _X("Sort toggle")}}},
         {_X("Context size"), 0, 80},
         {_X("Rulers"),  {
           {wxLC_HRULES, _X("Horizontal rulers")},
           {wxLC_VRULES, _X("Vertical rulers")}}, false}}},
      {_X("Font"),
#ifndef __WXOSX__
        {{_X("List font"), ITEM_FONTPICKERCTRL},
         {_X("List tab font"), ITEM_FONTPICKERCTRL}}},
#else
        {{_X("List font")},
         {_X("List tab font")}}},
#endif
      {_X("Colour"),
        {{_X("Background colour"), ITEM_COLOURPICKERWIDGET},
         {_X("Foreground colour"), ITEM_COLOURPICKERWIDGET},
         {_X("Readonly colour"), ITEM_COLOURPICKERWIDGET}}}}}}, data);
  }

  return (data.Button() & wxAPPLY) ?
    m_ConfigDialog->Show(): m_ConfigDialog->ShowModal();
}
          
void wxExListView::ConfigGet()
{
  ListViewDefaults use;
  wxConfigBase* cfg = use.Get();
  
  SetBackgroundColour(cfg->ReadObject(_("Background colour"), wxColour("WHITE")));
  SetFont(cfg->ReadObject(_("List font"), wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT)));
  SetSingleStyle(wxLC_HRULES, (cfg->ReadLong(_("Rulers"), 0) & wxLC_HRULES) > 0);
  SetSingleStyle(wxLC_VRULES, (cfg->ReadLong(_("Rulers"), 0) & wxLC_VRULES) > 0);
  SetSingleStyle(wxLC_NO_HEADER, !cfg->ReadBool(_("Header"), false));
  SetSingleStyle(wxLC_SINGLE_SEL, cfg->ReadBool(_("Single selection"), false));
  
  ItemsUpdate();
}
  
void wxExListView::CopySelectedItemsToClipboard()
{
  if (GetSelectedItemCount() == 0) return;

  wxBusyCursor wait;
  std::string clipboard;

  for (long i = GetFirstSelected(); i != -1; i = GetNextSelected(i))
    clipboard += ItemToText(i) + "\n";
    
  wxExClipboardAdd(clipboard);
}

void wxExListView::EditClearAll()
{
  DeleteAllItems();

  SortColumnReset();

#if wxUSE_STATUSBAR
  wxExFrame::UpdateStatusBar(this);
#endif
}

void wxExListView::EditDelete()
{
  if (GetSelectedItemCount() == 0) return;

  long i = -1, old_item = -1;

  while ((i = GetNextSelected(i)) != -1)
  {
    DeleteItem(i);
    old_item = i;
    i = -1;
  }

  if (old_item != -1 && old_item < GetItemCount())
    SetItemState(old_item, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);

  ItemsUpdate();
}
  
bool wxExListView::FindNext(const std::string& text, bool find_next)
{
  if (text.empty())
  {
    return false;
  }

  std::string text_use = text;

  if (!wxExFindReplaceData::Get()->MatchCase())
  {
    for (auto & c : text_use) c = std::toupper(c);
  }

  const int firstselected = GetFirstSelected();
  static bool recursive = false;
  static long start_item;
  static long end_item;

  if (find_next)
  {
    start_item = recursive ? 0: (firstselected != -1 ? firstselected + 1: 0);
    end_item = GetItemCount();
  }
  else
  {
    start_item = recursive ? GetItemCount() - 1: (firstselected != -1 ? firstselected - 1: 0);
    end_item = -1;
  }

  int match = -1;

  for (
    int index = start_item;
    index != end_item && match == -1;
    (find_next ? index++: index--))
  {
    std::string text;

    for (int col = 0; col < GetColumnCount() && match == -1; col++)
    {
      text = std::string(wxListView::GetItemText(index, col));

      if (!wxExFindReplaceData::Get()->MatchCase())
      {
        for (auto & c : text) c = std::toupper(c);
      }

      if (wxExFindReplaceData::Get()->MatchWord())
      {
        if (text == text_use)
        {
          match = index;
        }
      }
      else
      {
        if (text.find(text_use) != std::string::npos)
        {
          match = index;
        }
      }
    }
  }

  if (match != -1)
  {
    recursive = false;

    Select(match);
    EnsureVisible(match);

    if (firstselected != -1 && match != firstselected)
    {
      Select(firstselected, false);
    }

    return true;
  }
  else
  {
    wxExFrame::StatusText(wxExGetFindResult(text, find_next, recursive), std::string());
    
    if (!recursive)
    {
      recursive = true;
      FindNext(text, find_next);
      recursive = false;
    }
    
    return false;
  }
}

unsigned int wxExListView::GetArtID(const wxArtID& artid)
{
  const auto it = m_ArtIDs.find(artid);

  if (it != m_ArtIDs.end())
  {
    return it->second;
  }
  else
  {
    wxImageList* il = GetImageList(wxIMAGE_LIST_SMALL);
    
    if (il == nullptr)
    {
      wxFAIL;
      return 0;
    }

    m_ArtIDs.insert({artid, il->GetImageCount()});

    return il->Add(wxArtProvider::GetBitmap(
      artid, wxART_OTHER, wxSize(m_ImageWidth, m_ImageHeight)));
  }
}

const std::string wxExListView::GetItemText(
  long item_number, const std::string& col_name) const 
{
  if (col_name.empty())
  {
    return wxListView::GetItemText(item_number).ToStdString();
  }
  
  const int col = FindColumn(col_name);
  return col < 0 ? std::string(): wxListView::GetItemText(item_number, col).ToStdString();
}

bool wxExListView::InsertItem(const std::vector < std::string > & item)
{
  if (item.empty() || item.front().empty() || item.size() > m_Columns.size()) 
  {
    return false;
  }

  long no = 0;
  int index = 0;

  for (const auto& col : item)
  {
    try
    {
      switch (m_Columns[no].GetType())
      {
        case wxExColumn::COL_DATE:
          {
            struct tm tm;
            if (strptime(col.c_str(), "%c", &tm) == nullptr) return false;
          }
          break;
        case wxExColumn::COL_FLOAT: std::stof(col); break;
        case wxExColumn::COL_INT: std::stoi(col); break;
        case wxExColumn::COL_STRING: break;
      }

      if (no == 0)
      {
        index = wxListView::InsertItem(GetItemCount(), col);
        if (index == -1) return false;
      }
      else
      {
        if (!SetItem(index, no, col)) return false;
      }
      no++;
    }
    catch (std::exception& e)
    {
      std::cout << item.front() << ": " << col << ": " << e.what() << "\n";
      return false;
    }
  }

  return true;
}

void wxExListView::ItemActivated(long item_number)
{
  wxASSERT(item_number >= 0);
  
  if (m_Data.Type() == LIST_FOLDER)
  {
    wxDirDialog dir_dlg(
      this,
      _(wxDirSelectorPromptStr),
      wxListView::GetItemText(item_number),
      wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);

    if (dir_dlg.ShowModal() == wxID_OK)
    {
      SetItemText(item_number, dir_dlg.GetPath());
      wxExListItem(this, item_number).Update();
    }
  }
  else
  {
    // Cannot be const because of SetItem later on.
    wxExListItem item(this, item_number);
  
    if (item.GetFileName().FileExists())
    {
      wxExFrame* frame = dynamic_cast<wxExFrame*>(wxTheApp->GetTopWindow());

      if (frame != nullptr)
      {
        const std::string no(GetItemText(item_number, _("Line No").ToStdString()));

        wxExControlData data = 
          (m_Data.Type() == LIST_FIND && !no.empty() ?
             wxExControlData().
               Line(std::stoi(no)). 
               Find(GetItemText(item_number, _("Match").ToStdString())): 
             wxExControlData());

        frame->OpenFile(item.GetFileName(), data);
      }
    }
    else if (item.GetFileName().DirExists())
    {
      wxTextEntryDialog dlg(this,
        _("Input") + ":",
        _("Folder Type"),
        GetItemText(item_number, _("Type").ToStdString()));
  
      if (dlg.ShowModal() == wxID_OK)
      {
        item.SetItem(_("Type").ToStdString(), dlg.GetValue().ToStdString());
      }
    }
    
  }
}

bool wxExListView::ItemFromText(const std::string& text)
{
  if (text.empty())
  {
    return false;
  }

  bool modified = false;
  
  wxExTokenizer tkz(text, "\n");

  while (tkz.HasMoreTokens())
  {
    if (m_Data.Type() != LIST_NONE)
    {
      modified = true;
    
      if (!InReportView())
      {
        wxExListItem(this, tkz.GetNextToken()).Insert();
      }
      else
      {
        const std::string token(tkz.GetNextToken());
        wxExTokenizer tk(token, std::string(1, GetFieldSeparator()));
        
        if (tk.HasMoreTokens())
        {
          const std::string value = tk.GetNextToken();
          wxExPath fn(value);
    
          if (fn.FileExists())
          {
            wxExListItem item(this, fn);
            item.Insert();
    
            // And try to set the rest of the columns 
            // (that are not already set by inserting).
            int col = 1;
            while (tk.HasMoreTokens() && col < GetColumnCount() - 1)
            {
              const std::string value = tk.GetNextToken();
    
              if (col != FindColumn(_("Type").ToStdString()) &&
                  col != FindColumn(_("In Folder").ToStdString()) &&
                  col != FindColumn(_("Size").ToStdString()) &&
                  col != FindColumn(_("Modified").ToStdString()))
              {
                if (!SetItem(item.GetId(), col, value)) return false;
              }
    
              col++;
            }
          }
          else
          {
            // Now we need only the first column (containing findfiles). If more
            // columns are present, these are ignored.
            const std::string findfiles =
              (tk.HasMoreTokens() ? tk.GetNextToken(): tk.GetString());
    
            wxExListItem(this, value, findfiles).Insert();
          }
        }
        else
        {
          wxExListItem(this, token).Insert();
        }
      }
    }
    else
    {
      const std::string line = tkz.GetNextToken();
      if (InsertItem(wxExTokenizer(line, std::string(1, m_FieldSeparator)).
        Tokenize<std::vector < std::string >>()))
      {
        modified = true;
      }
    }
  }

  return modified;
}

const std::string wxExListView::ItemToText(long item_number) const
{
  std::string text;
    
  if (item_number == -1)
  {
    for (auto i = 0; i < GetItemCount(); i++)
    {
      text += wxListView::GetItemText(i) + "\n";
    }
    
    return text;
  }

  switch (m_Data.Type())
  {
    case LIST_FILE:
    case LIST_HISTORY:
      {
      const wxExListItem item(const_cast< wxExListView * >(this), item_number);
      wxString text = (item.GetFileName().GetStat().IsOk() ? 
        item.GetFileName().Path().string(): 
        item.GetFileName().GetFullName());

      if (item.GetFileName().DirExists() && !item.GetFileName().FileExists())
      {
        text += GetFieldSeparator() + GetItemText(item_number, _("Type").ToStdString());
      }
      }

    case LIST_FOLDER:
      return wxListView::GetItemText(item_number).ToStdString();
      break;
    
    default:
      for (int col = 0; col < GetColumnCount(); col++)
      {
        text += wxListView::GetItemText(item_number, col);

        if (col < GetColumnCount() - 1)
        {
          text += m_FieldSeparator;
        }
      }
    }

  return text;
}

void wxExListView::ItemsUpdate()
{
  if (m_Data.Type() != LIST_NONE)
  {
    for (auto i = 0; i < GetItemCount(); i++)
    {
      wxExListItem(this, i).Update();
    }
  }
}

void wxExListView::Print()
{
#if wxUSE_HTML & wxUSE_PRINTING_ARCHITECTURE
  wxBusyCursor wait;
  wxExPrinting::Get()->GetHtmlPrinter()->PrintText(BuildPage());
#endif
}

void wxExListView::PrintPreview()
{
#if wxUSE_HTML & wxUSE_PRINTING_ARCHITECTURE
  wxBusyCursor wait;
  wxExPrinting::Get()->GetHtmlPrinter()->PreviewText(BuildPage());
#endif
}

std::vector<wxString>* pitems;

int wxCALLBACK CompareFunctionCB(wxIntPtr item1, wxIntPtr item2, wxIntPtr sortData)
{
  const bool ascending = (sortData > 0);
  const wxExColumn::wxExColumnType type = 
    (wxExColumn::wxExColumnType)std::abs(sortData);

  switch (type)
  {
  case wxExColumn::COL_DATE:
    if (ascending) return (unsigned long)item1 > (unsigned long)item2;
    else           return (unsigned long)item1 < (unsigned long)item2;
  break;

  case wxExColumn::COL_INT:
  case wxExColumn::COL_FLOAT:
    if (ascending) return item1 > item2;
    else           return item1 < item2;
  break;

  case wxExColumn::COL_STRING:
    {
    const wxString& str1 = (*pitems)[item1];
    const wxString& str2 = (*pitems)[item2];

    if (!wxExFindReplaceData::Get()->MatchCase())
    {
      if (ascending) return strcmp(str1.Upper().c_str(), str2.Upper().c_str());
      else           return strcmp(str2.Upper().c_str(), str1.Upper().c_str());
    }
    else
    {
      if (ascending) return strcmp(str1.c_str(), str2.c_str());
      else           return strcmp(str2.c_str(), str1.c_str());
    }
    }
  break;

  default:
    wxFAIL;
  }

  return 0;
}

bool wxExListView::SetItem(
  long index, int column, const std::string &label, int imageId)
{
  if (label.empty()) return true;

  try
  {
    switch (m_Columns[column].GetType())
    {
      case wxExColumn::COL_DATE:
        {
          struct tm tm;
          if (strptime(label.c_str(), "%c", &tm) == nullptr) return false;
        }
        break;
      case wxExColumn::COL_FLOAT: std::stof(label); break;
      case wxExColumn::COL_INT: std::stoi(label); break;
      case wxExColumn::COL_STRING: break;
    }

    wxListView::SetItem(index, column, label, imageId);
    return true;
  }
  catch (std::exception& e)
  {
    std::cout << "index: " << index << " col: " << column << ": " <<
      label << e.what() << "\n";
    return false;
  }
}

bool wxExListView::SortColumn(int column_no, wxExSortType sort_method)
{
  if (column_no == -1 || column_no >= (int)m_Columns.size())
  {
    return false;
  }
  
  SortColumnReset();
  
  wxExColumn& sorted_col = m_Columns[column_no];
  
  sorted_col.SetIsSortedAscending(sort_method);

  wxBusyCursor wait;

  // Keeping the items is necessary for sorting strings.
  std::vector<wxString> items;
  pitems = &items;

  for (auto i = 0; i < GetItemCount(); i++)
  {
    const wxString val = wxListView::GetItemText(i, column_no);
    items.emplace_back(val);

    switch (sorted_col.GetType())
    {
      case wxExColumn::COL_DATE:
        if (!val.empty())
        {
          struct tm tm;
          if (strptime(val.c_str(), "%c", &tm) == nullptr) return false;

          time_t t = mktime(&tm);
          SetItemData(i, t);
        }
        else
        {
          SetItemData(i, 0);
        }
      break;

      case wxExColumn::COL_FLOAT: SetItemData(i, (long)atof(val.c_str())); break;
      case wxExColumn::COL_INT: SetItemData(i, atoi(val.c_str())); break;
      case wxExColumn::COL_STRING: SetItemData(i, i); break;
      default: wxFAIL;
    }
  }

  const wxIntPtr sortdata =
    (sorted_col.GetIsSortedAscending() ?
       sorted_col.GetType():
      (0 - sorted_col.GetType()));

  SortItems(CompareFunctionCB, sortdata);

  m_SortedColumnNo = column_no;

  if (m_Data.Image() != IMAGE_NONE)
  {
    SetColumnImage(column_no, GetArtID(
      sorted_col.GetIsSortedAscending() ? wxART_GO_DOWN: wxART_GO_UP));
  }

  if (GetItemCount() > 0)
  {
    ItemsUpdate();
    AfterSorting();
  }

  wxLogStatus(_("Sorted on") + ": " + sorted_col.GetText());
  
  return true;
}

void wxExListView::SortColumnReset()
{
  if (m_SortedColumnNo != -1 && !m_ArtIDs.empty()) // only if we are using images
  {
    ClearColumnImage(m_SortedColumnNo);
    m_SortedColumnNo = -1;
  }
}
#endif // wxUSE_GUI
