////////////////////////////////////////////////////////////////////////////////
// Name:      item.h
// Purpose:   Declaration of wex::item class
// Author:    Anton van Wezenbeek
// Copyright: (c) 2021 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <any>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <wex/data/item.h>
#include <wex/data/listview.h>
#include <wx/sizer.h> // for wxSizer, and wxSizerFlags
#include <wx/slider.h>

class wxFlexGridSizer;
class wxWindow;

namespace wex
{
template <class T> class item_template_dialog;

/*! \file */
/// Container class for using with item_dialog.
/// The next items can be set using specified data::control:
/// - id: as used by the window, see frame::on_command_item_dialog,
/// - is_required: ensures that the control must have a value otherwise OK,
///   APPLY is not enabled,
/// - style: the default value is translated to correct default
///
/// For corresponding window (such as wxFLP_DEFAULT_STYLE for FILEPICKERCTRL)
/// the style for the control used (e.g. wxTE_MULTILINE or wxTE_PASSWORD).
/// If the window supports it you can use a markup label.
class item
{
public:
  /// The item types supported.
  enum type_t
  {
    /// wxButton item
    BUTTON,

    /// wxCheckBox item
    CHECKBOX,

    /// wxCheckListBox item to set individual bits in a long
    CHECKLISTBOX_BIT,

    /// wxCheckListBox item using boolean choices
    CHECKLISTBOX_BOOL,

    /// wxColourPickerWidget item
    COLOURPICKERWIDGET,

    /// wxComboBox item
    COMBOBOX,

    /// wxComboBox item with a browse button for a directory
    COMBOBOX_DIR,

    /// wxComboBox item with a browse button for a file
    COMBOBOX_FILE,

    /// wxCommandLinkButton button
    COMMANDLINKBUTTON,

    /// wxDirPickerCtrl item
    DIRPICKERCTRL,

    /// empty item
    EMPTY,

    /// wxFilePickerCtrl item
    FILEPICKERCTRL,

    /// wxFontPickerCtrl item
    FONTPICKERCTRL,

    /// wex::grid item
    GRID,

    /// wxHyperlinkCtrl item
    HYPERLINKCTRL,

    /// wex::listview item
    LISTVIEW,

    /// wxNotebook item
    NOTEBOOK,

    /// wxAuiNotebook item
    NOTEBOOK_AUI,

    /// wxChoicebook item
    NOTEBOOK_CHOICE,

    /// wxListbook item
    NOTEBOOK_LIST,

    /// wxSimpleNotebook item
    NOTEBOOK_SIMPLE,

    /// wxToolbook item
    NOTEBOOK_TOOL,

    /// wxTreebook item
    NOTEBOOK_TREE,

    /// wex::notebook item
    NOTEBOOK_WEX,

    /// wxRadioBox item
    RADIOBOX,

    /// wxSlider item
    SLIDER,

    /// spacer item
    SPACER,

    /// wxSpinCtrl item
    SPINCTRL,

    /// wxSpinCtrlDouble item
    SPINCTRLDOUBLE,

    /// wxStaticBox item
    STATICBOX,

    /// wxStaticLine item
    STATICLINE,

    /// wxStaticText item
    STATICTEXT,

    /// wxTextCtrl item
    TEXTCTRL,

    /// wxTextCtrl item that only accepts a float (double)
    TEXTCTRL_FLOAT,

    /// wxTextCtrl item that only accepts an integer (long)
    TEXTCTRL_INT,

    /// wxToggleButton item
    TOGGLEBUTTON,

    /// provide your own window
    USER,
  };

  /// Choices for radioboxes.
  typedef std::map<
    /// value
    long,
    /// name, default the value is not set,
    /// but can be set by adding , '1' to the name
    const std::string>
    choices_t;

  /// Choices for listboxes with toggle options.
  typedef std::set<std::string> choices_bool_t;

  // A group is a pair of text with a vector of items.
  typedef std::pair<std::string, std::vector<item>> group_t;

  /// A notebook is a vector of groups.
  typedef std::vector<group_t> notebook_t;

  /// Sets dialog to parent, to allow subitems to be added
  /// to the template dialog.
  static void set_dialog(item_template_dialog<item>* dlg);

  /// Use config for getting and retrieving values.
  /// Default the config is used.
  /// The label is used as entry in the config.
  static void use_config(bool use) { m_use_config = use; }

  /// Default constructor for an EMPTY item.
  item()
    : item(EMPTY)
  {
    ;
  };

  /// Constructor for a SPACER item.
  /// The size is the size for the spacer used.
  item(int size)
    : item(SPACER)
  {
    m_data.window(data::window().style(size));
  };

  /// Constructor for a STATICLINE item.
  /// The orientation is wxHORIZONTAL or wxVERTICAL.
  item(wxOrientation orientation)
    : item(STATICLINE)
  {
    m_data.window(data::window().style(orientation));
  };

  /// Constructor from data::item.
  item(
    /// label for the window as on the dialog,
    const std::string& label,
    /// type of this item
    type_t type,
    /// item data
    const data::item& data);

