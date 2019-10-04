////////////////////////////////////////////////////////////////////////////////
// Name:      hexmodeline.cpp
// Purpose:   Implementation of class hexmode_line
// Author:    Anton van Wezenbeek
// Copyright: (c) 2019 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wex/stc.h>
#include "hexmodeline.h"

wex::hexmode_line::hexmode_line(hexmode* hex)
  : m_line(hex->get_stc()->GetCurLine())
  , m_lineNo(hex->get_stc()->GetCurrentLine())
  , m_columnNo(hex->get_stc()->GetColumn(hex->get_stc()->GetCurrentPos()))
  , m_Hex(hex)
  , m_StartAsciiField(hex->m_each_hex_field * hex->m_bytes_per_line)
{
  assert(m_Hex->is_active());
}  

wex::hexmode_line::hexmode_line(hexmode* hex, 
  int pos_or_offset, bool is_position)
  : m_Hex(hex)
  , m_StartAsciiField(hex->m_each_hex_field * hex->m_bytes_per_line)
{
  assert(m_Hex->is_active());
  
  if (is_position)
  {
    const int pos = (pos_or_offset != -1 ? 
      pos_or_offset: m_Hex->get_stc()->GetCurrentPos()); 

    m_columnNo = m_Hex->get_stc()->GetColumn(pos);
    m_lineNo = m_Hex->get_stc()->LineFromPosition(pos);

    if (m_columnNo >= m_StartAsciiField + (int)m_Hex->m_bytes_per_line)
    {
      m_columnNo = m_StartAsciiField;
      m_lineNo++;
    }

    m_line = m_Hex->get_stc()->GetLine(m_lineNo);
  }
  else
  {
    m_Hex->get_stc()->GotoLine(pos_or_offset >> 4);
    m_Hex->get_stc()->SelectNone();
    m_columnNo = (pos_or_offset & 0x0f);
    m_lineNo = m_Hex->get_stc()->GetCurrentLine();
    m_line = m_Hex->get_stc()->GetLine(m_lineNo);
  }
}

int wex::hexmode_line::buffer_index() const
{
  if (m_columnNo >= m_StartAsciiField + (int)m_Hex->m_bytes_per_line)
  {
    return wxSTC_INVALID_POSITION;
  }
  else if (m_columnNo >= m_StartAsciiField)
  {
    return convert(m_columnNo - m_StartAsciiField);
  }
  else if (m_columnNo >= 0)
  {
    if (m_line[m_columnNo] != ' ')
    {
      return convert(m_columnNo / m_Hex->m_each_hex_field);
    }
  }

  return wxSTC_INVALID_POSITION;
}
  
bool wex::hexmode_line::erase(int count, bool settext)
{
  const int index = buffer_index();
  
  if (is_readonly() || 
    index == wxSTC_INVALID_POSITION || 
    (size_t)index >= m_Hex->m_buffer.length()) return false;

  m_Hex->m_buffer.erase(index, count);
  
  if (settext)
  {
    m_Hex->set_text(m_Hex->m_buffer);
  }
  
  return true;
}

const std::string wex::hexmode_line::info() const
{
  if (is_hex_field())
  {
    const std::string word = m_Hex->get_stc()->get_word_at_pos(m_Hex->get_stc()->GetCurrentPos());
    
    if (!word.empty())
    {
      return std::string("index: ") + 
        std::to_string(buffer_index()) + std::string(" ") + std::to_string(std::stol(word, 0, 16));
    }
  }
  else if (is_ascii_field())
  {
    return std::string("index: ") + std::to_string(buffer_index());
  }
  
  return std::string();
}

bool wex::hexmode_line::insert(const std::string& text)
{
  const int index = buffer_index();
  
  if (is_readonly() || index == wxSTC_INVALID_POSITION) return false;
  
  if (m_columnNo >= m_StartAsciiField)
  {
    m_Hex->m_buffer.insert(index, text);
    m_Hex->set_text(m_Hex->m_buffer);

    if (m_columnNo + text.size() >= m_Hex->m_bytes_per_line + m_StartAsciiField)
    {
      int line_no = m_Hex->get_stc()->LineFromPosition(
        m_Hex->get_stc()->GetCurrentPos()) + 1;
      m_Hex->get_stc()->SetCurrentPos(
        m_Hex->get_stc()->PositionFromLine(line_no) + m_StartAsciiField);
    }
    else
    {
      m_Hex->get_stc()->SetCurrentPos(
        m_Hex->get_stc()->GetCurrentPos() + text.size());
    }

    m_Hex->get_stc()->SelectNone();
  }
  else
  {
    if (text.size() != 2 || 
       (!isxdigit(text[0]) && !isxdigit(text[1]))) return false;

    m_Hex->m_buffer.insert(index, 1, std::stoi(text.substr(0, 2), nullptr, 16));
    m_Hex->set_text(m_Hex->m_buffer);
  }

  return true;
}
  
