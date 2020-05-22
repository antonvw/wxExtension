////////////////////////////////////////////////////////////////////////////////
// Name:      bind.cpp
// Purpose:   Implementation of class wex::bind
// Author:    Anton van Wezenbeek
// Copyright: (c) 2020 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <wex/bind.h>
#include <wex/defs.h>

wex::bind::bind(wxEvtHandler* evt)
  : m_handler(evt)
{
}

void wex::bind::menus(
  std::vector<std::pair<std::function<void(wxCommandEvent&)>, int>> v)
{
  for (const auto& it : v)
  {
    switch (it.second)
    {
      case ID_EDIT_DEBUG_FIRST:
        m_handler->Bind(wxEVT_MENU, it.first, it.second, ID_EDIT_DEBUG_LAST);
        break;

      case ID_EDIT_VCS_LOWEST:
        m_handler->Bind(wxEVT_MENU, it.first, it.second, ID_EDIT_VCS_HIGHEST);
        break;

      case wxID_SORT_ASCENDING:
        m_handler->Bind(wxEVT_MENU, it.first, it.second, wxID_SORT_ASCENDING);
        break;

      default:
        if (it.second == ID_EDIT_EOL_DOS)
          m_handler->Bind(wxEVT_MENU, it.first, it.second, ID_EDIT_EOL_MAC);
        else
          m_handler->Bind(wxEVT_MENU, it.first, it.second);
    }
  }
}
