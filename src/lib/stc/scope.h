////////////////////////////////////////////////////////////////////////////////
// Name:      scope.h
// Purpose:   Implementation of class wex::scope
// Author:    Anton van Wezenbeek
// Copyright: (c) 2020 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <bitset>
#include <map>
#include <string>
#include <vector>

#include <wex/ctags-entry.h>

namespace wex
{
  class stc;

  /// This class offers scope functionality for maintaining filters
  /// during stc auto completion.
  class scope
  {
  public:
    /// A map of variable name with ctags_entry.
    typedef std::map<std::string, ctags_entry> map_t;

    /// Constructor.
    scope(stc* s);

    /// Returns true if iterator is at end of filters.
    bool end() const;

    /// Finds text in scope (from current up), returns true if found.
    /// Sets the iterator if found.
    bool find(const std::string& text);

    /// Returns active filter entry for text, might add entry
    /// if text is not yet present.
    ctags_entry& get(const std::string& text);

    /// Inserts entry in the filters map, and sets iterator.
    void insert(const std::string& text, const ctags_entry& ce);

    /// Returns the iterator to the map.
    auto& iter() const { return m_it; };

    /// Synchronizes scope filters with current level
    /// in current position stc.
    void sync();

  private:
    /// Check level flags.
    enum
    {
      LEVEL_DOWN = 0, ///< level down flag
      LEVEL_UP   = 1, ///< level up flag
    };

    typedef std::bitset<2> check_t;

    bool check_levels(check_t type = check_t().set());

    size_t get_current_level() const;

    stc*                  m_stc;
    std::vector<map_t>    m_filters;
    map_t::const_iterator m_it;
    size_t                m_level;
  };
}; // namespace wex