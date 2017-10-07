////////////////////////////////////////////////////////////////////////////////
// Name:      app.cpp
// Purpose:   Implementation of classes for syncsocketserver
// Author:    Anton van Wezenbeek
// Copyright: (c) 2017 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <functional>
#include <algorithm>
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/aboutdlg.h>
#include <wx/config.h>
#include <wx/numdlg.h>
#include <wx/extension/filedlg.h>
#include <wx/extension/grid.h>
#include <wx/extension/itemdlg.h>
#include <wx/extension/lexers.h>
#include <wx/extension/statistics.h>
#include <wx/extension/toolbar.h>
#include <wx/extension/util.h>
#include <wx/extension/version.h>
#include "app.h"

#ifndef __WXMSW__
#include "app.xpm"
#include "connect.xpm"
#include "notready.xpm"
#include "ready.xpm"
#endif

#ifdef __WXOSX__
  #undef wxUSE_TASKBARICON
#endif

#ifdef __WXGTK__
  #undef wxUSE_TASKBARICON
#endif

#if wxUSE_SOCKETS

enum
{
  ANSWER_OFF,
  ANSWER_ECHO,
  ANSWER_COMMAND,
  ANSWER_FILE,
};

enum
{
  DATA_MESSAGE,
  DATA_MESSAGE_RAW,
  DATA_READ,
  DATA_WRITE,
};

enum
{
  ID_CLEAR_STATISTICS,
  ID_CLIENT_BUFFER_SIZE,
  ID_CLIENT_ANSWER_COMMAND,
  ID_CLIENT_ANSWER_ECHO,
  ID_CLIENT_ANSWER_FILE,
  ID_CLIENT_ANSWER_OFF,
  ID_CLIENT_LOG_DATA,
  ID_CLIENT_LOG_DATA_COUNT_ONLY,
  ID_HIDE,
  ID_RECENT_FILE_MENU,
  ID_SERVER_CONFIG,
  ID_SETUP_REMOTE_SERVER,
  ID_REMOTE_SERVER_DISCONNECT,
  ID_REMOTE_SERVER_CONFIG,
  ID_TIMER_STOP,
  ID_TIMER_START,
  ID_VIEW_DATA,
  ID_VIEW_LOG,
  ID_VIEW_SHELL,
  ID_VIEW_STATISTICS,
  ID_WRITE_DATA,

  ID_SERVER,
  ID_SERVER_CLIENT,
  ID_CLIENT
};

wxIMPLEMENT_APP(App);

bool App::OnInit()
{
  SetAppName("syncsocketserver");

  if (!wxExApp::OnInit())
  {
    return false;
  }

  new Frame;

  return true;
}

