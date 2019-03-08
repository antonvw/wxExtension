////////////////////////////////////////////////////////////////////////////////
// Name:      listview.cpp
// Purpose:   Implementation of class wex::report::listview
// Author:    Anton van Wezenbeek
// Copyright: (c) 2019 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <thread>
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wex/config.h>
#include <wex/interruptable.h>
#include <wex/itemdlg.h>
#include <wex/lexers.h>
#include <wex/listitem.h>
#include <wex/util.h>
#include <wex/vcs.h>
#include <wex/report/listview.h>
#include <wex/report/defs.h>
#include <wex/report/dir.h>
#include <wex/report/frame.h>
#include <wex/report/stream.h>

wex::report::listview::listview(const listview_data& data)
  : wex::listview(data)
  , m_Frame(dynamic_cast<report::frame*>(wxTheApp->GetTopWindow()))
  , m_MenuFlags(data.menu())
{
  if (data.type() == listview_data::HISTORY)
  {
    m_Frame->use_file_history_list(this);
  }

  wxAcceleratorEntry entries[5];
  entries[0].Set(wxACCEL_NORMAL, WXK_DELETE, wxID_DELETE);
  entries[1].Set(wxACCEL_CTRL, WXK_INSERT, wxID_COPY);
  entries[2].Set(wxACCEL_SHIFT, WXK_INSERT, wxID_PASTE);
  entries[3].Set(wxACCEL_SHIFT, WXK_DELETE, wxID_CUT);
  entries[4].Set(wxACCEL_CTRL, 'M', ID_LIST_COMPARE);

  wxAcceleratorTable accel(WXSIZEOF(entries), entries);
  SetAcceleratorTable(accel);
  
  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    bool first = true;
    wxString file1,file2;
    listview* list = nullptr;
    for (int i = GetFirstSelected(); i != -1; i = GetNextSelected(i))
    {
      listitem li(this, i);
      const wex::path* filename = &li.get_filename();
      if (!filename->file_exists()) continue;
      switch (event.GetId())
      {
        case ID_LIST_COMPARE:
        {
          if (GetSelectedItemCount() == 1)
          {
            list = m_Frame->activate(listview_data::FILE);
            if (list == nullptr) return;
            const int main_selected = list->GetFirstSelected();
            comparefile(listitem(list, main_selected).get_filename(), *filename);
          }
          else
          {
            if (first)
            {
              first = false;
              file1 = filename->data().string();
            }
            else
            {
              first = true;
              file2 = filename->data().string();
            }
            if (first) comparefile(path(file1), path(file2));
          }
        }
        break;
      }
    }}, ID_LIST_COMPARE);

  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    make(listitem(this, GetFirstSelected()).get_filename());}, ID_LIST_RUN_MAKE);

  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    const wex::tool& tool(event.GetId());
    if (tool.id() == ID_TOOL_REPORT_KEYWORD && data.type() == listview_data::KEYWORD) return;
    if (tool.is_find_type() && m_Frame->find_in_files_dialog(tool.id()) == wxID_CANCEL) return;
    if (!report::stream::setup_tool(tool, m_Frame)) return;

#ifdef __WXMSW__    
    std::thread t([=] {
#endif

    statistics<int> stats;

    for (int i = GetFirstSelected(); i != -1; i = GetNextSelected(i))
    {
      const listitem item(this, i);
      log::status() << item.get_filename();
      if (item.get_filename().file_exists())
      {
        stream file(item.get_filename(), tool);
        file.run_tool();
        stats += file.get_statistics().get_elements();
      }
      else
      {
        tool_dir dir(tool, 
          item.get_filename().data().string(), 
          item.file_spec());
        dir.find_files();
        stats += dir.get_statistics().get_elements();
      }
    }
    log::status(tool.info(&stats));
#ifdef __WXMSW__    
    });
    t.detach();
