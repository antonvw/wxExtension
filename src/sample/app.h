////////////////////////////////////////////////////////////////////////////////
// Name:      app.h
// Purpose:   Declaration of wex sample classes
// Author:    Anton van Wezenbeek
// Copyright: (c) 2018
////////////////////////////////////////////////////////////////////////////////

#include <wex/app.h>
#include <wex/dir.h>
#include <wex/grid.h>
#include <wex/listview.h>
#include <wex/managedframe.h>
#include <wex/notebook.h>
#include <wex/process.h>
#include <wex/shell.h>
#include <wex/statistics.h>
#include <wex/stc.h>

/// Derive your application from wex::app.
class sample_app: public wex::app
{
public:
  /// Constructor.
  sample_app() {}
private:
  bool OnInit() override;
};

/// Use wex::dir.
class sample_dir: public wex::dir
{
public:
  /// Constructor.
  sample_dir(
    const std::string& fullpath, 
    const std::string& findfiles, 
    wex::grid* grid);
private:
  bool on_file(const wex::path& file) override;
  wex::grid* m_Grid;
};

/// Use wex::managedframe.
class sample_frame: public wex::managed_frame
{
public:
  /// Constructor.
  sample_frame();
  wex::listview* get_listview() override {return m_ListView;};
  virtual void on_command_item_dialog(
    wxWindowID id, 
    const wxCommandEvent& event) override;
protected:
  void OnCommand(wxCommandEvent& event);
private:
  wex::grid* m_Grid;
  wex::listview* m_ListView;
  wex::notebook* m_Notebook;
  wex::process* m_Process;
  wex::stc* m_stc;
  wex::stc* m_stcLexers;
  wex::shell* m_Shell;

  long m_FlagsSTC = 0;
  wex::statistics <int> m_Statistics;
};
