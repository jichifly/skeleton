#ifndef _ROSEX_DEFUSEGRAPH_H
#define _ROSEX_DEFUSEGRAPH_H

// IVDefUseGraph.h
// Since 7/5/2011, by jichi (jguo@cs.utsa.edu)
// Graph of define-use chain.
//
// The main components defined in this file are:
//
// + Implementation adaptors that are not part of the ROSE interface
//   - DefUseGraphImpl: Hold the underlying representation of the graph to build
//
//
// + Interface of define-use graph:
//   graph := (vertex, edge, vertex_iterator, edge_iterator)
//   - DefUseGraphVertex
//   - DefUseGraphEdge
//   - DefUseGraphVertexIterator
//   - DefUseGraphEdgeIterator
//   - DefUseGraphOption: Options to build the graph
//   - DefUseGraph: Interface of the define-use graph
//


#include <DefUseChain.h>
#include <boost/shared_ptr.hpp>
#include <boost/range.hpp>
#include <boost/foreach.hpp>
//#include <boost/typeof/typeof.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/properties.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/iterator/iterator_categories.hpp>
#include <boost/unordered/unordered_map.hpp>

#include <boost/version.hpp>
#if BOOST_VERSION >= 104000
  #include <boost/property_map/property_map.hpp>
#else
  #include <boost/property_map.hpp>
#endif // BOOST_VERSION

#include <list>
#include <string>
#include <utility>
#include <ostream>

#include <boost/version.hpp>
#if BOOST_VERSION >= 104600
  #include <boost/range/any_range.hpp>
#endif // BOOST_VERSION

class SgNode;
//class SgExpression;
class SgStatement;

struct DefUseGraphHeap; ///< \internal

///  \internal  Data structure holding the underlying implementation
struct DefUseGraphImpl
{
  //
  // Actual types:
  // - Graph:   class DefaultDUchain
  // - Vertex:  class DefUseChainNode
  // - Edge:    class MultiGraphElem
  // - VertexIterator   class IteratorWrap<Vertex*, IteratorImpl<Vertex*> >
  // - EdgeIterator     class IteratorWrap<Edge*, IteratorImpl<Edge*> >
  //

  typedef DefaultDUchain        Graph;
  typedef DefUseGraphHeap       Heap;
  typedef Graph::Node           Vertex;
  typedef Graph::Edge           Edge;
  typedef Graph::NodeIterator   VertexIterator;
  typedef Graph::EdgeIterator   EdgeIterator;

  Graph graph;
  Heap *heap;

  DefUseGraphImpl();
  ~DefUseGraphImpl();
};

// DefUseGraphVertex
/**
 *  \brief  Define-use graph vertex.
 *
 *  Represents the vertex in the define-use graph.
 *  Defined as \c DefUseGraph::vertex_type.
 */
class DefUseGraphVertex
{
  typedef DefUseGraphVertex         Self;
  typedef DefUseGraphImpl::Vertex   Impl;

  Impl *impl_;

  // -- General vertex methods --
public:
  ///  \internal  Create an empty instance or wrap with its implementation
  DefUseGraphVertex(Impl *p = 0) : impl_(p) { }
  DefUseGraphVertex(const Self &that) : impl_(that.impl_) { }

  Self &operator=(const Self &that) { this->impl_ = that.impl_; return *this; }
  operator bool() const { return valid(); } ///< Same as \fn valid
  operator Impl*() const { return impl_; }  ///< \internal
  bool operator==(const Self &that) const { return this->impl_ == that.impl_; }

  ///  Comparison to underlying pointer, rather than higher level order.
  // This function is provide to resolve ambiguity for std::less.
  bool operator<(const Self &that) const { return this->impl_ < that.impl_; }

