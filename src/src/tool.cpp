////////////////////////////////////////////////////////////////////////////////
// Name:      tool.cpp
// Purpose:   Implementation of wex::tool class
// Author:    Anton van Wezenbeek
// Copyright: (c) 2019 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <wex/tool.h>
#include <wex/statistics.h>
#include <wex/util.h>

std::map < int, wex::tool_info > wex::tool::m_tool_info {
  {ID_TOOL_REPORT_FIND, {_("Found %d matches in")}},
  {ID_TOOL_REPLACE, {_("Replaced %d matches in")}},
  {ID_TOOL_REPORT_KEYWORD, 
     {_("Reported %d keywords in"), 
      _("Report &Keyword")}}};

const std::string wex::tool::info() const
{
  if (const auto& it = m_tool_info.find(m_id); it != m_tool_info.end())
    return it->second.info();
  else 
    return std::string();
}

const std::string wex::tool::info(const wex::statistics<int>* stat) const
{
  std::string logtext(info());

  replace_all(logtext, "%d", std::to_string(stat->get(_("Actions Completed"))));

  logtext.append(" ");
  logtext.append(std::to_string(stat->get(_("Files"))));
  logtext.append(" ");
  logtext.append(_("file(s)"));

  return logtext;
}
