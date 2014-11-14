// loopdepgraph.cc
// 4/16/2014 jichi
// See: rose/src/midend/programTransformation/loopProcessing/depGraph/LoopAnalysis.{h,C}
// See: rose/src/midend/programTransformation/loopProcessing/driver/FusionAnal.{h,C}

#include "rosex/rosex.h"
#include "rosex/slice_p.h"
#include "rosex/loopdepgraph.h"
#include "rosex/loopdepgraph_p.h"
#include <DepInfo.h> // for DepInfoSetEdge
#include <boost/foreach.hpp>

#define foreach BOOST_FOREACH

#define SK_DEBUG "loopdepgraph"
#include "sk/skdebug.h"

// Construction

LoopDepGraph::LoopDepGraph(SgNode *root, const option_type *option)
//  : Base(root, option), d_(new D) {} // This will crash since d_ is null
  : d_(new D)
{
  if (root)
    init(root, option);
}

LoopDepGraph::~LoopDepGraph() { delete d_; }

bool LoopDepGraph::init(SgNode *root, const option_type *option)
{
  d_->clear();
  if (Base::init(root, option)) {
    d_->graph = new D::Graph(&Base::data()->graph);

    //D::Graph::NodeIterator p = Base::data()->graph.GetNodeIterator();
    //while (!p.ReachEnd()) {
    //  std::cerr << p.Current() <<":" << p.Current()->isLoop()<< std::endl;
    //  ++p;
    //}
  }
  return d_->graph;
}

void LoopDepGraph::dumpLoopGraph() const
{
  SK_DPRINT("enter");
  if (d_->graph)
    d_->graph->Dump();
  SK_DPRINT("leave");
}

// Queries

bool LoopDepGraph::fusible(const SgNode *upperLoop, const SgNode *lowerLoop) const
{
  if (!d_->graph)
    return false;

  SK_DPRINT("enter");

  foreach (edge_type e, edges()) {
    vertex_type s = source(e),
                t = target(e);
    if (rosex::isAncestor(s.statement(), upperLoop) &&
        rosex::isAncestor(t.statement(), lowerLoop) &&
        distance(s, t) < 0) {
      SK_DPRINT("leave: ret = false");
      return false;
    }
  }
  SK_DPRINT("leave: ret = true");
  return true;
}

int LoopDepGraph::distance(vertex_type v1, vertex_type v2) const
{
  if (!d_->graph)
    return 0;
  DepGraphNode *n1 = v1.data(),
               *n2 = v2.data();
  if (!n1 || !n2) {
    SK_DPRINT("warning: loop node does not exist");
    return 0;
  }

  int level1 = v1.level(),
      level2 = v2.level();

  DepInfoSetEdge *td12 = d_->graph->GetTransDep(n1, n2),
                 *td21 = d_->graph->GetTransDep(n2, n1);

  int mina = 1,
      maxa = -1;
  if (td12) {
    DepRel r12 = td12->GetInfo().GetDepRel(level1, level2);
    DepDirType dir = r12.GetDirType();
    if (dir == DEPDIR_LE || dir == DEPDIR_EQ)
      maxa = mina = td12->GetInfo().GetDepRel(level1, level2).GetMaxAlign();
  }
  if (mina <= maxa && td21) {
    DepRel r21 = td21->GetInfo().GetDepRel(level2, level1);
    DepDirType dir = r21.GetDirType();
    if (dir == DEPDIR_LE || dir == DEPDIR_EQ)
      maxa = - td21->GetInfo().GetDepRel(level2, level1).GetMaxAlign();
    else
      maxa = mina -1;
  }
  SK_DPRINT("mina = " << mina << ", maxa = " << maxa);
  return maxa - mina;
  //return mina <= maxa;
  //return LoopAlignInfo(mina,maxa);
}


// EOF

/*
template <typename Node>
struct LoopAnalInfo
{
  Node *n;
  int index;
  LoopAnalInfo(Node *n, int _level) : n(n), index(_level) {}
};

struct LoopAlignInfo {
  int mina, maxa;
  LoopAlignInfo() : mina(1), maxa(-1) {}
  LoopAlignInfo(int mina, int maxa) : mina(mina), maxa(maxa) {}

  operator bool() { return mina <= maxa; }

  void operator &= ( const LoopAlignInfo &that)
  {
    if (mina < that.mina) mina = that.mina;
    if (maxa > that.maxa) maxa = that.maxa;
  }

  void set(int m1, int m2) { mina  = m1; maxa = m2; }
};
*/
