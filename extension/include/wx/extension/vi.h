////////////////////////////////////////////////////////////////////////////////
// Name:      vi.h
// Purpose:   Declaration of class wxExVi
// Author:    Anton van Wezenbeek
// Copyright: (c) 2011 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#ifndef _EXVI_H
#define _EXVI_H

#include <map>
#include <wx/extension/ex.h>
#include <wx/extension/indicator.h>
#include <wx/extension/marker.h>

#if wxUSE_GUI

/// Offers a class that extends wxExSTC with vi behaviour.
class WXDLLIMPEXP_BASE wxExVi : public wxExEx
{
public:
  /// Constructor.
  wxExVi(wxExSTC* stc);
  
  /// Executes command in command mode (like 'j', or 'y').
  /// Returns true if the command was executed.
  virtual bool Command(const wxString& command);
  
  /// Returns whether we are in insert mode.
  bool GetInsertMode() const {return m_InsertMode;};
  
  /// Returns search flags.
  int GetSearchFlags() const {return m_SearchFlags;};
  
  /// Handles char events.
  /// Returns true if event is allowed to be skipped.
  /// This means that the char is not handled by vi,
  /// e.g. vi mode is not active, or we are in insert mode,
  /// so the char should be handled by stc.
  bool OnChar(const wxKeyEvent& event);

  /// Handles keydown events.
  /// See OnChar.
  bool OnKeyDown(const wxKeyEvent& event);
private:
  bool ChangeNumber(bool inc);
  virtual void DeleteMarker(const wxUniChar& marker);
  void FindWord(bool find_next = true);
  void GotoBrace();
  bool Indent(
    const wxString& begin_address, 
    const wxString& end_address, 
    bool forward);
  void InsertMode(
    const wxUniChar c = 'i', 
    int repeat = 1,
    bool overtype = false,
    bool dot = false);
  void Put(bool after);
  void SetIndicator(const wxExIndicator& indicator, int start, int end);
  void ToggleCase();
  void Yank(int lines);

  static wxString m_LastCommand;
  static wxString m_LastFindCharCommand;

  const wxExMarker m_MarkerSymbol;
  
  bool m_Dot;  
  bool m_InsertMode;
  bool m_SearchForward;
  
  int m_InsertRepeatCount;
  int m_SearchFlags;
  
  wxString m_Command;
  wxString m_InsertText;
};
#endif // wxUSE_GUI
#endif
