////////////////////////////////////////////////////////////////////////////////
// Name:      stc/ex-stream.cpp
// Purpose:   Implementation of class wex::ex_stream
// Author:    Anton van Wezenbeek
// Copyright: (c) 2021 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <regex>
#include <stdio.h>
#include <string.h>
#include <wex/addressrange.h>
#include <wex/core.h>
#include <wex/ex-stream.h>
#include <wex/ex.h>
#include <wex/factory/stc.h>
#include <wex/frame.h>
#include <wex/frd.h>
#include <wex/log.h>
#include <wex/temp-filename.h>

#include "ex-stream-line.h"

#define STREAM_LINE_ON_CHAR()                                            \
  {                                                                      \
    if (!range.is_ok())                                                  \
    {                                                                    \
      return false;                                                      \
    }                                                                    \
                                                                         \
    m_stream->seekg(0);                                                  \
                                                                         \
    int  i = 0;                                                          \
    char c;                                                              \
                                                                         \
    while (m_stream->get(c))                                             \
    {                                                                    \
      m_current_line[i++] = c;                                           \
                                                                         \
      if (c == '\n' || i == m_current_line_size)                         \
      {                                                                  \
        sl.handle(m_current_line, i);                                    \
      }                                                                  \
    }                                                                    \
                                                                         \
    sl.handle(m_current_line, i);                                        \
                                                                         \
    m_stream->clear();                                                   \
                                                                         \
    if (                                                                 \
      sl.actions() > 0 && sl.action() != ex_stream_line::ACTION_WRITE && \
      !copy(m_temp, m_work))                                             \
    {                                                                    \
      return false;                                                      \
    }                                                                    \
  }

const int default_line_size = 500;

wex::ex_stream::ex_stream(wex::ex* ex)
  : m_context_lines(50)
  , m_buffer_size(1000000)
  , m_buffer(new char[m_buffer_size])
  , m_current_line_size(default_line_size)
  , m_current_line(new char[m_current_line_size])
  , m_ex(ex)
  , m_stc(ex->get_stc())
{
}

wex::ex_stream::~ex_stream()
{
  delete[] m_buffer;
  delete[] m_current_line;

  delete m_temp;
  delete m_work;
}

bool wex::ex_stream::copy(file* from, file* to)
{
  if (from->stream().bad() || to->stream().bad())
  {
    log("ex stream copy") << from->stream().bad() << to->stream().bad();
    return false;
  }

  to->close();
  to->open(std::ios_base::out);

  from->close();
  from->open(std::ios_base::in);

  to->stream() << from->stream().rdbuf();

  to->close();
  to->open();

  from->close();
  std::remove(from->get_filename().string().c_str());
  from->open(std::ios_base::out);

  m_stream      = &to->stream();
  m_is_modified = true;

  return true;
}

bool wex::ex_stream::erase(const addressrange& range)
{
  if (m_stream == nullptr)
  {
    return false;
  }

  ex_stream_line sl(ex_stream_line::ACTION_ERASE, m_temp, range);

  STREAM_LINE_ON_CHAR();

  m_last_line_no = sl.lines() - sl.actions() - 1;

  m_ex->frame()->show_ex_message(std::to_string(sl.actions()) + " fewer lines");

  goto_line(0);

  return true;
}

bool wex::ex_stream::find(
  const std::string& text,
  int                find_flags,
  bool               find_next)
{
  if (m_stream == nullptr)
  {
    return false;
  }

  const auto       line_no = m_line_no;
  const auto       pos     = m_stream->tellg();
  const std::regex r(text);
  const bool       use_regex(find_replace_data::get()->is_regex());
  bool             found = false;

  while (!found && ((find_next && get_next_line()) ||
                    (!find_next && get_previous_line())))
  {
    if (
      (!use_regex && strstr(m_current_line, text.c_str()) != nullptr) ||
      (use_regex && std::regex_search(m_current_line, r)))
    {
      found = true;
    }
  }

  m_current_line_size = default_line_size;

  if (!found)
  {
    m_line_no = line_no;
    m_stream->clear();
    m_stream->seekg(pos);

    m_ex->frame()->statustext(get_find_result(text, true, true), std::string());
  }
  else
  {
    log::trace("ex stream found") << text << m_line_no;
    set_text();
  }

  return found;
}

