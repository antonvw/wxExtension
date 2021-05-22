////////////////////////////////////////////////////////////////////////////////
// Name:      test-dir.cpp
// Purpose:   Implementation for wex del unit testing
// Author:    Anton van Wezenbeek
// Copyright: (c) 2021 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <wex/del/dir.h>
#include <wex/del/listview-file.h>
#include <wex/frd.h>

#include "test.h"

TEST_CASE("wex::del::dir")
{
  SUBCASE("find_files")
  {
    auto* file = new wex::del::file(get_project());
    del_frame()->pane_add(file);
    auto* dir = new wex::del::dir(file, wex::test::get_path());
    REQUIRE(dir->find_files() == 0);
  }

  SUBCASE("find_files tool")
  {
    const wex::tool tool(wex::ID_TOOL_REPORT_FIND);
    REQUIRE(tool.id() == wex::ID_TOOL_REPORT_FIND);

    auto* lv = new wex::del::listview(
      wex::data::listview().type(wex::data::listview::FIND));
    del_frame()->pane_add(lv);

    wex::find_replace_data::get()->set_find_string("test");
    wex::dir dir(wex::path("./"), wex::data::dir().file_spec("*.cpp;*.h"));
    REQUIRE(dir.get_statistics().get_elements().get_items().empty());

    // we don't have set an event handler
    REQUIRE(!dir.find_files(tool));
    REQUIRE(dir.get_statistics().get_elements().get_items().empty());
  }
}