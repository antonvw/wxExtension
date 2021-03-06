////////////////////////////////////////////////////////////////////////////////
// Name:      version-dialog.h
// Purpose:   Declaration of class wex::version_info_dialog
// Author:    Anton van Wezenbeek
// Copyright: (c) 2021 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <string>
#include <wex/version.h>
#include <wx/aboutdlg.h>

namespace wex
{
class frame;

class about_info : public wxAboutDialogInfo
{
public:
  /// Constructor.
  about_info() { ; }

  /// Sets description.
  about_info& description(const std::string& rhs);

  /// Adds a developer.
  about_info& developer(const std::string& rhs);

  /// Sets icon.
  about_info& icon(const wxIcon& rhs);

  /// Sets licence.
  about_info& licence(const std::string& rhs);

  /// Sets website.
  about_info& website(const std::string& rhs);
};

/// This class offers a dialog to present version info.
class version_info_dialog
{
public:
  /// Default constructor, uses wex::get_version_info.
  version_info_dialog(
    /// about info
    const about_info& about = about_info());

  /// Constructor, for other version_info.
  version_info_dialog(
    /// version info
    const version_info& info,
    /// about info
    const about_info& about = about_info());

  /// Returns the about info.
  const auto& about() const { return m_about; }

  /// Shows the dialog.
  void show();

private:
  about_info m_about;
};
}; // namespace wex
