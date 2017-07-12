////////////////////////////////////////////////////////////////////////////////
// Name:      vcs.cpp
// Purpose:   Implementation of wxExVCS class
// Author:    Anton van Wezenbeek
// Copyright: (c) 2017 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <map>
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/config.h>
#include <wx/stdpaths.h>
#include <wx/extension/vcs.h>
#include <wx/extension/itemdlg.h>
#include <wx/extension/menus.h>
#include <wx/extension/path.h>
#include <wx/extension/util.h>

// The vcs id's here can be set using the config dialog, and are not
// present in the vcs.xml. 
enum
{
  VCS_NONE = -2, // no version control
  VCS_AUTO = -1, // uses the VCS appropriate for current file
  VCS_START = 0  // no where fixed VCS start (index in vector)
};

std::vector<wxExVCSEntry> wxExVCS::m_Entries;
wxExItemDialog* wxExVCS::m_ItemDialog = nullptr;

wxExVCS::wxExVCS(const std::vector< wxExPath > & files, int command_no)
  : m_Files(files)
  , m_Title("VCS")
{
  m_Entry = FindEntry(GetFile());
  
  if (!m_Entry.GetName().empty())
  {
    if (m_Entry.SetCommand(command_no))
    {
      m_Title = m_Entry.GetName() + " " + m_Entry.GetCommand().GetCommand();
  
      if (!m_Entry.GetCommand().IsHelp() && m_Files.size() == 1)
      {
        m_Title += " " + wxExPath(m_Files[0]).GetFullName();
      }
    }
  }
}

#if wxUSE_GUI
int wxExVCS::ConfigDialog(const wxExWindowData& par) const
{
  if (m_Entries.empty())
  {
    return wxID_CANCEL;
  }
  
  wxExItem::Choices choices{{(long)VCS_NONE, _("None").c_str()}};
  
  // Using auto vcs is not useful if we only have one vcs.
  if (m_Entries.size() > 1)
  {
    choices.insert({(long)VCS_AUTO, "Auto"});
  }
  
  long i = VCS_START;

  for (const auto& it : m_Entries)
  {
    choices.insert({i++, it.GetName().c_str()});
  }

  // Estimate number of columns used by the radiobox.
  int cols = 5;

  switch (choices.size())
  {
    case 6:
    case 11:
      cols = 3;
      break;

    case 7:
    case 8:
    case 12:
    case 16:
      cols = 4;
      break;
  }
  
  const wxExWindowData data(wxExWindowData(par).
    Title(_("Set VCS").ToStdString()));

  // use a radiobox 
  std::vector<wxExItem> v{{"VCS", choices, true, cols}};

  for (const auto& it2 : m_Entries)
  {
    v.push_back({it2.GetName(), ITEM_FILEPICKERCTRL});
  }

  if (data.Button() & wxAPPLY)
  {
    wxExItemDialog* dlg = new wxExItemDialog(v, data);
    return dlg->Show();
  }
  else
  {
    return wxExItemDialog(v, data).ShowModal();
  }
}
#endif

bool wxExVCS::DirExists(const wxExPath& filename)
{
  const wxExVCSEntry entry(FindEntry(filename));

  if (
    entry.AdminDirIsTopLevel() && 
    IsAdminDirTopLevel(entry.GetAdminDir(), filename))
  {
    return true;
  }
  else 
  {
    return IsAdminDir(entry.GetAdminDir(), filename);
  }
}

bool wxExVCS::Execute()
{
  if (GetFile().Path().empty())
  {
    return m_Entry.Execute(
      m_Entry.GetCommand().IsAdd() ? wxExConfigFirstOf(_("Path")): std::string(), 
      wxExLexer(), 
      true,
      wxExConfigFirstOf(_("Base folder")));
  }
  else
  {
    const wxExPath filename(GetFile());

    std::string args;
    std::string wd;
    
    if (m_Files.size() > 1)
    {
      for (const auto& it : m_Files)
      {
        args += "\"" + it.Path().string() + "\" ";
      }
    }
    else if (m_Entry.GetName() == "git")
    {
      const std::string admin_dir(FindEntry(filename).GetAdminDir());
      wd = GetTopLevelDir(admin_dir, filename);
      
      if (!filename.GetFullName().empty())
      {
        args = GetRelativeFile(
          admin_dir, 
          filename);
      }
    }
    else
    {
      args = "\"" + filename.Path().string() + "\"";
    }
    
    return m_Entry.Execute(args, filename.GetLexer(), true, wd);
  }
}