  ///  Unparse the AST node to string using  \c SgNode::unparseToString
  std::string str() const;
  bool valid() const { return impl_; }  ///< Return false for bad edge
  int hash() const { return reinterpret_cast<long>(impl_); }    ///< unique hash
  Impl *impl() const { return impl_; }  ///< Get underlying implementation
  Impl *get() const { return impl(); }  ///< Same as \fn impl
  void reset(Impl *impl = 0) { impl_ = impl; } ///< Reset implementation
  void clear() { reset(); } ///< Release the implementation pointer

  // -- Define-use information --
public:
  //@{
  ///  Properties saved in the vertex.

  enum def_or_use { use = 0, def = 1 };  ///< type of vertex, either use or def

  ///  If the node is an assignment (definition), otherwise it is a use.
  def_or_use type() const;
  bool is_def() const { return type() == def; }
  bool is_use() const { return type() == use; }

  ///  Corresponding lhs expression int the AST, should be unique for each node.
  SgNode *reference() const;

  ///  Enclosing statement.
  SgStatement *statement() const;
  //@}
};

// DefUseGraphEdge
/**
 *  \brief  Define-use graph edge.
 *
 *  Represents the edge in the define-use graph.
 *  Defined as \c DefUseGraph::vertex_type.
 */
class DefUseGraphEdge
{
  typedef DefUseGraphEdge       Self;
  typedef DefUseGraphImpl::Edge Impl;

  Impl *impl_;

  // -- General edge methods --
public:
  ///  \internal  Create an empty instance or wrap with its implementation
  DefUseGraphEdge(Impl *p = 0) : impl_(p) { }
  DefUseGraphEdge(const Self &that) : impl_(that.impl_) { }

  Self &operator=(const Self &that) { this->impl_ = that.impl_; return *this; }
  operator bool() const { return valid(); } ///< Same as \fn valid.
  operator Impl*() const { return impl_; }  ///< \internal
  bool operator==(const Self &that) const { return this->impl_ == that.impl_; }

  ///  Comparison to underlying pointer, rather than higher level order.
  // This function is provide to resolve ambiguity for std::less.
  bool operator<(const Self &that) const { return this->impl_ < that.impl_; }

  ///  Dump dependency information in the edge using \c DepInfoEdge::toString
  std::string str() const;
  bool valid() const { return impl_; }  ///< Return false for bad edge
  int hash() const { return reinterpret_cast<long>(impl_); } ///< unique hash
  Impl *impl() const { return impl_; }  ///< Get underlying implementation
  Impl *get() const { return impl(); }  ///< Same as \fn impl
  void reset(Impl *impl = 0) { impl_ = impl; } ///< Reset implementation
  void clear() { reset(); } ///< Release the implementation pointer

public:
  // No properties saved in the edge.
};

// DefUseGraphVertexItertor
/**
 *  \brief  Const forward iterator for traversing vertices in \c DefUseGraph
 *
 *  Represents the iterator for vertex in the define-use graph.
 *  Defined as \c DefUseGraph::vertex_iterator.
 */
class DefUseGraphVertexIterator : private DefUseGraphImpl::VertexIterator
{
  typedef DefUseGraphVertexIterator         Self;
  typedef DefUseGraphImpl::VertexIterator   Base;

  // Wrapper in order to provide \c reference value and avoid null pointer
  DefUseGraphVertex value_;

  // -- Adapting types --
public:
  // Consistent with \c std::iterator_traits<Self>
  typedef ptrdiff_t         difference_type;
  typedef DefUseGraphVertex value_type;
  typedef const value_type* pointer;
  typedef const value_type &reference;

  typedef boost::single_pass_traversal_tag iterator_category;

  // -- Underlying iterator operations --
protected:
  bool valid() const; ///< true if the iterator is null or has reached the end
  value_type current() const;       ///< current value the iterator pointed to
  void assign(const Base &b); ///< assign a new base
  void advance(); ///< ++iterator
  bool equal(const Self &that) const; ///< true if wrapped with the same pointer

