#ifndef _ROSEX_DEPGRAPHBUILDER_P_H
#define _ROSEX_DEPGRAPHBUILDER_P_H

// depgraphbuilder_p.h
// 7/14/2011 jichi
// This header is for internal use only.

#include "rosex/depgraphnode_p.h"
#include <DepGraph.h>
#include <boost/unordered_map.hpp>

class SgNode;

///  \internal  Represent the dependency graph, used to create the graph.
class DepGraphBuilder : public DepInfoGraphCreate<DepGraphNode>
{
  typedef DepGraphBuilder                   Self;
  typedef DepInfoGraphCreate<DepGraphNode>  Base;
  typedef boost::unordered_map<SgNode*, DepGraphNode*>  Map; ///< map from AST to DepNode

  SgNode *root_;
  Map map_;

  ///   Graph constructions
public:
  explicit DepGraphBuilder(SgNode *root = nullptr)
    : root_(root) { }

  /**
   *  \brief  AST root used to compute loop information.
   *
   *  If it is NULL, the root for the entire AST of the source file rather than
   *  root for DepGraph is used.
   */
  SgNode *root() const { return root_; }
  void setRoot(SgNode *root) { root_ = root; }

  ///  Map from pointer to AST statement to the corresponding DepGraphNode.
  const Map &map() const { return map_; }

  /**
   *  \name  Nodes and edges
   *
   *  Conventions:
   *  - All parameters passed to the methods must be valid.
   */
public:
  void AddNode(DepGraphNode*);
  //bool DeleteNode(DepGraphNode*);

  DepGraphNode *CreateNode(SgNode*);
  DepGraphNode *CreateNode(SgNode*, const DepGraphNode*);
  DepGraphNode *CreateNode(SgNode*, const DomainCond&);
  DepGraphNode *CreateNode(SgNode*, const DepInfo&, const DomainCond&);

  //DepInfoEdge *CreateEdge(DepGraphNode*, DepGraphNode*, const DepInfo&);
  DepInfoEdge *CreateEdgeFromOrigAst(DepGraphNode*, DepGraphNode*, const DepInfo&);
};

#endif //_ROSEX_DEPGRAPH_P_H