const wxExVCSEntry wxExVCS::FindEntry(const std::string& filename) 
{
  return FindEntry(wxExPath(filename));
}

const wxExVCSEntry wxExVCS::FindEntry(const wxExPath& filename)
{
  const int vcs = wxConfigBase::Get()->ReadLong("VCS", VCS_AUTO);

  if (vcs == VCS_AUTO)
  {
    if (!filename.Path().empty())
    {
      for (const auto& it : m_Entries)
      {
        const bool toplevel = it.AdminDirIsTopLevel();
        const std::string admin_dir = it.GetAdminDir();

        if (toplevel && IsAdminDirTopLevel(admin_dir, filename))
        {
          return it;
        }
        else if (IsAdminDir(admin_dir, filename))
        {
          return it;
        }
      }
    }
  }
  else if (vcs >= VCS_START && vcs < (int)m_Entries.size())
  {
    return m_Entries[vcs];
  }
  
  return wxExVCSEntry();
}

const wxExPath wxExVCS::GetFile() const
{
  return (m_Files.empty() ? wxExConfigFirstOf(_("Base folder")): m_Files[0]);
}

const std::string wxExVCS::GetName() const
{
  switch (wxConfigBase::Get()->ReadLong("VCS", VCS_AUTO))
  {
    case VCS_NONE: return std::string(); break;
    case VCS_AUTO: return "Auto"; break;
    default: return m_Entry.GetName();
  }
}

// See IsAdminDirTopLevel
const std::string wxExVCS::GetRelativeFile(
  const std::string& admin_dir, 
  const wxExPath& fn) const
{
  // The .git dir only exists in the root, so check all components.
  wxFileName root(fn.Path().string());
  std::vector< std::string > v;

  while (root.DirExists() && root.GetDirCount() > 0)
  {
    wxFileName path(root);
    path.AppendDir(admin_dir);

    if (path.DirExists() && !path.FileExists())
    {
      std::string relative_file;

      for (int i = v.size() - 1; i >= 0; i--)
      {
        relative_file += v[i] + "/";
      }
      
      return relative_file + fn.GetFullName();
    }

    v.emplace_back(root.GetDirs().Last());
    root.RemoveLastDir();
  }
  
  return std::string();
}

// See IsAdminDirTopLevel
const std::string wxExVCS::GetTopLevelDir(
  const std::string& admin_dir, 
  const wxExPath& fn) const
{
  if (fn.Path().empty() || admin_dir.empty())
  {
    return std::string();
  }
  
  // The .git dir only exists in the root, so check all components.
  wxFileName root(fn.Path().string());

  while (root.DirExists() && root.GetDirCount() > 0)
  {
    wxFileName path(root);
    path.AppendDir(admin_dir);
    if (path.DirExists() && !path.FileExists())
    {
      return root.GetPath().ToStdString();
    }

    root.RemoveLastDir();
  }

  return std::string();
}

bool wxExVCS::IsAdminDir(
  const std::string& admin_dir, 
  const wxExPath& fn)
{
  if (admin_dir.empty() || fn.Path().empty())
  {
    return false;
  }
  
  // these cannot be combined, as AppendDir is a void (2.9.1).
  wxFileName path(fn.Path().string());
  path.AppendDir(admin_dir);
  return path.DirExists() && !path.FileExists();
}

bool wxExVCS::IsAdminDirTopLevel(
  const std::string& admin_dir, 
  const wxExPath& fn)
{
  if (fn.Path().empty() || admin_dir.empty())
  {
    return false;
  }
  
  // The .git dir only exists in the root, so check all components.
  wxFileName root(fn.Path().string());

  while (root.DirExists() && root.GetDirCount() > 0)
  {
    wxFileName path(root);
    path.AppendDir(admin_dir);

    if (path.DirExists() && !path.FileExists())
    {
      return true;
    }

    root.RemoveLastDir();
  }

  return false;
}

bool wxExVCS::LoadDocument()
{
  const int old_entries = m_Entries.size();
  
  if (!wxExMenus::Load("vcs", m_Entries)) return false;

  if (old_entries == 0)
  {
    // Add default VCS.
    if (!wxConfigBase::Get()->Exists("VCS"))
    {
      wxConfigBase::Get()->Write("VCS", (long)VCS_AUTO);
    }
  }
  else if (old_entries != m_Entries.size())
  {
    // If current number of entries differs from old one,
    // we added or removed an entry. That might give problems
    // with the vcs id stored in the config, so reset it. 
    wxConfigBase::Get()->Write("VCS", (long)VCS_AUTO);
  }
  
  return true;
}

