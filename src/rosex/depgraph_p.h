#ifndef _ROSEX_DEPGRAPH_P_H
#define _ROSEX_DEPGRAPH_P_H

// depgraph_p.h
// 6/13/2011 jichi
// Internal header for depgraph.

#include "rosex/depgraphbuilder_p.h"

class SgNode;

///  \internal  DepGraph public option
struct DepGraphOption;

///  \internal  DepGraph private data
struct DepGraphPrivateData;

///  \internal  DepGraph private data
struct DepGraphPrivate
{
  // Actual types:
  // - Node:  class DepGraphNode
  // - Graph:   class DepGraphBuilder
  // - Vertex:  class SgNode
  // - Edge:  class DepInfoEdge
  // - VertexIterator:  class IteratorWrap<Vertex*, IteratorImpl<Vertex*> >
  // - EdgeIterator:  class IteratorWrap<Edge*, IteratorImpl<Vertex*> >
  //

  // Map between AST tree node and dependency graph vertex
  typedef DepGraphNode      Node;     // Implementation strategy
  typedef DepGraphBuilder   Graph;    // Dependency graph builder

  typedef Graph::Node Vertex;
  typedef Graph::Edge Edge;
  typedef Graph::NodeIterator VertexIterator;
  typedef Graph::EdgeIterator EdgeIterator;

  Graph graph; // Representation of the dependency graph
  DepGraphPrivateData *data;
  const DepGraphOption *option;

  explicit DepGraphPrivate(const DepGraphOption *option = nullptr);
  ~DepGraphPrivate();

  bool build(SgNode *root);
};

#endif // _ROSEX_DEPGRAPH_P_H
