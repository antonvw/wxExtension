////////////////////////////////////////////////////////////////////////////////
// Name:      stc-data.h
// Purpose:   Declaration of wex::stc_data
// Author:    Anton van Wezenbeek
// Copyright: (c) 2018 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <wex/control-data.h>

namespace wex
{
  class stc;
  
  const std::string DEFAULT_TAGFILE = "tags";

  /// Offers user data to be used by stc. 
  class stc_data
  {
  public:
    /// Menu and tooltip flags.
    enum
    {
      MENU_CONTEXT   = 0, ///< context menu
      MENU_OPEN_LINK = 1, ///< for adding link open menu
      MENU_OPEN_WWW  = 2, ///< for adding search on www open menu
      MENU_VCS       = 3, ///< for adding vcs menu
      MENU_DEBUG     = 4, ///< for adding debug menu
    };

    /// Window flags.
    enum
    {
      WIN_READ_ONLY    = 0, ///< window is readonly, 
                            ///<   overrides real mode from disk
      WIN_HEX          = 1, ///< window in hex mode
      WIN_NO_INDICATOR = 2, ///< a change indicator is not used
      WIN_IS_PROJECT   = 3  ///< open as project
    };
    
    typedef std::bitset<5> menu_t;
    typedef std::bitset<4> window_t;
    
    /// Default constructor.
    stc_data(stc* stc = nullptr);

    /// Constructor from control data.
    stc_data(control_data& data, stc* stc = nullptr);

    /// Constructor from window data.
    stc_data(window_data& data, stc* stc = nullptr);

    /// Copy constructor.
    stc_data(stc* stc, const stc_data& r);

    /// Assignment operator.
    stc_data& operator=(const stc_data& r);
    
    /// Returns control data.
    auto& control() {return m_Data;};

    /// Sets control data.
    stc_data& control(control_data& data) {m_Data = data; return *this;};

    /// Returns ctags filename.
    const auto& ctags_filename() const {return m_ctags_filename;};

    /// Sets ctags filename.
    stc_data& ctags_filename(const std::string& text);

    /// Returns window flags.
    const auto& flags() const {return m_WinFlags;};
    
    /// Set window flags.
    stc_data& flags(
      window_t flags, 
      control_data::action_t action = control_data::SET);

    /// injects data.  
    bool inject() const;

    /// Returns menu flags.
    const auto& menu() const {return m_MenuFlags;};

    /// Sets menu flags.
    stc_data& menu(
      menu_t flags, 
      control_data::action_t action = control_data::SET);

    /// Returns window data.
    const auto& window() const {return m_Data.window();};

    /// Sets window data.
    stc_data& window(window_data& data) {m_Data.window(data); return *this;};
  private:  
    stc* m_STC {nullptr};

    control_data m_Data;

    std::string m_ctags_filename {DEFAULT_TAGFILE};

    menu_t m_MenuFlags {menu_t().set(
      MENU_CONTEXT).set(MENU_OPEN_LINK).set(MENU_OPEN_WWW).set(MENU_VCS)};
    window_t m_WinFlags {0};
  };
};