#if wxUSE_GUI
wxStandardID wxExVCS::Request(const wxExWindowData& data)
{
  if (ShowDialog(data) == wxID_CANCEL)
  {
    return wxID_CANCEL;
  }  
  
  if (!Execute())
  {
    return wxID_CANCEL;
  }
    
  m_Entry.ShowOutput(m_Title);

  return wxID_OK;
}
#endif

bool wxExVCS::SetEntryFromBase(wxWindow* parent)
{
  if (!Use())
  {
    return false;
  }
  
  const std::string message = _("Select VCS Folder").ToStdString();
  
  // See also vcsentry, same item is used there.
  const std::vector<wxExItem> v{{
    _("Base folder"), ITEM_COMBOBOX_DIR, std::any(), wxExControlData().Required(true)}};
      
  if (wxExConfigFirstOf(_("Base folder")).empty()) 
  {
    if (
      parent != nullptr && 
      wxExItemDialog(v, wxExWindowData().
        Parent(parent).
        Title(message)).ShowModal() == wxID_CANCEL)
    {
      return false;
    }
    
    m_Entry = FindEntry(wxExConfigFirstOf(_("Base folder")));
  }
  else
  {
    m_Entry = FindEntry(wxExConfigFirstOf(_("Base folder")));
  
    if (m_Entry.GetName().empty())
    {
      if (
        parent != nullptr &&
        wxExItemDialog(v, wxExWindowData().
          Parent(parent).
          Title(message)).ShowModal() == wxID_CANCEL)
      {
        return false;
      }
      
      m_Entry = FindEntry(wxExConfigFirstOf(_("Base folder")));
    }
  }
  
  return !m_Entry.GetName().empty();
}

#if wxUSE_GUI
int wxExVCS::ShowDialog(const wxExWindowData& arg)
{
  if (m_Entry.GetCommand().GetCommand().empty())
  {
    return wxID_CANCEL;
  }
  
  wxExWindowData data(wxExWindowData(arg).Title(m_Title));
  const bool add_folder(m_Files.empty());

  if (m_Entry.GetCommand().UseFlags())
  {
    wxConfigBase::Get()->Write(
      _("Flags"), 
      wxConfigBase::Get()->Read(m_Entry.GetFlagsKey()));
  }

  if (m_ItemDialog != nullptr)
  {
    data.Pos(m_ItemDialog->GetPosition());
    data.Size(m_ItemDialog->GetSize());
    delete m_ItemDialog;
  }

  m_ItemDialog = new wxExItemDialog({
    m_Entry.GetCommand().IsCommit() ? 
      wxExItem(_("Revision comment"), ITEM_COMBOBOX, std::any(), wxExControlData().Required(true)) : wxExItem(),
    add_folder && !m_Entry.GetCommand().IsHelp() ? 
      wxExItem(_("Base folder"), ITEM_COMBOBOX_DIR, std::any(), wxExControlData().Required(true)) : wxExItem(),
    add_folder && !m_Entry.GetCommand().IsHelp() && m_Entry.GetCommand().IsAdd() ? wxExItem(
      _("Path"), ITEM_COMBOBOX, std::any(), wxExControlData().Required(true)) : wxExItem(), 
    m_Entry.GetCommand().UseFlags() ?  
      wxExItem(_("Flags"), wxEmptyString, ITEM_TEXTCTRL, wxExControlData(), LABEL_LEFT, 
        [=](wxWindow* user, const std::any& value, bool save) {
          wxConfigBase::Get()->Write(m_Entry.GetFlagsKey(), wxString(m_Entry.GetFlags()));}): wxExItem(),
    m_Entry.GetFlagsLocation() == wxExVCSEntry::VCS_FLAGS_LOCATION_PREFIX ? 
      wxExItem(_("Prefix flags"), wxEmptyString): wxExItem(),
    m_Entry.GetCommand().UseSubcommand() ? 
      wxExItem(_("Subcommand"), wxEmptyString): wxExItem()}, data);

  return (data.Button() & wxAPPLY) ? m_ItemDialog->Show(): m_ItemDialog->ShowModal();
}
#endif
  
bool wxExVCS::Use() const
{
  return wxConfigBase::Get()->ReadLong("VCS", VCS_AUTO) != VCS_NONE;
}
