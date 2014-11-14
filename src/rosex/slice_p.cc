// slice_p.cc
// 6/17/2011 jichi

#include "rosex/slice_p.h"
#include <LoopTransformInterface.h>

AstInterface *slice::globalAstInterface()
{
  static AstInterfaceImpl dummyImpl(nullptr);
  static AstInterface ret(&dummyImpl);
  static AssumeNoAlias aliasInfo;
  LoopTransformInterface::set_astInterface(ret);
  LoopTransformInterface::set_aliasInfo(&aliasInfo);
  return &ret;
}

// EOF
