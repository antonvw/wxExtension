////////////////////////////////////////////////////////////////////////////////
// Name:      main.cpp
// Purpose:   main for wex unit testing
// Author:    Anton van Wezenbeek
// Copyright: (c) 2019 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wex/macros.h>
#include <wex/managedframe.h>
#include <wex/process.h>
#include <wex/shell.h>

#include "../test.h"

IMPLEMENT_APP_NO_MAIN(wex::test::gui_app);

int main(int argc, char* argv[])
{
  return wex::test::main(argc, argv, new wex::test::gui_app());
}