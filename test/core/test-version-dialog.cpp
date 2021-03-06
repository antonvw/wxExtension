////////////////////////////////////////////////////////////////////////////////
// Name:      test-version.cpp
// Purpose:   Implementation for wex unit testing
// Author:    Anton van Wezenbeek
// Copyright: (c) 2020-2021 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include "../test.h"
#include <wex/version-dialog.h>

TEST_CASE("wex::about_info")
{
  wex::about_info about;

  REQUIRE(about.GetDescription().empty());
  REQUIRE(about.GetDevelopers().empty());
  REQUIRE(about.GetLicence().empty());
  REQUIRE(about.GetWebSiteURL().empty());

  REQUIRE(about.description("xx").GetDescription() == "xx");
  REQUIRE(about.developer("yy").GetDevelopers()[0] == "yy");
  REQUIRE(about.licence("zz").GetLicence() == "zz");
  REQUIRE(about.website("www.xyz").GetWebSiteURL() == "www.xyz");
}

TEST_CASE("wex::version_dialog")
{
  SUBCASE("default-constructor")
  {
    const wex::version_info_dialog info;

    REQUIRE(info.about().GetDescription().find("wex") != std::string::npos);
  }

  SUBCASE("constructor")
  {
    wex::about_info about;
    about.description("hello");
    const wex::version_info_dialog info(about);

    REQUIRE(info.about().GetDescription().find("hello") != std::string::npos);
  }
}
