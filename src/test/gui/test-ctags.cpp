////////////////////////////////////////////////////////////////////////////////
// Name:      test-ctags.cpp
// Purpose:   Implementation for wex unit testing
// Author:    Anton van Wezenbeek
// Copyright: (c) 2019 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <wex/ctags.h>
#include <wex/managedframe.h>
#include <wex/stc.h>
#include "test.h"

TEST_CASE("wex::ctags")
{
  wex::stc_data data;

  SUBCASE("tags default")
  {
    wex::ex* ex = &get_stc()->get_vi();

    // default find
    REQUIRE( wex::ctags::find("test_app") );
    REQUIRE(!wex::ctags::find("xest_app") );

    // default auto_complete
    REQUIRE( wex::ctags(ex).autocomplete("test_").find("test_app") == 0);

    // setup a filter using find
    wex::ctags_entry current;
    wex::ctags_entry filter;
    REQUIRE( wex::ctags::find("test_app", current, filter));
    REQUIRE( filter.class_name() == "test_app" );
    REQUIRE( filter.kind() == "f" );
    REQUIRE( current.kind() == "c" );
    REQUIRE( current.class_name() == "test_app" );
    
    // autocomplete using filter should now return member functions
    REQUIRE( wex::ctags(ex).autocomplete("me", filter).find("method") == 0);
    REQUIRE( wex::ctags(ex).autocomplete("he", filter).empty());
  }

  SUBCASE("tags non-existing file")
  {
    auto* stc = new wex::stc(std::string("test"), data);
    wex::ctags::close();
    wex::ctags::open("xxx");
    wex::test::add_pane(frame(), stc);
    wex::ex* ex = &stc->get_vi();

    REQUIRE(!wex::ctags::find("test_app") );
  }
  
  SUBCASE("tags own file")
  {
    auto* stc = new wex::stc(std::string("test"), data);
    wex::ctags::close();
    wex::ctags::open("test-ctags");
    wex::test::add_pane(frame(), stc);
    wex::ex* ex = &stc->get_vi();

    REQUIRE(!wex::ctags::find("") );
    REQUIRE(!wex::ctags::next() );
    REQUIRE(!wex::ctags::previous() );
    REQUIRE(!wex::ctags::find("xxxx") );
    REQUIRE( wex::ctags::find("test_app") );
    REQUIRE(!wex::ctags::next() );
    REQUIRE( wex::ctags(ex).separator() != ' ');
  }
}
