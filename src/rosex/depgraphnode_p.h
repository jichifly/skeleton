#ifndef _ROSEX_DEPGRAPHNODE_P_H
#define _ROSEX_DEPGRAPHNODE_P_H

// depgraphnode_p.h
// 7/14/2011 jichi
// Internal header for depgraph vertex

#include "rosex/rose_config.h"
#include <DepInfo.h>    // DepInfo in rose
#include <DomainInfo.h>   // DomainCond in rose
#include <MultiGraphCreate.h>   // MultiGraphElem in rose

class SgNode;

// This class must be consistent with LoopTreeDepGraphNode in LoopTreeDepComp.h
///  \internal  Represent node in the dependency graph.
class DepGraphNode : public MultiGraphElem
{
  typedef DepGraphNode  Self;
  typedef MultiGraphElem  Base;

  SgNode *root_, *stmt_;  // Each node holds pointers to AST statement and root

  DepInfo loopMap_;
  DomainCond domain_;

  // - Properties -
  ///  Property queries of the node.
public:
   /// Not null. Corresponding AST node pointer, guaranteed never to be NULL.
  SgNode *statement() const { return stmt_; }

  ///  AST root of the dependency graph, which is used to compute loop level.
  SgNode *root() const { return root_; }

  ///  Loop level of statement, computed from AST and could get changed if AST is modified.
  int level() const { return levelOf(stmt_, root_); }

  ///  If this node is treated as a loop during analysis. This property is computed from AST.
  bool isLoop() const  { return isLoop(stmt_); }

  ///  \reimp  Unparse corresponding AST statement.
  virtual std::string toString() const;

  // LoopTreeDepGraphNode members
  int NumOfLoops() const { return loopMap_.cols(); }
  //int AstTreeNumOfLoops() const { return loopMap_.rows(); }

  // - Constructions -
  /**
   *  Methods used to create the node.
   *
   *  Conventions:
   *  - Except AST root which could be 0, all parameters of the constructors must be valid.
   *  - The node instance is immutable after construction that all properties cannot be modified.
   */
public:
  DepGraphNode(MultiGraphCreate*, SgNode *root, SgNode *stmt);
  DepGraphNode(MultiGraphCreate*, SgNode *root, SgNode *stmt, const DomainCond&);
  DepGraphNode(MultiGraphCreate*, SgNode *root, SgNode *stmt, const DepInfo&, const DomainCond&);

  const DepInfo &loopMap() const  { return loopMap_; }
  const DomainCond &domain() const  { return domain_; }

protected:
  ///  \internal  Return the level of loops for \p node relative to \p root.
  static int levelOf(const SgNode *stmt, const SgNode *parent);

  ///  \internal  Return if the vertex represents a loop that can be handled by the our analysis.
  static bool isLoop(const SgNode *stmt);
};

#endif //_ROSEX_DEPGRAPHNODE_P_H
