////////////////////////////////////////////////////////////////////////////////
// Name:      ex.h
// Purpose:   Declaration of class wex::ex
// Author:    Anton van Wezenbeek
// Copyright: (c) 2019 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <functional>
#include <tuple>
#include <map>
#include <string>
#include <utility>
#include <vector>
#include <wex/ex-command.h>
#include <wex/marker.h>

namespace wex
{
  class ctags;
  class evaluator;
  class managed_frame;
  class stc;
  class stc_entry_dialog;
  class vi_macros;
  class vi_macros_mode;

  enum class info_message_t;

  /// Offers a class that adds ex editor to wex::stc.
  class ex
  {
    friend class vi_macros_mode;
  public:
    /// Constructor. 
    /// Sets ex mode.
    ex(stc* stc);
    
    /// Destructor.
    virtual ~ex();
    
    /// Adds text (to stc or register, if register is active).
    void add_text(const std::string& text);
   
    /// Returns calculated value with precision width of text.
    std::tuple<double, int> calculator(const std::string& text);
    
    /// Executes ex: command that was entered on the command line,
    /// or present as modeline command inside a file.
    /// Returns true if the command was executed.
    virtual bool command(const std::string& command);

    /// Copies data from other component.
    void copy(const ex* ex);
    
    /// Returns the ctags.
    auto & ctags() {return m_CTags;};
    
    /// Cuts selected text to yank register,
    /// and updates delete registers.
    void cut(bool show_message = true);

    /// Returns the frame.
    auto* frame() {return m_Frame;};
    
    /// Returns command.
    const auto & get_command() const {return m_Command;};

    /// Returns the macros.
    static auto & get_macros() {return m_Macros;};

    /// Returns selected text as a string.
    const std::string get_selected_text() const;
    
    /// Returns stc component.
    auto * get_stc() {return m_Command.get_stc();};

    /// Returns whether ex is active.
    auto is_active() const {return m_IsActive;};
    
    /// Writes info message.
    void info_message() const;
    
    /// Adds marker at the specified line.
    /// Returns true if marker could be added.
    bool marker_add(
      /// marker
      char marker,
      /// line to add marker, default current line
      int line = -1);
    
    /// Deletes specified marker.
    /// Returns true if marker was deleted.
    bool marker_delete(char marker);
    
    /// Goes to specified marker.
    /// Returns true if marker exists.
    bool marker_goto(char marker);
    
    /// Returns line for specified marker.
    /// Returns -1 if marker does not exist.
    int marker_line(char marker) const;
    
    /// Prints text in the dialog.
    void print(const std::string& text);

    /// Returns current register name.
    const auto register_name() const {return m_Register;};
    
    /// Returns text to be inserted.
    const std::string register_insert() const;
    
    /// Returns text from current register (or yank register if no register active).
    const std::string register_text() const;
    
    /// Resets search flags.
    void reset_search_flags();
    
    /// Returns search flags.
    auto search_flags() const {return m_SearchFlags;};
    
    /// Sets delete registers 1 - 9 (if value not empty).
    void set_registers_delete(const std::string& value) const;
    
    /// Sets insert register (if value not empty).
    void set_register_insert(const std::string& value) const;
    
    /// Sets yank register (if value not empty).
    void set_register_yank(const std::string& value) const;
    
    /// Set using ex mode.
    void use(bool mode) {m_IsActive = mode;};
    
    /// Yanks selected text to yank register, default to yank register.
    /// Returns false if no text was selected.
    bool yank(const char name = '0', bool show_message = true) const;
  protected:
    /// If autowrite is on and document is modified,
    /// save the document.
    bool auto_write();

    /// Sets register name.
    /// Setting register 0 results in
    /// disabling current register.
    void set_register(const char name) {m_Register = name;};

    ex_command m_Command;
  private:
    bool CommandHandle(const std::string& command) const;
    bool CommandAddress(const std::string& command);
    template <typename S, typename T> 
      bool HandleContainer(
        const std::string& kind,
        const std::string& command,
        const T * container,
        std::function<bool(const std::string&, const std::string&)> cb);
    void info_message(const std::string& text, info_message_t type) const;
    template <typename S, typename T>
    std::string ReportContainer(const T & container) const;
    void show_dialog(
      const std::string& title, const std::string& text, bool prop_lexer = false);
      
    const marker m_MarkerSymbol = marker(0);

    std::map<char, int> 
      // relate a marker to identifier
      m_MarkerIdentifiers,
      // relate a marker to mark number
      m_MarkerNumbers;
    
    static inline stc_entry_dialog* m_Dialog = nullptr;
    static vi_macros m_Macros;
    static evaluator m_Evaluator;

    bool 
      m_AutoWrite {false},
      m_IsActive {true}, // are we actively using ex mode?
      m_Copy {false};    // this is a copy, result of split
    
    int m_SearchFlags;
    
    char m_Register {0};
    
    managed_frame* m_Frame;  
    wex::ctags* m_CTags {nullptr};

    const std::vector<std::pair<
      const std::string, 
      std::function<bool(const std::string& command)>>> m_Commands;
  };
};