Frame::Frame()
  : wxExFrameWithHistory()
  , m_Timer(this)
  , m_Answer(ANSWER_OFF)
  , m_DataWindow(new wxExSTC)
  , m_LogWindow(new wxExSTC(std::string(), wxExSTCData().Flags(STC_WIN_NO_INDICATOR)))
  , m_Shell(new wxExShell)
{
  SetIcon(wxICON(app));

#if wxUSE_TASKBARICON
  m_TaskBarIcon = new TaskBarIcon(this);
#endif

  Show(); // otherwise statusbar is not placed correctly

#if wxUSE_STATUSBAR
  // Statusbar setup before STC construction.
  SetupStatusBar({
    {},
    {"PaneConnections", 75, _("Number of connections").ToStdString()},
    {"PaneTimer", 75, _("Repeat timer").ToStdString()},
    {"PaneBytes", 150, _("Number of bytes received and sent").ToStdString()},
    {"PaneFileType", 50, _("File type").ToStdString()},
    {"PaneTheme", 50, _("Theme").ToStdString()},
    {"PaneInfo", 100, _("Lines").ToStdString()}});
#endif

  m_LogWindow->ResetMargins();

  wxExMenu* menuFile = new wxExMenu();
  menuFile->Append(wxID_NEW);
  menuFile->Append(wxID_OPEN);
  GetFileHistory().UseMenu(ID_RECENT_FILE_MENU, menuFile);
  menuFile->AppendSeparator();
  menuFile->Append(wxID_SAVE);
  menuFile->Append(wxID_SAVEAS);
  menuFile->AppendSeparator();
  menuFile->Append(ID_CLEAR_STATISTICS, 
    _("Clear Statistics"), _("Clears the statistics"));
  menuFile->AppendSeparator();
#if wxUSE_TASKBARICON
  menuFile->Append(ID_HIDE, _("Hide"), _("Puts back in the task bar"));
#else
  menuFile->Append(wxID_EXIT);
#endif

  wxMenu* menuOther = new wxMenu();
  menuOther->Append(ID_REMOTE_SERVER_CONFIG, _("Configuration"), 
    _("Configures the remote server"));
  menuOther->AppendSeparator();
  menuOther->Append(ID_SETUP_REMOTE_SERVER, _("Connect"), 
    _("Tries to connect to remote server"));
  menuOther->Append(ID_REMOTE_SERVER_DISCONNECT, _("Disconnect"), 
    _("Disconnects from remote server"));

  wxMenu* menuServer = new wxMenu();
  menuServer->Append(ID_SERVER_CONFIG, _("Configuration"), 
    _("Configures the server"));
  menuServer->AppendSeparator();
  menuServer->AppendSubMenu(menuOther, _("&Remote"));
  menuServer->AppendSeparator();
  menuServer->Append(wxID_EXECUTE);
  menuServer->Append(wxID_STOP);

  wxMenu* menuAnswer = new wxMenu();
  menuAnswer->AppendRadioItem(ID_CLIENT_ANSWER_OFF, _("Off"),
    _("No answer back to connection"));
  menuAnswer->AppendRadioItem(ID_CLIENT_ANSWER_ECHO, _("Echo"),
    _("Echo's received data back to connection"));
  menuAnswer->AppendRadioItem(ID_CLIENT_ANSWER_COMMAND, _("Command"),
    _("Send last shell command back to connection"));
  menuAnswer->AppendRadioItem(ID_CLIENT_ANSWER_FILE, _("File"),
    _("Send file contents back to connection"));
    
  wxMenu* menuConnection = new wxMenu();
  menuConnection->AppendSubMenu(menuAnswer, _("&Answer"));
  menuConnection->AppendSeparator();
  menuConnection->AppendCheckItem(ID_CLIENT_LOG_DATA, _("Log Data"),
    _("Logs data read from and written to connection"));
  menuConnection->AppendCheckItem(ID_CLIENT_LOG_DATA_COUNT_ONLY, _("Count Only"),
    _("Logs only byte counts, no text"));
  menuConnection->AppendSeparator();
  menuConnection->Append(ID_CLIENT_BUFFER_SIZE, wxExEllipsed(_("Buffer Size")),
    _("Sets buffersize for data retrieved from connection"));
  menuConnection->AppendSeparator();
  menuConnection->Append(ID_TIMER_START, wxExEllipsed(_("Repeat Timer")),
    _("Repeats with timer writing last data to all connections"));
  menuConnection->Append(ID_TIMER_STOP, _("Stop Timer"), _("Stops the timer"));
  menuConnection->AppendSeparator();
  // Adding bitmap results in a checkable menu item.
  menuConnection->Append(ID_WRITE_DATA, _("Write"), _("Writes data to all connections"));

  wxExMenu* menuView = new wxExMenu();
  AppendPanes(menuView);
  menuView->AppendSeparator();
  menuView->AppendCheckItem(ID_VIEW_LOG, _("Log"));
  menuView->AppendCheckItem(ID_VIEW_DATA, _("Data"));
  menuView->AppendCheckItem(ID_VIEW_SHELL, _("Shell"));
  menuView->AppendCheckItem(ID_VIEW_STATISTICS, _("Statistics"));

  wxMenu* menuOptions = new wxMenu();
  menuOptions->Append(wxID_PREFERENCES);

  wxMenu* menuHelp = new wxMenu();
  menuHelp->Append(wxID_ABOUT);

  wxMenuBar* menuBar = new wxMenuBar();
  menuBar->Append(menuFile, wxGetStockLabel(wxID_FILE));
  menuBar->Append(menuView, _("&View"));
  menuBar->Append(menuServer, _("&Server"));
  menuBar->Append(menuConnection, _("&Connection"));
#ifndef __WXOSX__
  menuBar->Append(menuOptions, _("&Options"));
#endif
  menuBar->Append(menuHelp, wxGetStockLabel(wxID_HELP));
  SetMenuBar(menuBar);

  GetManager().AddPane(m_LogWindow,
    wxAuiPaneInfo().CenterPane().MaximizeButton(true).Caption(_("Log")).
      Name("LOG"));
  GetManager().AddPane(m_DataWindow,
    wxAuiPaneInfo().Hide().Left().
      MaximizeButton(true).Caption(_("Data")).
      Name("DATA"));
  GetManager().AddPane(m_Shell,
    wxAuiPaneInfo().Hide().Left().
      MaximizeButton(true).Caption(_("Shell")).
      Name("SHELL"));
  GetManager().AddPane(m_Statistics.Show(this),
    wxAuiPaneInfo().Hide().Left().
      MaximizeButton(true).Caption(_("Statistics")).
      Name("STATISTICS"));

  GetManager().LoadPerspective(wxConfigBase::Get()->Read("Perspective"));

  if (SetupSocketServer())
  {
#if wxUSE_TASKBARICON
    Hide();
#endif
  }

  if (!GetFileHistory().GetHistoryFile().Path().empty() && GetManager().GetPane("DATA").IsShown())
  {
    OpenFile(GetFileHistory().GetHistoryFile());
  }

  if (GetManager().GetPane("SHELL").IsShown())
  {
    m_Shell->SetFocus();
    m_Shell->DocumentEnd();
  }

  GetToolBar()->AddControls(false); // no realize yet
  GetToolBar()->AddTool(
    ID_WRITE_DATA,
    wxEmptyString,
    wxArtProvider::GetBitmap(
      wxART_GO_FORWARD, wxART_TOOLBAR, GetToolBar()->GetToolBitmapSize()),
    _("Write data"));
  GetToolBar()->Realize();
    
  GetOptionsToolBar()->AddControls();
  
  GetManager().Update();
  
  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    SetupSocketServer();}, wxID_EXECUTE);
    
  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    if (m_SocketRemoteServer != nullptr)
    {
      // In fact, this should not happen, 
      // but using Ubuntu the OnUpdateUI does not prevent this...
      wxLogStatus(_("First stop remote server"));
      return false;
    }
    
    // Create the address - defaults to localhost and port as specified
    if (!wxConfigBase::Get()->Exists(_("Hostname")))
    {
      wxConfigBase::Get()->SetRecordDefaults(true);
    }

    wxIPV4address addr;
    addr.Hostname(wxConfigBase::Get()->Read(_("Hostname"), "localhost"));
    addr.Service(wxConfigBase::Get()->ReadLong(_("Port"), 3000));

    if (wxConfigBase::Get()->IsRecordingDefaults())
    {
      wxConfigBase::Get()->SetRecordDefaults(false);
    }

    m_SocketRemoteServer = new wxSocketClient();

    m_SocketRemoteServer->SetEventHandler(*this, ID_SERVER_CLIENT);
    m_SocketRemoteServer->SetNotify(wxSOCKET_CONNECTION_FLAG);
    m_SocketRemoteServer->Notify(true);
    m_SocketRemoteServer->Connect(addr, false);
    }, ID_SETUP_REMOTE_SERVER);
    
  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    if (m_SocketRemoteServer == nullptr) return;
    SocketLost(m_SocketRemoteServer, false);
    m_SocketRemoteServer = nullptr;
    }, ID_REMOTE_SERVER_DISCONNECT);
    
  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    if (wxMessageBox(_("Stop server?"),
      _("Confirm"),
      wxOK | wxCANCEL | wxICON_QUESTION) == wxCANCEL)
    {
      return;
    }
    for (auto& it : m_Clients)
    {
      SocketLost(it, false);
    }
    m_Clients.clear();
    m_SocketServer->Destroy();
    m_SocketServer = nullptr;
    const wxString text = _("server stopped");

