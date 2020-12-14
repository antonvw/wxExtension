////////////////////////////////////////////////////////////////////////////////
// Name:      stc/config.cpp
// Purpose:   Implementation of config related methods of class wex::stc
// Author:    Anton van Wezenbeek
// Copyright: (c) 2020 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <vector>
#include <wex/beautify.h>
#include <wex/config.h>
#include <wex/core.h>
#include <wex/item-dialog.h>
#include <wex/item-vector.h>
#include <wex/lexers.h>
#include <wex/link.h>
#include <wex/stc.h>
#include <wx/settings.h>
#include <wx/stockitem.h>

namespace wex
{
  enum
  {
    INDENT_NONE,
    INDENT_WHITESPACE,
    INDENT_LEVEL,
    INDENT_ALL,
  };

  const std::string def(const wxString& v) { return std::string(v) + ",1"; }
}; // namespace wex

bool wex::stc::auto_indentation(int c)
{
  if (const auto ai =
        item_vector(m_config_items).find<long>(_("stc.Auto indent"));
      ai == INDENT_NONE)
  {
    return false;
  }

  bool is_nl = false;

  switch (GetEOLMode())
  {
    case wxSTC_EOL_CR:
      is_nl = (c == '\r');
      break;
    case wxSTC_EOL_CRLF:
      is_nl = (c == '\n');
      break; // so ignore first \r
    case wxSTC_EOL_LF:
      is_nl = (c == '\n');
      break;
  }

  const auto currentLine = GetCurrentLine();

  if (!is_nl || currentLine == 0)
  {
    return false;
  }

  const auto level  = get_fold_level();
  int        indent = 0;

  if (level <= 0)
  {
    // the current line has yet no indents, so use previous line
    indent = GetLineIndentation(currentLine - 1);

    if (indent == 0)
    {
      return false;
    }
  }
  else
  {
    indent = GetIndent() * level;
  }

  BeginUndoAction();

  SetLineIndentation(currentLine, indent);

  if (level < m_fold_level && m_adding_chars)
  {
    SetLineIndentation(currentLine - 1, indent);
  }

  EndUndoAction();

  m_fold_level = level;

  GotoPos(GetLineIndentPosition(currentLine));

  return true;
}

int wex::stc::config_dialog(const data::window& par)
{
  const data::window data(
    data::window(par).title(_("Editor Options").ToStdString()));

  if (m_config_dialog == nullptr)
  {
    m_config_dialog = new item_dialog(
      *m_config_items,
      data::window(data).title(
        data.id() == wxID_PREFERENCES ?
          wxGetStockLabel(data.id(), 0).ToStdString() :
          data.title()));
  }
  else
  {
    m_config_dialog->reload();
  }

  return (data.button() & wxAPPLY) ? m_config_dialog->Show() :
                                     m_config_dialog->ShowModal();
}

void wex::stc::config_get()
{
  const item_vector& iv(m_config_items);

  SetEdgeColumn(iv.find<int>(_("stc.Edge column")));

  if (!m_lexer.is_ok())
  {
    SetEdgeMode(wxSTC_EDGE_NONE);
  }
  else
  {
    if (const auto el = iv.find<long>(_("stc.Edge line"));
        el != wxSTC_EDGE_NONE)
    {
      const wxFont font(iv.find<wxFont>(_("stc.Default font")));
      SetEdgeMode(font.IsFixedWidth() ? el : wxSTC_EDGE_BACKGROUND);
    }
    else
    {
      SetEdgeMode(el);
    }
  }

  AnnotationSetVisible(iv.find<long>(_("stc.Annotation style")));
  AutoCompSetMaxWidth(iv.find<int>(_("stc.Autocomplete maxwidth")));
  SetCaretLineVisible(iv.find<bool>(_("stc.Caret line")));
  SetFoldFlags(iv.find<long>(_("stc.Fold flags")));
  SetIndent(iv.find<int>(_("stc.Indent")));
  SetIndentationGuides(iv.find<bool>(_("stc.Indentation guide")));
  SetMarginWidth(
    m_margin_divider_number,
    iv.find<int>(_("stc.margin.Divider")));
  SetPrintColourMode(iv.find<long>(_("stc.Print flags")));
  SetTabDrawMode(iv.find<long>(_("stc.Tab draw mode")));
  SetTabWidth(iv.find<int>(_("stc.Tab width")));
  SetUseHorizontalScrollBar(iv.find<bool>(_("stc.Scroll bars")));
  SetUseTabs(iv.find<bool>(_("stc.Use tabs")));

  if (m_data.flags().test(data::stc::WIN_EX))
  {
    SetUseVerticalScrollBar(false);
  }
  else
  {
    SetUseVerticalScrollBar(iv.find<bool>(_("stc.Scroll bars")));
  }

  SetViewEOL(iv.find<bool>(_("stc.End of line")));
  SetViewWhiteSpace(iv.find<long>(_("stc.Whitespace visible")));
  SetWrapMode(iv.find<long>(_("stc.Wrap line")));
  SetWrapVisualFlags(iv.find<long>(_("stc.Wrap visual flags")));

  if (
    GetProperty("fold") == "1" && m_lexer.is_ok() &&
    !m_lexer.scintilla_lexer().empty())
  {
    SetMarginWidth(
      m_margin_folding_number,
      iv.find<int>(_("stc.margin.Folding")));
    SetFoldFlags(iv.find<long>(_("stc.Fold flags")));
  }

  get_ex().use(iv.find<bool>(_("stc.vi mode")));

  show_line_numbers(iv.find<bool>(_("stc.Line numbers")));

  m_lexer.apply(); // at end, to prioritize local xml config
}