  // -- General iterator methods --
public:
  DefUseGraphVertexIterator() { }  ///< Make an empty iterator.
  DefUseGraphVertexIterator(const Self &that) : Base(that), value_(that.value_) { }

  ///  \internal
  DefUseGraphVertexIterator(const Base &b) : Base(b), value_(current()) { }

  Self &operator=(const Self &that)
  { assign(that); value_ = that.value_; return *this; }

  reference operator*() const { return value_; }
  pointer operator->() const { return &value_; }

  Self &operator++() { advance(); return *this; }
  Self operator++(int) { Self t = *this; advance(); return t; }
  bool operator==(const Self &that) const { return equal(that); }

  ///  True if the iterator can be safely dereferenced and forwarded.
  operator bool() const { return valid(); }

  int hash() const; ///< unique per instance
};

// DefUseGraphEdgeItertor
/**
 *  \brief  Const forward iterator for traversing edges in \c DefUseGraph
 *
 *  Represents the iterator for edge in the define-use graph.
 *  Defined as \c DefUseGraph::edge_iterator.
 */
class DefUseGraphEdgeIterator : private DefUseGraphImpl::EdgeIterator
{
  typedef DefUseGraphEdgeIterator          Self;
  typedef DefUseGraphImpl::EdgeIterator    Base;

  // Wrapper in order to provide \c reference value and avoid null pointer
  DefUseGraphEdge value_;

  // -- Adapting types --
public:
  // Consistent with \c std::iterator_traits<Self>
  typedef ptrdiff_t         difference_type;
  typedef DefUseGraphEdge   value_type;
  typedef const value_type* pointer;
  typedef const value_type &reference;

  typedef boost::single_pass_traversal_tag iterator_category;

  // -- Underlying iterator operations --
protected:
  bool valid() const; ///< true if the iterator is null or has reached the end
  value_type current() const;       ///< current value the iterator pointed to
  void assign(const Base &b); ///< assign a new base
  void advance();   ///< ++iterator
  bool equal(const Self &that) const; ///< true if wrapped with the same pointer

  // -- General iterator methods --
public:
  DefUseGraphEdgeIterator() { }    ///< Make an empty iterator.
  DefUseGraphEdgeIterator(const Self &that) : Base(that), value_(that.value_) { }

  ///  \internal
  DefUseGraphEdgeIterator(const Base &b) : Base(b), value_(current()) { }

  Self &operator=(const Self &that)
  { assign(that); value_ = that.value_; return *this; }

  reference operator*() const { return value_; }
  pointer operator->() const { return &value_; }

  Self &operator++() { advance(); return *this; }
  Self operator++(int) { Self t = *this; advance(); return t; }
  bool operator==(const Self &that) const { return equal(that); }

  ///  True if the iterator can be safely dereferenced and forwarded.
  operator bool() const { return valid(); }

  int hash() const; ///< unique per instance
};

// DefUseGraph
/**
 *  \brief  Graph interface for define-use chain.
 */
class DefUseGraph
{
  typedef DefUseGraph       Self;
public:
  typedef DefUseGraphImpl   Impl; ///> \internal Graph representation.
private:
  boost::shared_ptr<Impl>   impl_;

  // -- Adapting types --
public:
  // Consistent with \c boost::graph_traits<Self>
  typedef boost::directed_tag       directed_category;

  typedef DefUseGraphVertex         vertex_type;
  typedef DefUseGraphEdge           edge_type;
  typedef DefUseGraphVertexIterator vertex_iterator;
  typedef DefUseGraphEdgeIterator   in_edge_iterator;
  typedef in_edge_iterator          out_edge_iterator;
  typedef std::list<edge_type>::iterator    edge_iterator;

  static vertex_type null_vertex() { return vertex_type(); }
  static edge_type   null_edge() { return edge_type(); }

  /// type of vertex, either def or use
  //typedef vertex_type::def_or_use def_or_use;
  enum def_or_use { def = vertex_type::def, use = vertex_type::use };

