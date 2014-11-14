// skbuilder_unparse.cc
// 9/17/2012 jichi
// Unparse skeleton language

#include "sk/skbuilder.h"
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <sstream>
#include <iostream>

#define foreach BOOST_FOREACH

//#define SK_DEBUG "skbuilder_unparse"
#include "sk/skdebug.h"

SK_BEGIN_NAMESPACE

std::string SkBuilder::indent(const std::string &input)
{
  std::ostringstream out;

  enum { indent_width = 2 };

  int level = 0;
  std::list<std::string> l;
  boost::split(l, input, boost::is_any_of("\n"));
  foreach (const std::string &stmt, l) {
    if (stmt.find('}') != std::string::npos)
      level--;

    if (level > 0) {
      // FIXME: stringstream.width and std::setw are not working! why?
      //out.width(level * indent_width);
      out << std::string(level * indent_width, ' ');
    }
    out << stmt << '\n';

    if (stmt.find('{') != std::string::npos)
      level++;
  }

  return out.str();
}

std::string SkBuilder::purge(const std::string &input)
{
  std::ostringstream out;

  std::list<std::string> l;
  boost::split(l, input, boost::is_any_of("\n"));
  foreach (const std::string &stmt, l)
    if (stmt.find(" # ") == std::string::npos)
      out << stmt << '\n';
    else
      out << "// @unsupported " << stmt << '\n';

  return out.str();
}

SK_END_NAMESPACE
