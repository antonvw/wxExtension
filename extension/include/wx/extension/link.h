////////////////////////////////////////////////////////////////////////////////
// Name:      link.h
// Purpose:   Declaration of class wxExLink
// Author:    Anton van Wezenbeek
// Copyright: (c) 2011 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#ifndef _EXLINK_H
#define _EXLINK_H

#include <wx/filefn.h>

#if wxUSE_GUI

class wxExSTC;

/// Offers a class holding info about a link.
class WXDLLIMPEXP_BASE wxExLink
{
public:
  /// Constructor.
  wxExLink(wxExSTC* stc);
  
  /// Adds a possible base path, present in the stc component.
  /// Returns true if a path was added.
  bool AddBasePath();
  
  /// Gets a possible path from text, does not use pathlist
  /// to make an existing path of it.
  const wxString FindPath(const wxString& text) const;
  
  /// Gets a path from text, using path list if necessary.
  /// Returns empty string if no path could be found.
  const wxString GetPath(const wxString& text) const;
  
  /// Sets link with info from config.
  void SetFromConfig();
private:
  wxPathList m_PathList;
  wxExSTC* m_STC;
};
#endif // wxUSE_GUI
#endif
