// depgraphopt_p.cc
// 2/14/2014 jichi
// Internal header for depgraph.

#include "rosex/depgraphopt_p.h"
#include "xt/c++11.h"

namespace static_initializer {
  AssumeNoAlias AssumeNoAlias;
}

ArrayAbstractionInterface*
Hacked_LoopTransformInterface::arrayInfo_bak = nullptr;
AstInterface*
Hacked_LoopTransformInterface::fa_bak = nullptr;

AliasAnalysisInterface*
Hacked_LoopTransformInterface::ASSUME_NO_ALIAS = &static_initializer::AssumeNoAlias;

static_initializer::LoopTransformInterfaceInit
Hacked_LoopTransformInterface::init;

static_initializer::
LoopTransformInterfaceInit::LoopTransformInterfaceInit()
{
  Hacked_LoopTransformInterface::
  set_aliasInfo(&static_initializer::AssumeNoAlias);
}

// EOF
