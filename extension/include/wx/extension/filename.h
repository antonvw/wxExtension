////////////////////////////////////////////////////////////////////////////////
// Name:      filename.h
// Purpose:   Declaration of class wxExFileName
// Author:    Anton van Wezenbeek
// Copyright: (c) 2016 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/filename.h>
#include <wx/extension/lexer.h>
#include <wx/extension/lexers.h>
#include <wx/extension/stat.h>

class wxExFile;

/// Adds a wxExStat and a wxExLexer member to wxFileName.
class wxExFileName : public wxFileName
{
  friend class wxExFile; // it might update stat
public:
  /// Default constructor.
  wxExFileName() : wxFileName() {;};

  /// Constructor taking a string.
  wxExFileName(const wxString& fullpath, wxPathFormat format = wxPATH_NATIVE)
    : wxFileName(fullpath, format)
    , m_Stat(fullpath) {m_Lexer = wxExLexers::Get()->FindByFileName(*this);};

  /// Constructor taking a char array.
  wxExFileName(const char* fullpath, wxPathFormat format = wxPATH_NATIVE)
    : wxExFileName(wxString(fullpath), format) {;};

  /// Constructor from a wxFileName.
  wxExFileName(const wxFileName& filename)
    : wxExFileName(filename.GetFullPath()) {;};

  /// Returns the lexer.
  const auto & GetLexer() const {return m_Lexer;};

  /// Returns the stat.
  const auto & GetStat() const {return m_Stat;};
  
  /// Sets the lexer.
  void SetLexer(const wxExLexer& lexer) {m_Lexer = lexer;};
private:
  wxExLexer m_Lexer;
  wxExStat m_Stat;
};
