////////////////////////////////////////////////////////////////////////////////
// Name:      vi.h
// Purpose:   Declaration of class wxExVi
// Author:    Anton van Wezenbeek
// Copyright: (c) 2014 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#ifndef _EXVI_H
#define _EXVI_H

#include <wx/extension/ex.h>

#if wxUSE_GUI

class wxExSTCEntryDialog;

/// Offers a class that extends wxExSTC with vi behaviour.
class WXDLLIMPEXP_BASE wxExVi : public wxExEx
{
public:
  /// The possible vi modes.
  enum wxExViMode
  {
    MODE_NORMAL,      ///< normal (command or navigation) mode
    MODE_INSERT,      ///< pressing key inserts key
    MODE_VISUAL,      ///< navigation keys extend selection
    MODE_VISUAL_LINE, ///< complete lines are selected
  };

  /// Constructor.
  wxExVi(wxExSTC* stc);
  
  /// Executes vi command.
  /// Returns true if the command was executed.
  virtual bool Command(const std::string& command);
  
  /// Returns text to be inserted.
  const std::string& GetInsertText() const {return m_InsertText;};
  
  /// Returns the mode we are in.
  int GetMode() const {return m_Mode;};
  
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
  void AddText(const std::string& text);
  bool ChangeNumber(bool inc);
  void CommandCalc(const wxString& reg);
  bool CommandChar(int c, int repeat);
  void CommandReg(const char reg);
  void DeleteRange(int start, int end);
  void FindWord(bool find_next = true);
  void GotoBrace();
  bool Indent(
    const wxString& begin_address, 
    const wxString& end_address, 
    bool forward);
  bool InsertMode(const std::string& text);
  /// Adds recording to current macro.
  virtual void MacroRecord(const std::string& text);
  bool Put(bool after);
  bool SetInsertMode(
    const wxString& command,
    int repeat = 1);
  bool ToggleCase(); 
  bool YankedLines();   

  static wxExSTCEntryDialog* m_Dialog;
  static std::string m_LastFindCharCommand;

  bool m_Dot;  
  bool m_SearchForward;
  
  int m_InsertRepeatCount;
  int m_Mode;
  
  std::string m_Command;
  std::string m_InsertText;
};
#endif // wxUSE_GUI
#endif
