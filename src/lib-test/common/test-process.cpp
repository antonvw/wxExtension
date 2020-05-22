////////////////////////////////////////////////////////////////////////////////
// Name:      test-process.cpp
// Purpose:   Implementation for wex unit testing
// Author:    Anton van Wezenbeek
// Copyright: (c) 2019 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <boost/version.hpp>
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include "../test.h"
#include <wex/managedframe.h>
#include <wex/process.h>
#include <wex/shell.h>

TEST_SUITE_BEGIN("wex::process");

TEST_CASE("wex::process")
{
  wex::path cwd;
  auto*     process = new wex::process;
  process->get_shell()->SetText(std::string());

  SUBCASE("constructor")
  {
    REQUIRE(process->get_frame() != nullptr);
    REQUIRE(process->get_stdout().empty());
    REQUIRE(process->get_stderr().empty());
    REQUIRE(!process->is_debug());
    REQUIRE(!process->is_running());
  }

  SUBCASE("dialog")
  {
    process->config_dialog(wex::window_data().button(wxAPPLY | wxCANCEL));
  }

#ifdef __UNIX__
  SUBCASE("wait")
  {
    REQUIRE(process->execute("ls -l", wex::process::EXEC_WAIT));
    REQUIRE(!process->write("hello world"));
    REQUIRE(!process->get_stdout().empty());
    REQUIRE(!process->is_running());
    REQUIRE(!process->get_exec().empty());
    process->show_output();
  }

  SUBCASE("repeat")
  {
    REQUIRE(process->execute("ls -l", wex::process::EXEC_WAIT));
    REQUIRE(!process->is_running());
    REQUIRE(!process->get_stdout().empty());
  }

  SUBCASE("working directory")
  {
    REQUIRE(process->execute("ls -l", wex::process::EXEC_WAIT, "/"));
    REQUIRE(!process->get_stdout().empty());
    REQUIRE(wxGetCwd().Contains("data"));
  }

#ifndef __WXGTK__
  SUBCASE("invalid")
  {
    REQUIRE(!process->execute("xxxx", wex::process::EXEC_WAIT));
    wxMilliSleep(10);
    REQUIRE(!process->is_running());
    REQUIRE(!process->get_stderr().empty());
    REQUIRE(process->get_stdout().empty());
  }
#endif

  SUBCASE("invalid no wait")
  {
    REQUIRE(process->execute("xxxx"));
    wxMilliSleep(10);
    REQUIRE(!process->is_running());
  }

  SUBCASE("no wait")
  {
    REQUIRE(process->execute("bash"));
    REQUIRE(process->is_running());
    wex::shell* shell = process->get_shell();
    REQUIRE(shell != nullptr);
    REQUIRE(process->stop());
    REQUIRE(!process->is_running());
  }

  SUBCASE("working directory")
  {
    REQUIRE(process->execute("ls -l", wex::process::EXEC_WAIT, "/"));
    wex::path::current(cwd.original());
  }
#endif

  wex::process::prepare_output(frame()); // in fact already done
}

TEST_SUITE_END();