  /// Constructor for several items.
  item(
    /// label for the window as on the dialog,
    /// might also contain the note after a tab for a command link button
    /// you can use a parent child config item by using a
    /// dot in the label, the prefix is not shown on the window
    const std::string& label,
    /// initial value, also used as default for a hyperlink ctrl,
    /// or as lexer for STC
    const std::string& value = std::string(),
    /// type of this item:
    /// - GRID
    /// - HYPERLINKCTRL
    /// - STATICTEXT
    /// - STC
    /// - TEXTCTRL
    /// if the label contains a colon, it is a STATICTEXT,
    /// otherwise default TEXTCTRL
    /// if type is STATICTEXT then markup is allowed in the label text
    type_t type = TEXTCTRL,
    /// item data
    const data::item& data = data::item());

  /// Constructor for a SPINCTRL or a SLIDER item.
  item(
    /// label for this item
    const std::string& label,
    /// min value
    int min,
    /// max value
    int max,
    /// default value
    const std::any& value = std::any(),
    /// type of item:
    /// - SPINCTRL
    /// - SLIDER
    type_t type = SPINCTRL,
    /// item data
    const data::item& data = data::item())
    : item(
        type,
        label,
        value,
        data::item(data)
          .window(data::window().style(wxSP_ARROW_KEYS))
          .min(min)
          .max(max))
  {
  }

  /// Constructor for a SPINCTRLDOUBLE item.
  item(
    /// label for this item
    const std::string& label,
    /// min value
    double min,
    /// max value
    double max,
    /// default value
    const std::any& value = std::any(),
    /// item data
    const data::item& data =
      data::item().window(data::window().style(wxSP_ARROW_KEYS)))
    : item(SPINCTRLDOUBLE, label, value, data::item(data).min(min).max(max))
  {
  }

  /// Constructor for a CHECKLISTBOX_BOOL item.
  /// This checklistbox can be used to get/set several boolean values.
  item(
    /// the set with names of boolean items
    /// the default value is false, but can be changed by adding a
    /// ',1' postfix to the name
    const choices_bool_t& choices,
    /// item data
    const data::item& data = data::item().label_type(data::item::LABEL_NONE))
    : item(CHECKLISTBOX_BOOL, "checklistbox_bool", choices, data)
  {
  }

  /// Constructor for a NOTEBOOK item, being a vector
  /// of a pair of pages with a vector of items.
  /// e.g.:
  /// \code
  /// wex::item("notebook", {
  ///   {"page1",
  ///     {{"string1"},
  ///      {"string2"},
  ///      {"string3"}}},
  ///   {"page2",
  ///     {{"spin1", 5, 0, 10},
  ///      {"spin2", 5, 0, 10},
  ///      {"spin3", 5, 0, 10}}}})
  /// \endcode
  item(
    /// label for this item
    const std::string& label,
    /// notebook items
    const notebook_t& v,
  /// type of this item (kind of notebook):
  /// - NOTEBOOK
  /// - NOTEBOOK_AUI
  /// - NOTEBOOK_CHOICE
  /// - NOTEBOOK_LIST
  /// - NOTEBOOK_SIMPLE
  /// - NOTEBOOK_TOOL
  /// - NOTEBOOK_TREE
  /// - NOTEBOOK_WEX
#ifdef __WXMSW__
    type_t type = NOTEBOOK_LIST,
#else
    type_t type = NOTEBOOK,
#endif
    /// item data
    const data::item& data = data::item().label_type(data::item::LABEL_NONE))
    : item(type, label, v, data)
  {
  }

  /// Constructor for a STATICBOX item.
  item(
    /// group items
    const group_t& v,
    /// item data
    const data::item& data = data::item().label_type(data::item::LABEL_NONE))
    : item(STATICBOX, v.first, v, data)
  {
  }

  /// Constructor for a RADIOBOX, or a CHECKLISTBOX_BIT item.
  /// This checklistbox (not mutually exclusive choices)
  /// can be used to get/set individual bits in a long.
  /// A radiobox (mutually exclusive choices)
  /// should be used when a long value can have a short
  /// set of possible individual values.
  item(
    /// label for this item
    const std::string& label,
    /// the map with values and text
    const choices_t& choices,
    /// indicates whether to use a radiobox or a checklistbox.
    bool use_radiobox = true,
    /// item data
    const data::item& data = data::item())
    : item(
        use_radiobox ? RADIOBOX : CHECKLISTBOX_BIT,
        label,
        choices,
        data::item(data)
          .window(data::window().style(wxRA_SPECIFY_COLS))
          .label_type(data::item::LABEL_NONE))
  {
  }

  /// Constructor for a USER item.
  item(
    /// label for this item
    const std::string& label,
    /// the window (use default constructor for it)
    wxWindow* window,
    /// remember to set callback for window creation
    const data::item& data)
    : item(USER, label, std::string(), data)
  {
    m_window = window;
  };

