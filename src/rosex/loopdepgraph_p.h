#ifndef _ROSEX_LOOPDEPGRAPH_P_H
#define _ROSEX_LOOPDEPGRAPH_P_H

// loopdepgraph_p.h
// 4/16/2014 jichi

#include "rosex/depgraph_p.h"
//#include <LoopTreeDepComp.h>
#include <TransDepGraph.h>
#include <TransDepGraphImpl.h>

class LoopDepGraphPrivate
{
public:
  typedef TransDepGraphCreate<DepGraphNode> Graph;
  Graph *graph;

  LoopDepGraphPrivate() : graph(nullptr) {}
  ~LoopDepGraphPrivate() { clear(); }

  void clear() { if (graph) { delete graph; graph = nullptr; } }
};

#endif // _ROSEX_LOOPDEPGRAPH_P_H
