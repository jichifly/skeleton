// depgraphnode_p.cc
// 7/14/2011 jichi

#include "rosex/rosex.h"
//#include "rosex/slice_p.h"
#include "rosex/depgraphnode_p.h"

// - Construction -

DepGraphNode::DepGraphNode(MultiGraphCreate *c, SgNode *root, SgNode *stmt)
  : Base(c), root_(root), stmt_(stmt)
{
  ROSE_ASSERT(stmt_);

  int l = level();
  loopMap_ = DepInfoGenerator::GetIDDepInfo(l, false);
  domain_ = DomainCond(l);
}

DepGraphNode::DepGraphNode(MultiGraphCreate *c, SgNode *root, SgNode *stmt,
      const DomainCond &domain)
  : Base(c), root_(root), stmt_(stmt), domain_(domain)
{
  ROSE_ASSERT(stmt_);
  loopMap_ = DepInfoGenerator::GetIDDepInfo(level(), false);
}

DepGraphNode::DepGraphNode(MultiGraphCreate *c, SgNode *root, SgNode *stmt,
      const DepInfo &loopMap, const DomainCond &domain)
  : Base(c), root_(root), stmt_(stmt), loopMap_(loopMap), domain_(domain)
{ ROSE_ASSERT(stmt_); }

std::string DepGraphNode::toString() const
{ return stmt_? stmt_->unparseToString() : std::string(); }

// - Helpers -

bool DepGraphNode::isLoop(const SgNode *input)
{
  // This does not work!
  //return input &&
  //    slice::globalAstInterface()
  //    ->IsFortranLoop(slice::astnode_cast<AstNodePtr>(input));
  return input && rosex::isCanonicalLoop(input);
}

// See class LoopTreeNode:
// - virtual int LoopLevel() const { return Parent()->LoopLevel() + Parent()->IncreaseLoopLevel(); }
// - bool IsLoop() const { return IncreaseLoopLevel(); }
int DepGraphNode::levelOf(const SgNode *node, const SgNode *root)
{
  if (!node || node == root)
    return 0;

  SgNode *parent = node->get_parent();
  return !parent ? 0 :
         (isLoop(parent) ? 1 : 0) + levelOf(parent, root);
}

// EOF
