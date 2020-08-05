////////////////////////////////////////////////////////////////////////////////
// Name:      notebook-data.h
// Purpose:   Declaration of wex::data::notebook
// Author:    Anton van Wezenbeek
// Copyright: (c) 2020 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/bitmap.h>

class wxWindow;

namespace wex::data
{
  /// Offers user data to be used by notebook.
  class notebook
  {
  public:
    /// Returns bitmap for the page
    const auto& bitmap() const { return m_bitmap; };

    /// Sets bitmap.
    notebook& bitmap(const wxBitmap& rhs);

    /// Returns caption for the page, if empty uses key as caption.
    const auto& caption() const { return m_caption; };

    /// Sets caption.
    notebook& caption(const std::string& rhs);

    /// Returns index fo the page.
    size_t index() const { return m_page_index; };

    /// Sets index.
    notebook& index(size_t rhs);

    /// Returns key for the page.
    const auto& key() const { return m_key; };

    /// Sets key.
    notebook& key(const std::string& rhs);

    /// Returns page to add.
    wxWindow* page() const { return m_page; };

    /// Sets page.
    notebook& page(wxWindow* rhs);

    /// Returns select the page after it is inserted.
    bool select() const { return m_select; };

    /// Sets select.
    /// Default not selected.
    notebook& select();

  private:
    wxWindow*   m_page{nullptr};
    size_t      m_page_index{0};
    std::string m_caption, m_key;
    bool        m_select{false};
    wxBitmap    m_bitmap{wxNullBitmap};
  };
}; // namespace wex::data