void wex::stc::on_exit()
{
  if (item_vector(m_config_items).find<bool>(_("stc.Keep zoom")))
  {
    config("stc.zoom").set(m_zoom);
  }

  delete m_config_items;
  delete m_link;
}

void wex::stc::on_init()
{
  m_config_items = new std::vector<item>(
    {{"stc-notebook",
      {{_("General"),
        {{"stc-subnotebook",
          {{_("Switches"),
            {{{_("stc.End of line"),
               _("stc.Line numbers"),
               _("stc.Use tabs"),
               def(_("stc.Caret line")),
               def(_("stc.Scroll bars")),
               _("stc.Auto beautify"),
               _("stc.Auto blame"),
               _("stc.Auto complete"),
               def(_("stc.Keep zoom")),
               def(_("stc.Keep zoom")),
               def(_("stc.vi mode")),
               _("stc.vi tag fullpath")}},
             {_("stc.Beautifier"), item::COMBOBOX, beautify().list()},
             {_("stc.Search engine"),
              item::COMBOBOX,
              std::list<std::string>{{"https://duckduckgo.com"}}}}},
           {_("Choices"),
            {{_("stc.Auto indent"),
              {{INDENT_NONE, _("None")},
               {INDENT_WHITESPACE, _("Whitespace")},
               {INDENT_LEVEL, _("Level")},
               {INDENT_ALL, def(_("Both"))}},
              true,
              data::item().columns(4)},
             {_("stc.Wrap visual flags"),
              {{wxSTC_WRAPVISUALFLAG_NONE, _("None")},
               {wxSTC_WRAPVISUALFLAG_END, _("End")},
               {wxSTC_WRAPVISUALFLAG_START, _("Start")},
               {wxSTC_WRAPVISUALFLAG_MARGIN, _("Margin")}},
              true,
              data::item().columns(4)},
             {_("stc.Tab draw mode"),
              {{wxSTC_TD_LONGARROW, def(_("Longarrow"))},
               {wxSTC_TD_STRIKEOUT, _("Strikeout")}},
              true,
              data::item().columns(2)},
             {_("stc.Whitespace visible"),
              {{wxSTC_WS_INVISIBLE, _("Off")},
               {wxSTC_WS_VISIBLEAFTERINDENT, _("After indent")},
               {wxSTC_WS_VISIBLEALWAYS, _("Always")},
               {wxSTC_WS_VISIBLEONLYININDENT, _("Only indent")}},
              true,
              data::item().columns(2)},
             {_("stc.Annotation style"),
              {{wxSTC_ANNOTATION_HIDDEN, _("Hidden")},
               {wxSTC_ANNOTATION_STANDARD, _("Standard")},
               {wxSTC_ANNOTATION_BOXED, def(_("Boxed"))},
               {wxSTC_ANNOTATION_INDENTED, _("indented")}},
              true,
              data::item().columns(2)},
             {_("stc.Wrap line"),
              {{wxSTC_WRAP_NONE, _("None")},
               {wxSTC_WRAP_WORD, _("Word")},
               {wxSTC_WRAP_CHAR, _("Char")},
               {wxSTC_WRAP_WHITESPACE, _("Whitespace")}},
              true,
              data::item().columns(2)}}}}}}},
       {_("Font"),
        {{_("stc.Default font"),
          item::FONTPICKERCTRL,
          wxFont(
            12,
            wxFONTFAMILY_DEFAULT,
            wxFONTSTYLE_NORMAL,
            wxFONTWEIGHT_NORMAL),
          data::item().apply(
            [=](wxWindow* user, const std::any& value, bool save) {
              // Doing this once is enough, not yet possible.
              lexers::get()->load_document();
            })},
         {_("stc.Text font"),
          item::FONTPICKERCTRL,
          wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT)}}},
       {_("Edge"),
        {{_("stc.Edge column"), 0, 500, 80},
         {_("stc.Edge line"),
          {{wxSTC_EDGE_NONE, _("None")},
           {wxSTC_EDGE_LINE, _("Line")},
           {wxSTC_EDGE_BACKGROUND, _("Background")},
           {wxSTC_EDGE_MULTILINE, _("Multiline")}},
          true,
          data::item().columns(1)}}},
       {_("Margin"),
        {{_("<i>Margins:</i>")},
         {_("stc.Autocomplete maxwidth"), 0, 100, 0},
         {_("stc.Indent"), 0, 500, 2},
         {_("stc.Tab width"), 1, 500, 2},
         {_("<i>Line Margins:</i>")},
         {_("stc.margin.Divider"), 0, 40, 16},
         {_("stc.margin.Folding"), 0, 40, 16},
         {_("stc.margin.Line number"), 0, 100, 60},
         {_("stc.margin.Text"), -1, 500, -1}}},
       {_("Folding"),
        {{_("stc.Indentation guide"), item::CHECKBOX},
         {_("stc.Auto fold"), 0, INT_MAX, 1500},
         {_("stc.Fold flags"),
          {{wxSTC_FOLDFLAG_LINEBEFORE_EXPANDED, _("Line before expanded")},
           {wxSTC_FOLDFLAG_LINEBEFORE_CONTRACTED,
            def(_("Line before contracted"))},
           {wxSTC_FOLDFLAG_LINEAFTER_EXPANDED, _("Line after expanded")},
           {wxSTC_FOLDFLAG_LINEAFTER_CONTRACTED,
            def(_("Line after contracted"))},
           {wxSTC_FOLDFLAG_LEVELNUMBERS, _("Level numbers")}},
          false}}},
       {_("Linking"),
        {{_("<i>Includes:</i>")},
         {_("stc.link.Include directory"),
          data::listview()
            .type(data::listview::FOLDER)
            .window(data::window().size({200, 200})),
          std::any(),
          data::item()
            .label_type(data::item::LABEL_NONE)
            .apply([=](wxWindow* user, const std::any& value, bool save) {
              m_link->config_get();
            })},
         {_("<i>Matches:</i>")},
         {_("stc.link.Pairs"),
          data::listview()
            .type(data::listview::TSV)
            .window(data::window().size({200, 200})),
          // First try to find "..", then <..>, as in next example:
          // <A HREF="http://www.scintilla.org">scintilla</A> component.
          std::list<std::string>({"\"\t\"", "<\t>", "[\t]", "'\t'", "{\t}"})}}},
       {_("Printer"),
        {{_("stc.Print flags"),
          {{wxSTC_PRINT_NORMAL, _("Normal")},
           {wxSTC_PRINT_INVERTLIGHT, _("Invert on white")},
           {wxSTC_PRINT_BLACKONWHITE, def(_("Black on white"))},
           {wxSTC_PRINT_COLOURONWHITE, _("Colour on white")},
           {wxSTC_PRINT_COLOURONWHITEDEFAULTBG, _("Colour on white normal")}},
          true,
          data::item().columns(1)}}}}}});

  if (item_vector(m_config_items).find<bool>(_("stc.Keep zoom")))
  {
    m_zoom = config("stc.zoom").get(-1);
  }

  m_link = new link();
}