  // -- Graph constructions --
  //@{
  /**
   *  \name  Graph constructions
   *
   *  Graph constructions, status queries, and output methods.
   */
public:

  ///  Build a graph from an AST node.
  explicit DefUseGraph(const SgNode *root = 0)
  { if (root) build(root); }

  ///  Create a graph that holds the POINTER to the SAME graph of \p that.
  explicit DefUseGraph(const Self &that) : impl_(that.impl_) { }

  ///  \internal Get an implementation.
  explicit DefUseGraph(Impl *impl) : impl_(impl) { }

  ~DefUseGraph() { }

  /**
   *  \brief  Build the define-use graph.
   *
   *  Although not checked, the root should be a standalone scope such as
   *  function definition (\c SgFunctionDefinition).
   *  Node to build other than that could lead to incorrect graph or even
   *  segmentation fault.
   */
  void build(const SgNode *root);

  ///  Delete the underlying graph, same as \c reset().
  void clear()
  { reset(); }

  ///  Exchange the dependency graph with \p that.
  void swap(Self &that)
  { impl_.swap(that.impl_); }

  ///  Return if I am the only one who holds this graph
  bool unique() const
  { return impl_.unique(); }

  ///  True if the instance is holding a valid implementation.
  bool valid() const
  { return impl_; }

  ///  Return \c !bad()
  operator bool() const
  { return valid(); }

  ///  Return unique hash per implementation.
  int hash() const { return reinterpret_cast<long>(impl_.get()); }

  ///  Output the graph to DOT format.
  void toDOT(const std::string &filename, const std::string &graphname = "DefUseGraph") const;

  ///  Output the graph to plain text.
  template <typename OutputStream>
  inline void dump(OutputStream &out, const std::string &name = "def") const;

  void dump(std::ostream &out = std::cerr, const std::string &name = "def") const
  { dump<std::ostream>(out, name); }

  typedef boost::adjacency_list<
    boost::vecS, boost::vecS, boost::bidirectionalS,
    vertex_type, edge_type
    > adjacency_list_type;
  adjacency_list_type to_boost_adjacency_list() const; ///< Convert to boost::adjacency_list graph

public:
  ///  \internal Only for debug use. Return the implementation.
  Impl* impl() const
  { return impl_.get(); }

  Impl *get() const { return impl(); }  ///< Same as \fn impl

  ///  \internal Only for debug use. Reset the implementation.
  void reset(Impl *p = 0)
  { impl_.reset(p); }
  //@}
  // end of group: Graph constructions

  // -- Vertices and edges --
  //@{
  /**
   *  \name  Vertices and edges
   *
   *  Methods involving vertices and edges to traverse the graph.
   *  Note that all methods are \c const.
   */
public:

  ///  Return the first vertex in the graph.
  vertex_type entry() const
  { return valid()? *vertices_begin() : null_vertex(); }

  /**
   *  \brief  Create and add a new vertex
   *  \param  ref  Reference to the lhs value.
   *  \param  stmt  Parent statement of reference.
   *  \param  type  If \p ref in defined in \p stmt (lhs), or used in it (rhs).
   */
  std::pair<vertex_type, bool> add_vertex(SgNode *ref, SgStatement *stmt, def_or_use type);

  ///  Delete vertex \p v if existed.
  void remove_vertex(vertex_type v);

  ///  Create and add a new edge from \p u to \p v.
  std::pair<edge_type, bool> add_edge(vertex_type u, vertex_type v);

  std::pair<edge_type, bool> add_edge(std::pair<vertex_type, vertex_type> e)
  { return add_edge(e.first, e.second); }

  ///  Delete edge \p e if existed.
  void remove_edge(edge_type e);

  ///  Return if vertex \p v is in the graph
  bool contains(vertex_type v) const;

