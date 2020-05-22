////////////////////////////////////////////////////////////////////////////////
// Name:      test-cmdlineparser.cpp
// Purpose:   Implementation for wex unit testing
// Author:    Anton van Wezenbeek
// Copyright: (c) 2019 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <wex/cmdline.h>
#include "../test.h"

TEST_CASE("wex::cmdline")
{
  SUBCASE("static")
  {
    REQUIRE( wex::cmdline::toggle());
    wex::cmdline().toggle(false);
    REQUIRE(!wex::cmdline::toggle());
    wex::cmdline().toggle(true);
    REQUIRE( wex::cmdline::toggle());
  }
  
  SUBCASE("default constructor")
  {
    std::string help;
    wex::cmdline cmdl;
    
    REQUIRE( cmdl.parse(std::string(), help));
    REQUIRE(!cmdl.parse("xxx", help));
    REQUIRE(!cmdl.parse("-h", help));
  }
  
  SUBCASE("constructor")
  {
    int a {0};
    float b {0};
    bool s {false}, t {false}, u {false}, w {false}, x {false};
    std::string c,p,q,r,help;
    
    wex::cmdline cmdl(
       {{{"s1,s", "bool"}, [&](bool on){s = on;}},
        {{"s2,t", "bool"}, [&](bool on){t = on;}},
        {{"s3,u", "bool"}, [&](bool on){u = true;}},
        {{"s4,w", "bool"}, [&](bool on){w = on;}},
        {{"xx", "bool"}, [&](bool on){x = on;}}},
       {{{"o1,a", "int"}, {wex::cmdline::INT, 
          [&](const std::any& i) {a = std::any_cast<int>(i);}}},
        {{"o2,b", "float"}, {wex::cmdline::FLOAT, 
          [&](const std::any& f) {b = std::any_cast<float>(f);}}},
        {{"o3,c", "string"}, {wex::cmdline::STRING, 
          [&](const std::any& s) {c = std::any_cast<std::string>(s);}}}},
       {{"rest", "rest"}, 
          [&](const std::vector<std::string> & v) {
          p = v[0];
          q = v[1];
          r = v[2];}});

    const bool res(cmdl.parse(
      "-a 10 -b 5.1 -c test -s -t -u -w --xx one two three", help));
    
    CAPTURE( help );

    REQUIRE( res );

    REQUIRE( a == 10 );
    REQUIRE( b == 5.1f );
    REQUIRE( c == "test" );
    REQUIRE( s );
    REQUIRE( t );
    REQUIRE( u );
    REQUIRE( w );
    REQUIRE( x );
    REQUIRE( p == "one" );
    REQUIRE( q == "two" );
    REQUIRE( r == "three" );
  }

  SUBCASE("constructor prefix")
  {
    bool s {false}, t {false}, u {false}, w {false};
    int a {0};
    float b {0};
    std::string c,help;

    wex::cmdline cmdl(
       {{{"s1,s", "bool"}, [&](bool on){s = on;}},
        {{"s2,t", "bool"}, [&](bool on){t = on;}},
        {{"s3,u", "bool"}, [&](bool on){u = true;}},
        {{"s4,w", "bool"}, [&](bool on){w = on;}}},
       {{{"o1,a", "int"}, {wex::cmdline::INT, 
          [&](const std::any& i) {a = std::any_cast<int>(i);}}},
        {{"o2,b", "float"}, {wex::cmdline::FLOAT, 
          [&](const std::any& f) {b = std::any_cast<float>(f);}}},
        {{"o3,c", "string"}, {wex::cmdline::STRING, 
          [&](const std::any& s) {c = std::any_cast<std::string>(s);}}}},
       {},
      true,
      "prefix");

    const bool res(cmdl.parse(
      "-a 10 -b 5.1 -c test -s -t -u -w", help));

    CAPTURE( help );

    REQUIRE( res );

    REQUIRE( a == 10 );
    REQUIRE( b == 5.1f );
    REQUIRE( s );
    REQUIRE( t );
    REQUIRE( u );
    REQUIRE( w );
  }
}