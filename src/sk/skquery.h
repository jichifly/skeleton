#ifndef SKQUERY_H
#define SKQUERY_H

// skquery.h
// 9/12/2012 jichi
// Skeleton AST analyzer.

#include "sk/sknode.h"

SK_BEGIN_NAMESPACE

class SkNode;

/**
 *  \brief  Skeleton AST analyzer
 *
 *  Query an existing skeleton AST without modifying it
 */
namespace SkQuery {

  ///  Return all nodes with given class
  SkNodeList find(SkNode *root, Sk::ClassType cls, Sk::TraversalOrder = Sk::AnyOrder);
  SkConstNodeList find(const SkNode *root, Sk::ClassType cls, Sk::TraversalOrder = Sk::AnyOrder);

  SkNode *findfirst(SkNode *root, Sk::ClassType cls, Sk::TraversalOrder = Sk::AnyOrder);
  const SkNode *findfirst(const SkNode *root, Sk::ClassType cls, Sk::TraversalOrder = Sk::AnyOrder);

  ///  If there are children with the given class
  bool contains(const SkNode *root, Sk::ClassType cls);

} // namespace SkQuery

SK_END_NAMESPACE

#endif // SKQUERY_H
