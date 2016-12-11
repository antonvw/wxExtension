////////////////////////////////////////////////////////////////////////////////
// Name:      stream.cpp
// Purpose:   Implementation of class wxExStreamToListView
// Author:    Anton van Wezenbeek
// Copyright: (c) 2016 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <cctype> // for isspace
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/config.h>
#include <wx/extension/report/stream.h>
#include <wx/extension/frd.h>
#include <wx/extension/listitem.h>
#include <wx/extension/util.h>
#include <wx/extension/report/defs.h>
#include <wx/extension/report/frame.h>
#include <wx/extension/report/listview.h>

wxExListView* wxExStreamToListView::m_Report = nullptr;
wxExFrameWithHistory* wxExStreamToListView::m_Frame = nullptr;

wxExStreamToListView::wxExStreamToListView(
  const wxExFileName& filename,
  const wxExTool& tool)
  : wxExStream(filename, tool)
  , m_LastSyntaxType(SYNTAX_NONE)
  , m_SyntaxType(SYNTAX_NONE)
  , m_IsCommentStatement(false)
  , m_IsString(false)
{
}

wxExStreamToListView::wxExCommentType wxExStreamToListView::CheckCommentSyntax(
  const wxString& syntax_begin,
  const wxString& syntax_end,
  const wxString& text) const
{
  if (syntax_begin.empty() && syntax_end.empty())
  {
    return COMMENT_NONE;
  }
  
  if (syntax_begin == text)
  {
    return (syntax_end == text) ? COMMENT_BOTH: COMMENT_BEGIN;
  }
  else
  {
    if (syntax_end == text ||
       (syntax_end.empty() && text.empty()))
    {
      return COMMENT_END;
    }
  }

  if ( syntax_begin.StartsWith(text) || 
      (!syntax_end.empty() && syntax_end.StartsWith(text)))
  {
    return COMMENT_INCOMPLETE;
  }

  return COMMENT_NONE;
}

wxExStreamToListView::wxExCommentType wxExStreamToListView::CheckForComment(
  const wxString& text)
{
  if (GetFileName().GetLexer().GetCommentBegin2().empty())
  {
    return CheckCommentSyntax(
      GetFileName().GetLexer().GetCommentBegin(),
      GetFileName().GetLexer().GetCommentEnd(), text);
  }

  wxExCommentType comment_type1 = COMMENT_NONE;

  if (m_SyntaxType == SYNTAX_NONE || m_SyntaxType == SYNTAX_ONE)
  {
    if ((comment_type1 = CheckCommentSyntax(
      GetFileName().GetLexer().GetCommentBegin(),
      GetFileName().GetLexer().GetCommentEnd(), text)) == COMMENT_BEGIN)
      m_SyntaxType = SYNTAX_ONE;
  }

  wxExCommentType comment_type2 = COMMENT_NONE;

  if (m_SyntaxType == SYNTAX_NONE || m_SyntaxType == SYNTAX_TWO)
  {
    if ((comment_type2 = CheckCommentSyntax(
      GetFileName().GetLexer().GetCommentBegin2(),
      GetFileName().GetLexer().GetCommentEnd2(), text)) == COMMENT_BEGIN)
      m_SyntaxType = SYNTAX_TWO;
  }

  wxExCommentType comment_type = COMMENT_NONE;

  switch (comment_type1)
  {
  case COMMENT_NONE:  comment_type = comment_type2; break;
  case COMMENT_BEGIN: comment_type = COMMENT_BEGIN; break;
  case COMMENT_END:   comment_type = COMMENT_END; break;
  case COMMENT_BOTH:  comment_type = COMMENT_BOTH; break;
  case COMMENT_INCOMPLETE:
    comment_type = (comment_type2 == COMMENT_NONE) ? COMMENT_INCOMPLETE: comment_type2;
    break;
  default: wxFAIL;
  }

  if (comment_type == COMMENT_END)
  {
    // E.g. we have a correct /* */ comment, with */ at the end of the line.
    // Then the end of line itself should not generate a COMMENT_END.
    if (m_SyntaxType == SYNTAX_NONE) comment_type = COMMENT_NONE;
    // Keep the syntax type.
    m_LastSyntaxType = m_SyntaxType;
    m_SyntaxType = SYNTAX_NONE;
  }

  return comment_type;
}

void wxExStreamToListView::CommentStatementEnd()
{
  m_IsCommentStatement = false;
}

void wxExStreamToListView::CommentStatementStart()
{
  m_IsCommentStatement = true;
}

