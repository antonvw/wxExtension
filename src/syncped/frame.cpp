////////////////////////////////////////////////////////////////////////////////
// Name:      frame.cpp
// Purpose:   Implementation of class frame
// Author:    Anton van Wezenbeek
// Copyright: (c) 2019 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/aboutdlg.h>
#include <wx/imaglist.h>
#include <wex/config.h>
#include <wex/ctags.h>
#include <wex/debug.h>
#include <wex/filedlg.h>
#include <wex/itemdlg.h>
#include <wex/lexers.h>
#include <wex/menu.h>
#include <wex/menus.h>
#include <wex/printing.h>
#include <wex/shell.h>
#include <wex/stc.h>
#include <wex/tokenizer.h>
#include <wex/tostring.h>
#include <wex/toolbar.h>
#include <wex/util.h>
#include <wex/vcs.h>
#include <wex/version.h>
#include <wex/vi-macros.h>
#include <wex/vi-macros-mode.h>
#include <wex/report/listviewfile.h>
#include "frame.h"
#include "app.h"
#include "defs.h"

const int idEditPaneInfoToggle= wxWindow::NewControlId();

class editors : public wex::notebook
{
public:
  editors(const wex::window_data& data); 

  bool is_split() const {return m_split;};
  void reset() {m_split = false;};
private:
  bool m_split {false};
};

BEGIN_EVENT_TABLE(frame, decorated_frame)
  EVT_MENU(wxID_DELETE, frame::on_command)
  EVT_MENU(wxID_EXECUTE, frame::on_command)
  EVT_MENU(wxID_JUMP_TO, frame::on_command)
  EVT_MENU(wxID_SELECTALL, frame::on_command)
  EVT_MENU(wxID_STOP, frame::on_command)
  EVT_MENU_RANGE(wxID_CUT, wxID_CLEAR, frame::on_command)
  EVT_MENU_RANGE(wxID_CLOSE, wxID_CLOSE_ALL, frame::on_command)
  EVT_MENU_RANGE(ID_EDIT_MACRO_PLAYBACK, ID_EDIT_MACRO_STOP_RECORD, frame::on_command)
  EVT_MENU_RANGE(ID_SPLIT, ID_SPLIT_VERTICALLY, frame::on_command)
  EVT_UPDATE_UI(wex::ID_ALL_CLOSE, frame::on_update_ui)
  EVT_UPDATE_UI(wex::ID_ALL_SAVE, frame::on_update_ui)
  EVT_UPDATE_UI(wxID_CLOSE, frame::on_update_ui)
  EVT_UPDATE_UI(wxID_EXECUTE, frame::on_update_ui)
  EVT_UPDATE_UI(wxID_FIND, frame::on_update_ui)
  EVT_UPDATE_UI(wxID_JUMP_TO, frame::on_update_ui)
  EVT_UPDATE_UI(wxID_PRINT, frame::on_update_ui)
  EVT_UPDATE_UI(wxID_PREVIEW, frame::on_update_ui)
  EVT_UPDATE_UI(wxID_REPLACE, frame::on_update_ui)
  EVT_UPDATE_UI(wxID_UNDO, frame::on_update_ui)
  EVT_UPDATE_UI(wxID_REDO, frame::on_update_ui)
  EVT_UPDATE_UI(wxID_SAVE, frame::on_update_ui)
  EVT_UPDATE_UI(wxID_SAVEAS, frame::on_update_ui)
  EVT_UPDATE_UI(wxID_STOP, frame::on_update_ui)
  EVT_UPDATE_UI(wex::ID_EDIT_CONTROL_CHAR, frame::on_update_ui)
  EVT_UPDATE_UI(ID_EDIT_MACRO, frame::on_update_ui)
  EVT_UPDATE_UI(ID_EDIT_MACRO_MENU, frame::on_update_ui)
  EVT_UPDATE_UI(ID_EDIT_MACRO_PLAYBACK, frame::on_update_ui)
  EVT_UPDATE_UI(ID_EDIT_MACRO_START_RECORD, frame::on_update_ui)
  EVT_UPDATE_UI(ID_EDIT_MACRO_STOP_RECORD, frame::on_update_ui)
  EVT_UPDATE_UI(ID_OPTION_VCS, frame::on_update_ui)
  EVT_UPDATE_UI(wex::ID_PROJECT_SAVE, frame::on_update_ui)
  EVT_UPDATE_UI(ID_RECENT_FILE_MENU, frame::on_update_ui)
  EVT_UPDATE_UI(ID_RECENT_PROJECT_MENU, frame::on_update_ui)
  EVT_UPDATE_UI(ID_SORT_SYNC, frame::on_update_ui)
  EVT_UPDATE_UI_RANGE(wex::ID_EDIT_FIND_NEXT, wex::ID_EDIT_FIND_PREVIOUS, frame::on_update_ui)
  EVT_UPDATE_UI_RANGE(ID_PROJECT_OPENTEXT, ID_PROJECT_SAVEAS, frame::on_update_ui)
END_EVENT_TABLE()

