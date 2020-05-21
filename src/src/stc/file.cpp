////////////////////////////////////////////////////////////////////////////////
// Name:      stc/file.cpp
// Purpose:   Implementation of class wex::stc_file
// Author:    Anton van Wezenbeek
// Copyright: (c) 2020 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <thread>
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wex/defs.h>
#include <wex/filedlg.h>
#include <wex/lexers.h>
#include <wex/stc.h>
#include <wex/stcfile.h>

//#define USE_THREAD 1

#define FILE_POST(ACTION)                                                 \
  wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED, ID_EDIT_FILE_ACTION); \
  event.SetInt(ACTION);                                                   \
  wxPostEvent(m_stc, event);

namespace wex
{
#ifdef USE_THREAD
  // from wxWidgets/src/stc/scintilla/include/ILexer.h
  class ILoader
  {
  public:
    virtual int   Release()                       = 0;
    virtual int   AddData(char* data, int length) = 0;
    virtual void* ConvertToDocument()             = 0;
  };

  class loader : public ILoader
  {
  public:
    int   Release() override { return 0; };
    int   AddData(char* data, int length) override { return 0; };
    void* ConvertToDocument() override { return nullptr; };
  };
#endif
} // namespace wex

wex::stc_file::stc_file(stc* stc, const std::string& filename)
  : file(filename)
  , m_stc(stc)
{
}

bool wex::stc_file::do_file_load(bool synced)
{
  file_dialog dlg(this);

  if (get_contents_changed() && dlg.show_modal_if_changed() == wxID_CANCEL)
  {
    return false;
  }

  m_stc->use_modification_markers(false);
  m_stc->keep_event_data(synced);

  const bool hexmode =
    dlg.hexmode() || m_stc->data().flags().test(stc_data::WIN_HEX);

  const std::streampos offset =
    m_previous_size < m_stc->get_filename().stat().st_size &&
        m_stc->data().event().synced_log() ?
      m_previous_size :
      std::streampos(0);

  if (offset == std::streampos(0))
  {
    m_stc->clear();
  }

  m_previous_size = m_stc->get_filename().stat().st_size;

#ifdef USE_THREAD
  std::thread t([&] {
#endif
    if (const auto buffer(read(offset)); buffer != nullptr)
    {
      if (!m_stc->get_hexmode().is_active() && !hexmode)
      {
#ifdef USE_THREAD
        loader* load = (loader*)m_stc->CreateLoader(buffer->size());
#endif
        m_stc->append_text(*buffer);
        m_stc->DocumentStart();
      }
      else
      {
        if (!m_stc->get_hexmode().is_active())
        {
          m_stc->get_hexmode().set(true, false);
        }

        m_stc->get_hexmode().append_text(*buffer);
      }
    }
    else
    {
      m_stc->SetText("READ ERROR");
    }

    const int action =
      m_stc->data().event().synced() ? FILE_LOAD_SYNC : FILE_LOAD;
    FILE_POST(action);
#ifdef USE_THREAD
  });
  t.detach();
#endif

  return true;
}

void wex::stc_file::do_file_new()
{
  m_stc->SetName(get_filename().string());
  m_stc->properties_message();
  m_stc->clear();
  m_stc->get_lexer().set(get_filename().lexer(), true); // allow fold
}

void wex::stc_file::do_file_save(bool save_as)
{
  m_stc->SetReadOnly(true); // prevent changes during saving

  if (m_stc->get_hexmode().is_active())
  {
#ifdef USE_THREAD
    std::thread t([&] {
#endif
      if (write(m_stc->get_hexmode().buffer()))
      {
        FILE_POST(save_as ? FILE_SAVE_AS : FILE_SAVE);
      }
#ifdef USE_THREAD
    });
    t.detach();
#endif
  }
  else
  {
#ifdef USE_THREAD
    std::thread t([&] {
#endif
      if (write(m_stc->get_text()))
      {
        FILE_POST(save_as ? FILE_SAVE_AS : FILE_SAVE);
      }
#ifdef USE_THREAD
    });
    t.detach();
#endif
  }
}

bool wex::stc_file::get_contents_changed() const
{
  return m_stc->GetModify();
}

void wex::stc_file::reset_contents_changed()
{
  m_stc->SetSavePoint();
}