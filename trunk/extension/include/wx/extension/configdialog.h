/******************************************************************************\
* File:          configdialog.h
* Purpose:       Declaration of wxWidgets config dialog classes
* Author:        Anton van Wezenbeek
* RCS-ID:        $Id$
*
* Copyright (c) 1998-2009, Anton van Wezenbeek
* All rights are reserved. Reproduction in whole or part is prohibited
* without the written consent of the copyright owner.
\******************************************************************************/

#ifndef _EXCONFIGDIALOG_H
#define _EXCONFIGDIALOG_H

#include <map>
#include <set>
#include <vector>
#include <wx/extension/base.h>
#include <wx/extension/config.h>

#if wxUSE_GUI
/*! \file */
/// The config item types supported.
enum
{
  /// A checklistbox (not mutually exclusive choices).
  /// Should be used to get/set individual bits in a long.
  CONFIG_CHECKLISTBOX,   

  /// A checklistbox without a name (not mutually exclusive choices).
  /// Should be used to get/set several boolean values in one checklistbox.
  CONFIG_CHECKLISTBOX_NONAME,

  /// A radiobox (mutually exclusive choices).
  /// Should be used when a long value can have a short set of possible individual values.
  CONFIG_RADIOBOX,       

  CONFIG_CHECKBOX,       ///< a checkbox (use GetBool to retrieve value)
  CONFIG_COLOUR,         ///< a colour button
  CONFIG_COMBOBOX,       ///< a combobox
  CONFIG_COMBOBOXDIR,    ///< a combobox with a browse button
  CONFIG_DIRPICKERCTRL,  ///< a dirpicker ctrl
  CONFIG_FILEPICKERCTRL, ///< a filepicker ctrl
  CONFIG_FONTPICKERCTRL, ///< a fontpicker ctrl
  CONFIG_INT,            ///< a textctrl that only accepts an integer (a long integer)
  CONFIG_SPINCTRL,       ///< a spinctrl
  CONFIG_SPINCTRL_DOUBLE, ///< a spinctrl double
  CONFIG_SPACER,         ///< a spacer only, no config item
  CONFIG_STRING,         ///< a textctrl
};

/// Container class for using with exConfigDialog.
/// If you specify a page, then all items are placed on that page in a notebook.
class exConfigItem
{
  friend class exConfigDialog;
public:
  /// Contructor for a spacer item.
  exConfigItem()
  : m_Name("spacer")
  , m_Page(wxEmptyString)
  , m_Type(CONFIG_SPACER) {;}
  
  /// Constructor for a spin ctrl.
  exConfigItem(const wxString& name,
    int min,
    int max,
    const wxString& page = wxEmptyString)
  : m_IsRequired(false)
  , m_Min(min)
  , m_Max(max)
  , m_MaxItems(0)
  , m_Name(name)
  , m_Page(page)
  , m_Style(0)
  , m_Type(CONFIG_SPINCTRL) {;};

  /// Constructor for a spin ctrl double.
  exConfigItem(const wxString& name,
    double min,
    double max,
    double inc = 1,
    const wxString& page = wxEmptyString)
  : m_IsRequired(false)
  , m_MaxItems(0)
  , m_MinDouble(min)
  , m_MaxDouble(max)
  , m_Inc(inc)
  , m_Name(name)
  , m_Page(page)
  , m_Style(0)
  , m_Type(CONFIG_SPINCTRL_DOUBLE) {;};

  /// Constructor for a string.
  /// The extra style argument is the style for the wxTextCtrl used.
  /// (e.g. wxTE_MULTILINE or wxTE_PASSWORD)
  exConfigItem(const wxString& name,
    const wxString& page = wxEmptyString,
    long style = 0,
    bool is_required = false)
  : m_IsRequired(is_required)
  , m_Min(0)
  , m_Max(0)
  , m_MaxItems(0)
  , m_Name(name)
  , m_Page(page)
  , m_Style(style)
  , m_Type(CONFIG_STRING) {;};

  /// Constructor for a radiobox or a checklistbox. Just specify
  /// the map with values and text.
  exConfigItem(const wxString& name,
    const std::map<long, const wxString> & choices,
    bool use_radiobox = true,
    const wxString& page = wxEmptyString)
  : m_IsRequired(false)
  , m_Min(0)
  , m_Max(0)
  , m_MaxItems(0)
  , m_Name(name)
  , m_Page(page)
  , m_Style(0)
  , m_Type(use_radiobox ? CONFIG_RADIOBOX: CONFIG_CHECKLISTBOX)
  , m_Choices(choices) {;};