#if wxUSE_TASKBARICON
    m_TaskBarIcon->SetIcon(wxICON(notready), text);
#endif
    UpdateConnectionsPane();
    wxLogStatus(text);
    AppendText(m_LogWindow, text, DATA_MESSAGE);
    const wxString statistics = m_Statistics.Get();
    if (!statistics.empty())
    {
      AppendText(m_LogWindow, statistics, DATA_MESSAGE);
    }}, wxID_STOP);
    
  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    wxAboutDialogInfo info;
    info.SetIcon(GetIcon());
    info.SetDescription(_("This program offers a general socket server."));
    info.SetVersion(wxExGetVersionInfo().GetVersionOnlyString());
    info.SetCopyright(wxExGetVersionInfo().GetCopyright());
    wxAboutBox(info);
    }, wxID_ABOUT);

  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    Close(true);}, wxID_EXIT);

  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {;}, wxID_HELP);
  
  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    m_DataWindow->GetFile().FileNew(wxExPath());
    ShowPane("DATA");}, wxID_NEW);

  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    wxExOpenFilesDialog(
      this, 
      wxFD_OPEN | wxFD_CHANGE_DIR, 
      wxFileSelectorDefaultWildcardStr, 
      true);}, wxID_OPEN);

  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    m_DataWindow->GetFile().FileSave();}, wxID_SAVE);

  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    wxExFileDialog dlg(
      &m_DataWindow->GetFile(), 
      wxExWindowData().
        Style(wxFD_SAVE).
        Parent(this).
        Title(wxGetStockLabel(wxID_SAVEAS).ToStdString()));
    if (dlg.ShowModal())
    {
      m_DataWindow->GetFile().FileSave(dlg.GetPath().ToStdString());
    }}, wxID_SAVEAS);

  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    m_Statistics.Clear();}, ID_CLEAR_STATISTICS);

  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    m_Answer = ANSWER_COMMAND;}, ID_CLIENT_ANSWER_COMMAND);
  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    m_Answer = ANSWER_ECHO;}, ID_CLIENT_ANSWER_ECHO);
  Bind(wxEVT_MENU, [=](wxCommandEvent& event) { 
    m_Answer = ANSWER_FILE;}, ID_CLIENT_ANSWER_FILE);
  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    m_Answer = ANSWER_OFF;}, ID_CLIENT_ANSWER_OFF);

  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    long val;
    if ((val = wxGetNumberFromUser(
      _("Input") + ":",
      wxEmptyString,
      _("Buffer Size"),
      wxConfigBase::Get()->ReadLong(_("Buffer Size"), 4096),
      1,
      65536)) > 0)
      {
        wxConfigBase::Get()->Write(_("Buffer Size"), val);
      }
    }, ID_CLIENT_BUFFER_SIZE);

  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    wxConfigBase::Get()->Write(_("Log Data"), 
      !wxConfigBase::Get()->ReadBool(_("Log Data"), true));}, ID_CLIENT_LOG_DATA);

  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    wxConfigBase::Get()->Write(_("Count Only"), 
      !wxConfigBase::Get()->ReadBool(_("Count Only"), true));}, ID_CLIENT_LOG_DATA_COUNT_ONLY);

  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    Close(false);}, ID_HIDE);

  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    // Configuring only possible if no client is active,
    // otherwise just show settings readonly mode.
    wxExItemDialog({
        {_("Remote Hostname"), wxEmptyString, ITEM_TEXTCTRL, wxExControlData().Required(true)},
        // Well known ports are in the range from 0 to 1023.
        // Just allow here for most flexibility.
        {_("Remote Port"), 1, 65536}},
      wxExWindowData().
        Title(_("Remote Server Config").ToStdString()).
        Button(m_SocketRemoteServer == nullptr ? wxOK | wxCANCEL: wxCANCEL)).ShowModal();
    }, ID_REMOTE_SERVER_CONFIG);

  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    // Configuring only possible if server is stopped,
    // otherwise just show settings readonly mode.
    wxExItemDialog({
        {_("Hostname"), wxEmptyString, ITEM_TEXTCTRL, wxExControlData().Required(true)},
        // Well known ports are in the range from 0 to 1023.
        // Just allow here for most flexibility.
        {_("Port"), 1, 65536}},
      wxExWindowData().
        Title(_("Server Config").ToStdString()).
        Button(m_SocketServer == nullptr ? wxOK | wxCANCEL: wxCANCEL)).ShowModal();
    }, ID_SERVER_CONFIG);

  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    const wxString str = event.GetString() + "\n";
    const wxCharBuffer& buffer(str.c_str());
    for (auto& it : m_Clients)
    {
      WriteDataToSocket(buffer, it);
    }
    if (m_SocketRemoteServer != nullptr)
    {
      WriteDataToSocket(buffer, m_SocketRemoteServer);
    }

    m_Shell->Prompt();}, ID_SHELL_COMMAND);

  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    TimerDialog();}, ID_TIMER_START);

  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    m_Timer.Stop();
    AppendText(m_LogWindow, _("timer stopped"), DATA_MESSAGE);
