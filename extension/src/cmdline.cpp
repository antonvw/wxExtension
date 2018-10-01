////////////////////////////////////////////////////////////////////////////////
// Name:      cmdline.cpp
// Purpose:   Implementation of wxExCmdLine class
// Author:    Anton van Wezenbeek
// Copyright: (c) 2018 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <sstream> // for tclap!
#include <variant>
#include <tclap/CmdLine.h>
#include <wx/app.h>
#include <wx/config.h>
#include <wx/extension/cmdline.h>
#include <wx/extension/lexer-props.h>
#include <wx/extension/log.h>
#include <wx/extension/stc.h>
#include <wx/extension/stcdlg.h>
#include <wx/extension/tokenizer.h>
#include <wx/extension/version.h>

class wxExCmdLineOption
{
public:
  wxExCmdLineOption(
    TCLAP::ValueArg<float>* f, std::function<void(const std::any& any)> fu)
    : m_val(f), m_f(fu) {;};

  wxExCmdLineOption(
    TCLAP::ValueArg<int>* i, std::function<void(const std::any& any)> fu)
    : m_val(i), m_f(fu) {;};

  wxExCmdLineOption(
    TCLAP::ValueArg<std::string>* s, std::function<void(const std::any& any)> fu)
    : m_val(s), m_f(fu) {;};

 ~wxExCmdLineOption()
  {
    if (std::holds_alternative<TCLAP::ValueArg<float>*>(m_val))
      delete std::get<0>(m_val);
    else if (std::holds_alternative<TCLAP::ValueArg<int>*>(m_val))
      delete std::get<1>(m_val);
    else if (std::holds_alternative<TCLAP::ValueArg<std::string>*>(m_val))
      delete std::get<2>(m_val);
  };

  const std::string GetDescription() const {
    if (auto pval = std::get_if<TCLAP::ValueArg<float>*>(&m_val))
    {
      return (*pval)->getDescription();
    }
    else if (auto pval = std::get_if<TCLAP::ValueArg<int>*>(&m_val))
    {
      return (*pval)->getDescription();
    }
    else if (auto pval = std::get_if<TCLAP::ValueArg<std::string>*>(&m_val))
    {
      return (*pval)->getDescription();
    }
    return std::string();};

  
  const std::string GetName() const {
    if (auto pval = std::get_if<TCLAP::ValueArg<float>*>(&m_val))
    {
      return (*pval)->getName();
    }
    else if (auto pval = std::get_if<TCLAP::ValueArg<int>*>(&m_val))
    {
      return (*pval)->getName();
    }
    else if (auto pval = std::get_if<TCLAP::ValueArg<std::string>*>(&m_val))
    {
      return (*pval)->getName();
    }
    return std::string();};

  const std::string GetValue() const {
    if (auto pval = std::get_if<TCLAP::ValueArg<float>*>(&m_val))
    {
      if (const int v = (*pval)->getValue(); v != -1) 
      {
        return std::to_string(v);
      }
    }
    else if (auto pval = std::get_if<TCLAP::ValueArg<int>*>(&m_val))
    {
      if (const int v = (*pval)->getValue(); v != -1) 
      {
        return std::to_string(v);
      }
    }
    else if (auto pval = std::get_if<TCLAP::ValueArg<std::string>*>(&m_val))
    {
      if (const std::string v = (*pval)->getValue(); !v.empty()) 
      {
        return v;
      }
    }
    return std::string();};

  void Run(bool save) const
  {
    if (auto pval = std::get_if<TCLAP::ValueArg<float>*>(&m_val))
    {
      if (const float v = (*pval)->getValue(); v != -1) 
      {
        m_f(v);

        if (save)
        {
          wxConfigBase::Get()->Write(GetName(), v);
        }
      }
    }
    else if (auto pval = std::get_if<TCLAP::ValueArg<int>*>(&m_val))
    {
      if (const int v = (*pval)->getValue(); v != -1) 
      {
        m_f(v);

        if (save)
        {
          wxConfigBase::Get()->Write(GetName(), v);
        }
      }
    }
    else if (auto pval = std::get_if<TCLAP::ValueArg<std::string>*>(&m_val))
    {
      if (const std::string v = (*pval)->getValue(); !v.empty()) 
      {
        m_f(v);

        if (save)
        {
          wxConfigBase::Get()->Write(GetName(), v.c_str());
        }
      }
    }
  }
private:    
  std::function<void(const std::any& any)> m_f; 