frame::frame(app* app)
  : decorated_frame(app)
  , m_process(new wex::process())
  , m_editors(new editors(
      wex::window_data().id(wex::ID_NOTEBOOK_EDITORS).style(m_pane_flag)))
  , m_projects(new wex::notebook(
      wex::window_data().id(wex::ID_NOTEBOOK_PROJECTS).style(m_pane_flag)))
  , m_lists(new wex::notebook(
      wex::window_data().id(wex::ID_NOTEBOOK_LISTS).style(m_pane_flag)))
  , m_dirctrl(new wex::report::dirctrl(this))
{
  manager().AddPane(m_editors, wxAuiPaneInfo()
    .CenterPane()
    .MaximizeButton(true)
    .Name("FILES")
    .Caption(_("Files")));

  manager().AddPane(m_dirctrl, wxAuiPaneInfo()
    .Left()
    .MaximizeButton(true)
    .CloseButton(false)
    .Name("DIRCTRL")
    .MinSize(200, 150)
    .Caption(_("Explorer")));

  manager().AddPane(m_lists, wxAuiPaneInfo()
    .Bottom()
    .MaximizeButton(true)
    .MinSize(250, 100)
    .Name("OUTPUT")
    .Row(0)
    .Caption(_("Output")));

  manager().AddPane(m_process->get_shell(), wxAuiPaneInfo()
    .Bottom()
    .Name("PROCESS")
    .MinSize(250, 100)
    .Caption(_("Process")));

  manager().AddPane(m_projects, wxAuiPaneInfo()
    .Left()
    .MaximizeButton(true)
    .Name("PROJECTS")
    .MinSize(150, 150)
    .Caption(_("Projects")));

  if (const std::string perspective(wex::config("Perspective").get()); 
    perspective.empty())
  {
    manager().GetPane("DIRCTRL").Hide();
    manager().GetPane("HISTORY").Hide();
    manager().GetPane("LOG").Hide();
    manager().GetPane("PROCESS").Hide();
    manager().GetPane("PROJECTS").Hide();
  }
  else
  {
    manager().LoadPerspective(perspective);
  }
  
  if (wex::config("ShowHistory").get(false))
  {
    add_pane_history();
  }

  // Regardless of the perspective initially hide the next panels.
  manager().GetPane("OUTPUT").Hide();
  
  hide_ex_bar();
  
  if (!m_app->get_tag().empty())
  {
    wex::ctags::find(m_app->get_tag());
  }
  else if (m_app->get_files().empty())
  {
    if (const int count = wex::config("OpenFiles").get(0); count > 0)
    {
      wex::open_files(this, file_history().get_history_files(count), 
        m_app->data());
    }
      
    if (wex::config("ShowProjects").get(false) && 
      !get_project_history().get_history_file().empty())
    {
      open_file(
        wex::path(get_project_history().get_history_file()),
        wex::stc_data().flags(
          wex::stc_data::window_t().set(wex::stc_data::WIN_IS_PROJECT)));
    }
    else
    {
      manager().GetPane("PROJECTS").Hide();
    }
  }
  else
  {
    manager().GetPane("PROJECTS").Hide();
    
    if (m_app->get_files().size() == 1 && m_app->get_debug())
    {
      get_debug()->execute("file " + 
        m_app->get_files().front().string());
      
      if (const int count = wex::config("OpenFiles").get(0); count > 0)
      {
        wex::open_files(this, file_history().get_history_files(count), 
          m_app->data());
      }
    }
    else
    {
      wex::open_files(this, 
        m_app->get_files(), 
        m_app->data(), 
        wex::dir::type_t().set(wex::dir::FILES));
    }
  }
  
  statustext(wex::lexers::get()->theme(), "PaneTheme");
  
  // End with update, so all changes in the manager are handled.
  manager().Update();
  
  if (m_editors->GetPageCount() > 0)
  {
    m_editors->GetPage(m_editors->GetPageCount() - 1)->SetFocus();
  }
  
  get_toolbar()->add_standard(false); // no realize yet
  get_toolbar()->add_checkboxes(
    {{ID_VIEW_DIRCTRL, 
      _("Explorer"), "", "", _("Explorer"), 
      manager().GetPane("DIRCTRL").IsShown(),
      [&](wxCheckBox* cb) {
        toggle_pane("DIRCTRL"); 
        cb->SetValue(manager().GetPane("DIRCTRL").IsShown());
        get_toolbar()->Realize();
        if (manager().GetPane("DIRCTRL").IsShown() &&
            manager().GetPane("FILES").IsShown())
        {
          if (auto* editor = get_stc(); editor != nullptr)
          {
            m_dirctrl->expand_and_select_path(editor->get_filename());
          }
        }
      }},
     {ID_VIEW_HISTORY, 
      _("History"), "", "", _("History"), 
      wex::config("ShowHistory").get(false),
      [&](wxCheckBox* cb) {
        if (m_history == nullptr)
        {
          add_pane_history();
          manager().Update();
        }
        else
        {
          toggle_pane("HISTORY");
        }
        cb->SetValue(manager().GetPane("HISTORY").IsShown());
        get_toolbar()->Realize();
        update_statusbar(m_history);
      }}}); // realize
  
  get_find_toolbar()->add_find();

  get_options_toolbar()->add_checkboxes_standard();
  
  Bind(wxEVT_AUINOTEBOOK_BG_DCLICK, [=](wxAuiNotebookEvent& event) {
    file_history().popup_menu(this, wex::ID_CLEAR_FILES);}, 
    wex::ID_NOTEBOOK_EDITORS);
    
  Bind(wxEVT_AUINOTEBOOK_BG_DCLICK, [=] (wxAuiNotebookEvent& event) {
    get_project_history().popup_menu(this, wex::ID_CLEAR_PROJECTS);}, 
    wex::ID_NOTEBOOK_PROJECTS);
    
  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    get_debug()->execute(event.GetId() - wex::ID_EDIT_DEBUG_FIRST);}, 
    wex::ID_EDIT_DEBUG_FIRST, wex::ID_EDIT_DEBUG_LAST);
  
  Bind(wxEVT_CLOSE_WINDOW, [=](wxCloseEvent& event) {
    int count = 0;
    for (size_t i = 0; i < m_editors->GetPageCount(); i++)
    {
      if (auto* stc = wxDynamicCast(m_editors->GetPage(i), wex::stc);
        stc->get_filename().file_exists())
      {
        count++;
      }
    }

    const bool project_open (m_projects->IsShown());

    if (event.CanVeto())
    {
      if (
         m_process->is_running() ||
        !m_editors->for_each<wex::stc>(wex::ID_ALL_CLOSE) || 
        !m_projects->for_each<wex::report::file>(wex::ID_ALL_CLOSE))
      {
        event.Veto();
        if (m_process->is_running())
        {
          wex::log::status(_("Process is running"));
        }
        return;
      }
    }

    wex::vi_macros::save_document();

    wex::config("Perspective").set(manager().SavePerspective().ToStdString());
    wex::config("OpenFiles").set(count);
    wex::config("ShowHistory").set(
      m_history != nullptr && m_history->IsShown());
    wex::config("ShowProjects").set(project_open);

    if (m_app->data().control().command().empty())
    {
      delete m_process;
    }
    event.Skip();
    });
    
  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    wxAboutDialogInfo info;
    info.SetIcon(GetIcon());
    info.SetVersion(wex::get_version_info().get());
    wxString description(
      _("This program offers a portable text or binary editor\n"
        "with automatic syncing."));
