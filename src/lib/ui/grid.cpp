////////////////////////////////////////////////////////////////////////////////
// Name:      grid.cpp
// Purpose:   Implementation of wex::grid class
// Author:    Anton van Wezenbeek
// Copyright: (c) 2020 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wex/defs.h>
#include <wex/frame.h>
#include <wex/frd.h>
#include <wex/grid.h>
#include <wex/lexers.h>
#include <wex/printing.h>
#include <wex/tokenizer.h>
#include <wex/util.h>
#include <wx/dnd.h>
#include <wx/fdrepdlg.h>

namespace wex
{
  // A support class for implementing drag/drop on a grid.
  class text_droptarget : public wxTextDropTarget
  {
  public:
    explicit text_droptarget(grid* grid);

  private:
    bool  OnDropText(wxCoord x, wxCoord y, const wxString& data) override;
    grid* m_grid;
  };

  text_droptarget::text_droptarget(grid* grid)
    : wxTextDropTarget()
    , m_grid(grid)
  {
  }

  bool text_droptarget::OnDropText(wxCoord x, wxCoord y, const wxString& data)
  {
    const auto row = m_grid->YToRow(y - m_grid->GetColLabelSize());
    const auto col = m_grid->XToCol(x - m_grid->GetRowLabelSize());

    if (row == wxNOT_FOUND || col == wxNOT_FOUND)
    {
      return false;
    }

    const wxGridCellCoords coord(row, col);

    if (!m_grid->is_allowed_drop_selection(coord, data))
    {
      return false;
    }

    return m_grid->drop_selection(coord, data);
  }
}; // namespace wex

wex::grid::grid(const window_data& data)
  : wxGrid(
      data.parent(),
      data.id(),
      data.pos(),
      data.size(),
      data.style(),
      data.name())
{
  SetDropTarget(new text_droptarget(this));

  lexers::get()->apply_default_style(
    [=](const std::string& back) {
      SetDefaultCellBackgroundColour(wxColour(back));
    },
    [=](const std::string& fore) {
      SetDefaultCellTextColour(wxColour(fore));
    });

  Bind(
    wxEVT_MENU,
    [=](wxCommandEvent& event) {
      empty_selection();
    },
    wxID_DELETE);

  Bind(
    wxEVT_MENU,
    [=](wxCommandEvent& event) {
      SelectAll();
    },
    wxID_SELECTALL);

  Bind(
    wxEVT_MENU,
    [=](wxCommandEvent& event) {
      ClearSelection();
    },
    ID_EDIT_SELECT_NONE);

  Bind(
    wxEVT_MENU,
    [=](wxCommandEvent& event) {
      copy_selected_cells_to_clipboard();
    },
    wxID_COPY);

  Bind(
    wxEVT_MENU,
    [=](wxCommandEvent& event) {
      copy_selected_cells_to_clipboard();
      empty_selection();
    },
    wxID_CUT);

  Bind(
    wxEVT_MENU,
    [=](wxCommandEvent& event) {
      paste_cells_from_clipboard();
    },
    wxID_PASTE);

  Bind(wxEVT_FIND, [=](wxFindDialogEvent& event) {
    find_next(
      find_replace_data::get()->get_find_string(),
      find_replace_data::get()->search_down());
  });

  Bind(wxEVT_FIND_NEXT, [=](wxFindDialogEvent& event) {
    find_next(
      find_replace_data::get()->get_find_string(),
      find_replace_data::get()->search_down());
  });

  Bind(wxEVT_GRID_CELL_LEFT_CLICK, [=](wxGridEvent& event) {
    // Removed extra check for !IsEditable(),
    // drag/drop is different from editing, so allow that.
    if (!IsSelection())
    {
      event.Skip();
      return;
    }

    if (m_use_drag_and_drop)
    {
      // This is because drag/drop is not really supported by the wxGrid.
      // Even the wxEVT_GRID_CELL_BEGIN_DRAG does not seem to come in.
      // Therefore, we are really dragging if you click again in
      // your selection and move mouse and drop elsewhere.
      // So, if not clicked in the selection, do nothing, this was no drag.
      if (!IsInSelection(event.GetRow(), event.GetCol()))
      {
        event.Skip();
        return;
      }

      // Is it allowed to drag current selection??
      if (!is_allowed_drag_selection())
      {
        event.Skip();
        return;
      }

      // Start drag operation.
      wxTextDataObject textData(get_selected_cells_value());
      wxDropSource     source(textData, this);
      wxDragResult     result = source.DoDragDrop(wxDrag_DefaultMove);

      if (
        result != wxDragError && result != wxDragNone && result != wxDragCancel)
      {
        // The old contents is not deleted, as should be by moving.
        // To fix this, do not call Skip so selection remains active,
        // and call empty_selection.
        //  event.Skip();
        empty_selection();
        ClearSelection();
      }
      else
      {
        // Do not call Skip so selection remains active.
        // event.Skip();
      }
    }
    else
    {
      event.Skip();
    }
  });

  Bind(wxEVT_GRID_CELL_RIGHT_CLICK, [=](wxGridEvent& event) {
    menu::menu_t style(menu::menu_t().set(menu::IS_POPUP));

    if (!IsEditable())
      style.set(wex::menu::IS_READ_ONLY);
    if (IsSelection())
      style.set(wex::menu::IS_SELECTED);

    wex::menu menu(style);
    build_popup_menu(menu);
    PopupMenu(&menu);
  });

  Bind(wxEVT_GRID_SELECT_CELL, [=](wxGridEvent& event) {
    frame::statustext(
      std::to_string(1 + event.GetCol()) + "," +
        std::to_string(1 + event.GetRow()),
      "PaneInfo");
    event.Skip();
  });

  Bind(wxEVT_GRID_RANGE_SELECT, [=](wxGridRangeSelectEvent& event) {
    event.Skip();
    frame::statustext(
      std::to_string(GetSelectedCells().GetCount()),
      "PaneInfo");
  });

  Bind(wxEVT_SET_FOCUS, [=](wxFocusEvent& event) {
    wex::frame* frame = dynamic_cast<wex::frame*>(wxTheApp->GetTopWindow());
    if (frame != nullptr)
    {
      frame->set_find_focus(this);
    }
    event.Skip();
  });
}

