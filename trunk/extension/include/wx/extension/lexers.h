/******************************************************************************\
* File:          lexers.h
* Purpose:       Declaration of wxExLexers class
* Author:        Anton van Wezenbeek
* RCS-ID:        $Id$
*
* Copyright (c) 2009, Anton van Wezenbeek
* All rights are reserved. Reproduction in whole or part is prohibited
* without the written consent of the copyright owner.
\******************************************************************************/

#ifndef _EXLEXERS_H
#define _EXLEXERS_H

#include <map>
#include <vector>
#include <wx/colour.h> 
#include <wx/filename.h>
#include <wx/xml/xml.h>
#include <wx/extension/lexer.h>
#include <wx/extension/indicator.h>
#include <wx/extension/marker.h>
#include <wx/extension/property.h>
#include <wx/extension/style.h>

class wxStyledTextCtrl;

/// Reads the lexers, keywords, markers and styles
/// from the configuration file and makes
/// them available.
class wxExLexers
{
public:
  /// Constructor for lexers from specified filename.
  wxExLexers(const wxFileName& filename);

  /// Sets global styles for specified component.
  void ApplyGlobalStyles(wxStyledTextCtrl* stc) const;

  /// Sets hex styles for specified component.
  void ApplyHexStyles(wxStyledTextCtrl* stc) const;

  /// Sets indicators for specified component.
  void ApplyIndicators(wxStyledTextCtrl* stc) const;

  /// Applies macro to text:
  /// if text is referring to a macro, text is replaced by the macro value.
  /// Otherwise the same text is returned.
  const wxString ApplyMacro(const wxString& text) const;

  /// Sets markers for specified component.
  void ApplyMarkers(wxStyledTextCtrl* stc) const;

  /// Sets properties for specified component.
  void ApplyProperties(wxStyledTextCtrl* stc) const;

  /// Builds a wildcard string from available lexers using specified filename.
  const wxString BuildWildCards(const wxFileName& filename) const;

  /// Returns the number of lexers.
  size_t Count() const {return m_Lexers.size();};

  /// Finds a lexer specified by a filename.
  const wxExLexer FindByFileName(const wxFileName& filename) const;

  /// Finds a lexer specified by the (scintilla) name.
  const wxExLexer FindByName(const wxString& name) const;

  /// Finds a lexer if text starts with some special tokens.
  const wxExLexer FindByText(const wxString& text) const;

  /// Returns the lexers object.
  static wxExLexers* Get(bool createOnDemand = true);

  /// Returns the default style.
  const wxExStyle& GetDefaultStyle() const {return m_DefaultStyle;};

  /// Gets the filename.
  const wxFileName& GetFileName() const {return m_FileName;};

  /// Gets the macros.
  const std::map<wxString, wxString>& GetMacros() const {return m_Macros;};

  /// Gets the style macros.
  const std::map<wxString, wxString>& GetMacrosStyle() const {return m_MacrosStyle;};

  /// Returns true if specified indicator is available.
  bool IndicatorIsLoaded(int indic) const;

  /// Parses properties tag.
  const std::vector<wxExProperty> ParseTagProperties(const wxXmlNode* node) const;

  /// Reads the lexers, keywords, markers and styles from file.
  void Read();

  /// Sets the object as the current one, returns the pointer 
  /// to the previous current object 
  /// (both the parameter and returned value may be NULL). 
  static wxExLexers* Set(wxExLexers* lexers);

  /// Shows a dialog with all lexers, allowing you to choose one.
  /// Returns true if you selected one.
  bool ShowDialog(
    wxWindow* parent,
    wxString& lexer,
    const wxString& caption = _("Enter Lexer")) const;
private:
  const wxString GetLexerExtensions() const;
  void ParseTagGlobal(const wxXmlNode* node);
  void ParseTagMacro(const wxXmlNode* node);

  std::map<wxString, wxExLexer> m_Lexers;

  std::map<wxString, wxString> m_Macros;
  std::map<wxString, wxString> m_MacrosStyle;

  std::vector<wxExProperty> m_GlobalProperties;
  std::vector<wxExIndicator> m_Indicators;
  std::vector<wxExMarker> m_Markers;
  std::vector<wxExStyle> m_Styles;
  std::vector<wxExStyle> m_StylesHex;

  wxExStyle m_DefaultStyle;
  wxSortedArrayString m_SortedLexerNames;

  const wxFileName m_FileName;

  static wxExLexers* m_Self;
};
#endif
