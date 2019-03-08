////////////////////////////////////////////////////////////////////////////////
// Name:      test-blame.cpp
// Purpose:   Implementation for wex unit testing
// Author:    Anton van Wezenbeek
// Copyright: (c) 2019 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <wex/blame.h>
#include <wex/config.h>
#include "test.h"

TEST_CASE("wex::blame")
{
  pugi::xml_document doc;
  REQUIRE( doc.load_string(
    "<vcs name=\"git\" "
    "date-format=\"%Y-%m-%d %H:%M:%S\" "
    "pos-begin=\"0\" pos-end=\"+\" "
    "pos-id-begin=\"0\" pos-id-end=\" \" "
    "pos-author-begin=\"(\" >"
    "</vcs>"));

  wex::blame blame(doc.document_element());

  REQUIRE(!wex::blame().use());
  REQUIRE( blame.use());
  
  const std::string text("\
    bf5d87cc src/http_travel.cpp (A unknown user 2019-02-01 12:20:06 +0100 15) const std::string& http_travel:get_country()");
  
  wex::config("blame_autor").set(true);
  wex::config("blame_date").set(true);
  wex::config("blame_id").set(true);
    
  
  REQUIRE(!std::get<0> (blame.get("")));
  REQUIRE(!std::get<0> (blame.get(std::string())));
  REQUIRE( std::get<0> (blame.get(text)));
  
  REQUIRE( std::get<1> (blame.get(text)).find("A unknown user") != std::string::npos);
  REQUIRE( std::get<1> (blame.get(text)).find("2019-02-01 12:20:06") != std::string::npos);
  REQUIRE( std::get<1> (blame.get(text)).find("bf5d87cc") != std::string::npos);
  REQUIRE( std::get<2> (blame.get(text)) != wex::lexers::MARGIN_STYLE_UNKNOWN );
}