const std::string wex::grid::build_page()
{
  std::stringstream text;

  text << "<TABLE ";

  if (GridLinesEnabled())
    text << "border=1";
  else
    text << "border=0";

  text << " cellpadding=4 cellspacing=0 >\n";
  text << "<tr>\n";

  // Add the col labels only if they are shown.
  if (GetColLabelSize() > 0)
  {
    for (int c = 0; c < GetNumberCols(); c++)
    {
      text << "<td><i>" << GetColLabelValue(c) << "</i>\n";
    }
  }

  for (int i = 0; i < GetNumberRows(); i++)
  {
    text << "<tr>\n";

    for (int j = 0; j < GetNumberCols(); j++)
    {
      text << "<td>"
           << (GetCellValue(i, j).empty() ? "&nbsp" : GetCellValue(i, j))
           << "\n";
    }
  }

  text << "</TABLE>\n";

  // This can be useful for testing, paste in a file and
  // check in your browser (there indeed rules are okay).
  // clipboard_add(text);

  return text.str();
}

void wex::grid::build_popup_menu(wex::menu& menu)
{
  menu.append({{menu_item::EDIT}});
}

bool wex::grid::copy_selected_cells_to_clipboard() const
{
  wxBusyCursor wait;
  return clipboard_add(get_selected_cells_value());
}

bool wex::grid::drop_selection(
  const wxGridCellCoords& drop_coords,
  const std::string&      data)
{
  set_cells_value(drop_coords, data);

  return true;
}

void wex::grid::empty_selection()
{
  wxBusyCursor wait;

  for (int i = 0; i < GetNumberRows(); i++)
  {
    for (int j = 0; j < GetNumberCols(); j++)
    {
      if (IsInSelection(i, j) && !IsReadOnly(i, j))
      {
        set_cell_value(wxGridCellCoords(i, j), std::string());
      }
    }
  }
}

bool wex::grid::find_next(const std::string& text, bool forward)
{
  if (text.empty())
  {
    return false;
  }

  static bool recursive = false;
  static int  start_row;
  static int  end_row;
  static int  init_row;
  static int  start_col;
  static int  end_col;

  wxString text_use = text;

  if (!find_replace_data::get()->match_case())
  {
    text_use.MakeUpper();
  }

  wxGridCellCoords grid_cursor(GetGridCursorRow(), GetGridCursorCol());

  if (forward)
  {
    init_row = 0;

    if (recursive)
    {
      start_row = init_row;
      start_col = 0;
    }
    else
    {
      start_row = grid_cursor.GetRow() + 1;
      start_col = grid_cursor.GetCol();
    }

    end_row = GetNumberRows();
    end_col = GetNumberCols();
  }
  else
  {
    init_row = GetNumberRows() - 1;

    if (recursive)
    {
      start_row = init_row;
      start_col = GetNumberCols() - 1;
    }
    else
    {
      start_row = grid_cursor.GetRow() - 1;
      start_col = grid_cursor.GetCol();
    }

    end_row = -1;
    end_col = -1;
  }

  if (start_col == -1)
  {
    start_col = 0;
  }

  if (start_row == -1)
  {
    start_row = 0;
  }

  wxGridCellCoords match;

  for (int j = start_col; j != end_col && !match; (forward ? j++ : j--))
  {
    for (int i = (j == start_col ? start_row : init_row);
         i != end_row && !match;
         (forward ? i++ : i--))
    {
      wxString text = GetCellValue(i, j);

      if (!find_replace_data::get()->match_case())
      {
        text.MakeUpper();
      }

      if (find_replace_data::get()->match_word())
      {
        if (text == text_use)
        {
          match = wxGridCellCoords(i, j);
        }
      }
      else
      {
        if (text.Contains(text_use))
        {
          match = wxGridCellCoords(i, j);
        }
      }
    }
  }

  if (!match)
  {
    bool result = false;

    frame::statustext(get_find_result(text, forward, recursive), std::string());

    if (!recursive)
    {
      recursive = true;
      result    = find_next(text, forward);
      recursive = false;
    }

    return result;
  }
  else
  {
    recursive = false;
    SetGridCursor(match.GetRow(), match.GetCol());
    MakeCellVisible(match); // though docs say this isn't necessary, it is
    return true;
  }
}

