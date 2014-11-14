#ifndef _ROSEX_DEPGRAPH_H
#define _ROSEX_DEPGRAPH_H

// depgraph.h
// 6/13/2011 jichi
//
// This file defines interface for using ROSE dependency graph
//
// The main components defined in this file are:
// - Implementation adaptors that are not part of the ROSE interface
//   - DepGraphImpl: Hold the underlying representation of the graph to build
// - Interface of Dependency Graph:
//   graph := (vertex, edge, vertex_iterator, edge_iterator)
//   - DepGraphVertex
//   - DepGraphEdge
//   - DepGraphVertexIterator
//   - DepGraphEdgeIterator
//   - DepGraph: Interface of the dependency graph
//
// The dependency information are saved in DepInfo from "DepInfo.h" in each vertex and edge.
//

#include "rosex/rosex.h"
#include "rosex/depgraph_p.h"
#include <boost/foreach.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/properties.hpp>
#include <boost/iterator/iterator_categories.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/unordered/unordered_map.hpp>

#include <boost/typeof/typeof.hpp>
#include "xt/typeof.h"

#include <iterator>
#include <string>

class SgNode;

struct DepGraphPrivate;

/* defined in DepInfo.h
typedef enum {
  DEPTYPE_NONE = 0,     // B00000000
  DEPTYPE_TRUE = 1,     // B00000001
  DEPTYPE_ANTI = 2,     // B00000010
  DEPTYPE_OUTPUT = 4,   // B00000100
  DEPTYPE_SCALAR = 8,   // B00001000
  DEPTYPE_BACKSCALAR = 16,  // B00010000
  DEPTYPE_INPUT = 32,   // B00100000
  DEPTYPE_ARRAY = 39,   // B00100111 //union of true, anti, output, input dependencies
  DEPTYPE_IO = 64,    // B01000000
  DEPTYPE_DATA = 95,    // B01011111 //union of array, scalar/backscalar,IO,w/o input
  DEPTYPE_CTRL = 128,   // B10000000
  DEPTYPE_BACKCTRL = 256,   //B1 00000000
  DEPTYPE_ALL = 479,    //B1 11011111 //All but input dependence
  DEPTYPE_TRANS = 512   //B10 00000000
} DepType;
std::string DepType2String( DepType t);

//enum DepDirection {DEP_SRC = 1, DEP_SINK = 2, DEP_SRC_SINK = 3};
*/

// DepGraphOption
/**
 *  \brief  Options to build the dependency graph.
 *
 *  Also Defined as \c DepGraph::option_type.
 *  These options could be used to build the dendency graph, including
 *  - aliasInfo     Array alias (default is AssumeNoAlias)
 *  - funcInfo      Function side effect
 *  - stmtInfo      Statement side effect
 *  - arrayInfo     Array interface
 *
 *  The default value is nothing.
 */
class AliasAnalysisInterface;
class FunctionSideEffectInterface;
class ArrayAbstractionInterface;
class SideEffectAnalysisInterface;
struct DepGraphOption
{
  AliasAnalysisInterface        *aliasInfo;
  FunctionSideEffectInterface   *funcInfo;
  SideEffectAnalysisInterface   *stmtInfo;
  ArrayAbstractionInterface     *arrayInfo;

  explicit DepGraphOption(
      AliasAnalysisInterface        *alias = nullptr,
      FunctionSideEffectInterface   *func = nullptr,
      SideEffectAnalysisInterface   *stmt = nullptr,
      ArrayAbstractionInterface     *array = nullptr)
    : aliasInfo(alias), funcInfo(func), stmtInfo(stmt), arrayInfo(array) { }
};

//class FunctionSideEffect : public FunctionSideEffectInterface
//{
//public:
//  ///  Implement the interface.
//  virtual bool get_read(AstInterface &dummy, const AstNodePtr &function,
//                        CollectObject<AstNodePtr> *collect = 0);
//  ///  Implement the interface.
//  virtual bool get_modify(AstInterface &dummy, const AstNodePtr &function,
//                          CollectObject<AstNodePtr> *collect = nullptr);
//};

