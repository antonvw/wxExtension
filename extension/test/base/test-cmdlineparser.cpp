////////////////////////////////////////////////////////////////////////////////
// Name:      test-cmdlineparser.cpp
// Purpose:   Implementation for wxExtension unit testing
// Author:    Anton van Wezenbeek
// Copyright: (c) 2015 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/numformatter.h>
#include <wx/extension/cmdline.h>
#include "../catch.hpp"
#include "../test.h"

TEST_CASE("wxExCmdLineParser")
{
  long a;
  double b;
  wxString c;
  wxDateTime d;
  bool s;
  bool t;
  bool u;
  wxString p,q,r;
  const wxChar ds(wxNumberFormatter::GetDecimalSeparator());
  
  const wxString str(ds == '.' ?
    "-a 10 -b 10.1 -c test -d 01-jan-2000 -s -t- -u one two three":
    "-a 10 -b 10,1 -c test -d 01-jan-2000 -s -t- -u one two three");
  
  wxExCmdLineParser clp(str,
    wxExCmdLineParser::CmdSwitches { 
      {{"s", ""}, {wxCMD_LINE_SWITCH_NEGATABLE, [&](bool on){s = on;}}},
      {{"t", ""}, {wxCMD_LINE_SWITCH_NEGATABLE, [&](bool on){t = on;}}},
      {{"u", ""}, {0, [&](bool on){u = true;}}}},
    wxExCmdLineParser::CmdOptions {
      {{"a", ""}, {wxCMD_LINE_VAL_NUMBER, [&](wxAny any) {any.GetAs(&a);}}},
      {{"b", ""}, {wxCMD_LINE_VAL_DOUBLE, [&](wxAny any) {any.GetAs(&b);}}},
      {{"c", ""}, {wxCMD_LINE_VAL_STRING, [&](wxAny any) {any.GetAs(&c);}}},
      {{"d", ""}, {wxCMD_LINE_VAL_DATE, [&](wxAny any) {any.GetAs(&d);}}}},
    wxExCmdLineParser::CmdParams {
      {"p", {0, [&](std::vector<wxString> & v) {p = v[0];}}},
      {"q", {0, [&](std::vector<wxString> & v) {q = v[1];}}},
      {"r", {0, [&](std::vector<wxString> & v) {r = v[2];}}}});
    
  REQUIRE( clp.Parse() == 0 );

  REQUIRE( a == 10 );
  REQUIRE( b == 10.1 );
  REQUIRE( c == "test" );
  REQUIRE( d.IsValid() );
  REQUIRE( s );
  REQUIRE(!t );
  REQUIRE( u );
  REQUIRE( p == "one" );
  REQUIRE( q == "two" );
  REQUIRE( r == "three" );
}