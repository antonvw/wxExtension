////////////////////////////////////////////////////////////////////////////////
// Name:      cmdline.h
// Purpose:   Declaration of wxExCmdLine class
// Author:    Anton van Wezenbeek
// Copyright: (c) 2018 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <any>
#include <functional>
#include <utility>
#include <vector>
#include <wx/extension/window-data.h>

/// Types for commandline.
enum wxExCmdLineTypes
{
  CMD_LINE_FLOAT,
  CMD_LINE_INT,
  CMD_LINE_STRING,
};

class wxExCmdLineOption;
class wxExCmdLineParam;
class wxExCmdLineParser;
class wxExCmdLineSwitch;

/// This class offers a command line parser.
class wxExCmdLine
{
public:
  /// Switches: 
  typedef std::vector<std::pair<
    /// vector of switch flag, name, description
    /// - if sizeof first element is greater than one,
    ///   it is supposed to be the name, and a flag is generated,
    ///   starting with 'A'
    /// - after description you can also add a default true value,
    ///   otherwise false is assumed
    const std::vector<std::string>, 
    /// process callback if option is found
    std::function<void(bool)>>> CmdSwitches;

  /// Options:
  typedef std::vector<std::pair<
    /// vector of option flag, name, description
    /// - if sizeof first element is greater than one,
    ///   it is supposed to be the name, and a flag is generated
    ///   starting with 'A'
    /// - after description you can also add a default value,
    ///   otherwise 0 is assumed
    const std::vector<std::string>, 
    /// pair of command line param type and process callback if option is found
    std::pair<
      wxExCmdLineTypes, 
      std::function<void(const std::any& any)>>>> CmdOptions;

  /// Params (currently only string value supported): 
  typedef std::pair<
    /// pair of name and description
    const std::pair<const std::string, const std::string>, 
    /// process callback if param is present
    std::function<bool(const std::vector<std::string> &)>> CmdParams;

  /// Constructor, 
  wxExCmdLine(
    /// switches
    const CmdSwitches & s, 
    /// options
    const CmdOptions & o, 
    /// params
    const CmdParams & p = CmdParams(),
    /// message
    const std::string& message = std::string(),
    /// version, if empty use wxExtension version
    const std::string& version = std::string(), 
    /// show help
    bool helpAndVersion = true);

  /// Destructor.
 ~wxExCmdLine();

  /// Returns current delimiter.
  char Delimiter() const;
  
  /// Sets delimiter.
  void Delimiter(char c);

  /// Parses the specified command line 
  /// (should start with app name, and if empty
  /// the command line from wxTheApp is used).
  /// Returns false if error is found, or exit
  /// condition is true.
  bool Parse(
    /// command line
    const std::string& cmdline = std::string(),
    /// keep changed values in config
    bool save = false,
    /// delimiter
    const char delimiter = ' ');

  /// Shows current options.
  void ShowOptions(const wxExWindowData& data = wxExWindowData()) const;
private:
  std::vector<wxExCmdLineOption*> m_Options; 
  std::vector<wxExCmdLineParam*> m_Params; 
  std::vector<wxExCmdLineSwitch*> m_Switches; 
  
  wxExCmdLineParser* m_Parser;
};