// DepGraphVertex
/**
 *  \brief  Dependency graph vertex.
 *
 *  Represents the vertex in the dependency graph.
 *  Defined as DepGraph::vertex_type.
 *
 *  Constructions:
 *  - Returned from functions in DepGraph
 *  - Returned from functions in DepGraph::vertex_iterator
 *  Uninitialized/empty vertex is referred as bad vertex.
 *
 *  Properties saved in the vertex:
 *  - depInfo  dependency information
 *  - node   pointer to correnspoinding AST node
 *  - level  loop level in the AST
 *  These properties are immutable.
 *
 *  As most methods are const, the user are not supposed to directly modify
 *  the vertex.
 */
class DepGraphVertex
{
  typedef DepGraphVertex Self;
  typedef DepGraphPrivate::Vertex D;
  D *d_;

  // - General Vertex Methods -
public:
  ///  \internal  Create an empty instance or wrap with its implementation
  DepGraphVertex(D *d = nullptr) : d_(d) { }
  DepGraphVertex(const Self &that) : d_(that.d_) { }

  Self &operator=(const Self &that) { d_ = that.d_; return *this; }
  bool operator==(const Self &that) const { return d_ == that.d_; }
  operator bool() const { return valid(); }

   ///  \internal  Convert to the private date.
  operator D*() const { return d_; }

  ///  Comparison to underlying pointer, rather than higher level order.
  // This function is provide to resolve ambiguity for std::less.
  bool operator<(const Self &that) const { return d_ < that.d_; }

  ///  Unparse the AST node to string using SgNode::unparseToString
  std::string toString() const;

  ///  Return false for null vertex.
  bool valid() const { return d_; }

  ///  Clear the data.
  void clear() { reset(); }

  ///  \internal  Return the private data pointer.
  D *data() const { return d_; }
  ///  \internal  Reset the implementation.
  void reset(D *d = nullptr) { d_ = d; }

  // - Dependency Information -
public:
  ///  Properties saved in the vertex.

  ///  Return the dependency information, or DepInfo() for bad vertex
  const DepInfo &depInfo() const;

  ///  Return pointer to AST node, or nullptr for bad vertex
  SgNode *statement() const;

  ///  Level of loop nest in the AST starting from 0, or 0 for bad vertex
  int level() const;

  /**
   *  \brief  Return if the node is identified as a Fortran-style loop.
   *
   *  A Fortran-style loop is a loop in form of (start, stop, step).
   *  For C/C++, supported loop that could be analyzed should be in this form:
    <code>
    for (var = start; var CMP stop; var INC step)
      statements;
    </code>
   *  Where CMP must be one of "<=", ">=", "==", and INC be either "+=" or "-=".
   *  Notice that "<", ">", and "++", "--" are not supported yet.
   *  It is better to normalize the loop before invoke dependency test.
   */
  bool isLoop() const;
};

// DepGraphEdge
/**
 *  \brief  Dependency graph edge.
 *
 *  Represents the edge in the dependency graph.
 *  Defined as DepGraph::edge_type.
 *
 *  Constructions:
 *  - Returned from functions in DepGraph
 *  - Returned from functions in DepGraph::edge_iterator
 *  Uninitialized/empty edge is referred as bad edge.
 *
 *  Properties saved in the vertex:
 *  - depInfo  dependency information
 *  - depType  type of the dependency
 *  - depTypeName  depType to string
 *   - sourceNode   pointer to AST node of the source vertex
 *  - targetNode   pointer to AST node of the target vertex
 *  These properties are immutable.
 *
 *  As most methods are const, the user are not supposed to directly modify
 *  the edge.
 */
class DepGraphEdge
{
  typedef DepGraphEdge Self;
  typedef DepGraphPrivate::Edge D;

  D *d_;

  // - General edge methods -
public:
  ///  \internal  Create an empty instance or wrap with its implementation
  DepGraphEdge(D *d = nullptr) : d_(d) { }
  DepGraphEdge(const Self &that) : d_(that.d_) { }