int wex::ex_stream::get_current_line() const
{
  return m_line_no;
}

int wex::ex_stream::get_line_count() const
{
  return m_last_line_no;
}

int wex::ex_stream::get_line_count_request()
{
  if (m_stream == nullptr)
  {
    return LINE_COUNT_UNKNOWN;
  }

  const auto pos = m_stream->tellg();
  m_stream->clear();
  m_stream->seekg(0);

  int line_no = 0;

  while (m_stream->read(m_buffer, m_buffer_size))
  {
    const int count = m_stream->gcount();
    for (int i = 0; i < count; i++)
    {
      if (m_buffer[i] == '\n')
        line_no++;
    }
  }

  const int count = m_stream->gcount();
  for (int i = 0; i < count; i++)
  {
    if (m_buffer[i] == '\n')
      line_no++;
  }

  m_last_line_no = line_no;

  m_stream->clear();
  m_stream->seekg(pos);

  return m_last_line_no;
}

bool wex::ex_stream::get_next_line()
{
  if (!m_stream->getline(m_current_line, m_current_line_size))
  {
    if (m_stream->eof())
    {
      m_stream->clear();

      m_last_line_no = m_line_no;

      return false;
    }
    else
    {
      m_stream->clear();
      m_block_mode = true;
    }
  }

  m_line_no++;

  return true;
}

bool wex::ex_stream::get_previous_line()
{
  auto pos(m_stream->tellg());

  if ((int)pos - (int)m_current_line_size > 0)
  {
    m_stream->seekg((size_t)pos - m_current_line_size);
    pos = m_stream->tellg();
  }
  else
  {
    pos = 0;
    m_stream->seekg(0);
  }

  m_stream->read(m_buffer, m_current_line_size);

  if (m_stream->gcount() > 0)
  {
    // We have filled the m_buffer, now from end of m_buffer search
    // backwards for newline, this is the m_current_line to handle, and set the
    // stream pointer to position before that newline.
    for (int i = m_stream->gcount() - 1; i >= 0; i--)
    {
      if (m_buffer[i] == '\n')
      {
        const size_t sz(m_stream->gcount() - i - 1);
        strncpy(m_current_line, m_buffer + i + 1, sz);
        m_current_line[sz] = 0;

        m_stream->clear();

        if (pos == 0)
        {
          m_current_line_size = i - 1;
          m_stream->clear();
          m_stream->seekg(0);
        }
        else
        {
          m_stream->seekg((size_t)pos + i - 1);
        }

        m_line_no--;

        return true;
      }
    }

    // There was no newline, this implies block mode.
    strncpy(m_current_line, m_buffer, m_stream->gcount());
    m_current_line[m_stream->gcount()] = 0;
    m_stream->clear();
    m_stream->seekg((size_t)pos);

    if (m_line_no > 0)
    {
      m_line_no--;
    }

    m_block_mode = true;

    return m_stream->gcount() > m_current_line_size - 1;
  }

  return false;
}

void wex::ex_stream::goto_line(int no)
{
  if (m_stream == nullptr || no == m_line_no)
  {
    return;
  }

  log::trace("ex stream goto_line") << no;

  if (no == 0 || (no < 100 && no < m_line_no) || no < m_line_no - 1000)
  {
    m_line_no = LINE_COUNT_UNKNOWN;
    m_stream->clear();
    m_stream->seekg(0);

    m_stc->SetReadOnly(false);
    m_stc->ClearAll();
    m_stc->SetReadOnly(true);

    while ((m_line_no < no) && get_next_line())
      ;
  }
  else if (no < m_line_no)
  {
    while ((no < m_line_no) && get_previous_line())
      ;
  }
  else
  {
    while ((no > m_line_no) && get_next_line())
      ;
  }

  set_text();
}

