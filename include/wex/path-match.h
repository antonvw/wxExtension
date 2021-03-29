////////////////////////////////////////////////////////////////////////////////
// Name:      path-match.h
// Purpose:   Declaration of class wex::path_match
// Author:    Anton van Wezenbeek
// Copyright: (c) 2021 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <wex/path.h>

namespace wex
{
  /// Collects data to present a find in files match.
  class path_match
  {
  public:
    /// Constructor.
    path_match(
      /// path containing match
      const path& p,
      /// matching line
      const std::string& line,
      /// line number containing match
      size_t line_no,
      /// pos on line where match starts, -1 not known
      int pos)
      : m_line(line)
      , m_path(p)
      , m_pos(pos)
      , m_line_no(line_no)
    {
      ;
    };

    /// Returns matching line.
    auto& line() const { return m_line; };

    /// Returns matching line no.
    auto line_no() const { return m_line_no; };

    /// Returns path containing match.
    auto& path() const { return m_path; };

    /// Returns start pos of match.
    auto pos() const { return m_pos; };

  private:
    const wex::path   m_path;
    const std::string m_line;
    const int         m_pos;
    const size_t      m_line_no;
  };
}; // namespace wex