  const std::variant <
    TCLAP::ValueArg<float>*,
    TCLAP::ValueArg<int>*,
    TCLAP::ValueArg<std::string>*> m_val; 
};

class wxExCmdLineParam
{
public:
  wxExCmdLineParam(
    TCLAP::UnlabeledMultiArg<std::string>* arg, 
    std::function<bool(std::vector<std::string>)> f) 
  : m_val({arg, f}) {;}; 

 ~wxExCmdLineParam() {delete m_val.first;};

  bool Run() const
  {
    return 
      m_val.first->getValue().empty() || m_val.second(m_val.first->getValue());
  }
private:
  const std::pair<
    TCLAP::UnlabeledMultiArg<std::string>*, 
    std::function<bool(std::vector<std::string>)>> m_val; 
};

class wxExCmdLineParser : public TCLAP::CmdLine
{
public:
  wxExCmdLineParser(
    const std::string& message,
    char delim,
    const std::string& version,
    bool help)
  : TCLAP::CmdLine(message, delim, version, help) {;};
};

class wxExCmdLineSwitch
{
public:
  wxExCmdLineSwitch(
    TCLAP::SwitchArg* arg, 
    std::function<void(bool)> f)
  : m_val({arg, f}) {;};

 ~wxExCmdLineSwitch() {delete m_val.first;};

  const std::string GetDescription() const {return m_val.first->getDescription();};
  
  const std::string& GetName() const {return m_val.first->getName();};

  bool GetValue() const {return *m_val.first;};

  void Run(bool save) const
  {
    const bool def = wxConfigBase::Get()->ReadBool(GetName(), false);
    
    if (def != GetValue())
    {
      m_val.second(GetValue());

      if (save)
      {
        wxConfigBase::Get()->Write(GetName(), GetValue());
      }
    }
  }
private:
  const std::pair<
    TCLAP::SwitchArg*, 
    std::function<void(bool)>> m_val; 
};

wxExCmdLine::wxExCmdLine(
  const CmdSwitches & s, 
  const CmdOptions & o, 
  const CmdParams & p,
  const std::string& message,
  const std::string& version,
  bool helpAndVersion)
  : m_Parser(new wxExCmdLineParser(
      message, ' ', 
      version.empty() ? 
        wxExGetVersionInfo().Get(): version, 
      helpAndVersion))
{
  m_Parser->setExceptionHandling(false);

  char c = 'A';

  try
  {
    for (auto it = o.rbegin(); it != o.rend(); ++it)
    {
      std::string flag(it->first[0]);
      size_t p_n{1}, p_d{2}; // par name, description

      if (it->first[0].size() > 1)
      {
        flag = std::string(1, c++);
        p_n--;
        p_d--;
      }
      
      const std::string def_specified = 
        (it->first.size() > p_d + 1 ? it->first.back(): std::string());

      switch (it->second.first)
      {
        case CMD_LINE_FLOAT: {
          const float def = wxConfigBase::Get()->ReadDouble(
            it->first[p_n], def_specified.empty() ? -1: std::stod(def_specified));
          
          auto* arg = new TCLAP::ValueArg<float>(
            flag, it->first[p_n], it->first[p_d],
            false, def, "float");
          m_Parser->add(arg);
          m_Options.emplace_back(new wxExCmdLineOption(arg, it->second.second));
          }
          break;

        case CMD_LINE_INT: {
          const int def = wxConfigBase::Get()->ReadLong(
            it->first[p_n], def_specified.empty() ? -1: std::stoi(def_specified));
          
          auto* arg = new TCLAP::ValueArg<int>(
            flag, it->first[p_n], it->first[p_d],
            false, def, "int");
          m_Parser->add(arg);
          m_Options.emplace_back(new wxExCmdLineOption(arg, it->second.second));
          }
          break;
        
        case CMD_LINE_STRING: {
          const std::string def = wxConfigBase::Get()->Read(
            it->first[p_n], std::string());
          
          auto* arg = new TCLAP::ValueArg<std::string>(
            flag, it->first[p_n], it->first[p_d],
            false, def_specified, "string");
          m_Parser->add(arg);
          m_Options.emplace_back(new wxExCmdLineOption(arg, it->second.second));
          }
          break;
        
        default: wxExLog() << "unknown type";
      }
    }

    for (auto it = s.rbegin(); it != s.rend(); ++it)
    {
      std::string flag(it->first[0]);
      size_t p_n{1}, p_d{2}; // par name, description

      if (it->first[0].size() > 1)
      {
        flag = std::string(1, c++);
        p_n--;
        p_d--;
      }

      const std::string def_specified = 
        (it->first.size() > p_d + 1 ? it->first.back(): std::string());

      const bool def = wxConfigBase::Get()->ReadBool(
        it->first[p_n], def_specified.empty() ? false: std::stoi(def_specified));
      
      auto* arg = new TCLAP::SwitchArg(
        flag, it->first[p_n], it->first[p_d], def); 
      m_Switches.emplace_back(new wxExCmdLineSwitch(arg, it->second));
      m_Parser->add(arg);
    }

    if (!p.first.first.empty())
    {
      auto* arg = new TCLAP::UnlabeledMultiArg<std::string>(
        p.first.first, p.first.second, false, std::string());
      m_Params.emplace_back(new wxExCmdLineParam(arg, p.second));
      m_Parser->add(arg);
    }
  }
  catch (TCLAP::ArgException& e)
  {
    wxExLog(e) << "tclap";
  }
  catch (TCLAP::ExitException& )
  {
  }
  catch (std::exception& e)
  {
    wxExLog(e) << "wxExCmdLine";
  }
}