#ifdef __WXMSW__
    description +=
      _(" All its config files are read\n"
        "and saved in the same directory as where the executable is.");
#endif
    info.SetDescription(description);
    info.SetCopyright(wex::get_version_info().copyright());
    info.SetWebSite("http://sourceforge.net/projects/syncped/");
    wxAboutBox(info, this);}, wxID_ABOUT);
  
  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    show_pane("PROCESS"); 
    m_process->execute();}, wxID_EXECUTE);

  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    Close(true);}, wxID_EXIT);

  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    wxLaunchDefaultBrowser(
      "http://antonvw.github.io/syncped/v" + 
      wex::get_version_info().get() + 
      "/syncped.htm");}, wxID_HELP);
    
  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    wex::printing::get()->get_html_printer()->PageSetup();}, wxID_PRINT_SETUP);

  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    // In hex mode we cannot edit the file.
    if (wex::config("is_hexmode").get(false)) return;

    static std::string name = event.GetString();
    wxTextEntryDialog dlg(this, _("Input") + ":", _("File Name"), name);
    wxTextValidator validator(wxFILTER_EXCLUDE_CHAR_LIST);
    validator.SetCharExcludes("/\\?%*:|\"<>");
    dlg.SetTextValidator(validator);
    if (dlg.ShowModal() == wxID_CANCEL) return;

    if (name = dlg.GetValue(); !name.empty())
    {
      auto* page = new wex::stc(std::string(),
        wex::stc_data(m_app->data()).window(wex::window_data().
          parent(m_editors)));
      page->get_file().file_new(name);
      // This file does yet exist, so do not give it a bitmap.
      m_editors->add_page(page, name, name, true);
      show_pane("FILES");}}, wxID_NEW);
  
  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    m_process->stop(); 
    show_pane("PROCESS", false);}, wxID_STOP);

  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    m_editors->for_each<wex::stc>(event.GetId());}, 
    wex::ID_ALL_CLOSE, wex::ID_ALL_SAVE);

  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    open_file(wex::vi_macros::get_filename());}, ID_EDIT_MACRO);
  
  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    if (get_stc() != nullptr)
    {
      get_stc()->show_line_numbers(!get_stc()->is_shown_line_numbers());
    };}, idEditPaneInfoToggle);

  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    if (get_stc() != nullptr)
    {
      wxPostEvent(get_stc(), event);
    };}, wex::ID_EDIT_CONTROL_CHAR);

  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    wex::listview::config_dialog(wex::window_data().
      title(_("List Options").ToStdString()).
      button(wxOK | wxCANCEL | wxAPPLY).
      id(ID_OPTION_LIST));}, ID_OPTION_LIST);

  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    if (wex::vcs().config_dialog() == wxID_OK)
    { 
      wex::vcs vcs;
      vcs.set_entry_from_base(this);
      m_statusbar->show_field(
        "PaneVCS", 
        vcs.use());
      statustext(vcs.name(), "PaneVCS");
    };}, ID_OPTION_VCS);
    
  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    if (wex::process::config_dialog() == wxID_OK)
    {
      show_pane("PROCESS");
      m_process->execute();
    };}, ID_PROCESS_SELECT);

  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    if (auto* project = get_project();
      project != nullptr)
    {
      m_projects->delete_page(project->get_filename().string());
    };}, ID_PROJECT_CLOSE);

  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    const std::string text =
      wxString::Format("%s%d", _("project"), m_project_id++).ToStdString();
    const wex::path fn(
       (!get_project_history().get_history_file().empty() ? 
           get_project_history().get_history_file().get_path(): wex::config::dir()),
      text + ".prj");
    wxWindow* page = new wex::report::file(
      fn.string(), 
      wex::window_data().parent(m_projects));
      
    ((wex::report::file*)page)->file_new(fn.string());
    // This file does yet exist, so do not give it a bitmap.
    m_projects->add_page(page, fn.string(), text, true);
    set_recent_project(fn.string());
    show_pane("PROJECTS");}, 
    ID_PROJECT_NEW);

  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    wxFileDialog dlg(this,
      _("Select Projects"),
       (!get_project_history().get_history_file().empty() ? 
           get_project_history().get_history_file().get_path(): wex::config::dir()),
      wxEmptyString,
      m_project_wildcard,
      wxFD_OPEN | wxFD_MULTIPLE);
    if (dlg.ShowModal() == wxID_CANCEL) return;
    const std::vector < wex::path > v(
#ifdef __WXOSX__
      {dlg.GetPath().ToStdString()});
#else
      wex::to_vector_path(dlg).get());