#if wxUSE_STATUSBAR
    StatusText(std::string(), "PaneTimer");
#endif
   }, ID_TIMER_STOP);

  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    TogglePane("DATA");}, ID_VIEW_DATA);
  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    TogglePane("LOG");}, ID_VIEW_LOG);
  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    TogglePane("SHELL");}, ID_VIEW_SHELL);
  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    TogglePane("STATISTICS");}, ID_VIEW_STATISTICS);

  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    WriteDataWindowToConnections();}, ID_WRITE_DATA);

  Bind(wxEVT_SOCKET, [=](wxSocketEvent& event) {
    // Accept new connection if there is one in the pending
    // connections queue, else exit. We use Accept(false) for
    // non-blocking accept (although if we got here, there
    // should ALWAYS be a pending connection).
    wxSocketBase*sock = m_SocketServer->Accept(false);

    if (sock == nullptr)
    {
      AppendText(m_LogWindow,
        _("error: couldn't accept a new connection"),
        DATA_MESSAGE);
    }
    else
    {
      m_Statistics.Inc("Server Connections");
      sock->SetEventHandler(*this, ID_SERVER_CLIENT);
      sock->SetNotify(wxSOCKET_INPUT_FLAG | wxSOCKET_LOST_FLAG);
      sock->Notify(true);
      m_Clients.emplace_back(sock);
      UpdateConnectionsPane();
      LogConnection(sock, true);
      const wxCharBuffer& buffer = m_DataWindow->GetTextRaw();
      WriteDataToSocket(buffer, sock);
#if wxUSE_TASKBARICON
      UpdateTaskBar();
#endif
    };}, ID_SERVER);

  Bind(wxEVT_SOCKET, [=](wxSocketEvent& event) {
    wxSocketBase *sock = event.GetSocket();

    switch (event.GetSocketEvent())
    {
      case wxSOCKET_CONNECTION:
        {
        m_Statistics.Inc("Remote Connections");
        LogConnection(sock, true);
        sock->SetNotify(wxSOCKET_LOST_FLAG | wxSOCKET_INPUT_FLAG | wxSOCKET_OUTPUT_FLAG);
        const wxCharBuffer& buffer = m_DataWindow->GetTextRaw();
        WriteDataToSocket(buffer, sock);
        UpdateConnectionsPane();
        }
        break;

      case wxSOCKET_INPUT:
      case wxSOCKET_OUTPUT:
        {
        m_Statistics.Inc("Events");

        // We disable input events, so that the test doesn't trigger
        // wxSocketEvent again.
        sock->SetNotify(wxSOCKET_LOST_FLAG);

        const long size = wxConfigBase::Get()->ReadLong(_("Buffer Size"), 4096);

        if (size <= 0)
        {
          wxLogError("Illegal buffer size, skipping socket input data");
          return;
        }

        char* buffer = new char[size];
        sock->Read(buffer, size);

        m_Statistics.Inc("Bytes Received", sock->LastCount());

        if (sock->LastCount() > 0)
        {
          const wxString text(buffer, sock->LastCount());

          if (wxConfigBase::Get()->ReadBool(_("Log Data"), true))
          {
            if (wxConfigBase::Get()->ReadBool(_("Count Only"), true))
            {
              AppendText(m_LogWindow, 
                wxString::Format(_("read: %d bytes from: %s"), 
                  sock->LastCount(), SocketDetails(sock).c_str()),
                DATA_MESSAGE);
            }
            else
            {
              AppendText(m_LogWindow, text, DATA_READ);
            }
          }
          
          switch (m_Answer)
          {
            case ANSWER_COMMAND: 
              {
              const wxString command(m_Shell->GetCommand());
              if (command != "history")
              {
                WriteDataToSocket(command.ToAscii(), sock); 
              }
              }
              break;
              
            case ANSWER_ECHO: WriteDataToSocket(text.ToAscii(), sock); break;
            case ANSWER_FILE: WriteDataToSocket(m_DataWindow->GetTextRaw(), sock); break;
          }

          if (GetManager().GetPane("SHELL").IsShown())
          {
            AppendText(m_Shell, text, DATA_MESSAGE_RAW);
            m_Shell->Prompt(std::string(), false); // no eol
          }
        }

        delete [] buffer;

#if wxUSE_STATUSBAR
        StatusText(
          std::to_string(m_Statistics.Get("Bytes Received")) + "," +
          std::to_string(m_Statistics.Get("Bytes Sent")),
          "PaneBytes");
#endif

#if wxUSE_TASKBARICON
        UpdateTaskBar();
#endif
        // Enable input events again.
        sock->SetNotify(wxSOCKET_LOST_FLAG | wxSOCKET_INPUT_FLAG | wxSOCKET_OUTPUT_FLAG);
        }
        break;

      case wxSOCKET_LOST:
        m_Statistics.Inc("Connections Lost");
        switch (event.GetId())
        {
          case ID_CLIENT: 
            SocketLost(sock, false); 
            break;

          case ID_SERVER_CLIENT: 
            SocketLost(sock, true); 
            m_SocketRemoteServer = nullptr;
            break;
        }

        UpdateConnectionsPane();

#if wxUSE_TASKBARICON
        UpdateTaskBar();
#endif
        break;

      default:
        wxFAIL;
   };}, ID_SERVER_CLIENT, ID_CLIENT);

  Bind(wxEVT_CLOSE_WINDOW, [=](wxCloseEvent& event) {
#if wxUSE_TASKBARICON
    if (event.CanVeto())
    {
      Hide();
      return;
    }
#endif
    if (wxExFileDialog(
      &m_DataWindow->GetFile()).ShowModalIfChanged() == wxID_CANCEL)
    {
      return;
    }

    if (m_SocketRemoteServer != nullptr)
    {
      m_SocketRemoteServer->Destroy();
      m_SocketRemoteServer = nullptr;
    }

    for (auto c : m_Clients) c->Destroy();
    m_Clients.clear();

    wxConfigBase::Get()->Write("Perspective", GetManager().SavePerspective());
    event.Skip();});
    
  Bind(wxEVT_TIMER, [=](wxTimerEvent& event) {
    WriteDataWindowToConnections();});
    
  Bind(wxEVT_UPDATE_UI, [=](wxUpdateUIEvent& event) {
    event.Enable(m_SocketServer == nullptr);}, wxID_EXECUTE);
  Bind(wxEVT_UPDATE_UI, [=](wxUpdateUIEvent& event) {
    event.Enable(m_DataWindow->GetModify());}, wxID_SAVE);
  Bind(wxEVT_UPDATE_UI, [=](wxUpdateUIEvent& event) {
    event.Enable(m_SocketServer != nullptr);}, wxID_STOP);

  Bind(wxEVT_UPDATE_UI, [=](wxUpdateUIEvent& event) {
    event.Enable(m_SocketRemoteServer == nullptr);}, ID_SETUP_REMOTE_SERVER);
  Bind(wxEVT_UPDATE_UI, [=](wxUpdateUIEvent& event) {
   event.Enable(m_SocketRemoteServer != nullptr);}, ID_REMOTE_SERVER_DISCONNECT);

  Bind(wxEVT_UPDATE_UI, [=](wxUpdateUIEvent& event) {
    event.Enable(!m_Statistics.GetItems().empty());}, ID_CLEAR_STATISTICS);
  Bind(wxEVT_UPDATE_UI, [=](wxUpdateUIEvent& event) {
    event.Check(wxConfigBase::Get()->ReadBool(_("Log Data"), true));}, ID_CLIENT_LOG_DATA);
  Bind(wxEVT_UPDATE_UI, [=](wxUpdateUIEvent& event) {
    event.Enable(wxConfigBase::Get()->ReadBool(_("Log Data"), true));
    event.Check(wxConfigBase::Get()->ReadBool(_("Count Only"), true));}, ID_CLIENT_LOG_DATA_COUNT_ONLY);
  Bind(wxEVT_UPDATE_UI, [=](wxUpdateUIEvent& event) {
    event.Enable(!GetFileHistory().GetHistoryFile().Path().empty());}, ID_RECENT_FILE_MENU);
  Bind(wxEVT_UPDATE_UI, [=](wxUpdateUIEvent& event) {
    event.Enable(m_Timer.IsRunning());}, ID_TIMER_STOP);
  Bind(wxEVT_UPDATE_UI, [=](wxUpdateUIEvent& event) {
    event.Check(GetManager().GetPane("DATA").IsShown());}, ID_VIEW_DATA);
  Bind(wxEVT_UPDATE_UI, [=](wxUpdateUIEvent& event) {
    event.Check(GetManager().GetPane("LOG").IsShown());}, ID_VIEW_LOG);
  Bind(wxEVT_UPDATE_UI, [=](wxUpdateUIEvent& event) {
    event.Check(GetManager().GetPane("SHELL").IsShown());}, ID_VIEW_SHELL);
  Bind(wxEVT_UPDATE_UI, [=](wxUpdateUIEvent& event) {
    event.Check(GetManager().GetPane("STATISTICS").IsShown());}, ID_VIEW_STATISTICS);
  Bind(wxEVT_UPDATE_UI, [=](wxUpdateUIEvent& event) {
    event.Enable(
      (!m_Clients.empty() || (m_SocketRemoteServer != nullptr && m_SocketRemoteServer->IsConnected())) && 
      m_DataWindow->GetLength() > 0);}, ID_WRITE_DATA);

  StatusText(wxExLexers::Get()->GetTheme(), "PaneTheme");
}

