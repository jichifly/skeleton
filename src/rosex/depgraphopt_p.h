#ifndef _ROSEX_DEPGRAPHOPT_P_H
#define _ROSEX_DEPGRAPHOPT_P_H

// depgraphopt_p.h
// 2/14/2014 jichi
// Internal header for depgraph.

#ifdef __clang__
#  pragma GCC diagnostic ignored "-Wunused-parameter" // in qingyi's code
#  pragma GCC diagnostic ignored "-Woverloaded-virtual" // in qingyi's code
#endif // __clang__

#include <LoopTransformInterface.h>

namespace static_initializer {
  struct LoopTransformInterfaceInit
  { LoopTransformInterfaceInit(); };

} // namespace static_initializer

class Hacked_LoopTransformInterface : public LoopTransformInterface
{
  typedef Hacked_LoopTransformInterface   Self;
  typedef LoopTransformInterface          Base;

  static ArrayAbstractionInterface* arrayInfo_bak;
  static AstInterface* fa_bak;

  static AliasAnalysisInterface *ASSUME_NO_ALIAS;
  static static_initializer::LoopTransformInterfaceInit init;

public:
  static void set_aliasInfo(AliasAnalysisInterface* alias)
  { Base::set_aliasInfo(alias? alias : ASSUME_NO_ALIAS); }

  static void set_astInterfacePtr(AstInterface *fa)
  { fa_bak = fa; if (fa) Base::set_astInterface(*fa); }

  static AstInterface *getAstInterfacePtr() { return fa_bak; }
  static ArrayAbstractionInterface *getArrayInfo() { return arrayInfo_bak; }

  static void set_arrayInfo(ArrayAbstractionInterface *arrayInfo)
  { arrayInfo_bak = arrayInfo; Base::set_arrayInfo(arrayInfo); }
};

class AutoBackup_LoopTransformInterface
{
  typedef Hacked_LoopTransformInterface la;

  AstInterface                *fa;
  AliasAnalysisInterface      *aliasInfo;
  FunctionSideEffectInterface *funcInfo;
  ArrayAbstractionInterface   *arrayInfo;
  //SideEffectAnalysisInterface *stmtInfo;

public:
  // Auto-backup on construction.
  AutoBackup_LoopTransformInterface()
    : fa(la::getAstInterfacePtr())
    , aliasInfo(la::getAliasInfo())
    , funcInfo(la::getSideEffectInterface())
    , arrayInfo(la::getArrayInfo())
    // CHECKPOINT 2/14/2014: Retore statement sideeffect interface
    //, stmtInfo(la::getStmtSideEffectInterface())
  { }

  // Auto-restore on destruction.
  ~AutoBackup_LoopTransformInterface()
  {
    la::set_astInterfacePtr(fa);
    la::set_aliasInfo(aliasInfo);
    la::set_sideEffectInfo(funcInfo);
    la::set_arrayInfo(arrayInfo);
    //la::set_stmtSideEffectInterface(stmtInfo);
  }
};

#endif // _ROSEX_DEPGRAPHOPT_P_H
