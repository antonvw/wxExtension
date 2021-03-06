////////////////////////////////////////////////////////////////////////////////
// Name:      lex-rfw.h
// Purpose:   Declaration of wex::lex_rfw class
// Author:    Anton van Wezenbeek
// Copyright: (c) 2020-2021 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <stack>

#define RFW_CMD_BODY 0
#define RFW_CMD_START 1
#define RFW_CMD_WORD 2
#define RFW_CMD_TEST 3
#define RFW_CMD_ARITH 4
#define RFW_CMD_DELIM 5
#define RFW_CMD_SKW_PARTIAL 6
#define RFW_CMD_TESTCASE 7

// state constants for nested delimiter pairs, used by
// SCE_SH_STRING and SCE_SH_BACKTICKS processing
#define RFW_DELIM_LITERAL 0
#define RFW_DELIM_STRING 1
#define RFW_DELIM_CSTRING 2
#define RFW_DELIM_LSTRING 3
#define RFW_DELIM_COMMAND 4
#define RFW_DELIM_BACKTICK 5

#define RFW_BASE_ERROR 65
#define RFW_BASE_DECIMAL 66
#define RFW_BASE_HEX 67

// should be part of interface
#define SCE_SH_TESTCASE SCE_SH_HERE_DELIM // i.e. testcase in lexers.xml
#define SCE_SH_WORD2 SCE_SH_HERE_Q

namespace wex
{
  /// The robotframework lexer class.
  /// It is compiled during wxWidgets LexBash compiling,
  /// and uses c++11.
  class lex_rfw
  {
  public:
    /// Default constructor, sets accessor and line.
    lex_rfw(Accessor& styler, int line = -1)
      : m_line(line)
      , m_styler(styler)
    {
      ;
    };

    /// Returns count.
    int count() const { return m_count; };

    /// Decreases count.
    void decrease() { m_count--; };

    /// Returns down.
    int down() const { return m_down; };

    /// Performs global scan.
    int glob_scan(StyleContext& sc) const;

    /// Increases count.
    void increase() { m_count++; };

    /// Initializes.
    Sci_Position init(Sci_PositionU startPos) const;

    /// Returns whether this is a comment line.
    bool is_comment_line(int offset = 0) const;

    /// Returns whether this is a piped line.
    bool is_pipe_line(int offset = 0) const;

    /// Returns whether this is a tabbed line.
    bool is_tab_line(int offset = 0) const;

    /// Returns number base.
    int number_base(char* s) const;

    /// Returns translated digit.
    int translate_digit(int ch) const;

    /// Return up.
    int up() const { return m_up; };

  protected:
    int opposite(int ch) const;

    int m_count{0}, m_down{0}, m_up{0};

  private:
    bool get_line_pos_eol(int offset, char i) const;

    Sci_Position m_line;
    Accessor&    m_styler;
  };

  // Class to manage quote pairs (simplified vs LexPerl)
  class quote : public lex_rfw
  {
  public:
    /// Constructor.
    quote(Accessor& styler, int line = -1)
      : lex_rfw(styler, line)
    {
      ;
    };

    /// Opens.
    void open(int u);

    /// Starts.
    void start(int u);
  };

  // Class to manage quote pairs that nest
  class quote_stack : public lex_rfw
  {
  public:
    /// Constructor.
    quote_stack(Accessor& styler, int line = -1)
      : lex_rfw(styler, line)
    {
      ;
    };

    /// Returns depth.
    size_t depth() const { return m_stack.size(); };

    /// Pops.
    void pop(void);

    /// Pushes.
    void push(int u, int s);

    /// Starts.
    void start(int u, int s);

    /// Returns style.
    int style() const { return m_style; };

  private:
    int m_style{0};

    struct stack_t
    {
      int m_count;
      int m_style;
      int m_up;
    };

    std::stack<stack_t> m_stack;
  };
}; // namespace wex

// inline implementation lex_rfw