  Self &operator=(const Self &that) { d_ = that.d_; return *this; }
  bool operator==(const Self &that) const { return d_ == that.d_; }
  operator bool() const { return valid(); }

  ///  \internal  Return the data pointer.
  operator D*() const { return d_; }

  ///  Comparison to underlying pointer, rather than higher level order.
  // This function is provide to resolve ambiguity for std::less.
  bool operator<(const Self &that) const { return d_ < that.d_; }

  ///  Dump dependency information in the edge using DepInfoEdge::toString
  std::string toString() const;

  ///  Return false for null edge
  bool valid() const { return d_; }

  ///  Clear the data.
  void clear() { reset(); }

  ///  \internal  Return the private data pointer.
  D *data() const { return d_; }

  ///  \internal  Reset implementation
  void reset(D *d = nullptr) { d_ = d; }

  // - Dependency information -
public:
  ///  Properties saved in the edge.

  ///  Return dependency information, or DepInfo() for bad edge
  const DepInfo &depInfo() const;

  ///  Shortcut DepInfo::DepType(), or DEPTYPE_NONE for bad edge
  DepType depType() const;

  ///  Return string format of depType
  std::string depTypeName() const { return ::DepType2String(depType()); }

  ///  Return source vertex's AST node, shortcut of casting depInfo::SrcRef()
  SgNode *sourceExpression() const;

  ///  Return target vertex's AST node, shortcut of casting depInfo::SnkRef()
  SgNode *targetExpression() const;
};

// DepGraphVertexItertor
/**
 *  \brief  Const forward iterator for traversing vertices in DepGraph
 *
 *  Represents the iterator for vertex in the dependency graph.
 *  Defined as DepGraph::vertex_iterator.
 *
 *  It is not possible to modify the vertex it pointed to.
 *
 *  Constructions:
 *  - Returned from functions in DepGraph
 *  Uninitialized, empty iterators, and those reach the end of the iteration
*   are referred as bad iterator.
 */
class DepGraphVertexIterator : private DepGraphPrivate::VertexIterator
{
  typedef DepGraphVertexIterator Self;
  typedef DepGraphPrivate::VertexIterator Base;

  // Wrapper in order to provide reference value and avoid null pointer
  DepGraphVertex value_;

  // - Adapting types -
public:
  // Consistent with std::iterator_traits<Self>
  typedef ptrdiff_t       difference_type;
  typedef DepGraphVertex  value_type;
  typedef const value_type* pointer;
  typedef const value_type &reference;

  typedef boost::single_pass_traversal_tag iterator_category;

  // - Underlying iterator operations -
protected:
  ///  Return false if the iterator is null or has reached the end
  bool valid() const;
  ///  Current value the iterator pointed to
  value_type current() const;
  ///  Assign a new base
  void assign(const Base &b);
  ///  ++iterator
  void advance();
  ///  Return true if wrapped with the same pointer
  bool equal(const Self &that) const;

  // - General iterator methods -
public:
  ///  Create an empty iterator.
  DepGraphVertexIterator() { }
  DepGraphVertexIterator(const Self &that) : Base(that), value_(that.value_) { }

  ///  \internal
  DepGraphVertexIterator(const Base &b) : Base(b), value_(current()) { }

  Self &operator=(const Self &that)
  { assign(that); value_ = that.value_; return *this; }

  reference operator*() const { return value_; }
  pointer operator->() const { return &value_; }

  Self &operator++() { advance(); return *this; }
  Self operator++(int) { Self t = *this; advance(); return t; }
  bool operator==(const Self &that) const { return equal(that); }
  operator bool() const { return valid(); }
};

// DepGraphEdgeItertor
/**
 *  \brief  Const forward iterator for traversing edges in DepGraph
 *
 *  Represents the iterator for edge in the dependency graph.
 *  Defined as DepGraph::edge_iterator.
 *
 *  It is not possible to modify the edge it pointed to.
 *
 *  Constructions:
 *  - Returned from functions in DepGraph
 *  Uninitialized, empty iterators, and those reach the end of the iteration
 *  are referred as bad iterator.
 */
