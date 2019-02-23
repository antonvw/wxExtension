////////////////////////////////////////////////////////////////////////////////
// Name:      presentation.h
// Purpose:   Declaration of class wex::presentation
// Author:    Anton van Wezenbeek
// Copyright: (c) 2019 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <pugixml.hpp>

namespace wex
{
  class stc;

  /// This class defines our scintilla presentations.
  class presentation
  {
  public:
    enum presentation_t
    {
      INDICATOR,
      MARKER,
    };
    
    /// Constructor.
    presentation(
      presentation_t t, const pugi::xml_node& node = pugi::xml_node());

    /// Constructor.
    /// Only sets no and style, and not the colour and under.
    presentation(
      presentation_t t, int no, int style = -1);

    /// < Operator.
    bool operator<(const presentation& i) const;

    /// == Operator. 
    /// Returns true if no and style are equal
    /// (if style is not -1).
    bool operator==(const presentation& i) const;
    
    /// != Operator.
    bool operator!=(const presentation& i) const {return !operator==(i);};

    /// Applies this presentation to stc component.
    void apply(stc* stc) const;

    /// Returns background colour.
    const auto & background_colour() const {return m_BackgroundColour;};
    
    /// Returns foreground colour.
    const auto & foreground_colour() const {return m_ForegroundColour;};
    
    /// Returns true if this presentation is valid.
    bool is_ok() const;
    
    /// Returns name of presentation.
    const std::string name() const;
  
    /// Returns the no.
    int number() const {return m_No;};

    /// Returns the style.
    int style() const {return m_Style;};
    
    /// Returns underlined.
    bool underlined() const {return m_Under;};
  private:
    std::string m_BackgroundColour, m_ForegroundColour;
    int m_No = -1, m_Style = -1;
    bool m_Under = false;
    presentation_t m_type;
  };
};