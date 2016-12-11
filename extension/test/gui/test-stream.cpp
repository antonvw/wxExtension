////////////////////////////////////////////////////////////////////////////////
// Name:      test-stream.cpp
// Purpose:   Implementation for wxExtension unit testing
// Author:    Anton van Wezenbeek
// Copyright: (c) 2016 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <chrono>
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/buffer.h>
#include <wx/extension/stream.h>
#include <wx/extension/frd.h>
#include "test.h"

TEST_CASE("wxExStreamStatistics")
{
  wxExStreamStatistics ss;
  
  REQUIRE(ss.Get().empty());
  REQUIRE(ss.Get("xx") == 0);

  wxExStreamStatistics ss2;
  REQUIRE(ss2.Get().empty());

  ss += ss2;
  
  REQUIRE(ss.Get().empty());
}

TEST_CASE("wxExStream")
{
  // Test find.
  wxExStream s(GetTestFile(), ID_TOOL_REPORT_FIND);
  
  REQUIRE( s.GetFileName() == GetTestFile());
  REQUIRE( s.GetTool().GetId() == ID_TOOL_REPORT_FIND);
  
  wxExFindReplaceData::Get()->SetFindString("test");
  wxExFindReplaceData::Get()->SetMatchCase(true);
  wxExFindReplaceData::Get()->SetMatchWord(true);
  wxExFindReplaceData::Get()->SetUseRegEx(false);
  
  const auto start = std::chrono::system_clock::now();
  
  REQUIRE( s.RunTool());
  
  const auto milli = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start);
  
  REQUIRE(milli.count() < 100);
  
  INFO(wxString::Format(
    "wxExStream::matching %d items in %d ms", 
    s.GetStatistics().Get("Actions Completed"), (int)milli.count()).ToStdString());
    
  REQUIRE(!s.GetStatistics().GetElements().GetItems().empty());
  REQUIRE( s.GetStatistics().Get("Actions Completed") == 193);
  
  // Test replace.
  wxExStream s2(GetTestFile(), ID_TOOL_REPORT_REPLACE);
  
  wxExFindReplaceData::Get()->SetReplaceString("test");
  
  const auto start2 = std::chrono::system_clock::now();
  REQUIRE( s2.RunTool());
  const auto milli2 = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start2);
  
  REQUIRE(milli2.count() < 100);
  
  INFO(wxString::Format(
    "wxExStream::replacing %d items in %d ms", 
    s2.GetStatistics().Get("Actions Completed"), (int)milli2.count()).ToStdString());
    
  REQUIRE(!s2.GetStatistics().GetElements().GetItems().empty());
  REQUIRE( s2.GetStatistics().Get("Actions Completed") == 194);
}