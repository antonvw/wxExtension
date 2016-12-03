////////////////////////////////////////////////////////////////////////////////
// Name:      stc-enums.h
// Purpose:   Declaration of enums for wxExSTC
// Author:    Anton van Wezenbeek
// Copyright: (c) 2016 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#pragma once

/// Config dialog flags.
enum wxExSTCConfigFlags
{
  STC_CONFIG_MODAL      = 0,      ///< modal dialog with all options
  STC_CONFIG_MODELESS   = 1 << 0, ///< use as modeless dialog
  STC_CONFIG_WITH_APPLY = 1 << 1, ///< add the apply button
};

/// Margin flags.
enum wxExSTCMarginFlags
{
  STC_MARGIN_NONE       = 0,      ///< no margins
  STC_MARGIN_DIVIDER    = 1 << 1, ///< divider margin
  STC_MARGIN_FOLDING    = 1 << 2, ///< folding margin
  STC_MARGIN_LINENUMBER = 1 << 3, ///< line number margin
  STC_MARGIN_ALL        = 0xFFFF, ///< all margins
};

/// Menu and tooltip flags.
enum wxExSTCMenuFlags
{
  STC_MENU_NONE      = 0,      ///< no context menu
  STC_MENU_CONTEXT   = 1 << 1, ///< context menu
  STC_MENU_OPEN_LINK = 1 << 2, ///< for adding link open menu
  STC_MENU_VCS       = 1 << 3, ///< for adding vcs menu
  STC_MENU_DEBUG     = 1 << 4, ///< for adding debug menu
};

/// Window flags.
enum wxExSTCWindowFlags
{
  STC_WIN_DEFAULT      = 0,      ///< default, not readonly, not hex mode
  STC_WIN_READ_ONLY    = 1 << 1, ///< window is readonly, 
                                 ///<   overrides real mode from disk
  STC_WIN_HEX          = 1 << 2, ///< window in hex mode
  STC_WIN_NO_INDICATOR = 1 << 3, ///< a change indicator is not used
  STC_WIN_IS_PROJECT   = 1 << 4  ///< open as project
};