#endif
    wex::open_files(this, v, wex::stc_data().flags(
        wex::stc_data::window_t().set(wex::stc_data::WIN_IS_PROJECT)));}, 
    ID_PROJECT_OPEN);

  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    if (auto* project = get_project(); project != nullptr)
    {
      if (wex::file_dialog(project).show_modal_if_changed() != wxID_CANCEL)
      {
        open_file(project->get_filename());
      }
    };}, 
    ID_PROJECT_OPENTEXT);
  
  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    if (auto* project = get_project();
      project != nullptr && m_projects != nullptr)
    {
      wex::file_dialog dlg(
        project, 
        wex::window_data().
          style(wxFD_SAVE).
          parent(this).
          title(_("Project Save As").ToStdString()),
        m_project_wildcard);
      if (dlg.ShowModal() == wxID_OK)
      {
        project->file_save(dlg.GetPath().ToStdString());
        m_projects->set_page_text(
          m_projects->key_by_page(project),
          project->get_filename().string(),
          project->get_filename().name());
      }
    };}, ID_PROJECT_SAVEAS);

  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    m_editors->rearrange(wxTOP);}, ID_REARRANGE_HORIZONTALLY);

  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    m_editors->rearrange(wxLEFT);}, ID_REARRANGE_VERTICALLY);
    
  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    wex::config("List/SortSync").set(
      !wex::config("List/SortSync").get(true));}, ID_SORT_SYNC);

  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    wex::vcs(std::vector< wex::path >(), event.GetId() - wex::ID_VCS_LOWEST - 1).request();},
    wex::ID_VCS_LOWEST, wex::ID_VCS_HIGHEST);

  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    if (m_ascii_table == nullptr)
    {
      m_ascii_table = new wex::stc();
      manager().AddPane(m_ascii_table, wxAuiPaneInfo()
        .Left()
        .Name("ASCIITABLE")
        .MinSize(500, 150)
        .Caption(_("Ascii Table")));
      manager().Update();
      // Do not show an edge, eol or whitespace for ascii table.
      m_ascii_table->SetEdgeMode(wxSTC_EDGE_NONE);
      m_ascii_table->SetViewEOL(false);
      m_ascii_table->SetViewWhiteSpace(wxSTC_WS_INVISIBLE);
      m_ascii_table->SetTabWidth(5);
      for (int i = 1; i <= 255; i++)
      {
        switch (i)
        {
          case  9: m_ascii_table->add_text(wxString::Format("%3d\tTAB", i)); break;
          case 10: m_ascii_table->add_text(wxString::Format("%3d\tLF", i)); break;
          case 13: m_ascii_table->add_text(wxString::Format("%3d\tCR", i)); break;
          default:
            m_ascii_table->add_text(wxString::Format("%3d\t%c", i, (wxUniChar)i));
        }
        m_ascii_table->add_text((i % 5 == 0) ? m_ascii_table->eol(): "\t");
      }
      m_ascii_table->EmptyUndoBuffer();
      m_ascii_table->SetSavePoint();
      m_ascii_table->SetReadOnly(true);
    }
    else
    {
      toggle_pane("ASCIITABLE"); 
    };}, ID_VIEW_ASCII_TABLE);
    
  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    toggle_pane("DIRCTRL");}, ID_VIEW_DIRCTRL);
    
  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    toggle_pane("FILES"); 
    if (!manager().GetPane("FILES").IsShown())
    {
      if (manager().GetPane("PROJECTS").IsShown())
      {
        manager().GetPane("PROJECTS").Maximize();
        manager().Update();
      }
    };}, ID_VIEW_FILES);
    
  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    if (m_history == nullptr)
    {
      add_pane_history();
      manager().Update();
    }
    else
    {
      toggle_pane("HISTORY");}}, ID_VIEW_HISTORY);
    
  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    toggle_pane("OUTPUT");}, ID_VIEW_OUTPUT);
    
  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    toggle_pane("PROJECTS");}, ID_VIEW_PROJECTS);
    
  Bind(wxEVT_SIZE, [=](wxSizeEvent& event) {
    event.Skip();
    if (IsMaximized())
    {   
      m_maximized = true;
    }
    else if (m_maximized)
    {
      if (m_editors->is_split())
      {
        m_editors->rearrange(wxLEFT);
        m_editors->reset();
      }

      m_maximized = false;
    };});

  Bind(wxEVT_UPDATE_UI, [=](wxUpdateUIEvent& event) {
    event.Check(
      m_ascii_table != nullptr && 
      manager().GetPane("ASCIITABLE").IsShown());}, ID_VIEW_ASCII_TABLE);
  Bind(wxEVT_UPDATE_UI, [=](wxUpdateUIEvent& event) {
    event.Check(manager().GetPane("DIRCTRL").IsShown());}, ID_VIEW_DIRCTRL);
  Bind(wxEVT_UPDATE_UI, [=](wxUpdateUIEvent& event) {
    event.Check(manager().GetPane("FILES").IsShown());}, ID_VIEW_FILES);
  Bind(wxEVT_UPDATE_UI, [=](wxUpdateUIEvent& event) {
    event.Check(m_history != nullptr && manager().GetPane("HISTORY").IsShown());}, 
    ID_VIEW_HISTORY);
  Bind(wxEVT_UPDATE_UI, [=](wxUpdateUIEvent& event) {
    event.Check(manager().GetPane("OUTPUT").IsShown());}, ID_VIEW_OUTPUT);
  Bind(wxEVT_UPDATE_UI, [=](wxUpdateUIEvent& event) {
    event.Check(m_projects != nullptr && manager().GetPane("PROJECTS").IsShown());}, 
    ID_VIEW_PROJECTS);
  
  Bind(wxEVT_UPDATE_UI, [=](wxUpdateUIEvent& event) {
    event.Enable(get_debug()->process() != nullptr);}, 
    wex::ID_EDIT_DEBUG_FIRST + 2, wex::ID_EDIT_DEBUG_LAST);
    
  m_app->reset();
}    

wex::report::listview* frame::activate(
  wex::listview_data::type_t type, 
  const wex::lexer* lexer)
{
  if (type == wex::listview_data::FILE)
  {
    return get_project();
  }
  else
  {
    show_pane("OUTPUT");

    const std::string name = wex::listview_data().type(type).type_description() +
      (lexer != nullptr ?  " " + lexer->display_lexer(): std::string());
    auto* list = (wex::report::listview*)m_lists->page_by_key(name);

    if (list == nullptr && type != wex::listview_data::FILE)
    {
      list = new wex::report::listview(
        wex::listview_data(wex::window_data().parent(m_lists)).
          type(type).
          lexer(lexer));

      m_lists->add_page(list, name, name, true);
    }

    return list;
  }
}

void frame::add_pane_history()
{
  m_history = new wex::report::listview(
    wex::listview_data().type(wex::listview_data::HISTORY));
        
  manager().AddPane(m_history, wxAuiPaneInfo()
    .Left()
    .MaximizeButton(true)
    .Name("HISTORY")
    .CloseButton(false)
    .MinSize(150, 150)
    .Caption(_("History")));
}

bool frame::exec_ex_command(wex::ex_command& command)
{
  if (command.command() == ":") return false;

  bool handled = false;

  if (m_editors->GetPageCount() > 0)
  {
    if (command.command() == ":n")
    {
      if (m_editors->GetSelection() == m_editors->GetPageCount() - 1) return false;
      
      m_editors->AdvanceSelection();
      handled = true;
    }
    else if (command.command() == ":prev")
    {
      if (m_editors->GetSelection() == 0) return false;
      
      m_editors->AdvanceSelection(false);
      handled = true;
    }

    if (handled && wex::ex::get_macros().mode()->is_playback())
    {
      command.set(((wex::stc *)m_editors->GetPage(
        m_editors->GetSelection()))->get_vi().get_command());
    }
  }

  return handled;
}