inline bool wex::lex_rfw::get_line_pos_eol(int offset, char c) const
{
  Sci_Position pos = m_styler.LineStart(m_line + offset);
  Sci_Position eol = m_styler.LineStart(m_line + 1 + offset) - 1;

  for (Sci_Position i = pos; i < eol; i++)
  {
    char ch = m_styler[i];
    if (ch == c)
      return true;
    else if (c != '\t' && ch != ' ' && ch != '\t')
      return false;
  }

  return false;
}

inline int wex::lex_rfw::glob_scan(StyleContext& sc) const
{
  // forward scan for a glob-like (...), no whitespace allowed
  int c, sLen = 0;
  while ((c = sc.GetRelativeCharacter(++sLen)) != 0)
  {
    if (IsASpace(c))
    {
      return 0;
    }
    else if (c == ')')
    {
      return sLen;
    }
  }
  
  return 0;
}

inline Sci_Position wex::lex_rfw::init(Sci_PositionU startPos) const
{
  // Always backtracks to the start of a line that is not a continuation
  // of the previous line (i.e. start of a rfw command segment)
  Sci_Position ln = m_styler.GetLine(startPos);

  if (ln > 0 && startPos == static_cast<Sci_PositionU>(m_styler.LineStart(ln)))
    ln--;

  for (;;)
  {
    startPos = m_styler.LineStart(ln);
    if (ln == 0 || m_styler.GetLineState(ln) == RFW_CMD_START)
      break;
    ln--;
  }

  return ln;
}

inline bool wex::lex_rfw::is_comment_line(int offset) const
{
  return get_line_pos_eol(offset, '#');
}

inline bool wex::lex_rfw::is_pipe_line(int offset) const
{
  return get_line_pos_eol(offset, '|');
}

inline bool wex::lex_rfw::is_tab_line(int offset) const
{
  return get_line_pos_eol(offset, '\t');
}

inline int wex::lex_rfw::number_base(char* s) const
{
  int i    = 0;
  int base = 0;
  
  while (*s)
  {
    base = base * 10 + (*s++ - '0');
    i++;
  }
  
  if (base > 64 || i > 2)
  {
    return RFW_BASE_ERROR;
  }

  return base;
}

inline int wex::lex_rfw::opposite(int ch) const
{
  switch (ch)
  {
    case '(':
      return ')';
    case '[':
      return ']';
    case '{':
      return '}';
    case '<':
      return '>';
    default:
      return ch;
  }
}

inline int wex::lex_rfw::translate_digit(int ch) const
{
  if (ch >= '0' && ch <= '9')
  {
    return ch - '0';
  }
  else if (ch >= 'a' && ch <= 'z')
  {
    return ch - 'a' + 10;
  }
  else if (ch >= 'A' && ch <= 'Z')
  {
    return ch - 'A' + 36;
  }
  else if (ch == '@')
  {
    return 62;
  }
  else if (ch == '_')
  {
    return 63;
  }
  
  return RFW_BASE_ERROR;
}

// inline implementation quote

inline void wex::quote::open(int u)
{
  m_count++;

  m_up   = u;
  m_down = opposite(m_up);
}

inline void wex::quote::start(int u)
{
  m_count = 0;
  open(u);
}

// inline implementation quote_stack

inline void wex::quote_stack::pop(void)
{
  if (m_stack.empty())
    return;

  m_stack.pop();

  m_count = m_stack.top().m_count;
  m_up    = m_stack.top().m_up;
  m_style = m_stack.top().m_style;

  m_down = opposite(m_up);
}

inline void wex::quote_stack::push(int u, int s)
{
  m_stack.push({m_count, m_style, m_up});

  m_count = 1;
  m_up    = u;
  m_style = s;

  m_down = opposite(m_up);
}

inline void wex::quote_stack::start(int u, int s)
{
  m_count = 1;

  m_up    = u;
  m_down  = opposite(m_up);
  m_style = s;
}
