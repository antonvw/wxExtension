////////////////////////////////////////////////////////////////////////////////
// Name:      test-dirctrl.cpp
// Purpose:   Implementation for wxExtension report cpp unit testing
// Author:    Anton van Wezenbeek
// Copyright: (c) 2015 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <wx/extension/report/dir.h> // necessary?
#include <wx/extension/report/dirctrl.h>
#include "test.h"

void wxExGuiReportTestFixture::testDirCtrl()
{
  wxExGenericDirCtrl* ctrl = new wxExGenericDirCtrl(m_Frame, m_Frame);
  ctrl->ExpandAndSelectPath("test");

  wxCommandEvent event(ID_TREE_COPY);  
  wxPostEvent(ctrl, event);
}