class DepGraphEdgeIterator : private DepGraphPrivate::EdgeIterator
{
  typedef DepGraphEdgeIterator Self;
  typedef DepGraphPrivate::EdgeIterator Base;

  // Wrapper in order to provide reference value and avoid null pointer
  DepGraphEdge value_;

  // - Adapting types -
public:
  // Consistent with std::iterator_traits<Self>
  typedef ptrdiff_t       difference_type;
  typedef DepGraphEdge    value_type;
  typedef const value_type* pointer;
  typedef const value_type &reference;

  typedef boost::single_pass_traversal_tag iterator_category;

  // - Underlying iterator operations -
protected:
  ///  False if the iterator is null or has reached the end
  bool valid() const;
  ///  Current value the iterator pointed to
  value_type current() const;
  ///  Assign a new base
  void assign(const Base &b);
  ///  ++iterator
  void advance();
  bool equal(const Self &that) const; ///< true if wrapped with the same pointer

  // - General iterator methods -
public:
  DepGraphEdgeIterator() { }  ///< Make an empty iterator.
  DepGraphEdgeIterator(const Self &that) : Base(that), value_(that.value_) { }

  ///  \internal
  DepGraphEdgeIterator(const Base &b) : Base(b), value_(current()) { }

  Self &operator=(const Self &that)
  { assign(that); value_ = that.value_; return *this; }

  reference operator*() { return value_; }
  pointer operator->() { return &value_; }

  Self &operator++() { advance(); return *this; }
  Self operator++(int) { Self t = *this; advance(); return t; }
  bool operator==(const Self &that) const { return equal(that); }
  operator bool() const { return valid(); }
};

// DepGraph
/**
 *  \brief  Interface to create and traverse dependency graph.
 *
 *  Types:
 *  - vertex_type     holding immutable vertex, see DepGraphVertex
 *  - edge_type       holding immutable edge, see DepGraphEdge
 *  - vertex_iterator   const forward iterator, see DepGraphVertexIterator
 *  - edge_iterator   const forward iterator, see DepGraphEdgeIterator
 *
 *  In the current implementation, vertex_type and edge_type are just
 *  only holding pointers to the underlying implementation.
 *  They serve as the descriptors of their implementations.
 *  It does not cost more for copying vertex_type/edge_type instance
 *  comparing to using const reference.
 *
 *  Constructions:
 *  - Build the graph using its constructor
 *  - Build the graph by invoking build()
 *  Copy construction is implemented in an efficient way.
 *  Uninitialized/empty instance is referred as a gad graph.
 *
 *  Notice that vertices_begin() is the same as the entry of the graph,
 *  whose node in the AST is the root provided to the build function.
 *
 *  Conventions:
 *  - Return 0 in counting methods for bad graph.
 */
class DepGraph
{
  typedef DepGraph Self;
  typedef DepGraphPrivate D;

  // Pimp private data pointer
  boost::shared_ptr<D> d_;

  // - Adapting types -
public:
  // Consistent with boost::graph_traits<Self>
  typedef boost::directed_tag     directed_category;

  typedef DepGraphOption        option_type;

  typedef DepGraphVertex        vertex_type;
  typedef DepGraphEdge        edge_type;
  typedef DepGraphVertexIterator    vertex_iterator;
  typedef DepGraphEdgeIterator    in_edge_iterator;
  typedef in_edge_iterator      out_edge_iterator;
  typedef std::list<edge_type>::iterator  edge_iterator;

  static vertex_type null_vertex()  { return vertex_type(); }
  static edge_type   null_edge()  { return edge_type(); }

  // - Graph constructions -
  /**
   *  \name  Graph constructions
   *
   *  Graph constructions, status queries, and output methods.
   */
public:

  ///  Build a graph from an AST node.
  explicit DepGraph(SgNode *root = nullptr, const option_type *option = nullptr)
  { if (root) init(root, option); }

  ///  Create a graph that holds the POINTER to the SAME graph of \p that.
  explicit DepGraph(const Self &that) : d_(that.d_) { }

  virtual ~DepGraph() {}