Frame::~Frame()
{
#if wxUSE_TASKBARICON
  delete m_TaskBarIcon;
#endif
  delete m_SocketRemoteServer;
  delete m_SocketServer;
}

void Frame::AppendText(wxExSTC* stc, const wxString& text, int mode)
{
  const bool pos_at_end = (stc->GetCurrentPos() == stc->GetTextLength());
  
  wxString prefix;
  
  switch (mode)
  {
    case DATA_MESSAGE: break;
    case DATA_MESSAGE_RAW: break;
    case DATA_READ: prefix = "r: "; break;
    case DATA_WRITE: prefix = "w: "; break;
  }

  if (mode != DATA_MESSAGE_RAW)
  {
    stc->AppendText(wxDateTime::Now().Format() + " " + prefix);
  }
  
  if (!stc->HexMode() || mode == DATA_MESSAGE || mode == DATA_MESSAGE_RAW)
  {
    stc->AppendText(text);
  }
  else
  {
    stc->GetHexMode().AppendText(text.ToStdString());
  }

  if (!text.EndsWith("\n"))
  {
    stc->AppendText(stc->GetEOL());
  }

  stc->EmptyUndoBuffer();
  stc->SetSavePoint();

  if (pos_at_end)
  {
    stc->DocumentEnd();
  }
}

