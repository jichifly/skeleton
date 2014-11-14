#ifndef _ROSEX_SLICE_P_H
#define _ROSEX_SLICE_P_H

// slice_p.h
// 9/20/2012 jichi
// Internal header.

#include "rosex/rose_config.h"
#include <vector> // needed by <rose/AstInterface_ROSE> orz
#include <AstInterface_ROSE.h>

///  \internal  Helpers to adapt bad-written slice headers in ROSE.
namespace slice {
  template <typename T, typename S>
  inline T astnode_cast(S input)
  { return S::__cast_not_defined__(input); } // static_assert(0)

  template <>
  inline SgNode*
  astnode_cast<SgNode*>(AstNodePtr input)
  { return static_cast<SgNode*>(input.get_ptr()); }

  template <>
  inline AstNodePtr
  astnode_cast<AstNodePtr>(SgNode *input)
  { return AstNodePtrImpl(input); }

  template <>
  inline AstNodePtr
  astnode_cast<AstNodePtr>(const SgNode *input)
  { return astnode_cast<AstNodePtr>(const_cast<SgNode*>(input)); }

  AstInterface *globalAstInterface();

} // namespace slice

#endif // _ROSEX_SLICE_P_H
