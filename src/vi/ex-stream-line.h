////////////////////////////////////////////////////////////////////////////////
// Name:      stc/ex-stream-line.cpp
// Purpose:   Implementation of class wex::ex_stream_line
// Author:    Anton van Wezenbeek
// Copyright: (c) 2020-2021 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <string.h>
#include <wex/addressrange.h>
#include <wex/data/substitute.h>
#include <wex/file.h>

namespace wex
{
class ex_stream_line
{
public:
  enum action_t
  {
    ACTION_ERASE,
    ACTION_INSERT,
    ACTION_JOIN,
    ACTION_SUBSTITUTE,
    ACTION_WRITE,
    ACTION_YANK,
  };

  enum handle_t
  {
    HANDLE_CONTINUE,
    HANDLE_ERROR,
    HANDLE_STOP,
  };

  /// Constructor for ACTION_INSERT action.
  ex_stream_line(
    file*               work,
    const addressrange& range,
    const std::string&  text);

  /// Constructor for ACTION_SUBSTITUTE action.
  ex_stream_line(
    file*                   work,
    const addressrange&     range,
    const data::substitute& data);

  /// Constructor for ACTION_YANK action.
  ex_stream_line(file* work, const addressrange& range, char name);

  /// Constructor for other action.
  ex_stream_line(file* work, action_t type, const addressrange& range);

  /// Destructor.
  ~ex_stream_line();

  /// Returns current action.
  auto action() const { return m_action; }

  /// Returns actions.
  int actions() const { return m_actions; }

  /// Handles a line.
  handle_t handle(char* line, int& pos);

  /// Returns lines.
  int lines() const { return m_line; }

private:
  const action_t         m_action;
  const data::substitute m_data;
  const std::string      m_text;
  const char             m_register{0};
  const int              m_begin, m_end;

  file* m_file;
  int   m_actions{0}, m_line{0};
};
}; // namespace wex