  ///  Swap the implementation.
  void swap(Self &that) { d_.swap(that.d_); }

  /**
   *  \brief  Build the dependency graph from the sub AST rooted at \p root.
   *
   *  The \p root of the AST could be an function definition, or anything within
   *  the body of a function definition.
   *
   *  If building failed, calling valid() will return false.
   *
   *  \param  root  Pointed to root of the AST to build.
   *  \param  option  Options used to build the graph.
   *  \return  true only if building succeed.
   */
  virtual bool init(SgNode *root, const option_type *option = nullptr);

  ///  Delete the graph, same as reset().
  void clear()
  { reset(); }

  ///  True if the instance is holding a valid implementation.
  bool valid() const { return d_; }

  ///  Output the graph to DOT format.
  void toDOT(const std::string &filename, const std::string &graphname = "DepGraph") const;

  ///  Output the graph to plain text.
  template <typename OutputStream>
  inline void dump(OutputStream &out) const;

  void dump(std::ostream &out = std::cerr) const
  { dump<std::ostream>(out); }

  std::string toString() const ;

  boost::adjacency_list<
  boost::vecS, boost::vecS, boost::bidirectionalS,
  vertex_type, edge_type
  >
  to_boost_adjacency_list() const; ///< Convert to boost::adjacency_list graph

public:
  ///  \internal Only for debug use. Return the implementation.
  D *data() const { return d_.get(); }

  ///  \internal  Reset the implementation.
  void reset(D *d = nullptr) { d_.reset(d); }

  // - Vertices and edges -
  /**
   *  \name Vertices and edges
   *
   *  Methods involving vertices and edges to traverse the graph.
   *  Note that all methods are const.
   */
public:
  ///  Return the first vertex in the graph.
  vertex_type entry() const
  { return valid()? *vertices_begin() : null_vertex(); }

  ///  Return the vertex in the dependency graph, or null_vertex if not exist
  vertex_type find(const SgNode *stmt) const;

  ///  Returns if vertex \p v is in the graph
  bool contains(vertex_type v) const;

  ///  Returns if edge \p e is in the graph
  bool contains(edge_type v) const;

  ///  Return if edge (\a u, \p v) is in the graph
  bool contains(vertex_type u, vertex_type v) const;

  bool contains(std::pair<vertex_type, vertex_type> e) const
  { return contains(e.first, e.second); }

  ///  Return (begin, end) pair to traverse all vertices in the graph
  std::pair<vertex_iterator, vertex_iterator> vertices() const
  { return std::make_pair(vertices_begin(), vertices_end()); }

  ///  Begin of the vertices
  vertex_iterator vertices_begin() const;
  ///  Bnd of the vertices
  vertex_iterator vertices_end() const;

  ///  Return (begin, end) pair to traverse in-edges of vertex \p v
  std::pair<in_edge_iterator, in_edge_iterator> in_edges(vertex_type v) const
  { return std::make_pair(in_edges_begin(v), in_edges_end(v)); }

  ///  Begin of in-edges
  in_edge_iterator in_edges_begin(vertex_type v) const;
  ///  End of in-edges
  in_edge_iterator in_edges_end(vertex_type v) const;

  ///  Return (begin, end) pair to traverse out-edges of vertex \p v
  std::pair<out_edge_iterator, out_edge_iterator> out_edges(vertex_type v) const
  { return std::make_pair(out_edges_begin(v), out_edges_end(v)); }

  ///  Begin of out-edges
  out_edge_iterator out_edges_begin(vertex_type v) const;
  ///  End of out-edges
  out_edge_iterator out_edges_end(vertex_type v) const;

  ///  Return all edges, expensive to compute.
  std::pair<edge_iterator, edge_iterator> edges() const;

  ///  Source vertex of the edge \p e.
  vertex_type source(edge_type e) const;
  ///  Target vertex of edge \p e.
  vertex_type target(edge_type e) const;

  ///  In-degree of the vertex \p v.
  size_t in_degree(vertex_type v) const;
  ///  Out-degree of the vertex \p v.
  size_t out_degree(vertex_type v) const;

