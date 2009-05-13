/******************************************************************************\
* File:          support.h
* Purpose:       Declaration of support classes
* Author:        Anton van Wezenbeek
* RCS-ID:        $Id$
*
* Copyright (c) 1998-2009, Anton van Wezenbeek
* All rights are reserved. Reproduction in whole or part is prohibited
* without the written consent of the copyright owner.
\******************************************************************************/

#ifndef _SUPPORT_H
#define _SUPPORT_H

#include <wx/extension/report/report.h>

/// Adds a toolbar and checkboxes to wxExFrameWithHistory.
class Frame : public wxExFrameWithHistory
{
public:
  /// Constructor.
  Frame(const wxString& project_wildcard);

#if wxUSE_CHECKBOX
  /// Gets hex mode.
  wxCheckBox* GetHexModeCheckBox() const {return m_HexModeCheckBox;};

  /// Gets sync mode.
  wxCheckBox* GetSyncCheckBox() const {return m_SyncCheckBox;};
#endif
private:
  // Interface from wxExFrame.
  virtual bool AllowClose(wxWindowID id, wxWindow* page);
  virtual void OnNotebook(wxWindowID id, wxWindow* page);

#if wxUSE_CHECKBOX
  wxCheckBox* m_HexModeCheckBox;
  wxCheckBox* m_SyncCheckBox;
#endif
};
#endif
