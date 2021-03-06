////////////////////////////////////////////////////////////////////////////////
// Name:      test-indicator.cpp
// Purpose:   Implementation for wex unit testing
// Author:    Anton van Wezenbeek
// Copyright: (c) 2021 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <wex/indicator.h>
#include <wx/stc/stc.h>

#include "../test.h"

TEST_CASE("wex::indicator")
{
  SUBCASE("constructor") { REQUIRE(!wex::indicator().is_ok()); }

  SUBCASE("constructor-2")
  {
    wex::indicator indx(5, 2);
    wex::indicator indy(7, 5);

    REQUIRE(!wex::indicator(5).is_ok());
    REQUIRE(indx.is_ok());
    REQUIRE(indy.is_ok());
    REQUIRE(indx < indy);
    REQUIRE(indx == indx);
    REQUIRE(indx != indy);
    REQUIRE(wex::indicator(5) == wex::indicator(5));
    REQUIRE(wex::indicator(5) == wex::indicator(5, 2));
    REQUIRE(wex::indicator(5) != wex::indicator(4));
    REQUIRE(wex::indicator(5, 2) == wex::indicator(5, 2));
    REQUIRE(wex::indicator(5, 1) != wex::indicator(5, 2));
  }

  SUBCASE("constructor-3")
  {
    pugi::xml_document     doc;
    pugi::xml_parse_result result =
      doc.load_string("<indicator no = \"5\">indic_box,green</indicator>");
    REQUIRE(result);

    wex::indicator ind(doc.document_element());
    REQUIRE(ind.foreground_colour() == "green");
    REQUIRE(ind.number() == 5);
    REQUIRE(ind.style() == 6);
    REQUIRE(!ind.is_underlined());
    REQUIRE(ind.is_ok());

    wxStyledTextCtrl s;
    ind.apply(&s);

    REQUIRE(ind.is_ok());
  }

  SUBCASE("constructor-4")
  {
    pugi::xml_document doc;
    REQUIRE(doc.load_string("<indicator no = \"x\"></indicator>"));
    wex::indicator ind(doc.document_element());
    REQUIRE(!ind.is_ok());
  }
}
