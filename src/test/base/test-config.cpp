////////////////////////////////////////////////////////////////////////////////
// Name:      test-util.cpp
// Purpose:   Implementation for wex unit testing
// Author:    Anton van Wezenbeek
// Copyright: (c) 2019 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <wex/config.h>
#include "../test.h"

TEST_CASE("wex::config")
{
  SUBCASE("default constructor")
  {
    REQUIRE( wex::config().get("x") == "x");
    REQUIRE(!wex::config().is_child());
    REQUIRE(!wex::config().child_start());
    REQUIRE(!wex::config().child_end());
  }
  
  SUBCASE("dir")
  {
#ifdef __WXMSW__
    REQUIRE(!wex::config::dir().empty());
#else
    REQUIRE( wex::config::dir().find(".config") != std::string::npos);
#endif
  }
  
  const wex::config::statusbar_t sb {{"one", 1 , 2}, {"two", 3, 4}};

  SUBCASE("getters")
  {
    REQUIRE( wex::config("x").get(4) == 4);
    REQUIRE( wex::config("xcvb").get(4) == 4);
    REQUIRE(!wex::config("xcvb").exists());
    REQUIRE( wex::config("xcvb").empty());
    REQUIRE( wex::config("x").item() == "x");
    REQUIRE( wex::config("x").get("space") == "space");
    REQUIRE( wex::config("l").get(std::list<std::string>{}).empty());
    REQUIRE( wex::config("l").get_firstof().empty());
    REQUIRE( wex::config("l").empty());
    REQUIRE(!wex::config("m").get(std::list<std::string>{"one"}).empty());
    
    REQUIRE( std::get<0>(wex::config("sb").get(sb)[0]) == "one");
    REQUIRE( std::get<1>(wex::config("sb").get(sb)[0]) == 1);
    REQUIRE( std::get<2>(wex::config("sb").get(sb)[0]) == 2);
    REQUIRE( std::get<0>(wex::config("sb").get(sb)[1]) == "two");
    REQUIRE( std::get<1>(wex::config("sb").get(sb)[1]) == 3);
    REQUIRE( std::get<2>(wex::config("sb").get(sb)[1]) == 4);
    REQUIRE( wex::config("sbg").get(wex::config::statusbar_t{}).empty());
  }
  
  SUBCASE("setters")
  {
    REQUIRE( wex::config("m").set_firstof("one") == "one");
    REQUIRE( wex::config("m").set_firstof("two") == "two");
    REQUIRE( wex::config("m").get(std::list<std::string>{}).size() == 2);
    REQUIRE( wex::config("m").get_firstof() == "two");

    wex::config("y").set(4);
    REQUIRE( wex::config("y").exists());
    REQUIRE( wex::config("y").get(0) == 4);

    wex::config("y").set("WW");
    REQUIRE( wex::config("y").exists());
    REQUIRE(!wex::config("y").empty());
    REQUIRE( wex::config("y").get("zzzz") == "WW");
    REQUIRE( wex::config("y").item("z").item() == "z");

    wex::config("list_items").set({"1","2","3"});
    REQUIRE( wex::config("list_items").get(std::list<std::string>{}).front() == "1");
    
    wex::config("y").erase();
    REQUIRE(!wex::config("y").exists());
    
    wex::config::statusbar_t sbs {{"three", 1 , 2}, {"four", 3, 4}};
    std::get<0>(wex::config("sbs").get(sb)[0]);
    wex::config("sbs").set(sbs);
    REQUIRE( std::get<0>(wex::config("sbs").get(sb)[0]) == "three");
  }
  
  SUBCASE("constructor child")
  {
    wex::config c("parent", "child-x");
    REQUIRE( c.is_child());

    c.item("child-x").set("x");
    REQUIRE( c.item("child-x").get("y") == "x");

    wex::config("parent", "child-y").set("y");
    REQUIRE( wex::config("parent.child-y").get("z") == "y");
  }

  SUBCASE("child")
  {
    wex::config c("child");

    REQUIRE(!c.child_end());
    REQUIRE( c.child_start());
    REQUIRE(!c.child_start());

    c.item("child-x").set(1);
    c.item("child-y").set(2);
    c.item("child-z").set(3);

    REQUIRE(!c.item("child").exists());
    REQUIRE( c.item("child-x").get(99) == 1);
    REQUIRE( c.item("child-y").get(99) == 2);
    REQUIRE( c.item("child-z").get(99) == 3);

    REQUIRE( c.child_end());
    REQUIRE(!c.child_end());

    REQUIRE( c.item("child").exists());
    REQUIRE( c.item("child.child-x").get(99) == 1);
    REQUIRE( c.item("child.child-y").get(99) == 2);
    REQUIRE( c.item("child.child-z").get(99) == 3);
  }
  
  SUBCASE("toggle")
  {
    wex::config c("toggle");
    c.set(true);
    REQUIRE( c.get(true));
    
    c.toggle();
    REQUIRE(!c.get(true));
    
    c.toggle();
    REQUIRE( c.get(true));

    REQUIRE(!c.toggle());
    REQUIRE( c.toggle());
  }
}