wxExCmdLine::~wxExCmdLine()
{
  for (auto& it : m_Options) delete it;
  for (auto& it : m_Params) delete it;
  for (auto& it : m_Switches) delete it;

  delete m_Parser;
}
  
char wxExCmdLine::Delimiter() const
{
  return TCLAP::Arg::delimiter();
}
  
void wxExCmdLine::Delimiter(char c)
{
  TCLAP::Arg::setDelimiter(c);
}

bool wxExCmdLine::Parse(
  const std::string& cmdline, bool save, const char delimiter)
{
  Delimiter(delimiter);
  
  try
  {
    if (cmdline.empty())
    {
      m_Parser->parse(wxTheApp->argc, wxTheApp->argv);
    }
    else
    {
      wxExTokenizer tkz(cmdline);
      auto v = tkz.Tokenize<std::vector<std::string>>();
      m_Parser->parse(v);
    }

    for (const auto& it : m_Switches) 
    {
      it->Run(save);
    }

    for (const auto& it : m_Options)
    {
      it->Run(save);
    }

    if (!m_Params.empty() && !m_Params[0]->Run())
    {
      wxExLog() << "could not run params";
      return false;
    }
    
    return true;
  }
  catch (TCLAP::ArgException& e)
  {
    wxExLog(e) << "tclap";
    return false;
  }
  catch (TCLAP::ExitException& )
  {
    return false;
  }
  catch (std::exception& e)
  {
    wxExLog(e) << "parse";
    return false;
  }
}

void wxExCmdLine::ShowOptions(const wxExWindowData& data) const
{
  static wxExSTCEntryDialog* dlg = nullptr;
  const wxExLexerProps l;

  if (dlg == nullptr)
  {
    dlg = new wxExSTCEntryDialog(
      std::string(),
      std::string(),
      wxExWindowData(data).Button(wxOK).Title("Options"));

    dlg->GetSTC()->GetLexer().Set(l);
  }
  else
  {
    dlg->GetSTC()->ClearAll();
  }

  for (const auto& it : m_Options)
  {
    dlg->GetSTC()->InsertText(0, l.MakeKey(
      it->GetName(), 
      it->GetValue(), 
      it->GetDescription() != it->GetName() ? it->GetDescription(): std::string()));
  }

  dlg->GetSTC()->InsertText(0, "\n" + l.MakeSection("options"));
  
  for (const auto& it : m_Switches)
  {
    dlg->GetSTC()->InsertText(0, l.MakeKey(
      it->GetName(), 
      std::to_string(it->GetValue()), 
      it->GetDescription() != it->GetName() ? it->GetDescription(): std::string()));
  }

  dlg->GetSTC()->InsertText(0, l.MakeSection("switches"));
  dlg->GetSTC()->EmptyUndoBuffer();
  dlg->ShowModal();
}