wex::process* frame::get_process(const std::string& command)
{
  if (!m_app->get_debug()) return nullptr;
  m_process->execute(command, wex::process::EXEC_NO_WAIT);
  return m_process;
}

wex::report::file* frame::get_project()
{
  if (m_projects == nullptr)
  {
    return nullptr;
  }

  if (!m_projects->IsShown() || 
       m_projects->GetPageCount() == 0)
  {
    return nullptr;
  }
  else
  {
    return (wex::report::file*)m_projects->
      GetPage(m_projects->GetSelection());
  }
}

bool frame::is_open(const wex::path& filename)
{
  return m_editors->page_index_by_key(filename.string()) != wxNOT_FOUND;
}

void frame::on_command(wxCommandEvent& event)
{
  auto* editor = get_stc();

  switch (event.GetId())
  {
  // edit commands
  // Do not change the wxID* in wxID_LOWEST and wdID_HIGHEST,
  // as wxID_ABOUT etc. is used here and not in the editor.
  // That causes appl to hang.
  case wxID_UNDO:
  case wxID_REDO:
  case wxID_CLEAR:
  case wxID_COPY:
  case wxID_CUT:
  case wxID_DELETE:
  case wxID_JUMP_TO:  
  case wxID_PASTE:
  case wxID_SELECTALL:
    if (editor != nullptr)
    {
      wxPostEvent(editor, event);
    }
    else if (get_listview() != nullptr)
    {
      wxPostEvent(get_listview(), event);
    }
  break;

  case wxID_CLOSE:
    if (editor != nullptr)
    {
      if (!allow_close(m_editors->GetId(), editor)) return;
      m_editors->delete_page(m_editors->key_by_page(editor));
    }
    break;
  case wxID_PREVIEW:
    if (editor != nullptr)
    {
      editor->print_preview();
    }
    else if (get_listview() != nullptr)
    {
      get_listview()->print_preview();
    }
    break;
  case wxID_PRINT:
    if (editor != nullptr)
    {
      editor->print();
    }
    else if (get_listview() != nullptr)
    {
      get_listview()->print();
    }
    break;
  case wxID_SAVE:
    if (editor != nullptr)
    {
      if (!editor->IsModified() || !editor->get_file().file_save()) return;

      set_recent_file(editor->get_filename());
      
      if (editor->get_filename() == wex::lexers::get()->get_filename())
      {
        if (wex::lexers::get()->load_document())
        {
          m_editors->for_each<wex::stc>(wex::ID_ALL_STC_SET_LEXER);
          update_listviews();

          // As the lexer might have changed, update status bar field as well.
          update_statusbar(editor, "PaneLexer");
        }
      }
      else if (editor->get_filename() == wex::menus::get_filename())
      {
        wex::vcs::load_document();
      }
      else if (editor->get_filename() == wex::vi_macros::get_filename())
      {
        wex::vi_macros::load_document();
      }
    }
    break;
  case wxID_SAVEAS:
    if (editor != nullptr)
    {
      if (!event.GetString().empty())
      {
        if (!editor->get_file().file_save(event.GetString().ToStdString()))
        {
          return;
        }
      }
      else
      {
        wex::file_dialog dlg(
          &editor->get_file(), 
          wex::window_data().
            style(wxFD_SAVE). 
            parent(this).
            title(wxGetStockLabel(wxID_SAVEAS, wxSTOCK_NOFLAGS).ToStdString()));

        if (dlg.ShowModal() != wxID_OK)
        {
          return;
        }

        if (!editor->get_file().file_save(dlg.GetPath().ToStdString()))
        {
          return;
        }
      }
      
      const wxBitmap bitmap = (editor->get_filename().stat().is_ok() ? 
        wxTheFileIconsTable->GetSmallImageList()->GetBitmap(
          wex::get_iconid(editor->get_filename())) : 
        wxNullBitmap);

      m_editors->set_page_text(
        m_editors->key_by_page(editor),
        editor->get_filename().string(),
        editor->get_filename().fullname(),
        bitmap);
          
      editor->properties_message();
      
      set_recent_file(editor->get_filename());
    }
    break;

  case ID_EDIT_MACRO_PLAYBACK: 
    if (editor != nullptr) 
      editor->get_vi().get_macros().mode()->transition(
        "@", &editor->get_vi(), true); 
    break;
  case ID_EDIT_MACRO_START_RECORD: 
  case ID_EDIT_MACRO_STOP_RECORD: 
    if (editor != nullptr) 
      editor->get_vi().get_macros().mode()->transition(
        "q", &editor->get_vi(), true); 
    break;
  
  case ID_SPLIT:
  case ID_SPLIT_HORIZONTALLY:
  case ID_SPLIT_VERTICALLY:
    if (editor == nullptr)
    {
      wex::log::status("No valid focus");
    }
    else
    {
      auto* stc = new wex::stc(editor->get_filename(), 
        wex::stc_data().window(wex::window_data().parent(m_editors)));
      editor->sync(false);
      stc->sync(false);
      stc->get_vi().copy(&editor->get_vi());

      wxBitmap bitmap(wxNullBitmap);
      
      if (editor->get_filename().file_exists())
      {
        bitmap = wxTheFileIconsTable->GetSmallImageList()->GetBitmap(
          wex::get_iconid(editor->get_filename()));
      }
      else if (!editor->get_lexer().scintilla_lexer().empty())
      {
        stc->get_lexer().set(editor->get_lexer().scintilla_lexer());
      }
      
      // key should be unique
      const std::string key("split" + std::to_string(m_split_id++));
      
      // Place new page before page for editor.
      m_editors->insert_page(
        m_editors->GetPageIndex(editor),
        stc,
        key,
        editor->get_filename().fullname(),
        true,
        bitmap);

      stc->SetDocPointer(editor->GetDocPointer());
      
      if (event.GetId() == ID_SPLIT_HORIZONTALLY)
      {
        m_editors->split(key, wxBOTTOM);
        m_editors->set_selection(editor->get_filename().string());
      }
      else if (event.GetId() == ID_SPLIT_VERTICALLY)
      {
        m_editors->split(key, wxRIGHT);
        m_editors->set_selection(editor->get_filename().string());
      }
    }
    break;
    
  default: 
      assert(0);
    break;
  }
}

