////////////////////////////////////////////////////////////////////////////////
// Name:      test-vcsentry.cpp
// Purpose:   Implementation for wex unit testing
// Author:    Anton van Wezenbeek
// Copyright: (c) 2019 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <wex/vcsentry.h>
#include <wex/defs.h>
#include "test.h"

TEST_SUITE_BEGIN("wex::vcs");

TEST_CASE("wex::vcs_entry")
{
  SUBCASE("default constructor")
  {
    REQUIRE( wex::vcs_entry().get_commands().empty());
    REQUIRE( wex::vcs_entry().flags_location() == wex::vcs_entry::FLAGS_LOCATION_POSTFIX);
  }
  
  SUBCASE("constructor using xml")
  {
    pugi::xml_document doc;

    REQUIRE( doc.load_string(
      "<vcs name=\"git\" admin-dir=\"./\" log-flags=\"-n 1\" blame-format=\" yyyy\">"
      "  <commands>"
      "     <command> help </command>"
      "     <command> blame </command>"
      "  </commands>"
      "</vcs>"));

    wex::vcs_entry entry(doc.document_element());
    REQUIRE( entry.name() == "git");
    REQUIRE(!entry.log( wex::test::get_path("test.h"), "x"));
    REQUIRE( entry.log( wex::test::get_path("test.h"), "-1"));
    REQUIRE( entry.get_blame().use());

    REQUIRE( entry.get_commands().size() == 2);
    REQUIRE(!entry.get_command().get_command().empty());
    REQUIRE(!entry.admin_dir_is_toplevel());
    REQUIRE( entry.admin_dir() == "./");
    REQUIRE(!entry.get_branch().empty());
    REQUIRE( entry.get_flags().empty());
    REQUIRE(!entry.get_stdout().empty());
    entry.show_output();
    
    wex::menu menu;
    REQUIRE( entry.build_menu(0, &menu) == 2);

#ifndef __WXMSW__
    // This should have no effect.  
    REQUIRE(!entry.set_command(5));
    REQUIRE(!entry.set_command(wex::ID_EDIT_VCS_LOWEST));
    REQUIRE(!entry.set_command(wex::ID_VCS_LOWEST));
    
    REQUIRE( entry.get_commands().size() == 2);
    REQUIRE( entry.get_flags().empty());
    REQUIRE(!entry.get_stdout().empty());
    
    REQUIRE( entry.execute()); // executes just git, shows help
    REQUIRE( entry.get_stdout().find("usage: ") != std::string::npos);
    entry.show_output();
    
    auto* other = new wex::vcs_entry(doc.document_element());
    REQUIRE( other->execute(
      std::string(), 
      wex::lexer(), wex::process::EXEC_WAIT));
    other->show_output();
#endif
  }
}

TEST_SUITE_END();
