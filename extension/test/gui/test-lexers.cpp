////////////////////////////////////////////////////////////////////////////////
// Name:      test-lexers.cpp
// Purpose:   Implementation for wex unit testing
// Author:    Anton van Wezenbeek
// Copyright: (c) 2019 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wex/lexers.h>
#include <wex/managedframe.h>
#include <wex/path.h>
#include <wex/stc.h>
#include "test.h"

TEST_CASE("wex::lexers")
{
  SUBCASE("Get")
  {
    REQUIRE( wex::lexers::get() != nullptr);
    REQUIRE(!wex::lexers::get()->get_lexers().empty());
  }
  
  SUBCASE("lexer and global macros")
  {
    for (const auto& macro : std::vector<
      std::pair<
        std::pair<std::string,std::string>,
        std::string>> {
      {{"number","asm"},"2"},
      {{"number","cpp"},"4"},
      {{"XXX","global"},"XXX"},
      {{"mark_lcorner","global"},"10"},
      {{"mark_circle","global"},"0"},
      {{"iv_none","global"},"0"}})
    {
      REQUIRE( wex::lexers::get()->apply_macro(
        macro.first.first, macro.first.second) == macro.second);
    }
  }

  SUBCASE("Properties")
  {
    REQUIRE( wex::lexers::get()->properties().empty());
  }
  
  SUBCASE("FindBy")
  {
    REQUIRE( wex::lexers::get()->find_by_filename(
      wex::test::get_path("test.h").fullname()).scintilla_lexer() == "cpp");
      
    REQUIRE( wex::lexers::get()->find_by_name(
      "xxx").scintilla_lexer().empty());
      
    REQUIRE( wex::lexers::get()->find_by_name(
      "cpp").scintilla_lexer() == "cpp");
    
    for (const auto& findby : std::vector<std::pair<
      std::string, 
      std::pair<std::string, std::string>>> {
      {"// this is a cpp comment text",{"cpp","cpp"}},
      {"#!/bin/bash",{"bash","bash"}},
      {"#!/bin/bash\n",{"bash","bash"}},
      {"#!/usr/bin/csh",{"bash","bash"}},
      {"#!/bin/csh",{"bash","csh"}},
      {"#!/bin/env python",{"python","python"}},
      {"#!/bin/sh",{"bash","sh"}},
      {"#!/bin/tcsh",{"bash","tcsh"}},
      {"<html>",{"hypertext","hypertext"}},
      {"<?xml",{"hypertext","xml"}}})
    {
      REQUIRE( wex::lexers::get()->find_by_text(
        findby.first).scintilla_lexer() == findby.second.first);
      REQUIRE( wex::lexers::get()->find_by_text(
        findby.first).display_lexer() == findby.second.second);
    }
  }
    
  SUBCASE("Rest")
  {
    REQUIRE(!wex::lexers::get()->get_filename().data().empty());

    REQUIRE(!wex::lexers::get()->get_macros("global").empty());
    REQUIRE(!wex::lexers::get()->get_macros("cpp").empty());
    REQUIRE(!wex::lexers::get()->get_macros("pascal").empty());
    REQUIRE( wex::lexers::get()->get_macros("XXX").empty());
    
    REQUIRE(!wex::lexers::get()->theme().empty());
    REQUIRE(!wex::lexers::get()->theme_macros().empty());
    REQUIRE( wex::lexers::get()->get_themes_size() > 1);

    wex::lexers::get()->reset_theme();
    REQUIRE( wex::lexers::get()->theme().empty());
    wex::lexers::get()->restore_theme();
    REQUIRE(!wex::lexers::get()->theme().empty());
    
    REQUIRE(!wex::lexers::get()->indicator_is_loaded(wex::indicator(99)));
    REQUIRE( wex::lexers::get()->indicator_is_loaded(wex::indicator(0)));
    REQUIRE( wex::lexers::get()->marker_is_loaded(wex::marker(0)));
    REQUIRE( wex::lexers::get()->get_indicator(wex::indicator(0)).is_ok());
    REQUIRE( wex::lexers::get()->get_marker(wex::marker(0)).is_ok());
    
    REQUIRE(!wex::lexers::get()->keywords("cpp").empty());
    REQUIRE(!wex::lexers::get()->keywords("csh").empty());
    REQUIRE( wex::lexers::get()->keywords("xxx").empty());
    REQUIRE( wex::lexers::get()->keywords(std::string()).empty());

    REQUIRE( wex::lexers::get()->load_document());

    get_stc()->get_lexer().set("cpp");
    get_stc()->open(wex::test::get_path());
    wex::lexers::get()->apply_global_styles(get_stc());
    wex::lexers::get()->apply(get_stc());
    wex::lexers::get()->apply_margin_text_style(get_stc(), 30, wex::lexers::MARGIN_STYLE_DAY);
  }
}
