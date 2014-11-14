// rosex_indent.cc
// 9/17/2012 jichi
// Unparse skeleton language

#ifdef __clang__
#  pragma GCC diagnostic ignored "-Wchar-subscripts" // used in boost regex
#endif // __clang__

#include "rosex/rosex.h"
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/regex.hpp>
#include <sstream>
#include <iostream>

#define foreach BOOST_FOREACH

//#define SK_DEBUG "skbuilder_unparse"
#include "sk/skdebug.h"

std::string rosex::indent(const std::string &input)
{
  std::string
  output = boost::regex_replace(input, boost::regex("([;{}])"), "$1\n");
  output = boost::regex_replace(output, boost::regex("([{])"), "\n$1");
  boost::replace_all(output, "\n\n", "\n");

  std::ostringstream out;

  enum { indent_width = 2 };

  int level = 0;
  std::list<std::string> l;
  boost::split(l, output, boost::is_any_of("\n"));
  foreach (const std::string &stmt, l) {
    if (stmt.find('}') != std::string::npos)
      level--;

    if (level > 0)
      // FIXME: stringstream.width and std::setw are not working! why?
      //out.width(level * indent_width);
      out << std::string(level * indent_width, ' ');
    out << stmt << '\n';

    if (stmt.find('{') != std::string::npos)
      level++;
  }

  return out.str();
}

// EOF