  ///  Return if edge \p e is in the graph
  bool contains(edge_type v) const;

  ///  Return if edge (\a u, \p v) is in the graph
  bool contains(vertex_type u, vertex_type v) const;

  bool contains(std::pair<vertex_type, vertex_type> e) const
  { return contains(e.first, e.second); }

  ///  Return (begin, end) pair to traverse all vertices in the graph
  std::pair<vertex_iterator, vertex_iterator> vertices() const
  { return std::make_pair(vertices_begin(), vertices_end()); }

  vertex_iterator vertices_begin() const;///< begin of the vertices
  vertex_iterator vertices_end() const;  ///< end of the vertices

  ///  Reversed vertices
  std::list<vertex_type> rvertices() const;

  ///  Return (begin, end) pair to traverse in-edges of vertex \p v
  std::pair<in_edge_iterator, in_edge_iterator> in_edges(vertex_type v) const
  { return std::make_pair(in_edges_begin(v), in_edges_end(v)); }

  in_edge_iterator in_edges_begin(vertex_type v) const;  ///< begin of in-edges
  in_edge_iterator in_edges_end(vertex_type v) const;    ///< end of in-edges

  ///  Return (begin, end) pair to traverse out-edges of vertex \p v
  std::pair<out_edge_iterator, out_edge_iterator> out_edges(vertex_type v) const
  { return std::make_pair(out_edges_begin(v), out_edges_end(v)); }

  out_edge_iterator out_edges_begin(vertex_type v) const; ///< begin of out-edges
  out_edge_iterator out_edges_end(vertex_type v) const;   ///< end of out-edges

   ///  Return all edges, expensive to compute.
  std::pair<edge_iterator, edge_iterator> edges() const;

  vertex_type source(edge_type e) const; ///< Source vertex of the edge \p e.
  vertex_type target(edge_type e) const; ///< Target vertex of edge \p e.

  size_t in_degree(vertex_type v) const; ///< In-degree of the vertex \p v.
  size_t out_degree(vertex_type v) const;///< Out-degree of the vertex \p v.

  size_t num_vertices() const;///< Total number of vertices, or 0 for bad graph.
  size_t num_edges() const;   ///< Total number of edges, or 0 for bad graph.

  bool has_vertices() const { return vertices_begin(); }
  bool has_in_edges(vertex_type v) const { return in_edges_begin(v); }
  bool has_out_edges(vertex_type v) const { return out_edges_begin(v); }
  bool has_edges() const;

  typedef boost::associative_property_map<
    boost::unordered_map<vertex_type, int>
    > indices_type;
  ///  Return indices of vertex
  indices_type indices() const;

  //@}
  // end of group: Vertices and edges

  // -- Advanced queries --
  //@{
  /**
   *  \name  Advanced queries
   *
   *  Search for vertices/edges qualified for specified conditions.
   *
   *  All these methods are const.
   */
public:
  ///  Return vertex whose has the same \p reference, or null_vertex if not found.
  //vertex_type vertex(const SgExpression *reference) const;

  ///  Return the def vertex of the \p statement.
  vertex_type vertex(const SgStatement *statement) const;

  ///  Return set of vertices whose has the same \p statement.
  std::list<vertex_type> vertices(const SgStatement *statement) const;

  ///  Return range of in edges of all vertices involving statement.
#if BOOST_VERSION >= 104600
  typedef boost::any_range<
    in_edge_iterator::value_type,
    in_edge_iterator::iterator_category,
    in_edge_iterator::reference,
    in_edge_iterator::difference_type
    > in_edges_type;
#else
  typedef std::list<edge_type> in_edges_type;
#endif // BOOST_VERSION
  in_edges_type in_edges(const SgStatement *statement) const;

  ///  If \c in_edges(statement) is larger than 1
  bool has_in_edges(const SgStatement *statement) const;

  ///  Total in degree of all vertices involving \p statement, i.e. count of \c in_edges(statement)
  size_t in_degree(const SgStatement *statement) const;

