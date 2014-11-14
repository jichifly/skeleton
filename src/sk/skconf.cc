// skconf.cc
// 12/8/2012 jichi

#include "sk/skconf.h"
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <fstream>
#include <streambuf>
#include <string>

#define SK_DEBUG "skconf"
#include "sk/skdebug.h"

#define foreach BOOST_FOREACH

#define COMMENT_PREFIX  '#'

#define K_FUNCTION_DEFINITION       "FunctionDefinition.keyword"
#define K_BLOCK_STATEMENT           "BlockStatement.keyword"
#define K_LOAD_STATEMENT            "LoadStatement.keyword"
#define K_STORE_STATEMENT           "StoreStatement.keyword"
#define K_FLOATING_POINT_STATEMENT  "FloatingPointStatement.keyword"
#define K_FIXED_POINT_STATEMENT     "FixedPointStatement.keyword"
#define K_ALLOCATE_STATEMENT        "AllocaetStatement.keyword"
#define K_DEALLOCATE_STATEMENT      "DeallocateStatement.keyword"

SK_BEGIN_NAMESPACE

SkConf SkConf::fromFile(const std::string &fileName)
{
  SK_DPRINT("enter: file = " << fileName);
  std::ifstream f(fileName.c_str());
  std::string content((std::istreambuf_iterator<char>(f)),
                       std::istreambuf_iterator<char>());
  SK_DPRINT("leave: empty = " << content.empty());
  return fromString(content);
}

SkConf SkConf::fromString(const std::string &input)
{
  SkConf conf;
  if (input.empty())
    return conf;

  std::list<std::string> lines;
  boost::split(lines, input, boost::is_any_of("\n"));
  foreach (const std::string &line, lines) {
    std::string trimmedLine = boost::algorithm::trim_copy(line);
    if (trimmedLine.empty() || trimmedLine[0] == COMMENT_PREFIX)
      continue;
    std::list<std::string> words;
    boost::split(words, trimmedLine, boost::is_any_of("="));
    if (words.size() != 2) {
      SK_DPRINT("warning: malformat line: " << line);
      continue;
    }
    std::string key = boost::algorithm::trim_copy(words.front()),
                value = boost::algorithm::trim_copy(words.back());

    if (key == K_FUNCTION_DEFINITION)
      conf.k_functionDefinition = value;
    else if (key == K_BLOCK_STATEMENT)
      conf.k_blockStatement = value;
    else if (key == K_LOAD_STATEMENT)
      conf.k_loadStatement = value;
    else if (key == K_STORE_STATEMENT)
      conf.k_storeStatement = value;
    else if (key == K_FLOATING_POINT_STATEMENT)
      conf.k_floatingPointStatement = value;
    else if (key == K_FIXED_POINT_STATEMENT)
      conf.k_fixedPointStatement = value;
    else if (key == K_ALLOCATE_STATEMENT)
      conf.k_allocateStatement = value;
    else if (key == K_DEALLOCATE_STATEMENT)
      conf.k_deallocateStatement = value;
    else
      SK_DPRINT("warning: unknown key: " << key << ", line: " << line);
  }
  return conf;
}

SK_END_NAMESPACE