bool wex::hexmode_line::replace(char c)
{
  const int index = buffer_index();
  
  if (is_readonly() || index == wxSTC_INVALID_POSITION) return false;
  
  const int pos = m_Hex->get_stc()->PositionFromLine(m_lineNo);
  
  // Because m_buffer is changed, begin and end undo action
  // cannot be used, as these do not operate on the hex buffer.
  
  if (is_ascii_field())
  {
    // replace ascii field with value
    m_Hex->get_stc()->wxStyledTextCtrl::Replace(
      pos + m_columnNo, pos + m_columnNo + 1, c);       

    // replace hex field with code
    char buffer[3];
    sprintf(buffer, "%02X", c);
    
    m_Hex->get_stc()->wxStyledTextCtrl::Replace(
      pos + other_field(), pos + other_field() + 2, buffer);
  }
  else if (is_hex_field())
  {
    // hex text should be entered.
    if (!isxdigit(c)) return false;
      
    // replace hex field with value
    m_Hex->get_stc()->wxStyledTextCtrl::Replace(
      pos + m_columnNo, pos + m_columnNo + 1, c);
        
    // replace ascii field with code
    std::string hex;
    
    if (m_line[m_columnNo + 1] == ' ')
    {
      hex += m_line[m_columnNo - 1];
      hex += c;
    }
    else
    {
      hex += c;
      hex += m_line[m_columnNo];
    }
    
    const int code = std::stoi(hex, nullptr, 16);
    
    m_Hex->get_stc()->wxStyledTextCtrl::Replace(
      pos + other_field(), pos + other_field() + 1, wex::printable(code, m_Hex->get_stc()));
      
    c = code;
  }
  else
  {
    return false;
  }

  m_Hex->m_buffer[index] = c;
  
  return true;
}

void wex::hexmode_line::replace(const std::string& hex, bool settext)
{
  const int index = buffer_index();
  
  if (is_readonly() || index == wxSTC_INVALID_POSITION) return;
  
  m_Hex->m_buffer[index] = std::stoi(hex, nullptr, 16);

  if (settext)
  {
    m_Hex->set_text(m_Hex->m_buffer);
  }
}
  
void wex::hexmode_line::replace_hex(int value)
{
  const int index = buffer_index();
  
  if (is_readonly() || index == wxSTC_INVALID_POSITION) return;
  
  const int pos = m_Hex->get_stc()->PositionFromLine(m_lineNo);
  
  char buffer[2];
  sprintf(buffer, "%X", value);
  
  // replace hex field with value
  m_Hex->get_stc()->wxStyledTextCtrl::Replace(
    pos + m_columnNo, pos + m_columnNo + 2, buffer);
        
  // replace ascii field with code
  m_Hex->get_stc()->wxStyledTextCtrl::Replace(
    pos + other_field(), pos + other_field() + 1, printable(value, m_Hex->get_stc()));
      
  m_Hex->m_buffer[index] = value;
}

bool wex::hexmode_line::set_pos() const
{
  if (m_lineNo < 0 || m_columnNo < 0) return false;
  
  m_Hex->get_stc()->SetFocus(); 
  m_Hex->get_stc()->SetCurrentPos(
    m_Hex->get_stc()->PositionFromLine(m_lineNo) + m_StartAsciiField + m_columnNo);
  m_Hex->get_stc()->SelectNone();
  
  return true;
}

void wex::hexmode_line::set_pos(const wxKeyEvent& event) const
{
  const int start = m_Hex->get_stc()->PositionFromLine(m_lineNo);
  const bool right = (event.GetKeyCode() == WXK_RIGHT);
  const int pos = m_Hex->get_stc()->GetCurrentPos();
  
  if (is_hex_field())
  {
    right ? 
      m_Hex->get_stc()->SetCurrentPos(pos + 3):
      m_Hex->get_stc()->SetCurrentPos(pos - 3);
      
    if (m_Hex->get_stc()->GetCurrentPos() >= start + m_StartAsciiField)
    {
      m_Hex->get_stc()->SetCurrentPos(
        m_Hex->get_stc()->PositionFromLine(m_lineNo + 1));
    }
    else if (m_Hex->get_stc()->GetCurrentPos() < start)
    {
      if (m_lineNo > 0)
      {
        m_Hex->get_stc()->SetCurrentPos(
          m_Hex->get_stc()->PositionFromLine(m_lineNo - 1) 
            + m_StartAsciiField - m_Hex->m_each_hex_field);
      }
    }
  }
  else
  {
    right ? 
      m_Hex->get_stc()->SetCurrentPos(pos + 1):
      m_Hex->get_stc()->SetCurrentPos(pos - 1);
      
    if (m_Hex->get_stc()->GetCurrentPos() >= start + m_StartAsciiField + (int)m_Hex->m_bytes_per_line)
    {
      m_Hex->get_stc()->SetCurrentPos(
        m_Hex->get_stc()->PositionFromLine(m_lineNo + 1) + m_StartAsciiField);
    }
    else if (m_Hex->get_stc()->GetCurrentPos() < start + m_StartAsciiField)
    {
      if (m_lineNo > 0)
      {
        m_Hex->get_stc()->SetCurrentPos(m_Hex->get_stc()->GetLineEndPosition(m_lineNo - 1) - 1);
      }
    }
  }
}

char wex::printable(unsigned int c, wex::stc* stc) 
{
  // We do not want control chars (\n etc.) to be printed,
  // as that disturbs the hex view field.
  if (isascii(c) && !iscntrl(c))
  {
    return c;
  }
  else
  {
    // If we already defined our own symbol, use that one,
    // otherwise print an ordinary ascii char.
    const int symbol = stc->GetControlCharSymbol();
    return symbol == 0 ?  '.': symbol;
  }
}