  ///  Total number of vertices, or 0 for bad graph.
  size_t num_vertices() const;
  ///  Total number of edges, or 0 for bad graph.
  size_t num_edges() const;

  bool has_vertices() const { return vertices_begin(); }
  bool has_in_edges(vertex_type v) const { return in_edges_begin(v); }
  bool has_out_edges(vertex_type v) const { return out_edges_begin(v); }
  bool has_edges() const;

  ///  Return indices of vertex
  boost::associative_property_map<
  boost::unordered_map<vertex_type, int>
  >
  indices() const;
};

// DepGraphPropertiesWriter
/**
 *  \brief  Properties writer for vertex and edge of dependency graph.
 *
 *  Here's an example to generate DOT graph using <boost/graph/graphviz.hpp>:
  <code>
    std::ofstream fout(some_file);
    DepGraph g(some_node);
    DepGraphPropertiesWriter vw, ew;
    boost::write_graphviz(fout, g, vw, ew);
  </code>
 */
struct DepGraphPropertiesWriter
{
  template <typename Descriptor, typename OutputStream>
  void operator()(OutputStream &out, Descriptor d)
  { out << "[label=\"" << d.toString() << "\"]"; }
};

// Specialize STL iterator_traits for graph iterators:
namespace std {

  template <>
  struct iterator_traits<DepGraphVertexIterator>
  {
  typedef DepGraphVertexIterator      iterator;
  typedef iterator::difference_type   difference_type;
  typedef iterator::value_type        value_type;
  typedef iterator::pointer           pointer;
  typedef iterator::reference         reference;

  typedef forward_iterator_tag        iterator_category;
  };

  template <>
  struct iterator_traits<DepGraphEdgeIterator>
  {
  typedef DepGraphEdgeIterator        iterator;
  typedef iterator::difference_type   difference_type;
  typedef iterator::value_type        value_type;
  typedef iterator::pointer           pointer;
  typedef iterator::reference         reference;

  typedef forward_iterator_tag        iterator_category;
  };

} // namespace std

// Specialize and extend boost graph utilities:
namespace boost {

  // - Graph traits -

  template <>
  struct graph_traits<DepGraph> {
  typedef DepGraph G;

  typedef G::directed_category directed_category;

  // vertex_type and edge_type are used as their own descriptor,
  // since they are holding pointers to their vertex/edge representations.
  typedef G::vertex_type       vertex_descriptor;
  typedef G::edge_type         edge_descriptor;

  // in_edge_iterator and out_edge_iterator are the same.
  typedef G::vertex_iterator   vertex_iterator;
  typedef G::edge_iterator     edge_iterator;
  typedef G::in_edge_iterator  in_edge_iterator;
  typedef G::out_edge_iterator out_edge_iterator;

  typedef size_t vertices_size_type;
  typedef size_t edges_size_type;
  typedef size_t degree_size_type;

  static vertex_descriptor null_vertex() { return G::null_vertex(); }
  static edge_descriptor   null_edge()   { return G::null_edge(); }
  };

} // namespace boost

typedef boost::graph_traits<DepGraph> DepGraphTraits; ///< \internal

namespace boost {

  // - Graph traversal -

  inline DepGraphTraits::vertex_descriptor
  source(DepGraphTraits::edge_descriptor e, const DepGraph &g)
  { return g.source(e); }

  inline DepGraphTraits::vertex_descriptor
  target(DepGraphTraits::edge_descriptor e, const DepGraph &g)
  { return g.target(e); }

  inline std::pair<
  DepGraphTraits::vertex_iterator,
  DepGraphTraits::vertex_iterator
  >
  vertices(const DepGraph &g)
  { return g.vertices(); }

  inline std::pair<
  DepGraphTraits::edge_iterator,
  DepGraphTraits::edge_iterator
  >
  edges(const DepGraph &g)
  { return g.edges(); }

  inline std::pair<
  DepGraphTraits::in_edge_iterator,
  DepGraphTraits::in_edge_iterator
  >
  in_edges(DepGraphTraits::vertex_descriptor v, const DepGraph &g)
  { return g.in_edges(v); }