void frame::on_command_item_dialog(
  wxWindowID dialogid,
  const wxCommandEvent& event)
{
  switch (dialogid)
  {
    case wxID_PREFERENCES:
      if (event.GetId() != wxID_CANCEL)
      {
        m_editors->for_each<wex::stc>(wex::ID_ALL_CONFIG_GET);
        
        if (m_process->get_shell() != nullptr)
        {
          m_process->get_shell()->config_get();
        }
        
        m_statusbar->show_field(
          "PaneMacro", 
          wex::config(_("vi mode")).get(true));
      }
      break;

    case ID_OPTION_LIST:
      if (event.GetId() != wxID_CANCEL)
      {
        update_listviews();
      }
      break;
    
    default:
      decorated_frame::on_command_item_dialog(dialogid, event);
  }
}

void frame::on_update_ui(wxUpdateUIEvent& event)
{
  switch (event.GetId())
  {
    case wxID_EXECUTE: 
      event.Enable(
        !is_closing() && m_process != nullptr &&
		    !m_process->get_exec().empty() && !m_process->is_running()); 
      break;
    case wxID_STOP: event.Enable(m_process->is_running()); break;
    case wxID_PREVIEW:
    case wxID_PRINT:
      event.Enable(
        (get_stc() != nullptr && get_stc()->GetLength() > 0) ||
        (get_listview() != nullptr && get_listview()->GetItemCount() > 0));
      break;

    case wxID_CLOSE:
    case wxID_SAVEAS:
      event.Enable(m_editors->IsShown() && m_editors->GetPageCount() > 0);
    break;
    
    case wex::ID_ALL_CLOSE:
    case wex::ID_ALL_SAVE:
      event.Enable(m_editors->GetPageCount() > 2);
    break;

    case ID_OPTION_VCS:
      event.Enable(wex::vcs::size() > 0);
      break;

    case ID_PROJECT_CLOSE:
    case ID_PROJECT_SAVEAS:
      event.Enable(get_project() != nullptr && get_project()->IsShown());
      break;
    case ID_PROJECT_OPENTEXT:
      event.Enable(
        get_project() != nullptr && !get_project()->get_filename().empty());
      break;
    case wex::ID_PROJECT_SAVE:
      event.Enable(get_project() != nullptr && get_project()->get_contents_changed());
      break;

    case ID_RECENT_FILE_MENU:
      event.Enable(!file_history().get_history_file().empty());
      break;
    case ID_RECENT_PROJECT_MENU:
      event.Enable(!get_project_history().get_history_file().empty());
      break;

    case ID_SORT_SYNC:
      event.Check(wex::config("List/SortSync").get(true));
      break;

    default:
    {
      if (auto* editor = get_stc(); editor != nullptr)
      {
        event.Enable(true);

        switch (event.GetId())
        {
        case wxID_FIND:
        case wxID_JUMP_TO:
        case wxID_REPLACE:
        case wex::ID_EDIT_FIND_NEXT:
        case wex::ID_EDIT_FIND_PREVIOUS:
          event.Enable(editor->GetLength() > 0);
          break;
        case ID_EDIT_MACRO:
          event.Enable(
             editor->get_vi().is_active() &&
            !editor->get_vi().get_macros().mode()->is_recording() &&
             wex::vi_macros::get_filename().file_exists());
          break;
        case ID_EDIT_MACRO_MENU:
          event.Enable(
             editor->get_vi().is_active());
          break;
        case ID_EDIT_MACRO_PLAYBACK:
          event.Enable(
             editor->get_vi().is_active() &&
             editor->get_vi().get_macros().size() > 0 &&
            !editor->get_vi().get_macros().mode()->is_recording());
          break;
        case ID_EDIT_MACRO_START_RECORD:
          event.Enable(
             editor->get_vi().is_active() && 
            !editor->get_vi().get_macros().mode()->is_recording());
          break;
        case ID_EDIT_MACRO_STOP_RECORD:
          event.Enable(editor->get_vi().get_macros().mode()->is_recording());
          break;

        case wxID_SAVE:
          event.Enable(
            !editor->get_filename().empty() &&
             editor->GetModify());
          break;
        case wxID_REDO:
          event.Enable(editor->CanRedo());
          break;
        case wxID_UNDO:
          event.Enable(editor->CanUndo());
          break;
          
        case wex::ID_EDIT_CONTROL_CHAR:
          if (editor->GetReadOnly() && editor->GetSelectedText().length() != 1)
          {
            event.Enable(false);
          }
          break;

        default:
          assert(0);
        }
      }
      else if (auto* list = (wex::report::file*)get_listview();
        list != nullptr && list->IsShown())
      {
        event.Enable(false);

        if (
          event.GetId() > wex::ID_TOOL_LOWEST &&
          event.GetId() < wex::ID_TOOL_HIGHEST)
        {
          event.Enable(list->GetSelectedItemCount() > 0);
        }
        else if (event.GetId() == wxID_FIND)
        {
          event.Enable(list->GetItemCount() > 0);
        }
      }
      else
      {
        event.Enable(false);
      }
    }
  }
}

wex::stc* frame::open_file(
  const wex::path& filename,
  const wex::vcs_entry& vcs,
  const wex::stc_data& data)
{
  if (vcs.get_command().is_blame())
  {
    if (auto* page = (wex::stc*)m_editors->set_selection(filename.string());
      page != nullptr)
    {
      if (page->show_blame(&vcs)) return page;
    }
  }

  const std::string unique = 
    vcs.get_command().get_command() + " " + vcs.get_flags();
  const std::string key = filename.string() + " " + unique;

  auto* page = (wex::stc*)m_editors->set_selection(key);
  
  if (page == nullptr)
  {
    page = new wex::stc(
      vcs.get_stdout(),
      wex::stc_data(data).window(wex::window_data().
        parent(m_editors).
        name(filename.fullname() + " " + unique)));

    wex::vcs_command_stc(
      vcs.get_command(), filename.lexer(), page);
      
    if (const int index = m_editors->page_index_by_key(filename.string());
      index != -1)
    {
      // Place new page before the one used for vcs.
      m_editors->insert_page(index, page, key, filename.fullname() + " " + unique, true);
    }
    else
    {
      // Just add at the end.
      m_editors->add_page(page, key, filename.fullname() + " " + unique, true);
    }
  }

  return page;
}

