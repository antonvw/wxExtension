////////////////////////////////////////////////////////////////////////////////
// Name:      test-tool.cpp
// Purpose:   Implementation for wex unit testing
// Author:    Anton van Wezenbeek
// Copyright: (c) 2018-2021 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include "../test.h"
#include <wex/statistics.h>
#include <wex/tool.h>

TEST_CASE("wex::tool")
{
  wex::tool tool(wex::ID_ALL_CLOSE);
  wex::tool::add_info(wex::ID_ALL_CLOSE, "this is ok");

  wex::statistics<int> stat;

  REQUIRE(tool.id() == wex::ID_ALL_CLOSE);
  REQUIRE(!tool.get_tool_info().empty());
  REQUIRE(tool.info(&stat) == "this is ok 0 file(s)");
  REQUIRE(!tool.is_find_type());
  REQUIRE(!tool.is_report_type());

  REQUIRE(!wex::tool(wex::ID_TOOL_REPORT_FIND).info().empty());

  REQUIRE(wex::tool(wex::ID_TOOL_REPORT_FIND).is_find_type());
  REQUIRE(wex::tool(wex::ID_TOOL_REPLACE).is_find_type());
}
