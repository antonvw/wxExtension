////////////////////////////////////////////////////////////////////////////////
// Name:      process.h
// Purpose:   Declaration of class wex::process
// Author:    Anton van Wezenbeek
// Copyright: (c) 2019 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <memory>
#include <signal.h>  
#include <wex/window-data.h>

namespace wex
{
  class managed_frame;
  class process_imp;
  class shell;
  
  /// Offers a process, capturing execution output.
  class process
  {
  public:
    /// process execute type
    /// - EXEC_NO_WAIT this call immediately returns.
    ///   The stc component will be filled with output from the process.
    /// - EXEC_WAIT this call returns after execute ends, 
    ///   and the output is available using get_stdout.
    enum exec_t
    {
      EXEC_NO_WAIT = 0, ///< do not wait
      EXEC_WAIT    = 1, ///< wait for process finish
    };

    /// Default constructor.
    process();
    
    /// Destructor.
    virtual ~process();

    /// Copy constructor.
    process(const process& process);

    /// Assignment operator.
    process& operator=(const process& p);

    /// Shows a config dialog, allowing you to set the command and folder.
    /// Returns dialog return code.
    static int config_dialog(const window_data& data = window_data());
    
    /// Executes the process.
    /// Return value is false if process could not execute, 
    /// or if config dialog was invoked and cancelled.
    bool execute(
      /// command to be executed, if empty
      /// last given command is used
      const std::string& command = std::string(),
      /// process excute type
      exec_t type = EXEC_NO_WAIT,
      /// working dir, if empty last working dir is used
      const std::string& wd = std::string());

    /// Returns command executed.
    const auto & get_command_executed() const {return m_command;};

    /// Returns the frame.
    auto * get_frame() {return m_frame;};
    
    /// Returns the shell component 
    /// (might be nullptr if prepare_output is not yet invoked).
    static auto* get_shell() {return m_shell;};
    
    /// Returns the stderr.
    const auto & get_stderr() const {return m_stderr;};
    
    /// Returns the stdout.
    const auto & get_stdout() const {return m_stdout;};
    
    /// Callback for finished pid.
    void is_finished(int pid);
    
    /// Returns true if this process is running.
    bool is_running() const;

    /// Sends specified signal to this process.
    /// Returns true if signalling is ok.
    /// (SIGKILL is not ISO C99 and not known by windows).
    bool kill(
#ifdef __UNIX__
      int sig = SIGKILL);
#else
      int sig = SIGTERM);
#endif
    
    /// Construct the shell component.
    static void prepare_output(wxWindow* parent);

    /// Shows stdout or stderr from execute on the shell component.
    /// You can override this method to e.g. prepare a lexer on get_shell
    /// before calling this base method.
    virtual void show_output(const std::string& caption = std::string()) const;

    /// Writes text to stdin of process.
    bool write(const std::string& text);
  private:
    std::string m_command, m_stderr, m_stdout;
    static std::string m_working_dir_key;
    static inline shell* m_shell = nullptr;
    std::unique_ptr<process_imp> m_process;
    managed_frame* m_frame;
  };
};
