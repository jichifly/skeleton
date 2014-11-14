#ifndef _ROSEX_LOOPDEPGRAPH_H
#define _ROSEX_LOOPDEPGRAPH_H

// loopdepgraph.h
// 4/16/2014 jichi
#include "rosex/depgraph.h"

class LoopDepGraphPrivate;
class LoopDepGraph : public DepGraph
{
  typedef LoopDepGraph Self;
  typedef LoopDepGraphPrivate D;
  typedef DepGraph Base;

  D *d_;
public:
  // Construction
  explicit LoopDepGraph(SgNode *root = nullptr, const option_type *option = nullptr);
  ~LoopDepGraph();

  bool init(SgNode *root, const option_type *option = nullptr) override;

  D *data() const { return d_; } // for debugging purpose

  void dumpLoopGraph() const;

  // Queries

  /**
   * @param  upperLoop  upper loop in the code
   * @param  lowerLoop  lower loop in the code
   * @return  if two loops are fusible
   */
  bool fusible(const SgNode *upperLoop, const SgNode *lowerLoop) const;

  /**
   * @param  v1  vertex in the upper loop
   * @param  v2  vertex in the lower loop
   * @return  distance between index variables
   */
  int distance(vertex_type v1, vertex_type v2) const;
};


#endif // _ROSEX_LOOPDEPGRAPH_H