void Frame::LogConnection(
  wxSocketBase* sock,
  bool accepted,
  bool show_connections)
{
  wxString text;

  if (sock != nullptr)
  {
    text << (accepted ? _("accepted"): _("lost")) << " " << SocketDetails(sock);
  }
  else
  {
    text << (accepted ? _("accepted"): _("lost")) << " " << _("connection");
  }

  if (show_connections)
  {
    text << " " << _("connections: ") << m_Clients.size();
  }

  AppendText(m_LogWindow, text, DATA_MESSAGE);
}

void Frame::OnCommandItemDialog(
  wxWindowID dialogid,
  const wxCommandEvent& event)
{
  if (dialogid == wxID_PREFERENCES)
  {
    if (event.GetId() != wxID_CANCEL)
    {
      m_DataWindow->ConfigGet();
      m_LogWindow->ConfigGet();
      m_Shell->ConfigGet();
    }
  }
  else
  {
    wxExFrameWithHistory::OnCommandItemDialog(dialogid, event);
  }
}

wxExSTC* Frame::OpenFile(const wxExPath& filename, const wxExSTCData& data)
{
  if (m_DataWindow->Open(filename, data))
  {
    ShowPane("DATA");
  }
  
  return m_DataWindow;
}

bool Frame::SetupSocketServer()
{
  if (m_SocketServer != nullptr)
  {
    // In fact, this should not happen, 
    // but using Ubuntu the OnUpdateUI does not prevent this...
    wxLogStatus(_("First stop server"));
    return false;
  }
  
  // Create the address - defaults to localhost and port as specified
  if (!wxConfigBase::Get()->Exists(_("Hostname")))
  {
    wxConfigBase::Get()->SetRecordDefaults(true);
  }

  wxIPV4address addr;
  addr.Hostname(wxConfigBase::Get()->Read(_("Hostname"), "localhost"));
  addr.Service(wxConfigBase::Get()->ReadLong(_("Port"), 3000));

  if (wxConfigBase::Get()->IsRecordingDefaults())
  {
    wxConfigBase::Get()->SetRecordDefaults(false);
  }

  m_SocketServer = new wxSocketServer(addr);

  wxString text;

  // We use Ok() here to see if the server is really listening
  if (!m_SocketServer->Ok())
  {
    text = wxString::Format(_("could not listen at %ld"), 
      wxConfigBase::Get()->ReadLong(_("Port"), 3000));
      
#if wxUSE_TASKBARICON
    m_TaskBarIcon->SetIcon(wxICON(notready), text);
#endif

    m_SocketServer->Destroy();
    m_SocketServer = nullptr;
    
    wxLogStatus(text);
    AppendText(m_LogWindow, text, DATA_MESSAGE);
    
    return false;
  }
  else
  {
    text = wxString::Format(_("server listening at %ld"), 
        wxConfigBase::Get()->ReadLong(_("Port"), 3000));

#if wxUSE_TASKBARICON
    m_TaskBarIcon->SetIcon(wxICON(ready), text);
#endif
  }

  wxLogStatus(text);
  AppendText(m_LogWindow, text, DATA_MESSAGE);

  // Setup the event handler and subscribe to connection events
  m_SocketServer->SetEventHandler(*this, ID_SERVER);
  m_SocketServer->SetNotify(wxSOCKET_CONNECTION_FLAG);
  m_SocketServer->Notify(true);

  return true;
}

