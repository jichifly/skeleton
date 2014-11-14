// depgraphbuilder_p.cc
// 7/14/2011 jichi

#include "rosex/rosex.h"
#include "rosex/slice_p.h"
#include "rosex/depgraphbuilder_p.h"

void DepGraphBuilder::AddNode(DepGraphNode *result)
{
  ROSE_ASSERT(result);
  Base::AddNode(result);
  if (!result->isLoop()) {
    SgNode *stmt = result->statement();
    ROSE_ASSERT(stmt);
    if (!map_[stmt])
      map_[stmt] = result;
    else
      ROSE_ASSERT(map_[stmt] == result);
  }
}

DepGraphNode *DepGraphBuilder::CreateNode(SgNode *stmt, const DomainCond &c)
{
  ROSE_ASSERT(stmt);
  DepGraphNode *result = map_[stmt];
  if (!result) {
    result = new DepGraphNode(this, root_, stmt, c);
    AddNode(result);
  }
  return result;
}

DepGraphNode *DepGraphBuilder::CreateNode(SgNode *stmt, const DepInfo &m, const DomainCond &c)
{
  ROSE_ASSERT(stmt);
  DepGraphNode *result = map_[stmt];
  if (!result) {
    result = new DepGraphNode(this, root_, stmt, m, c);
    AddNode(result);
  }
  return result;
}

DepGraphNode *DepGraphBuilder::CreateNode(SgNode *stmt)
{
  ROSE_ASSERT(stmt);
  DepGraphNode *result = map_[stmt];
  if (!result) {
    result = new DepGraphNode(this, root_, stmt);
    AddNode(result);
  }
  return result;
}

DepGraphNode *DepGraphBuilder::CreateNode(SgNode *stmt, const DepGraphNode *that)
{
  ROSE_ASSERT(stmt);
  ROSE_ASSERT(that);

  ROSE_ASSERT(root_ == that->root());

  DepGraphNode *result = map_[stmt];
  if (!result) {
    result = new DepGraphNode(this, root_, stmt, that->loopMap(), that->domain());
    AddNode(result);
  }
  return result;
}

//bool
//DepGraphBuilder::DeleteNode(DepGraphNode *node)
//{ return Base::DeleteNode(node); }

//DepInfoEdge*
//DepGraphBuilder::CreateEdge(DepGraphNode *source, DepGraphNode *target, const DepInfo &info)
//{ return Base::CreateEdge(source, target, info); }

DepInfoEdge *DepGraphBuilder::CreateEdgeFromOrigAst(DepGraphNode *source, DepGraphNode *target, const DepInfo &info)
{
  ROSE_ASSERT(source);
  ROSE_ASSERT(target);
  DepInfo edge_info = source->isLoop() || source->loopMap().IsID() ? info :
                      ::Reverse(source->loopMap()) * info;
  if (!target->isLoop() && !target->loopMap().IsID())
    edge_info = edge_info * target->loopMap();
  return CreateEdge(source, target, edge_info);
}

// EOF