bool wex::ex_stream::insert_text(
  const address&     a,
  const std::string& text,
  loc_t              loc)
{
  if (m_stream == nullptr)
  {
    return false;
  }

  const auto line(loc == INSERT_BEFORE ? a.get_line() : a.get_line() + 1);
  const addressrange range(
    m_ex,
    std::to_string(line) + "," + std::to_string(line + 1));

  ex_stream_line sl(m_temp, range, text);

  STREAM_LINE_ON_CHAR();

  goto_line(line);

  return true;
}

bool wex::ex_stream::join(const addressrange& range)
{
  if (m_stream == nullptr)
  {
    return false;
  }

  ex_stream_line sl(ex_stream_line::ACTION_JOIN, m_temp, range);

  STREAM_LINE_ON_CHAR();

  m_last_line_no = sl.lines() - sl.actions() - 1;

  m_ex->frame()->show_ex_message(std::to_string(sl.actions()) + " fewer lines");

  goto_line(range.get_begin().get_line() - 1);

  return true;
}

bool wex::ex_stream::marker_add(char marker, int line)
{
  if (!isascii(marker))
  {
    return false;
  }

  m_markers[marker] = line;

  return true;
}

bool wex::ex_stream::marker_delete(char marker)
{
  if (const auto& it = m_markers.find(marker); it != m_markers.end())
  {
    m_markers.erase(marker);
    return true;
  }

  return false;
}

int wex::ex_stream::marker_line(char marker) const
{
  if (const auto& it = m_markers.find(marker); it != m_markers.end())
  {
    return it->second;
  }

  return -1;
}

void wex::ex_stream::set_text()
{
  m_stc->SetReadOnly(false);

  m_stc->AppendText(m_current_line);
  m_stc->AppendText("\n");

  if (m_stc->GetLineCount() > m_context_lines)
  {
    m_stc->DeleteRange(0, m_stc->PositionFromLine(1));
  }

  m_stc->DocumentEnd();
  m_stc->EmptyUndoBuffer();
  m_stc->SetSavePoint();
  m_stc->SetReadOnly(true);
  m_stc->use_modification_markers(false);
}

void wex::ex_stream::stream(file& f)
{
  if (!f.is_open())
  {
    log("file is not open") << f.get_filename();
    return;
  }

  m_file   = &f;
  m_stream = &f.stream();
  f.use_stream();

  m_temp = new file(temp_filename().name(), std::ios_base::out);
  m_temp->use_stream();

  m_work = new file(temp_filename().name(), std::ios_base::out);
  m_work->use_stream();

  goto_line(0);
}

bool wex::ex_stream::substitute(
  const addressrange&     range,
  const data::substitute& data)
{
  if (m_stream == nullptr)
  {
    return false;
  }

  ex_stream_line sl(m_temp, range, data);

  STREAM_LINE_ON_CHAR();

  m_ex->frame()->show_ex_message(
    "Replaced: " + std::to_string(sl.actions()) +
    " occurrences of: " + data.pattern());

  goto_line(0);

  return true;
}

bool wex::ex_stream::write()
{
  log::trace("ex stream write");

  if (!copy(m_work, m_file))
  {
    return false;
  }

  m_is_modified = false;

  return true;
}

bool wex::ex_stream::write(
  const addressrange& range,
  const std::string&  filename,
  bool                append)
{
  if (m_stream == nullptr)
  {
    return false;
  }

  wex::file file(
    filename,
    append ? std::ios::out | std::ios_base::app : std::ios::out);

  ex_stream_line sl(ex_stream_line::ACTION_WRITE, &file, range);

  STREAM_LINE_ON_CHAR();

  return true;
}