bool Frame::SocketCheckError(const wxSocketBase* sock)
{
  if (sock->Error())
  {
    const std::string error = 
      "Socket Error: " + std::to_string(sock->LastError());
      
    wxLogStatus(error.c_str());

    m_Statistics.Inc(error);
    
    return true;
  }

  return false;
}

const wxString Frame::SocketDetails(const wxSocketBase* sock) const
{
  // See invocation at SocketLost.
  // If that is solved we can wxASSERT(sock != nullptr) again.
  if (sock == nullptr)
  {
    return wxEmptyString;
  }

  wxIPV4address peer_addr;

  if (!sock->GetPeer(peer_addr))
  {
    wxLogError("Could not get peer address");
    return wxEmptyString;
  }

  wxIPV4address local_addr;

  if (!sock->GetLocal(local_addr))
  {
    wxLogError("Could not get local address");
    return wxEmptyString;
  }

  wxString value;

  value <<
    _("socket: ") <<
    local_addr.IPAddress() << "." << (int)local_addr.Service() << ", " <<
    peer_addr.IPAddress() << "." << (int)peer_addr.Service();

  return value;
}

void Frame::SocketLost(wxSocketBase* sock, bool remove_from_connections)
{
  wxASSERT(sock != nullptr);

  if (remove_from_connections)
  {
    m_Clients.remove(sock);
  }

#ifdef __WXMSW__
  LogConnection(sock, false, remove_from_connections);
#else
  LogConnection(nullptr, false, remove_from_connections); // otherwise a crash!
#endif

  sock->Destroy();
}

void Frame::StatusBarClicked(const std::string& pane)
{
  if (pane == "PaneTimer")
  {
    TimerDialog();
  }
  else if (pane == "PaneTheme")
  {
    if (wxExLexers::Get()->ShowThemeDialog(this))
    {
      m_DataWindow->GetLexer().Set(m_DataWindow->GetLexer().GetDisplayLexer());
      m_LogWindow->GetLexer().Set(m_LogWindow->GetLexer().GetDisplayLexer());
      m_Shell->GetLexer().Set(m_Shell->GetLexer().GetDisplayLexer());

      m_StatusBar->ShowField(
        "PaneLexer", 
        wxExLexers::Get()->GetThemeOk());
        
      StatusText(wxExLexers::Get()->GetTheme(), "PaneTheme");
    }
  }
  else
  {
    wxExFrameWithHistory::StatusBarClicked(pane);
  }
}