  ///  Return range of out edges of all vertices involving statement.
#if BOOST_VERSION >= 104600
  typedef boost::any_range<
    out_edge_iterator::value_type,
    out_edge_iterator::iterator_category,
    out_edge_iterator::reference,
    out_edge_iterator::difference_type
    > out_edges_type;
#else
  typedef std::list<edge_type> out_edges_type;
#endif // BOOST_VERSION
  out_edges_type out_edges(const SgStatement *statement) const;

  ///  If \c out_edges(statement) is larger than 1
  bool has_out_edges(const SgStatement *statement) const;

  ///  Total in degree of all vertices involving \p statement, i.e. count of \c out_edges(statement)
  size_t out_degree(const SgStatement *statement) const;

  //std::pair<in_edge_iterator, in_edge_iterator> in_edges(const SgExpression *reference) const
  //{ return in_edges(vertex(reference)); }

  //std::pair<out_edge_iterator, out_edge_iterator> out_edges(const SgExpression *reference) const
  //{ return out_edges(vertex(reference)); }
  //@}
  // end of group: Advance queries
};

// DefUseGraphPropertiesWriter
/**
 *  \brief  Properties writer for vertex and edge of define-use graph.
 *
 *  Here's an example to generate DOT graph using <boost/graph/graphviz.hpp>:
    <code>
      std::ofstream fout(some_file);
      DefUseGraph g(some_node);
      DefUseGraphPropertiesWriter vw, ew;
      boost::write_graphviz(fout, g, vw, ew);
    </code>
 */
struct DefUseGraphPropertiesWriter
{
  template <typename Descriptor, typename OutputStream>
  void operator()(OutputStream &out, Descriptor d)
  { out << "[label=\"" << d.str() << "\"]"; }
};


// Specialize STL iterator_traits for graph iterators:
namespace std {

  template <>
  struct iterator_traits<DefUseGraphVertexIterator>
  {
    typedef DefUseGraphVertexIterator   iterator;
    typedef iterator::difference_type   difference_type;
    typedef iterator::value_type        value_type;
    typedef iterator::pointer           pointer;
    typedef iterator::reference         reference;

    typedef forward_iterator_tag        iterator_category;
  };

  template <>
  struct iterator_traits<DefUseGraphEdgeIterator>
  {
    typedef DefUseGraphEdgeIterator     iterator;
    typedef iterator::difference_type   difference_type;
    typedef iterator::value_type        value_type;
    typedef iterator::pointer           pointer;
    typedef iterator::reference         reference;

    typedef forward_iterator_tag        iterator_category;
  };

} // namespace std

// Specialize and extend boost graph utilities:
namespace boost {

  // -- Graph traits --

  template <>
  struct graph_traits<DefUseGraph> {
    typedef DefUseGraph G;

    typedef G::directed_category    directed_category;

    // \c vertex_type and \c edge_type are used as their own descriptor,
    // since they are holding pointers to their vertex/edge representations.
    typedef G::vertex_type          vertex_descriptor;
    typedef G::edge_type            edge_descriptor;

    // \c in_edge_iterator and \c out_edge_iterator are the same.
    typedef G::vertex_iterator      vertex_iterator;
    typedef G::edge_iterator        edge_iterator;
    typedef G::in_edge_iterator     in_edge_iterator;
    typedef G::out_edge_iterator    out_edge_iterator;

    typedef size_t vertices_size_type;
    typedef size_t edges_size_type;
    typedef size_t degree_size_type;

    static vertex_descriptor null_vertex() { return G::null_vertex(); }
    static edge_descriptor   null_edge()   { return G::null_edge(); }

  };

} // namespace boost

typedef boost::graph_traits<DefUseGraph> DefUseGraphTraits; ///< \internal

namespace boost {

  // -- Graph traversal --

