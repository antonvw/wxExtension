////////////////////////////////////////////////////////////////////////////////
// Name:      stream.cpp
// Purpose:   Implementation of wxExStream class
// Author:    Anton van Wezenbeek
// Copyright: (c) 2017 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <wx/config.h>
#include <wx/extension/stream.h>
#include <wx/extension/frd.h>
#include <wx/extension/util.h>

bool wxExStream::m_Asked = false;

wxExStream::wxExStream(const wxExFileName& filename, const wxExTool& tool)
  : m_FileName(filename)
  , m_Tool(tool)
  , m_FRD(wxExFindReplaceData::Get())
  , m_Threshold(wxConfigBase::Get()->ReadLong(_("Replacements"), -1))
{
}

bool wxExStream::Process(std::string& line, size_t line_no)
{
  bool match = false;
  int count = 1;
  int pos = -1;

  if (m_FRD->UseRegEx())
  {
    pos = m_FRD->RegExMatches(line);
    match = (pos >= 0);

    if (match && m_Tool.GetId() == ID_TOOL_REPLACE)
    {
      count = m_FRD->RegExReplaceAll(line);
      if (!m_Modified) m_Modified = (count > 0);
    }
  }
  else
  {
    if (m_Tool.GetId() == ID_TOOL_REPORT_FIND)
    {
      const auto it = (!m_FRD->MatchCase() ?
        std::search(line.begin(), line.end(), m_FindString.begin(), m_FindString.end(),
          [](char ch1, char ch2) {return std::toupper(ch1) == ch2;}):
        std::search(line.begin(), line.end(), m_FindString.begin(), m_FindString.end()));

      if (it != line.end())
      {
        match = true;
        pos = it - line.begin();

        if (m_FRD->MatchWord() && 
            ((it != line.begin() && IsWordCharacter(*std::prev(it))) ||
              IsWordCharacter(*std::next(it, m_FindString.length()))))
        {
          match = false;
        }
      }
    }
    else
    {
      count = wxExReplaceAll(
        line, 
        m_FRD->GetFindString(), 
        m_FRD->GetReplaceString(),
        &pos);

      match = (count > 0);
      if (!m_Modified) m_Modified = match;
    }
  }

  if (match)
  {
    if (m_Tool.GetId() == ID_TOOL_REPORT_FIND)
    {
      ProcessMatch(line, line_no, pos);
    }
    
    const auto ac = IncActionsCompleted(count);

    if (!m_Asked && m_Threshold != -1 && (ac - m_Prev > m_Threshold))
    {
      if (wxMessageBox(
        "More than " + std::to_string(m_Threshold) + " matches in: " + 
          m_FileName.GetFullPath() + "?",
        _("Continue"),
        wxYES_NO | wxICON_QUESTION) == wxNO)
      {
        return false;
      }
      else
      {
        m_Asked = true;
      }
    }
  }

  return true;
}

bool wxExStream::ProcessBegin()
{
  if (
    !m_Tool.IsFindType() || 
    (m_Tool.GetId() == ID_TOOL_REPLACE && m_FileName.GetStat().IsReadOnly()) ||
     wxExFindReplaceData::Get()->GetFindString().empty())
  {
    return false;
  }

  m_FindString = wxExFindReplaceData::Get()->GetFindString();
  m_Prev = m_Stats.Get(_("Actions Completed").ToStdString());
  m_Write = (m_Tool.GetId() == ID_TOOL_REPLACE);

  if (!wxExFindReplaceData::Get()->MatchCase())
  {
    for (auto & c : m_FindString) c = std::toupper(c);
  }
  
  return true;
}
  
bool wxExStream::RunTool()
{
  std::ifstream ifs(m_FileName.GetFullPath());

  if (!ifs.is_open() || !ProcessBegin())
  {
    return false;
  }

  m_Stats.m_Elements.Set(_("Files").ToStdString(), 1);
  
  std::string line;
  int line_no = 0;
  std::vector<std::string> v;

  while (std::getline(ifs, line))
  {
    if (!Process(line, line_no++)) return false;

    if (m_Write)
    {
      v.emplace_back(line);
    }
  }

  if (m_Modified && m_Write)
  {
    std::ofstream ofs(m_FileName.GetFullPath().c_str());
  
    for (const auto & it : v)
    {
      ofs << it << std::endl;
    }
  }
  
  ProcessEnd();

  return true;
}

void wxExStream::Reset()
{
  m_Asked = false;
}
