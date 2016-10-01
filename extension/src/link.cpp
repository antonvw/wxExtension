////////////////////////////////////////////////////////////////////////////////
// Name:      link.cpp
// Purpose:   Implementation of class wxExLink
// Author:    Anton van Wezenbeek
// Copyright: (c) 2016 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/config.h>
#include <wx/tokenzr.h>
#include <wx/extension/link.h>
#include <wx/extension/lexer.h>
#include <wx/extension/stc.h>
#include <wx/extension/util.h>

wxExLink::wxExLink(wxExSTC* stc)
  : m_STC(stc)
{
}

const std::string wxExLink::FindPath(const std::string& text, int line_no) const
{
  if (
    text.empty() ||
    // wxPathList cannot handle links over several lines.
    // add trimmed argument, to skip eol
    wxExGetNumberOfLines(text, true) > 1)
  {
    return std::string();
  }

  // Path in .po files.
  if (
    m_STC != nullptr &&
    m_STC->GetLexer().GetScintillaLexer() == "po" && text.substr(0, 3) == "#: ")
  {
    return text.substr(3);
  }
  
  // hypertext link
  std::vector <wxString> v;
  if (line_no < 0 &&
      (wxExMatch("(https?:.*)", text, v) > 0 || 
       wxExMatch("(www.*)", text, v) > 0))
  {
    // with a possible delimiter
    std::string match(v[0]);
    const std::string delimiters("\")");
    
    for (const auto c : delimiters)
    {
      size_t pos = match.find(c);
      
      if (pos != std::string::npos)
      {
        return match.substr(0, pos);
      }
    }
    
    // without delimiter
    return match;
  }
  
  // hypertext file
  if (
    line_no == -1 &&
    m_STC != nullptr && 
    m_STC->GetLexer().GetScintillaLexer() == "hypertext")
  {
    return m_STC->GetFileName().GetFullPath().ToStdString();
  }
  
  if (line_no < 0) return std::string();

  // Better first try to find "...", then <...>, as in next example.
  // <A HREF="http://www.scintilla.org">scintilla</A> component.

  // So, first get text between " signs.
  size_t pos_char1 = text.find("\"");
  size_t pos_char2 = text.rfind("\"");

  // If that did not succeed, then get text between < and >.
  if (pos_char1 == wxString::npos || 
      pos_char2 == wxString::npos || 
      pos_char2 <= pos_char1)
  {
    pos_char1 = text.find("<");
    pos_char2 = text.rfind(">");
  }

  // If that did not succeed, then get text between ' and '.
  if (pos_char1 == wxString::npos ||
      pos_char2 == wxString::npos || 
      pos_char2 <= pos_char1)
  {
    pos_char1 = text.find("'");
    pos_char2 = text.rfind("'");
  }
  
  std::string out;

  // If we did not find anything.
  if (pos_char1 == std::string::npos || 
      pos_char2 == std::string::npos || 
      pos_char2 <= pos_char1)
  {
    out = text;
  }
  else
  {
    // Okay, get everything inbetween.
    out = text.substr(pos_char1 + 1, pos_char2 - pos_char1 - 1);
  }

  // And make sure we skip white space.
  out = wxExSkipWhiteSpace(out);
  
  return out;
}

// text contains selected text, or current line
const std::string wxExLink::GetPath(
  const std::string& text,
  int& line_no,
  int& column_no) const
{
  // line_no:
  // - -1: look for browse, and browse file
  // - -2: look for browse
  const std::string path(FindPath(text, line_no));
  
  if (line_no < 0)
  { 
    return path;
  }
  
  std::string link(path);
  
  SetLink(link, line_no, column_no);
  
  if (
    link.empty() || 
    // Otherwise, if you happen to select text that 
    // ends with a separator, wx asserts.
    wxFileName::IsPathSeparator(link.back()))
  {
    return std::string();
  }

  wxFileName file(link);
  std::string fullpath;

  if (file.FileExists())
  {
    file.MakeAbsolute();
    fullpath = file.GetFullPath();
  }
  else
  {
    if (
      file.IsRelative() && 
      m_STC != nullptr && 
      m_STC->GetFileName().FileExists())
    {
      if (file.MakeAbsolute(m_STC->GetFileName().GetPath()))
      {
        if (file.FileExists())
        {
          fullpath = file.GetFullPath();
        }
      }
    }

    if (fullpath.empty())
    {
      int pos = path.find_last_of(' ');
      
      // Check whether last word is a file.
      std::string word = wxExSkipWhiteSpace((pos != std::string::npos ? path.substr(pos): std::string()));
    
      if (
       !word.empty() && 
       !wxFileName::IsPathSeparator(link.back()) &&
        wxFileExists(word))
      {
        wxFileName file(word);
        file.MakeAbsolute();
        fullpath = file.GetFullPath();
        // And reset line or column.
        line_no = 0;
        column_no = 0;
      }
    
      if (fullpath.empty() && !m_PathList.empty())
      {
        fullpath = m_PathList.FindAbsoluteValidPath(link);
      
        if (
          fullpath.empty() && 
         !word.empty() &&
          SetLink(word, line_no, column_no))
        {
          fullpath = m_PathList.FindAbsoluteValidPath(word);
        }
      }
      
      // Do nothing if fullpath.empty(),
      // as we return empty string if no path could be found.
    }
  }
  
  return fullpath;
}

bool wxExLink::SetLink(std::string& link, int& line_no, int& column_no) const
{
  if (link.size() < 2)
  {
    return false;
  }

  // Using backslash as separator does not yet work.
  std::replace(link.begin(), link.end(), '\\', '/');

  // The harddrive letter is filtererd, it does not work
  // when adding it to wxExMatch.
  std::string prefix;

#ifdef __WXMSW__
  if (isalpha(link[0]) && link[1] == ':')
  {
    prefix = link.substr(0,1);
    link = link.substr(2);
  }
#endif

  // file[:line[:column]]
  std::vector <wxString> v;
  
  if (wxExMatch("([0-9A-Za-z _/.-]+):([0-9]*):?([0-9]*)", link, v))
  {
    link = v[0];
    line_no = 0;
    column_no = 0;
      
    if (v.size() > 1)
    {
      line_no = atoi(v[1]);
        
      if (v.size() > 2)
      {
        column_no = atoi(v[2]);
      }
    }
      
    link = wxExSkipWhiteSpace(prefix + link);
    
    return true;
  }
  
  return false;
}
  
void wxExLink::SetFromConfig()
{
  wxStringTokenizer tkz(
    wxConfigBase::Get()->Read(_("Include directory")),
    "\r\n");
    
  m_PathList.Empty();
  
  while (tkz.HasMoreTokens())
  {
    m_PathList.Add(tkz.GetNextToken());
  }
}