  inline DefUseGraphTraits::vertex_descriptor
  source(DefUseGraphTraits::edge_descriptor e, const DefUseGraph &g)
  { return g.source(e); }

  inline DefUseGraphTraits::vertex_descriptor
  target(DefUseGraphTraits::edge_descriptor e, const DefUseGraph &g)
  { return g.target(e); }

  inline std::pair<
    DefUseGraphTraits::vertex_iterator,
    DefUseGraphTraits::vertex_iterator
  >
  vertices(const DefUseGraph &g)
  { return g.vertices(); }

  inline std::pair<
    DefUseGraphTraits::edge_iterator,
    DefUseGraphTraits::edge_iterator
  >
  edges(const DefUseGraph &g)
  { return g.edges(); }

  inline std::pair<
    DefUseGraphTraits::in_edge_iterator,
    DefUseGraphTraits::in_edge_iterator
  >
  in_edges(DefUseGraphTraits::vertex_descriptor v, const DefUseGraph &g)
  { return g.in_edges(v); }

  inline std::pair<
    DefUseGraphTraits::out_edge_iterator,
    DefUseGraphTraits::out_edge_iterator
  >
  out_edges(DefUseGraphTraits::vertex_descriptor v, const DefUseGraph &g)
  { return g.out_edges(v); }

  inline DefUseGraphTraits::degree_size_type
  in_degree(DefUseGraphTraits::vertex_descriptor v, const DefUseGraph &g)
  { return g.in_degree(v); }

  inline DefUseGraphTraits::degree_size_type
  out_degree(DefUseGraphTraits::vertex_descriptor v, const DefUseGraph &g)
  { return g.out_degree(v); }

  inline DefUseGraphTraits::vertices_size_type
  num_vertices(const DefUseGraph &g)
  { return g.num_vertices(); }

  inline DefUseGraphTraits::edges_size_type
  num_edges(const DefUseGraph &g)
  { return g.num_edges(); }

  // -- Graph modification --

  inline std::pair<DefUseGraphTraits::edge_descriptor, bool>
  add_edge(DefUseGraphTraits::vertex_descriptor u,
           DefUseGraphTraits::vertex_descriptor v,
           DefUseGraph &g)
  { return g.add_edge(u, v); }

  inline void
  remove_edge(DefUseGraphTraits::edge_descriptor e, DefUseGraph &g)
  { return g.remove_edge(e); }

  inline void
  remove_vertex(DefUseGraphTraits::vertex_descriptor v, DefUseGraph &g)
  { return g.remove_vertex(v); }

  // -- Properties --

  template <>
  struct property_map<DefUseGraph, vertex_index_t>
  {
    typedef DefUseGraph::indices_type type;
    typedef type const_type;
  };

  inline property_map<DefUseGraph, vertex_index_t>::type
  get(vertex_index_t, const DefUseGraph &g)
  {
    property_map<DefUseGraph, vertex_index_t>::type indices;
    int index = 0;
    BOOST_FOREACH (DefUseGraphTraits::vertex_descriptor v, vertices(g))
      indices[v] = index++;
    return indices;
  }

} // namespace boost

// Extend STL output stream:

template <typename OutputStream>
inline OutputStream&
operator<<(OutputStream &out, const DefUseGraphVertex &v)
{ return out << v.str(); }

template <typename OutputStream>
inline OutputStream&
operator<<(OutputStream &out, const DefUseGraphEdge &e)
{ return out << e.str(); }

template <typename OutputStream>
inline OutputStream&
operator<<(OutputStream &out, const DefUseGraph &g)
{ g.dump(out); }

// Template instantiations:

template <typename OutputStream>
inline void
DefUseGraph::dump(OutputStream &out, const std::string &name) const
{
  if (valid())
    write_graph(impl_->graph, out, name);
  else
    out << name << " bad define-use graph\n";
}

#endif // _ROSEX_DEFUSEGRAPH_H