  /// Constructor a LISTVIEW item.
  item(
    /// label for this item
    const std::string& label,
    /// listview data
    const data::listview& data,
    /// initial value
    /// expects std::list< std::string>
    const std::any& value = std::any(),
    /// item data
    const data::item& d = data::item().label_type(data::item::LABEL_NONE))
    : item(LISTVIEW, label, value, d)
  {
    m_data_listview = data;
  };

  /// Constructor several items.
  item(
    /// label for this item
    /// if type is BUTTON then markup is allowed in the label text
    const std::string& label,
    /// type of item:
    /// - BUTTON
    /// - CHECKBOX
    /// - COLOURPICKERWIDGET
    /// - COMBOBOX
    /// - COMBOBOX_DIR
    /// - COMBOBOX_FILE
    /// - COMMANDLINKBUTTON
    /// - DIRPICKERCTRL
    /// - FILEPICKERCTRL
    /// - FONTPICKERCTRL
    /// - TEXTCTRL_FLOAT
    /// - TEXTCTRL_INT
    /// - TOGGLEBUTTON
    type_t type,
    /// initial value for the control, if appropriate:
    /// - CHECKBOX expects bool
    /// - COMBOXBOX expects std::list< std::string>
    /// - COLOURPICKERWIDGET expects a wxColour
    /// - TEXTCTRL_FLOAT expects std::string with float contents
    /// - TEXTCTRL_INT expects std::string with int contents
    const std::any& value = std::any(),
    /// item data
    const data::item& data = data::item())
    : item(
        type,
        label,
        value,
        data::item(data).label_type(
          type == BUTTON || type == CHECKBOX || type == COMMANDLINKBUTTON ||
              type == TOGGLEBUTTON ?
            data::item::LABEL_NONE :
            data.label_type()))
  {
  }

  /// If apply callback has been provided calls apply.
  /// Otherwise return false.
  bool apply(bool save = true) const
  {
    if (m_data.apply() != nullptr)
    {
      (m_data.apply())(m_window, get_value(), save);
      return true;
    }
    return false;
  }

  /// Returns item data.
  const auto& data() const { return m_data; }

  /// Returns true if this item is empty.
  bool empty() const { return m_type == EMPTY; }

  /// Returns actual value, or empty object if this item
  /// has no (or not yet) associated window, or conversion is not implemented.
  const std::any get_value() const;

  /// Returns true if this item is a notebook.
  bool is_notebook() const;

  /// Is this item allowed to be expanded on a row.
  auto is_row_growable() const { return m_is_row_growable; }

  /// Returns the label.
  const auto& label() const { return m_label; }

  /// layouts this item (creates the window) on the specified sizer.
  /// It returns the flex grid sizer that was used for creating the item
  /// sizer. Or it returns nullptr if no flex grid sizer was used.
  wxFlexGridSizer* layout(
    /// the parent
    wxWindow* parent,
    /// the sizer
    wxSizer* sizer,
    /// specify the item will be readonly, it will not be changeable
    /// if underlying control supports this
    bool readonly = false,
    /// specify the sizer for creating the item, or nullptr,
    /// than a new one is created
    wxFlexGridSizer* fgz = nullptr);

  /// Logs info about this item.
  std::stringstream log() const;

  /// Returns the page.
  const auto& page() const { return m_page; }

  /// Sets this item to be growable.
  /// Default whether the item row is growable is determined
  /// by the kind of item. You can override this using SetRowGrowable.
  void set_row_growable(bool value) { m_is_row_growable = value; }

  /// Sets actual value for the associated window.
  /// Returns false if window is nullptr, or value was not set.
  bool set_value(const std::any& value) const;

  /// Loads or saves this item to the config.
  /// Returns true if the config was accessed, as not all
  /// config items associate with the config.
  bool to_config(bool save) const;

  /// Returns the type.
  auto type() const { return m_type; }

  /// Returns the window (first call layout, to create it,
  /// otherwise it is nullptr).
  auto* window() const { return m_window; }

private:
  /// Delegate constructor.
  item(
    type_t             type,
    const std::string& label = std::string(),
    const std::any&    value = std::string(),
    const data::item&        = data::item());

  wxFlexGridSizer* add(wxSizer* sizer, wxFlexGridSizer* current) const;
  wxFlexGridSizer* add_browse_button(wxSizer* sizer) const;
  wxFlexGridSizer* add_static_text(wxSizer* sizer) const;

  void add_items(group_t& page, bool readonly);
  void add_items(
    wxWindow*          parent,
    wxFlexGridSizer*   sizer,
    std::vector<item>& v,
    bool               readonly);

  bool create_window(wxWindow* parent, bool readonly);

  bool m_is_row_growable = false;

  int m_max_items{25};

  type_t m_type;

  std::string m_label, m_label_window, m_page;

  data::item     m_data;
  data::listview m_data_listview;

  wxSizerFlags m_sizer_flags;
  wxWindow*    m_window{nullptr};

  static inline item_template_dialog<item>* m_dialog     = nullptr;
  static inline bool                        m_use_config = true;
};
}; // namespace wex
