/******************************************************************************\
* File:          app.cpp
* Purpose:       Implementation of exApp class
* Author:        Anton van Wezenbeek
* RCS-ID:        $Id$
*
* Copyright (c) 2009 Anton van Wezenbeek
* All rights are reserved. Reproduction in whole or part is prohibited
* without the written consent of the copyright owner.
\******************************************************************************/

#include <wx/dir.h>
#include <wx/stdpaths.h>
#include <wx/extension/app.h>
#include <wx/extension/stc.h>
#include <wx/extension/tool.h>

bool exApp::m_Logging = false;
wxString exApp::m_CatalogDir;
exConfig* exApp::m_Config = NULL;
exLexers* exApp::m_Lexers = NULL;
wxLocale exApp::m_Locale;
#if wxUSE_HTML & wxUSE_PRINTING_ARCHITECTURE
wxHtmlEasyPrinting* exApp::m_Printer = NULL;
#endif

int exApp::OnExit()
{
#if wxUSE_GUI
  exSTC::CleanUp();
#endif

  delete m_Lexers;
  delete m_Config;
#if wxUSE_HTML & wxUSE_PRINTING_ARCHITECTURE
  delete m_Printer;
#endif
  return wxApp::OnExit();
}

bool exApp::OnInit()
{
  // Init the localization, from now on things will be translated.
  // So do this before constructing config and exTool::Initialize, as these use localization.
  if (m_Locale.Init())
  {
    // If there are catalogs in the catalog_dir, then add them to the locale.
    wxArrayString files;

    // README: We use the canonical name, also for windows, not sure whether that is
    // the best.
    m_CatalogDir = wxStandardPaths::Get().GetLocalizedResourcesDir(
      m_Locale.GetCanonicalName(),
      // This seems to be necessarty for wxGTK. For wxMSW it isn't used.
      wxStandardPaths::ResourceCat_Messages);

    if (wxFileName::DirExists(m_CatalogDir))
    {
      wxDir::GetAllFiles(m_CatalogDir, &files);

      for (size_t i = 0 ; i < files.GetCount(); i++)
      {
        // Default the wxstd is already loaded by m_Locale.Init(),
        // so do not do it twice.
        const wxFileName fn(files.Item(i));

        if (!m_Locale.IsLoaded(fn.GetName()))
        {
          if (!m_Locale.AddCatalog(fn.GetName()))
          {
            wxLogError("Catalog could not be added: " + fn.GetName());
          }
        }
      }
    }
  }

  // Now construct the config, as most classes use it.
#ifdef EX_PORTABLE
  m_Config = new exConfig(
    wxFileName(
      wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath(),
      GetAppName() + wxString(".cfg")).GetFullPath(),
    wxCONFIG_USE_LOCAL_FILE);
#else
  // As wxStandardPaths::GetUserDataDir is used, subdir is necessary for config.
  // (ignored on non-Unix system)
  m_Config = new exConfig(
    wxEmptyString,
    wxCONFIG_USE_LOCAL_FILE | wxCONFIG_USE_SUBDIR);
#endif

  // And construct and read the lexers.
  m_Lexers = new exLexers(exFileName(
#ifdef EX_PORTABLE
      wxPathOnly(wxStandardPaths::Get().GetExecutablePath())
#else
      wxStandardPaths::Get().GetUserDataDir()
#endif
      + wxFileName::GetPathSeparator() + "lexers.xml")
    );
  m_Lexers->Read();

#if wxUSE_HTML & wxUSE_PRINTING_ARCHITECTURE
  m_Printer = new wxHtmlEasyPrinting();

  const int fontsizes[] = {4, 6, 8, 10, 12, 16, 22};
  GetPrinter()->SetFonts(wxEmptyString, wxEmptyString, fontsizes);
  GetPrinter()->GetPageSetupData()->SetMarginBottomRight(wxPoint(15, 5));
  GetPrinter()->GetPageSetupData()->SetMarginTopLeft(wxPoint(15, 5));
#endif

  // Finally call all available static initializers.
  exTool::Initialize();
  exSTC::PathListInit();

  return wxApp::OnInit();
}
