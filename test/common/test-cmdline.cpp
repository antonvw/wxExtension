////////////////////////////////////////////////////////////////////////////////
// Name:      test-cmdline.cpp
// Purpose:   Implementation for wex unit testing
// Author:    Anton van Wezenbeek
// Copyright: (c) 2021 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <wex/cmdline.h>

#include "../test.h"

TEST_CASE("wex::cmdline")
{
  SUBCASE("default constructor")
  {
    wex::cmdline cmdl;

    SUBCASE("1")
    {
      wex::data::cmdline data("");
      REQUIRE(cmdl.parse(data));
    }

    SUBCASE("2")
    {
      wex::data::cmdline data("xxx");
      REQUIRE(!cmdl.parse(data));
    }

    SUBCASE("3")
    {
      wex::data::cmdline data("-h");
      REQUIRE(!cmdl.parse(data));
    }
  }

  SUBCASE("constructor")
  {
    int         a{0};
    float       b{0};
    bool        s{false}, t{false}, u{false}, w{false}, x{false};
    std::string c, p, q, r;

    wex::cmdline cmdl(
      {{{"s1,s", "bool"},
        [&](bool on)
        {
          s = on;
        }},
       {{"s2,t", "bool"},
        [&](bool on)
        {
          t = on;
        }},
       {{"s3,u", "bool"},
        [&](bool on)
        {
          u = true;
        }},
       {{"s4,w", "bool"},
        [&](bool on)
        {
          w = on;
        }},
       {{"xx", "bool"},
        [&](bool on)
        {
          x = on;
        }}},
      {{{"o1,a", "int"},
        {wex::cmdline::INT,
         [&](const std::any& i)
         {
           a = std::any_cast<int>(i);
         }}},
       {{"o2,b", "float"},
        {wex::cmdline::FLOAT,
         [&](const std::any& f)
         {
           b = std::any_cast<float>(f);
         }}},
       {{"o3,c", "string"},
        {wex::cmdline::STRING,
         [&](const std::any& s)
         {
           c = std::any_cast<std::string>(s);
         }}}},
      {{"rest", "rest"},
       [&](const std::vector<std::string>& v)
       {
         p = v[0];
         q = v[1];
         r = v[2];
       }});

    SUBCASE("help")
    {
      wex::data::cmdline data("-h");
      const bool         res(cmdl.parse(data));

      REQUIRE(!res);
      REQUIRE(!data.help().empty());
    }

    SUBCASE("parse")
    {
      wex::data::cmdline data(
        "-a 10 -b 5.1 -c test -s -t -u -w --xx one two three");
      const bool res(cmdl.parse(data));

      CAPTURE(data.help());

      REQUIRE(res);

      REQUIRE(a == 10);
      REQUIRE(b == 5.1f);
      REQUIRE(c == "test");
      REQUIRE(s);
      REQUIRE(t);
      REQUIRE(u);
      REQUIRE(w);
      REQUIRE(x);
      REQUIRE(p == "one");
      REQUIRE(q == "two");
      REQUIRE(r == "three");
    }

    SUBCASE("parse_set")
    {
      wex::data::cmdline data("all");
      REQUIRE(cmdl.parse_set(data));
      REQUIRE(!data.help().empty());

      const bool res1(cmdl.parse_set(data.string("s1 s2")));

      CAPTURE(data.help());
      REQUIRE(res1);

      REQUIRE(!cmdl.parse_set(data.string("s9")));
      REQUIRE(cmdl.parse_set(data.string("s1?")));
      REQUIRE(cmdl.parse_set(data.string("nos1")));
    }
  }
}