const std::string wex::grid::get_cells_value() const
{
  std::stringstream text;

  for (int i = 0; i < GetNumberRows(); i++)
  {
    bool value_added = false;

    for (int j = 0; j < GetNumberCols(); j++)
    {
      if (value_added)
      {
        text << "\t";
      }

      text << GetCellValue(i, j);

      value_added = true;
    }

    if (value_added)
    {
      text << "\n";
    }
  }

  return text.str();
}

const std::string wex::grid::get_find_string() const
{
  if (IsSelection())
  {
    // This does not work (if single cell selected, array count is 0!
    // const wxGridCellCoordsArray cells(GetSelectedCells());
    tokenizer tkz(get_selected_cells_value(), "\n");

    // Only if we have one cell, so one EOL.
    if (tkz.count_tokens() == 1)
    {
      find_replace_data::get()->set_find_string(tkz.get_next_token());
    }
  }
  else
  {
    // Just take current cell value, if not empty.
    const auto        row = GetGridCursorRow();
    const auto        col = GetGridCursorCol();
    const std::string val = GetCellValue(row, col);

    if (!val.empty())
    {
      find_replace_data::get()->set_find_string(val);
    }
  }

  return find_replace_data::get()->get_find_string();
}

const std::string wex::grid::get_selected_cells_value() const
{
  // This does not work, only filled in for singly selected cells.
  // wxGridCellCoordsArray cells = GetSelectedCells();
  std::stringstream text;

  for (int i = 0; i < GetNumberRows(); i++)
  {
    bool value_added = false;

    for (int j = 0; j < GetNumberCols(); j++)
    {
      if (IsInSelection(i, j))
      {
        if (value_added)
        {
          text << "\t";
        }

        text << GetCellValue(i, j);

        value_added = true;
      }
    }

    if (value_added)
    {
      text << "\n";
    }
  }

  return text.str();
}

bool wex::grid::is_allowed_drag_selection()
{
  return true;
}

bool wex::grid::is_allowed_drop_selection(
  const wxGridCellCoords& drop_coords,
  const std::string&      data)
{
  tokenizer tkz(data, "\n");

  auto start_at_row = drop_coords.GetRow();

  while (tkz.has_more_tokens())
  {
    const auto line(tkz.get_next_token());

    tokenizer tkz(line, "\t");

    int next_col = drop_coords.GetCol();
    while (tkz.has_more_tokens() && next_col < GetNumberCols())
    {
      tkz.get_next_token(); // skip the value

      // If readonly, or this cell is part of the current selection, or outside
      // grid do not allow. Otherwise when dropping and clearing old selection
      // afterwards, we also cleared the new cells. If moving is really
      // supported by wxGrid, this might be changed.
      if (
        IsReadOnly(start_at_row, next_col) ||
        IsInSelection(start_at_row, next_col) ||
        start_at_row > GetNumberRows() || next_col > GetNumberCols())
      {
        return false;
      }

      next_col++;
    }

    start_at_row++;
  }

  return true;
}

void wex::grid::paste_cells_from_clipboard()
{
  set_cells_value(
    wxGridCellCoords(GetGridCursorRow(), GetGridCursorCol()),
    clipboard_get());
}

void wex::grid::print()
{
  wxBusyCursor wait;
  printing::get()->get_html_printer()->PrintText(build_page());
}

void wex::grid::print_preview()
{
  wxBusyCursor wait;
  printing::get()->get_html_printer()->PreviewText(build_page());
}

void wex::grid::set_cell_value(
  const wxGridCellCoords& coords,
  const std::string&      data)
{
  SetCellValue(coords, data);
}

void wex::grid::set_cells_value(
  const wxGridCellCoords& start_coords,
  const std::string&      data)
{
  tokenizer tkz(data, "\n");

  auto start_at_row = start_coords.GetRow();

  while (tkz.has_more_tokens())
  {
    const auto line(tkz.get_next_token());

    tokenizer tkz(line, "\t");

    auto next_col = start_coords.GetCol();

    while (tkz.has_more_tokens() && next_col < GetNumberCols())
    {
      const std::string value = tkz.get_next_token();

      if (!IsReadOnly(start_at_row, next_col))
      {
        set_cell_value(wxGridCellCoords(start_at_row, next_col), value);
      }

      next_col++;
    }

    start_at_row++;
  }
}

void wex::grid::use_drag_and_drop(bool use)
{
  m_use_drag_and_drop = use;
}