wex::stc* frame::open_file(
  const wex::path& filename,
  const std::string& text,
  const wex::stc_data& data)
{
  auto* page = (wex::stc*)m_editors->set_selection(filename.string());

  if (page == nullptr)
  {
    page = new wex::stc(text, 
      wex::stc_data(data).window(wex::window_data().
        parent(m_editors).
        name(filename.string())));
    page->get_lexer().set(filename.lexer());
    m_editors->add_page(page, filename.string(), filename.fullname(), true);
  }
  else
  {
    page->SetText(text);
  }

  return page;
}
  
wex::stc* frame::open_file(const wex::path& filename, const wex::stc_data& data)
{
  wex::notebook* notebook = (data.flags().test(wex::stc_data::WIN_IS_PROJECT)
    ? m_projects : m_editors);
    
  assert(notebook != nullptr);
  
  wxWindow* page = notebook->set_selection(filename.string());

  if (data.flags().test(wex::stc_data::WIN_IS_PROJECT))
  {
    if (page == nullptr)
    {
      auto* project = new wex::report::file(
        filename.string(), wex::window_data().parent(m_projects));

      notebook->add_page(
        project,
        filename.string(),
        filename.name(),
        true,
        wxTheFileIconsTable->GetSmallImageList()->GetBitmap(
          wex::get_iconid(filename)));
    }

    if (!manager().GetPane("PROJECTS").IsShown())
    {
      show_pane("PROJECTS");
    }
  }
  else
  {
    if (!manager().GetPane("FILES").IsShown())
    {
      if (manager().GetPane("PROJECTS").IsMaximized())
      {
        manager().GetPane("PROJECTS").Restore();
      }

      show_pane("FILES");
    }

    if (filename == wex::vi_macros::get_filename())
    {
      wex::vi_macros::save_document();
    }
    
    auto* editor = (wex::stc*)page;

    if (page == nullptr)
    {
      wex::stc_data::menu_t mf(m_app->data().menu());
      wex::stc_data::window_t wf(m_app->data().flags());
    
      if (wex::config("is_hexmode").get(false)) wf.set(wex::stc_data::WIN_HEX);
      if (m_app->get_debug()) mf.set(wex::stc_data::MENU_DEBUG);
      
      editor = new wex::stc(filename, 
        wex::stc_data(data).
          window(wex::window_data().
            parent(m_editors)).flags(wf, wex::control_data::OR).
          menu(mf));
      
      if (m_app->get_debug())
      {
        get_debug()->apply_breakpoints(editor);
      }

      const std::string key(filename.string());

      notebook->add_page(
        editor,
        key,
        filename.fullname(),
        true,
        wxTheFileIconsTable->GetSmallImageList()->GetBitmap(
          wex::get_iconid(filename)));
          
      if (notebook->GetPageCount() >= 2 && m_app->get_split() != -1)
      {
        notebook->split(key, m_app->get_split());
      }
      
      if (manager().GetPane("DIRCTRL").IsShown())
      {
        m_dirctrl->expand_and_select_path(key);
      }
      
      // Do not show an edge for project files opened as text.
      if (filename.extension() == ".prj")
      {
        editor->SetEdgeMode(wxSTC_EDGE_NONE);
      }

      if (wex::config(_("Auto blame")).get(false))
      {
        if (wex::vcs vcs {{filename}};
          vcs.execute("blame " + filename.string()))
        {
          editor->show_blame(&vcs.entry());
        }
      }

      if (!m_app->get_scriptin().empty())		
      {
        editor->get_vi().command(":so " + m_app->get_scriptin());
      }
    }
    else
    {
      wex::stc_data(editor, data).inject();
    }
    
    editor->SetFocus();
    
    return editor;
  }
  
  return (wex::stc*)page;
}

void frame::print_ex(wex::ex* ex, const std::string& text)
{
  auto* page = (wex::stc*)m_editors->set_selection("Print");

  if (page == nullptr)
  {
    page = new wex::stc(text, wex::stc_data().
      window(wex::window_data().name("Print").parent(m_editors)));
    m_editors->add_page(page, "Print", "Print", true);
    m_editors->split("Print", wxBOTTOM);
  }
  else
  {
    page->AppendText(text);
    page->DocumentEnd();
    page->SetSavePoint();
  }
  
  page->get_lexer().set(ex->get_stc()->get_lexer());
}
  
void frame::record(const std::string& command)
{
  if (!m_app->get_scriptout().empty())
  {
    wex::file scriptout(
      m_app->get_scriptout(), std::ios_base::out | std::ios_base::app);
    scriptout.write(command + "\n");
  }
}

wex::stc* frame::restore_page(const std::string& key)
{
  if (!m_saved_page.empty() && is_open(m_saved_page))
  {
    m_editors->change_selection(m_saved_page);
    return (wex::stc *)m_editors->page_by_key(m_saved_page);
  }
  
  return nullptr;
}
  
bool frame::save_current_page(const std::string& key)
{
  m_saved_page = m_editors->current_page_key();
  return true;
}

void frame::statusbar_clicked(const std::string& pane)
{
  if (pane == "PaneDBG")
  {
    if (get_debug()->show_dialog(this))
    {
      statustext(get_debug()->debug_entry().name(), "PaneDBG");
    }
  }
  else if (pane == "PaneMacro")
  {
    if (auto* editor = get_stc(); editor != nullptr) 
      editor->get_vi().get_macros().mode()->transition(
        "@", &editor->get_vi(), true);
  }
  else if (pane == "PaneTheme")
  {
    if (wex::lexers::get()->show_theme_dialog(m_editors))
    {
      m_editors->for_each<wex::stc>(wex::ID_ALL_STC_SET_LEXER_THEME);
      update_listviews();

      m_statusbar->show_field(
        "PaneLexer", 
        !wex::lexers::get()->theme().empty());
        
      statustext(wex::lexers::get()->theme(), "PaneTheme");
    }
  }
  else if (pane == "PaneVCS")
  {
    if (wex::vcs::size() > 0)
    {
      auto* menu = new wex::menu;
      
      if (menu->append_vcs())
      {
        PopupMenu(menu);
      }
      
      delete menu;
    }
  }
  else
  {
    decorated_frame::statusbar_clicked(pane);
  }
}

