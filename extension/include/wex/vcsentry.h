////////////////////////////////////////////////////////////////////////////////
// Name:      vcs_entry.h
// Purpose:   Declaration of wex::vcs_entry class
// Author:    Anton van Wezenbeek
// Copyright: (c) 2019 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <vector>
#include <pugixml.hpp>
#include <wex/lexer.h>
#include <wex/menucommands.h>
#include <wex/process.h>
#include <wex/vcscommand.h>

namespace wex
{
  class menu;

  /// This class collects a single vcs.
  class vcs_entry : public process, public menu_commands < vcs_command >
  {
  public:
    enum
    {
      FLAGS_LOCATION_POSTFIX,
      FLAGS_LOCATION_PREFIX 
    };
    
    /// Default constructor.
    vcs_entry(
      /// name of the vcs
      const std::string& name = std::string(),
      /// which dir is used for vcs admin (like .svn, .git)
      const std::string& admin_dir = std::string(),
      /// commands for this vcs,
      /// default adds empty vcs command with id 0
      const std::vector<vcs_command> & commands = std::vector<vcs_command>{vcs_command()},
      /// vcs flags
      int flags_location = FLAGS_LOCATION_POSTFIX);
    
    /// Constructor using xml node.
    vcs_entry(const pugi::xml_node& node);

    /// Returns the administrative directory.
    const auto& admin_dir() const {return m_AdminDir;};

    /// Returns true if admin dir is only at top level.
    bool admin_dir_is_toplevel() const {return m_admin_dir_is_toplevel;};

    /// Returns blame pos begin.
    const auto & blame_pos_begin() const {return m_blame_pos_begin;};

    /// Returns blame pos end.
    const auto & blame_pos_end() const {return m_blame_pos_end;};

    /// Builds a menu from all vcs commands.
    /// Returns (total) number of items in menu.
    int build_menu(
      /// menu id to be added to the vcs commands
      int base_id, 
      /// menu to be built
      menu* menu, 
      /// default assumes this is a popup menu
      bool is_popup = true) const;

    /// Executes the current vcs command (from SetCommand), or
    /// the first command if SetCommand was not yet invoked.
    /// Might ask for vcs binary if it is not yet known.
    /// Return code is code from process Execute,
    /// and also can be false if dialog for vcs bin was cancelled.
    bool execute(
      /// args, like filenames, or vcs flags
      const std::string& args = std::string(),
      /// lexer that is used for presenting the output
      const lexer& lexer = wex::lexer(),
      /// wait to finish
      process::exec_t type = process::EXEC_WAIT,
      /// working directory
      const std::string& wd = std::string());
    
    /// Returns flags location.
    auto flags_location() const {return m_FlagsLocation;};

    /// Returns the name of current branch.
    const std::string get_branch() const;

    /// Returns the flags used to run the command.
    const std::string get_flags() const;

    /// Returns margin size.
    auto margin_width() const {return m_MarginWidth;};

    virtual void show_output(const std::string& caption = std::string()) const override;
  private:
    // no const, as entry is set using operator+ in vcs.
    bool m_admin_dir_is_toplevel {false};
    int m_FlagsLocation, m_MarginWidth;
    std::string m_AdminDir, m_blame_pos_begin, m_blame_pos_end;
    lexer m_Lexer;
  };
};