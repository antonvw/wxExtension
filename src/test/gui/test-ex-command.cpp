////////////////////////////////////////////////////////////////////////////////
// Name:      test-ex-command.cpp
// Purpose:   Implementation for wex unit testing
// Author:    Anton van Wezenbeek
// Copyright: (c) 2019 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <wex/ex-command.h>
#include <wex/stc.h>
#include "test.h"

TEST_SUITE_BEGIN("wex::ex");

TEST_CASE("wex::ex_command")
{
  auto* stc = get_stc();
  stc->set_text("more text\notherline\nother line");

  SUBCASE("constructor stc")
  {
    wex::ex_command command(stc);

    REQUIRE( command.empty());
    REQUIRE( command.get_stc() == stc);

    SUBCASE("clear")
    {
      command.set("12345");
      REQUIRE(!command.empty());
      command.clear();
      REQUIRE( command.empty());
    }
    
    SUBCASE("type")
    {
      REQUIRE( command.type() == wex::ex_command::type_t::NONE);
      
      command.set("G");
      REQUIRE( command.type() == wex::ex_command::type_t::VI);

      command.set(":");
      REQUIRE( command.type() == wex::ex_command::type_t::COMMAND);
      
      command.set("=");
      REQUIRE( command.type() == wex::ex_command::type_t::CALC);

      command.set("!");
      REQUIRE( command.type() == wex::ex_command::type_t::EXEC);
      
      command.set("/");
      REQUIRE( command.type() == wex::ex_command::type_t::FIND);
      
      command.set("?");
      REQUIRE( command.type() == wex::ex_command::type_t::FIND);
      
      command.set("w");
      REQUIRE( command.type() == wex::ex_command::type_t::VI);
    }

    SUBCASE("exec")
    {
      command.set("G");
      REQUIRE( command.command() == "G");
      REQUIRE( command.exec() );
      REQUIRE( stc->GetCurrentLine() == 2);
    }

    SUBCASE("change")
    {
      command.append('g');
      REQUIRE( command.command() == "g");
      command.append('g');
      REQUIRE( command.command() == "gg");
      REQUIRE( command.front() == 'g');
      REQUIRE( command.back() == 'g');
      REQUIRE( command.size() == 2);
      command.pop_back();
      REQUIRE( command.size() == 1);
      REQUIRE( command.append_exec('g'));
      REQUIRE( stc->GetCurrentLine() == 0);

      command.set(wex::ex_command("dd"));
      REQUIRE( command.command() == "dd");
      REQUIRE( command.get_stc() == stc);
      command.restore(wex::ex_command("ww"));
      REQUIRE( command.command() == "ww");
      command.append("ww");
      REQUIRE( command.command() == "wwww");
      REQUIRE( command.get_stc() == stc);
    }
  }
  
  SUBCASE("constructor command")
  {
    wex::ex_command command("G");

    REQUIRE( command.command() == "G");
    REQUIRE( command.get_stc() == nullptr);
    REQUIRE( command.type() == wex::ex_command::type_t::VI);

    REQUIRE(!command.exec() );
    REQUIRE( stc->GetCurrentLine() == 0);
  }
}

TEST_SUITE_END();