#endif
    }, ID_TOOL_LOWEST, ID_TOOL_HIGHEST);
  
  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    std::vector< path > files;
    for (int i = GetFirstSelected(); i != -1; i = GetNextSelected(i))
    {
      files.emplace_back(listitem(this, i).get_filename().data());
    }
    vcs_execute(m_Frame, event.GetId() - ID_EDIT_VCS_LOWEST - 1, files);
    }, ID_EDIT_VCS_LOWEST, ID_EDIT_VCS_HIGHEST);
}

void wex::report::listview::build_popup_menu(wex::menu& menu)
{
  bool exists = true, is_folder = false, is_make = false, read_only = false;

  if (GetSelectedItemCount() >= 1)
  {
    const listitem item(this, GetFirstSelected());

    exists = item.get_filename().stat().is_ok();
    is_folder = item.get_filename().dir_exists();
    read_only = item.get_filename().stat().is_readonly();
    is_make = item.get_filename().lexer().scintilla_lexer() == "makefile";
  }

  listview::build_popup_menu(menu);

  if (GetSelectedItemCount() > 1 && exists &&
     !config(_("Comparator")).empty())
  {
    menu.append_separator();
    menu.append(ID_LIST_COMPARE, _("C&ompare") + "\tCtrl+M");
  }

  if (GetSelectedItemCount() == 1)
  {
    if (is_make)
    {
      menu.append_separator();
      menu.append(ID_LIST_RUN_MAKE, _("&Make"));
    }

    if (data().type() != listview_data::FILE &&
        !wex::vcs().use() &&
         exists && !is_folder)
    {
      if (auto* list = m_Frame->activate(listview_data::FILE);
        list != nullptr && list->GetSelectedItemCount() == 1)
      {
        listitem thislist(this, GetFirstSelected());
        const wxString current_file = thislist.get_filename().data().string();

        listitem otherlist(list, list->GetFirstSelected());

        if (const std::string with_file = otherlist.get_filename().data().string(); 
          current_file != with_file &&
            !config(_("Comparator")).empty())
        {
          menu.append_separator();
          menu.append(ID_LIST_COMPARE,
            _("&Compare With") + " " + wxString(get_endoftext(with_file)));
        }
      }
    }
  }

  if (GetSelectedItemCount() >= 1)
  {
    if (exists && !is_folder)
    {
      if (vcs::dir_exists(
        listitem(this, GetFirstSelected()).get_filename()))
      {
        menu.append_separator();
        menu.append_vcs(listitem(this, GetFirstSelected()).get_filename());
      }
    }

    // Finding in the listview_data::FIND would result in recursive calls, do not add it.
    if (exists &&
        data().type() != listview_data::FIND && 
        m_MenuFlags.test(listview_data::MENU_REPORT_FIND))
    {
      menu.append_separator();
      menu.append(ID_TOOL_REPORT_FIND, 
        ellipsed(m_Frame->find_in_files_title(ID_TOOL_REPORT_FIND)));

      if (!read_only)
      {
        menu.append(ID_TOOL_REPLACE, 
          ellipsed(m_Frame->find_in_files_title(ID_TOOL_REPLACE)));
      }
    }
  }

  if (GetSelectedItemCount() > 0 && exists && 
      m_MenuFlags.test(listview_data::MENU_TOOL) && 
     !lexers::get()->get_lexers().empty())
  {
    menu.append_separator();
    menu.append_tools();
  }
}

bool wex::report::listview::Destroy()	
{
  interruptable::cancel();
  return wex::listview::Destroy();
}

wex::listview_data::type_t wex::report::listview::type_tool(
  const tool& tool)
{
  switch (tool.id())
  {
    case ID_TOOL_REPLACE:
    case ID_TOOL_REPORT_FIND: 
      return listview_data::FIND;
    
    case ID_TOOL_REPORT_KEYWORD: 
      return listview_data::KEYWORD;
    
    default: 
      return listview_data::NONE;
  }
}