  /// Constructor for a checklistbox without a name. Just specify
  /// the map with values and text.
  exConfigItem(const std::set<wxString> & choices,
    const wxString& page = wxEmptyString)
  : m_IsRequired(false)
  , m_Min(0)
  , m_Max(0)
  , m_MaxItems(0)
  , m_Name("checklistbox_noname")
  , m_Page(page)
  , m_Style(0)
  , m_Type(CONFIG_CHECKLISTBOX_NONAME)
  , m_ChoicesBool(choices) {;};

  /// Constuctor for other types.
  exConfigItem(const wxString& name,
    int type,
    const wxString& page = wxEmptyString,
    bool is_required = false,
    int max_items = 25) // used by CONFIG_COMBOBOX
  : m_IsRequired(is_required)
  , m_Min(0)
  , m_Max(0)
  , m_MaxItems(max_items)
  , m_Name(name)
  , m_Page(page)
  , m_Style(0)
  , m_Type(type) {;};
private:
  // cannot be const, otherwise
  // error C2582: 'operator =' function is unavailable in 'exConfigItem'
  bool m_IsRequired;
  int m_Min;
  int m_Max;
  int m_MaxItems;
  double m_MinDouble;
  double m_MaxDouble;
  double m_Inc;
  wxString m_Name;
  wxString m_Page;
  long m_Style;
  int m_Type;
  wxControl* m_Control;
  std::map<long, const wxString> m_Choices;
  std::set<wxString> m_ChoicesBool;
};
#endif // wxUSE_GUI

#if wxUSE_GUI
/// Offers a dialog to set several items in the config.
/// You can also use the dialog modeless (then you can use wxAPPLY
/// to store the items in the config).
/// When pressing the apply button ConfigDialogApplied is invoked from exFrame.
/// If you only specify a wxCANCEL button, the dialog is readonly.
class exConfigDialog: public exDialog
{
public:
  /// Constructor, specify the vector of config items
  /// to be used. When wxOK or wxAPPLY is pressed, any change in one of the
  /// config items is saved in the config.
  /// \todo The dialog does not set it's window size correctly when
  /// notebooks are used, you have to specify size yourself.
  exConfigDialog(wxWindow* parent,
    exConfig* config,
    std::vector<exConfigItem> v,
    const wxString& title = _("Options"),
    const wxString& configGroup = wxEmptyString,
    int rows = 0,
    int cols = 2,
    long flags = wxOK | wxCANCEL,
    wxWindowID id = wxID_ANY,
    const wxPoint& pos = wxDefaultPosition,
    const wxSize& size = wxDefaultSize,
    long style = wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
protected:
  void OnCommand(wxCommandEvent& event);
  void OnUpdateUI(wxUpdateUIEvent& event);
private:
  wxControl* Add(
    wxSizer* sizer,
    wxWindow* parent,
    wxControl* control,
    const wxString& text,
    bool expand = true);
  wxControl* AddCheckBox(
    wxWindow* parent,
    wxSizer* sizer,
    const wxString& text);
  wxControl* AddCheckListBox(
    wxWindow* parent,
    wxSizer* sizer,
    const wxString& text,
    std::map<long, const wxString> & choices);
  wxControl* AddCheckListBoxNoName(
    wxWindow* parent,
    wxSizer* sizer,
    std::set<wxString> & choices);
  wxControl* AddColourButton(
    wxWindow* parent,
    wxSizer* sizer,
    const wxString& text);
  wxControl* AddComboBox(
    wxWindow* parent,
    wxSizer* sizer,
    const wxString& text);
  wxControl* AddComboBoxDir(
    wxWindow* parent,
    wxSizer* sizer,
    const wxString& text);
  wxControl* AddDirPickerCtrl(
    wxWindow* parent,
    wxSizer* sizer,
    const wxString& text);
  wxControl* AddFilePickerCtrl(
    wxWindow* parent,
    wxSizer* sizer,
    const wxString& text);
  wxControl* AddFontPickerCtrlCtrl(
    wxWindow* parent,
    wxSizer* sizer,
    const wxString& text);
  wxControl* AddRadioBox(
    wxWindow* parent,
    wxSizer* sizer,
    const wxString& text,
    std::map<long, const wxString> & choices);
  wxControl* AddSpinCtrl(
    wxWindow* parent,
    wxSizer* sizer,
    const wxString& text,
    int min,
    int max);
  wxControl* AddSpinCtrlDouble(
    wxWindow* parent,
    wxSizer* sizer,
    const wxString& text,
    double min,
    double max,
    double inc);
  wxControl* AddTextCtrl(
    wxWindow* parent,
    wxSizer* sizer,
    const wxString& text,
    bool is_numeric = false,
    long style = 0);

  exConfig* m_Config;
  const wxString m_ConfigGroup;
  std::vector<exConfigItem> m_ConfigItems;

  DECLARE_EVENT_TABLE()
};
#endif // wxUSE_GUI
#endif
