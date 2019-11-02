////////////////////////////////////////////////////////////////////////////////
// Name:      interruptable.h
// Purpose:   Declaration of class wex::interruptable
// Author:    Anton van Wezenbeek
// Copyright: (c) 2019 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#pragma once

namespace wex
{
  /// Offers methods to start, stop things.
  class interruptable
  {
  public:
    /// Cancel interruptable process.
    /// Returns false if process was not running.
    static bool cancel();

    /// Check whether process was cancelled.
    static bool is_cancelled() {return m_cancelled;};

    /// Is process running.
    static bool is_running() {return m_running;};
    
    /// Starts interruptable process.
    /// Returns false if process is already running.
    static bool start();
    
    /// Stops interruptable process.
    static void stop();
  private:
    static bool 
      m_cancelled,
      m_running;
  };
};
