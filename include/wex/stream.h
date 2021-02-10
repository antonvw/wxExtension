////////////////////////////////////////////////////////////////////////////////
// Name:      stream.h
// Purpose:   Declaration of wex::stream class
// Author:    Anton van Wezenbeek
// Copyright: (c) 2019 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <wex/path.h>
#include <wex/stream-statistics.h>
#include <wex/tool.h>

namespace wex
{
  class find_replace_data;

  /// Adds run_tool methods and statistics to a file stream.
  class stream
  {
  public:
    /// Constructor.
    stream(
      const path& filename,
      const tool& tool);
    
    /// Destructor.
    virtual ~stream() {;};

    /// Returns the filename.
    const auto & get_filename() const {return m_path;};

    /// Returns the statistics.
    const auto & get_statistics() const {return m_stats;}

    /// Returns the tool.
    const auto & get_tool() const {return m_tool;};
    
    /// Runs the tool.
    bool run_tool();

    /// Resets static members.
    static void reset();
  protected:
    /// Processes line.
    /// The default performs a ID_TOOL_REPORT_FIND or REPLACE.
    virtual bool process(
      /// contents to be processed (depends on std::getline)
      std::string& text, 
      /// line number
      size_t line_no);
    
    /// Override to do action before processing begins.
    /// The default checks correct tool for find and replace.
    virtual bool process_begin();
    
    /// Override to do action after processing has ended.
    virtual void process_end() {;};
    
    /// Override to do action for a match.
    /// Data is available in find replace data.
    virtual void process_match(
      /// matching line
      const std::string& line, 
      /// line number containing match
      size_t line_no,
      /// pos on line where match starts, -1 not known
      int pos) {;};
  protected:
    /// Increments the actions completed.
    auto inc_actions_completed(int inc_value = 1) {return
      m_stats.m_elements.inc(_("Actions Completed").ToStdString(), inc_value);};
      
    /// Increments statistics keyword.
    auto inc_statistics(const std::string& keyword) {return
      m_stats.m_elements.inc(keyword);};
  private:
    bool is_word_character(int c) const {return isalnum(c) || c == '_';};
    int replace_all(std::string& text, int* match_pos);

    const path m_path;
    const tool m_tool;
    const int m_threshold;

    stream_statistics m_stats;
    int m_prev;

    bool 
      m_modified {false}, 
      m_write {false};

    find_replace_data* m_frd;
    std::string m_find_string;
    static inline bool m_asked {false};
  };
};