void frame::statusbar_clicked_right(const std::string& pane)
{
  if (pane == "PaneInfo")
  {
    if (auto* stc = get_stc(); stc != nullptr)
    {
      wxMenu* menu = new wxMenu();
      menu->Append(idEditPaneInfoToggle, 
        stc->is_shown_line_numbers() ? "&Hide": "&Show");
      PopupMenu(menu);
    }
  }
  else if (pane == "PaneLexer" || pane == "PaneTheme")
  {
    std::string match;
    
    if (pane == "PaneLexer")
    {
      if (auto* stc = get_stc(); stc != nullptr)
      {
        if (
          !stc->get_lexer().scintilla_lexer().empty() && 
           stc->get_lexer().scintilla_lexer() == stc->get_lexer().display_lexer())
        {
          match = "lexer *name *= *\"" + stc->get_lexer().scintilla_lexer() + "\"";
        }
        else if (!stc->get_lexer().display_lexer().empty())
        {
          match = "display *= *\"" + stc->get_lexer().display_lexer() + "\"";
        }
        else
        {
          return;
        }
      }
    }
    else
    {
      if (wex::lexers::get()->theme().empty())
      {
        return;
      }
      
      match = "theme *name *= *\"" + wex::lexers::get()->theme() + "\"";
    }
    
    open_file(wex::lexers::get()->get_filename(),
      wex::control_data().find(match, wxSTC_FIND_REGEXP | wxSTC_FIND_CXX11REGEX));
  }
  else if (pane == "PaneMacro")
  {
    if (wex::vi_macros::get_filename().file_exists())
    {
      open_file(wex::vi_macros::get_filename(),
        wex::control_data().find(!get_statustext(pane).empty() ? 
          " name=\"" + get_statustext(pane) + "\"":
          std::string()));
    }
  }
  else if (pane == "PaneDBG" || pane == "PaneVCS")
  {
    std::string match(get_statustext(pane));

    if (auto* stc = get_stc(); stc != nullptr)
    {
      match = (pane == "PaneVCS" ?
        wex::vcs({stc->get_filename().string()}).entry().name():
        wex::debug(this).debug_entry().name());
    }

    open_file(wex::menus::get_filename(), wex::control_data().find(match));
  }
  else
  {
    decorated_frame::statusbar_clicked_right(pane);
  }
}

void frame::sync_all()
{
  m_editors->for_each<wex::stc>(wex::ID_ALL_STC_SYNC);
}

void frame::sync_close_all(wxWindowID id)
{
  decorated_frame::sync_close_all(id);
  
  if (is_closing()) return;
  
  switch (id)
  {
  case wex::ID_NOTEBOOK_EDITORS:
    SetTitle(wxTheApp->GetAppDisplayName());
    statustext(std::string(), std::string());
    statustext(std::string(), "PaneFileType");
    statustext(std::string(), "PaneInfo");
    statustext(std::string(), "PaneLexer");
    
    if (manager().GetPane("PROJECTS").IsShown() && m_projects != nullptr)
    {
      manager().GetPane("PROJECTS").Maximize();
      manager().Update();
    }
    break;
  case wex::ID_NOTEBOOK_LISTS: show_pane("OUTPUT", false); break;
  case wex::ID_NOTEBOOK_PROJECTS: show_pane("PROJECTS", false); break;
  default: assert(0);
  }
}

void frame::update_listviews()
{
  m_lists->for_each<wex::report::file>(wex::ID_ALL_CONFIG_GET);

  if (m_projects != nullptr) 
    m_projects->for_each<wex::report::file>(wex::ID_ALL_CONFIG_GET);

  if (m_history != nullptr)
    m_history->config_get();

  if (wex::item_dialog* dlg = wex::stc::get_config_dialog();
    dlg != nullptr)
  {
    const wex::item item (dlg->get_item(_("Include directory")));
    
    if (wex::listview* lv = (wex::listview*)item.window(); lv != nullptr)
    {
      lv->config_get();
    }
  }
}
          
editors::editors(const wex::window_data& data)
  : wex::notebook(data)
{
  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    wxPostEvent(wxAuiNotebook::GetCurrentPage(), event);
    }, wex::ID_EDIT_VCS_LOWEST, wex::ID_EDIT_VCS_HIGHEST);

  Bind(wxEVT_AUINOTEBOOK_END_DRAG, [=](wxAuiNotebookEvent& event) {
    event.Skip();
    m_split = true;
    });

  Bind(wxEVT_AUINOTEBOOK_TAB_RIGHT_UP, [=](wxAuiNotebookEvent& event) {
    wex::menu menu(wex::menu::menu_t().set(wex::menu::IS_POPUP));
    
    auto* split = new wex::menu;
    split->append(ID_SPLIT_VERTICALLY, _("Split Vertically"));
    split->append(ID_SPLIT_HORIZONTALLY, _("Split Horizontally"));
    split->append_separator();
    split->append(ID_SPLIT, _("Split"));
    
    if (GetPageCount() > 1)
    {
      split->append_separator();
      split->append(ID_REARRANGE_VERTICALLY, _("Rearrange Vertically"));
      split->append(ID_REARRANGE_HORIZONTALLY, _("Rearrange Horizontally"));
    }

    menu.append_submenu(split, _("Split"), std::string(), ID_SPLIT_MENU);
    menu.append_separator();
    menu.append(wxID_CLOSE);
    menu.append(wex::ID_ALL_CLOSE, _("Close A&ll"));
    
    if (GetPageCount() > 2)
    {
      menu.append(wex::ID_ALL_CLOSE_OTHERS, _("Close Others"));
    }

    if (auto* stc = wxDynamicCast(wxAuiNotebook::GetCurrentPage(), wex::stc);
      stc->get_file().get_filename().file_exists() && 
        wex::vcs::dir_exists(stc->get_file().get_filename()))
    {
      menu.append_separator();
      menu.append_vcs(stc->get_file().get_filename());
    }
    
    PopupMenu(&menu);});
}
