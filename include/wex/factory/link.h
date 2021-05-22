////////////////////////////////////////////////////////////////////////////////
// Name:      link.h
// Purpose:   Declaration of class wex::factory::link
// Author:    Anton van Wezenbeek
// Copyright: (c) 2011-2021 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <memory>
#include <string>
#include <wex/line-data.h>
#include <wex/path.h>

namespace wex
{
  namespace factory
  {
    class paths;
    class stc;

    /// Offers a class holding info about a link.
    class link
    {
    public:
      /// line number to be used for line_data
      /// Afterwards line and col from data are filled in if possible.
      enum
      {
        LINE_OPEN_URL  = -2,
        LINE_OPEN_MIME = -3,
      };

      /// Default constructor, initializes paths from config.
      link();

      /// Destructor.
      virtual ~link();

      /// Sets paths with info from config.
      /// If there is no config, paths will be empty.
      void config_get();

      /// Returns a path from text, using paths if necessary,
      /// returns empty path if no path could be found.
      const path get_path(
        /// text containing a path somewhere
        const std::string& text,
        /// data to be filled in
        line_data& data,
        /// stc component
        factory::stc* stc = nullptr) const;

    protected:
      /// Returns link pairs.
      virtual std::string get_link_pairs(const std::string& text) const
      {
        return std::string();
      };

    private:
      const path find_between(const std::string& text, factory::stc* stc) const;
      const path find_filename(const std::string& text, line_data& data) const;
      const path find_url_or_mime(
        const std::string& text,
        const line_data&   data,
        factory::stc*      stc) const;

      std::unique_ptr<paths> m_paths;
    };
  }; // namespace factory
};   // namespace wex