  inline std::pair<
  DepGraphTraits::out_edge_iterator,
  DepGraphTraits::out_edge_iterator
  >
  out_edges(DepGraphTraits::vertex_descriptor v, const DepGraph &g)
  { return g.out_edges(v); }


  inline DepGraphTraits::degree_size_type
  in_degree(DepGraphTraits::vertex_descriptor v, const DepGraph &g)
  { return g.in_degree(v); }

  inline DepGraphTraits::degree_size_type
  out_degree(DepGraphTraits::vertex_descriptor v, const DepGraph &g)
  { return g.out_degree(v); }

  inline DepGraphTraits::vertices_size_type
  num_vertices(const DepGraph &g)
  { return g.num_vertices(); }

  inline DepGraphTraits::edges_size_type
  num_edges(const DepGraph &g)
  { return g.num_edges(); }

  // There is no edges(g) function implemented
  // There is no vertex() / edge() builder implemented.

  // Specialize range iterator type for boost foreach.
  /*
  template <>
  struct range_mutable_iterator<std::pair<DepGraph::vertex_iterator, DepGraph::vertex_iterator> >
  { typedef DepGraphTraits::vertex_iterator type; };

  template <>
  struct range_const_iterator<std::pair<DepGraph::vertex_iterator, DepGraph::vertex_iterator> >
  { typedef DepGraphTraits::vertex_iterator type; };

  template <>
  struct range_mutable_iterator<std::pair<DepGraph::edge_iterator, DepGraph::edge_iterator> >
  { typedef DepGraphTraits::edge_iterator type; };

  template <>
  struct range_const_iterator<std::pair<DepGraph::edge_iterator, DepGraph::edge_iterator> >
  { typedef DepGraphTraits::edge_iterator type; };

  namespace foreach {
  template <>
  struct is_lightweight_proxy<std::pair<DepGraph::edge_iterator, DepGraph::edge_iterator>  >
    : mpl::true_ { };
  }

  // Because std::pair is always valid, there is no need to check.
  namespace foreach {
  template <>
  struct is_lightweight_proxy<
    std::pair<DepGraph::vertex_iterator, DepGraph::vertex_iterator>
    >
    : mpl::true_ { }; // always return true_

  template <>
  struct is_lightweight_proxy<
    std::pair<DepGraph::edge_iterator, DepGraph::edge_iterator>
    >
    : mpl::true_ { }; // always return true_
  }
  */

  // - Properties -

  template <>
  struct property_map<DepGraph, vertex_index_t>
  {
    typedef typeof(DepGraph().indices()) type;
    typedef type const_type;
  };

  inline property_map<DepGraph, vertex_index_t>::type
  get(vertex_index_t, const DepGraph &g)
  { return g.indices(); }

} // namespace boost

// Extend STL-style output stream:
template <typename OutputStream>
inline OutputStream&
operator<<(OutputStream &out, const DepGraphVertex &v)
{ return out << v.toString(); }

template <typename OutputStream>
inline OutputStream&
operator<<(OutputStream &out, const DepGraphEdge &e)
{ return out << e.toString(); }

template <typename OutputStream>
inline OutputStream&
operator<<(OutputStream &out, const DepGraph &g)
{ g.dump(out); }

// Template instantiations:

#include <GraphIO.h> // in rose, needed by write_graph
template <typename OutputStream>
inline void
DepGraph::dump(OutputStream &out) const
{
  //write_graph(d_->graph);
  if (!valid()) {
    out << "error: bad denpendency graph.\n";
    return;
  }

  vertex_type v;
  edge_type e;

  // for each vertex in the dependency graph
  BOOST_FOREACH (v, vertices())
  if (out_degree(v) == 0) // if no edges, output vertex only
    out << v << " => (none)" << "\n";
  else // otherwise, output source vertex, target vertex, and the edge
    BOOST_FOREACH (e, out_edges(v))
      out << v << " => " << target(e) << " => " << e << "\n";
}

#endif // _ROSEX_DEPGRAPH_H
