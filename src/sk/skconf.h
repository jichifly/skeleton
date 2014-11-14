#ifndef SKCONF_H
#define SKCONF_H

// skconf.h
// 12/8/2012 jichi
// Skeleton configuration parser.

#include "sk/skdef.h"
#include <string>

SK_BEGIN_NAMESPACE

struct SkConf
{
  typedef SkConf Self;

  std::string k_functionDefinition;
  std::string k_blockStatement;
  std::string k_loadStatement;
  std::string k_storeStatement;
  std::string k_floatingPointStatement;
  std::string k_fixedPointStatement;
  std::string k_allocateStatement;
  std::string k_deallocateStatement;

  SkConf() :
    k_functionDefinition("def"),
    k_blockStatement("blk"),
    k_loadStatement("ld"),
    k_storeStatement("st"),
    k_floatingPointStatement("fp"),
    k_fixedPointStatement("xp"),
    k_allocateStatement("alloc"),
    k_deallocateStatement("free") {}


  static Self fromFile(const std::string &fileName);
  static Self fromString(const std::string &content);
};

SK_END_NAMESPACE

#endif // SKCONF_H