void Frame::TimerDialog()
{
  const long val = wxGetNumberFromUser(
    _("Input (seconds):"),
    wxEmptyString,
    _("Repeat Timer"),
    wxConfigBase::Get()->ReadLong(_("Timer"), 60),
    1,
    3600 * 24);

  // If cancelled, -1 is returned.
  if (val == -1) return;

  wxConfigBase::Get()->Write(_("Timer"), val);

  if (val > 0)
  {
    m_Timer.Start(1000 * val);
    
    AppendText(m_LogWindow,
      wxString::Format(_("timer set to: %ld seconds (%s)"),
      val,
      wxTimeSpan(0, 0, val, 0).Format().c_str()),
      DATA_MESSAGE);
      
#if wxUSE_STATUSBAR
    StatusText(std::to_string(val), "PaneTimer");
#endif
  }
  else if (val == 0)
  {
    m_Timer.Stop();
    
    AppendText(m_LogWindow, _("timer stopped"), DATA_MESSAGE);
    
#if wxUSE_STATUSBAR
    StatusText(std::string(), "PaneTimer");
#endif
  }
}

void Frame::UpdateConnectionsPane() const
{
#if wxUSE_STATUSBAR
    const int connections = m_Clients.size() + 
      (m_SocketRemoteServer != nullptr && m_SocketRemoteServer->IsConnected() ? 1: 0);
    StatusText(
      std::to_string(connections), "PaneConnections");
#endif
}

#if wxUSE_TASKBARICON
void Frame::UpdateTaskBar()
{
  if (m_Clients.empty())
  {
    m_TaskBarIcon->SetIcon(
      wxICON(ready), 
      wxString::Format(_("server listening at %ld"), 
        wxConfigBase::Get()->ReadLong(_("Port"), 3000)));
  }
  else
  {
    const wxString text =
      wxString::Format(
        _("%s %ld connections connected at %ld\nreceived: %d bytes sent: %d bytes"),
        wxTheApp->GetAppName().c_str(),
        m_Clients.size() + 
          (m_SocketRemoteServer != nullptr && m_SocketRemoteServer->IsConnected() ? 1: 0),
        wxConfigBase::Get()->ReadLong(_("Port"), 3000),
        m_Statistics.Get("Bytes Received"),
        m_Statistics.Get("Bytes Sent"));

    m_TaskBarIcon->SetIcon(wxICON(connect), text);
  }
}
#endif

void Frame::WriteDataToSocket(const wxCharBuffer& buffer, wxSocketBase* sock)
{
  if (buffer.length() == 0) return;

  sock->Write(buffer, buffer.length());

  if (SocketCheckError(sock))
  {
    return;
  }

  if (sock->LastCount() != buffer.length())
  {
    AppendText(m_LogWindow, _("not all bytes sent to socket"), DATA_MESSAGE);
  }

  m_Statistics.Inc("Bytes Sent", sock->LastCount());
  m_Statistics.Inc("Messages Sent");

#if wxUSE_STATUSBAR
  StatusText(
    std::to_string(m_Statistics.Get("Bytes Received")) + "," + 
    std::to_string(m_Statistics.Get("Bytes Sent")),
    "PaneBytes");
#endif

#if wxUSE_TASKBARICON
  UpdateTaskBar();
#endif

  if (wxConfigBase::Get()->ReadBool(_("Log Data"), true))
  {
    if (wxConfigBase::Get()->ReadBool(_("Count Only"), true))
    {
      AppendText(m_LogWindow,
        wxString::Format(_("write: %d bytes to: %s"),
          sock->LastCount(), SocketDetails(sock).c_str()),
        DATA_MESSAGE);
    }
    else
    {
      AppendText(m_LogWindow, buffer, DATA_WRITE);
    }
  }
}

void Frame::WriteDataWindowToConnections()
{
  const wxCharBuffer& buffer = m_DataWindow->GetTextRaw();

  for (auto& it : m_Clients)
  {
    WriteDataToSocket(buffer, it);
  }

  if (m_SocketRemoteServer != nullptr && m_SocketRemoteServer->IsConnected())
  {
    WriteDataToSocket(buffer, m_SocketRemoteServer);
  }
}

enum
{
  ID_OPEN = ID_CLIENT + 1
};

#if wxUSE_TASKBARICON
TaskBarIcon::TaskBarIcon(Frame* frame)
  : m_Frame(frame) 
{
  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    m_Frame->Close(true);}, wxID_EXIT);

  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    m_Frame->Show();}, ID_OPEN);

  Bind(wxEVT_TASKBAR_LEFT_DCLICK, [=](wxTaskBarIconEvent&) {
    m_Frame->Show();});
    
  Bind(wxEVT_UPDATE_UI, [=](wxUpdateUIEvent& event) {
    event.Enable(m_Frame->ServerNotListening());}, wxID_EXIT);
}

wxMenu *TaskBarIcon::CreatePopupMenu()
{
  wxExMenu* menu = new wxExMenu;
  menu->Append(ID_OPEN, _("Open"));
  menu->AppendSeparator();
  menu->Append(wxID_EXIT);
  return menu;
}
#endif // wxUSE_TASKBARICON
#endif // wxUSE_SOCKETS