bool wxExStreamToListView::Process(std::string& line, size_t line_no)
{
  if (GetTool().GetId() != ID_TOOL_REPORT_KEYWORD)
  {
    return wxExStream::Process(line, line_no);
  }
  
  bool sequence = false;
  std::string codeword;

  for (size_t i = 0; i < line.length(); i++) // no auto
  {
    if (m_IsCommentStatement)
    {
    }
    else if (line[i] == '"')
    {
      m_IsString = !m_IsString;
    }

    // Comments and codewords only appear outside strings.
    if (!m_IsString)
    {
      if (line.length() == 0) continue;

      if (i == 0) 
      {
        if (!isspace(line[0]))
        {
          codeword = line[i];
        }

        continue;
      }

      const size_t max_check_size = 
        GetFileName().GetLexer().GetCommentBegin().size();
      const size_t check_size = (i > max_check_size ? max_check_size: i + 1);

      const wxString text = line.substr(i + 1 - check_size, check_size);

      switch (CheckForComment(text))
      {
      case COMMENT_BEGIN:
        if (!m_IsCommentStatement) CommentStatementStart();
        break;

      case COMMENT_END:
        CommentStatementEnd();
        break;

      case COMMENT_BOTH:
        !m_IsCommentStatement ? CommentStatementStart(): CommentStatementEnd();
        break;

      case COMMENT_NONE:
        if (!isspace(line[i]) && !m_IsCommentStatement)
        {
          if (!wxExIsCodewordSeparator(line[i]))
          {
            if (!sequence)
            {
              sequence = true;
            }

            codeword += line[i];
          }
        }
      break;

      case COMMENT_INCOMPLETE:
        break;

      default: 
        wxFAIL;
        break;
      }

      if ( sequence && 
          (wxExIsCodewordSeparator(line[i]) || i ==0 || i == line.length() - 1))
      {
        if (GetTool().GetId() == ID_TOOL_REPORT_KEYWORD)
        {
          if (GetFileName().GetLexer().IsKeyword(codeword))
          {
            IncStatistics(codeword);
          }
        }

        sequence = false;
        codeword.clear();
      }
    }
  }

  if (CheckForComment(wxEmptyString) == COMMENT_END)
  {
    CommentStatementEnd();
  }

  return true;
}

bool wxExStreamToListView::ProcessBegin()
{
  if (GetTool().GetId() != ID_TOOL_REPORT_KEYWORD)
  {
    return wxExStream::ProcessBegin();
  }
  else
  {
    if (m_Frame == nullptr)
    {
      return false;
    }
    
    m_Report = m_Frame->Activate(
      wxExListViewWithFrame::GetTypeTool(GetTool()),
      &GetFileName().GetLexer());

    if (m_Report == nullptr)
    {
      return false;
    }
  }
  
  return true;
}

void wxExStreamToListView::ProcessEnd()
{
  if (GetTool().GetId() == ID_TOOL_REPORT_KEYWORD)
  {
    if (!GetFileName().GetLexer().GetKeywordsString().empty())
    {
      IncActionsCompleted();
    }

    wxExListItem item(m_Report, GetFileName());
    item.Insert();

    int total = 0;
    int col = 1;
    
    for (const auto& setit : GetFileName().GetLexer().GetKeywords())
    {
      const wxExStatistics<int>& stat = GetStatistics().GetElements();
      const auto& it = stat.GetItems().find(setit);
      
      if (it != stat.GetItems().end())
      {
        m_Report->SetItem(
          item.GetId(), 
          col, 
          std::to_string(it->second));
          
        total += it->second;
      }
      
      col++;
    }
    
    m_Report->SetItem(
      item.GetId(),
      col,
      std::to_string(total));
  }
}

void wxExStreamToListView::ProcessMatch(const std::string& line, size_t line_no)
{
  wxASSERT(m_Report != nullptr);

  wxExListItem item(m_Report, GetFileName());
  item.Insert();

  item.SetItem(_("Line No"), std::to_string((int)line_no + 1));

  switch (GetTool().GetId())
  {
  case ID_TOOL_REPORT_REPLACE:
    item.SetItem(_("Replaced"), wxExFindReplaceData::Get()->GetReplaceString());
    // fall through
  case ID_TOOL_REPORT_FIND:
    item.SetItem(_("Line"), wxString(line).Strip(wxString::both));
    item.SetItem(_("Match"), wxExFindReplaceData::Get()->GetFindString());
  break;

  default: wxFAIL;
  }
}

bool wxExStreamToListView::SetupTool(
  const wxExTool& tool, 
  wxExFrameWithHistory* frame,
  wxExListView* report)
{
  m_Frame = frame;

  if (report == nullptr)
  {
    if (tool.IsReportType())
    {
      if (tool.GetId() != ID_TOOL_REPORT_KEYWORD)
      {
        m_Report = m_Frame->Activate(wxExListViewWithFrame::GetTypeTool(tool));
  
        if (m_Report == nullptr)
        {
          return false;
        }
      }
    }
  }
  else
  {
    m_Report = report;
  }

  return true;
}