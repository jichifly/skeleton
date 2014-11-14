// defusegraph.cc
// Since 7/5/2011

#include "rosex/rosex.h"

#include "defusegraph.h"
#include <AstInterface_ROSE.h>
#include <GraphDotOutput.h>
#include <StmtInfoCollect.h>

#include <boost/smart_ptr.hpp>
#include <boost/typeof/typeof.hpp>

#include <boost/version.hpp>
#if BOOST_VERSION >= 104600
  #include <boost/range/join.hpp>
#endif // BOOST_VERSION

#if BOOST_VERSION >= 104600
  #include <boost/range/algorithm.hpp>
#else

#if defined(__GNUC__)
# pragma GCC diagnostic ignored "-Wparentheses" // assignment as a condition
#endif // __GNUC__

namespace boost {

  namespace range {
    template <class SinglePassRange, class OutputIterator>
    inline OutputIterator copy(const SinglePassRange &rng, OutputIterator out)
    { return std::copy(boost::begin(rng), boost::end(rng), out); }
  } // namespace range
  using range::copy;

} // namespace boost

#endif // BOOST_VERSION

#define foreach BOOST_FOREACH

// -- Helper functions --

namespace { // anonymous, algorithm

  template <typename InputIterator>
  inline size_t
  count(InputIterator first, InputIterator last)
  {
    if (first == last)
      return 0;

    size_t ret = 1;
    while (++first != last) ++ret;
    return ret;
  }

  template <typename SinglePassRange>
  inline BOOST_DEDUCED_TYPENAME boost::range_difference<SinglePassRange>::type
  count(const SinglePassRange &r)
  { return count(boost::begin(r), boost::end(r)); }

} // namespace anonymous

namespace { // anonymous, type cast

  // Helper functions for casting between different vertex pointer representations

  template <typename T, typename S>
  T type_cast(S input)
  { return S::__cast_not_defined__(input); } // static_assert(0)

  template <>
  inline SgNode*
  type_cast<SgNode*>(AstNodePtr input)
  { return static_cast<SgNode*>(input.get_ptr()); }

  template <>
  inline AstNodePtr
  type_cast<AstNodePtr>(SgNode *input)
  { return AstNodePtrImpl(input); }

} // anonymous namespace

// -- Alias analysis --


namespace { // anonymous, alias interface

  class AliasAnalysis : public StmtVarAliasCollect
  {
    typedef AliasAnalysis Self;
    typedef StmtVarAliasCollect Base;

  public:
    // override
    virtual bool may_alias(AstInterface &fa, const AstNodePtr &input1, const AstNodePtr &input2);
  };

  bool
  AliasAnalysis:: may_alias(AstInterface &fa, const AstNodePtr &input1, const AstNodePtr &input2)
  {
    SgNode *n1 = type_cast<SgNode*>(input1);
    SgNode *n2 = type_cast<SgNode*>(input2);

    if (!n1 || !n2)
      return true;

    foreach (SgNode *r1, NodeQuery::querySubTree(n1, V_SgVarRefExp))
      foreach (SgNode *r2, NodeQuery::querySubTree(n2, V_SgVarRefExp))
        if (Base::may_alias(fa, type_cast<AstNodePtr>(r1), type_cast<AstNodePtr>(r2)))
          return true;

    return false;
  }
} // anonymous namespace

// -- DefUseGraphHeap --

struct DefUseGraphHeap
{
  typedef DefUseGraph Graph;

  boost::scoped_ptr<
    std::list<Graph::edge_type>
    >
    p_edges;

  boost::scoped_ptr<
    boost::unordered_map<Graph::vertex_type, int>
    >
    p_indices;
};

DefUseGraphImpl::DefUseGraphImpl()
{ heap = new Heap; }

DefUseGraphImpl::~DefUseGraphImpl()
{ if (heap) delete heap; }

// -- Graph constructions implementation --

namespace { // anonymous, graph builders

  void buildDefUseGraph(DefUseGraphImpl::Graph &g, const SgNode *node)
  {
    BOOST_AUTO(ast_to_analyze, const_cast<SgNode*>(node));
    ROSE_ASSERT(ast_to_analyze);

    BOOST_AUTO(enclosing_function, isSgFunctionDefinition(ast_to_analyze));
    if (!enclosing_function)
      enclosing_function = SageInterface::getEnclosingFunctionDefinition(ast_to_analyze);

    ROSE_ASSERT(enclosing_function);

    AstInterfaceImpl fa_impl(enclosing_function);
    AstInterface fa(&fa_impl);

    //StmtVarAliasCollect alias;
    AliasAnalysis alias;
    alias(fa, type_cast<AstNodePtr>(ast_to_analyze));

    ReachingDefinitionAnalysis r;
    r(fa, type_cast<AstNodePtr>(ast_to_analyze));

    FunctionSideEffectInterface *FUNCTION_NO_SIDE_EFFECT = 0;

    g.build(fa, r, alias, FUNCTION_NO_SIDE_EFFECT);
  }

} // namespace anonymous

// -- Graph vertices and edges --

std::string
DefUseGraphVertex::str() const
{ return impl_?  impl_->toString() : std::string(); }

std::string
DefUseGraphEdge::str() const
{ return impl_? impl_->toString() : std::string(); }

DefUseGraphVertex::def_or_use
DefUseGraphVertex::type() const
{ return static_cast<def_or_use>(impl_ && impl_->is_definition()); }

SgNode*
DefUseGraphVertex::reference() const
{ return impl_? type_cast<SgNode*>(impl_->get_ref()) : 0; }

SgStatement*
DefUseGraphVertex::statement() const
{
  return impl_? isSgStatement( type_cast<SgNode*>(impl_->get_stmt()) ) : 0;
}

// -- Graph iterators --

bool
DefUseGraphVertexIterator::valid() const
{ return !Base::ReachEnd() && Base::Current(); }

DefUseGraphVertexIterator::value_type
DefUseGraphVertexIterator::current() const
{
  if (valid())
    return Base::Current();
  else
    return value_type();
}

void
DefUseGraphVertexIterator::assign(const Base &b)
{ Base::operator=(b); }

void
DefUseGraphVertexIterator::advance()
{
  if (valid()) {
    Base::Advance();
    value_ = current();
  }
}

bool
DefUseGraphVertexIterator::equal(const Self &that) const
{
  return (!valid() && !that.valid())
    || Base::operator==(that);
}

int
DefUseGraphVertexIterator::hash() const
{ return reinterpret_cast<long>(ConstPtr()); }

bool
DefUseGraphEdgeIterator::valid() const
{ return !Base::ReachEnd() && Base::Current(); }

DefUseGraphEdgeIterator::value_type
DefUseGraphEdgeIterator::current() const
{
  if (valid())
    return Base::Current();
  else
    return value_type();
}

void
DefUseGraphEdgeIterator::assign(const Base &b)
{ Base::operator=(b); }

void
DefUseGraphEdgeIterator::advance()
{
  if (valid()) {
    Base::Advance();
    value_ = current();
  }
}

bool
DefUseGraphEdgeIterator::equal(const Self &that) const
{
  return (!valid() && !that.valid())
    || Base::operator==(that);
}

int
DefUseGraphEdgeIterator::hash() const
{ return reinterpret_cast<long>(ConstPtr()); }

// --  Graph constructions --

void
DefUseGraph::build(const SgNode *root)
{
  clear();
  if (!root)
    return;

  impl_.reset(new Impl);
  buildDefUseGraph(impl_->graph, root);
}

void
DefUseGraph::toDOT(const std::string &filename, const std::string &graphname) const
{
  ROSE_ASSERT(valid());
  GraphDotOutput<DefUseGraphImpl::Graph>(impl_->graph)
    .writeToDOTFile(filename, graphname);
}

// -- Graph modifications --

void
DefUseGraph::remove_edge(edge_type e)
{
  ROSE_ASSERT(valid());
  if (e)
    impl_->graph.DeleteEdge(e);
}

void
DefUseGraph::remove_vertex(vertex_type v)
{
  ROSE_ASSERT(valid());
  if (v)
    impl_->graph.DeleteNode(v);
}

std::pair<DefUseGraph::edge_type, bool>
DefUseGraph::add_edge(vertex_type u, vertex_type v)
{
  ROSE_ASSERT(valid());
  if (!u || !v)
    return std::make_pair(null_edge(), false);

  edge_type e = impl_->graph.CreateEdge(u, v);
  return std::make_pair(e, true);
}

std::pair<DefUseGraph::vertex_type, bool>
DefUseGraph::add_vertex(SgNode *ref, SgStatement *stmt, def_or_use is_definition)
{
  ROSE_ASSERT(valid());
  if (!ref || !stmt)
    return std::make_pair(null_vertex(), false);

  AstInterface dummy(0);

  vertex_type v = impl_->graph.CreateNode(dummy,
                         type_cast<AstNodePtr>(isSgNode(ref)),
                         type_cast<AstNodePtr>(isSgNode(stmt)),
                         static_cast<bool>(is_definition));
  return std::make_pair(v, true);
}

// -- Graph properties --

bool
DefUseGraph::contains(vertex_type v) const
{
  ROSE_ASSERT(valid());
  return v && impl_->graph.ContainNode(v);
}

bool
DefUseGraph::contains(edge_type e) const
{
  ROSE_ASSERT(valid());
  return e && impl_->graph.ContainEdge(e);
}

bool
DefUseGraph::contains(vertex_type u, vertex_type v) const
{
  ROSE_ASSERT(valid());
  if (u && v)
    foreach (edge_type e, out_edges(u))
      if (target(e) == v)
        return true;
  return false;
}

DefUseGraph::vertex_iterator
DefUseGraph::vertices_begin() const
{
  ROSE_ASSERT(valid());
  return impl_->graph.GetNodeIterator();
}

DefUseGraph::vertex_iterator
DefUseGraph::vertices_end() const
{ return vertex_iterator(); }

std::list<DefUseGraph::vertex_type>
DefUseGraph::rvertices() const
{
  std::list<DefUseGraph::vertex_type> l;
  foreach (vertex_type v, vertices())
    l.push_front(v);
  return l;
}

DefUseGraph::in_edge_iterator
DefUseGraph::in_edges_begin(vertex_type v) const
{
  ROSE_ASSERT(valid());
  if (v)
    return impl_->graph.GetNodeEdgeIterator(v, GraphAccess::EdgeIn);
  else
    return in_edge_iterator();
}

DefUseGraph::in_edge_iterator
DefUseGraph::in_edges_end(vertex_type v) const
{ return in_edge_iterator(); }

DefUseGraph::out_edge_iterator
DefUseGraph::out_edges_begin(vertex_type v) const
{
  ROSE_ASSERT(valid());
  if (v)
    return impl_->graph.GetNodeEdgeIterator(v, GraphAccess::EdgeOut);
  else
    return out_edge_iterator();
}

DefUseGraph::out_edge_iterator
DefUseGraph::out_edges_end(vertex_type v) const
{ return out_edge_iterator(); }

DefUseGraph::vertex_type
DefUseGraph::source(edge_type e) const
{
  ROSE_ASSERT(valid());
  if (e)
    return impl_->graph.GetEdgeEndPoint(e, GraphAccess::EdgeOut);
  else
    return null_vertex();
}

DefUseGraph::vertex_type
DefUseGraph::target(edge_type e) const
{
  ROSE_ASSERT(valid());
  if (e)
    return impl_->graph.GetEdgeEndPoint(e, GraphAccess::EdgeIn);
  else
    return null_vertex();
}

// TODO jichi 6/24/2011: Hack the VirtualGraphCreate to directly get the number
// rather than counting
// Template class.

size_t
DefUseGraph::in_degree(vertex_type v) const
{
  ROSE_ASSERT(valid());
  return count(in_edges(v));
}

size_t
DefUseGraph::out_degree(vertex_type v) const
{
  ROSE_ASSERT(valid());
  return count(out_edges(v));
}

size_t
DefUseGraph::num_vertices() const
{
  ROSE_ASSERT(valid());
  return count(vertices());
}

size_t
DefUseGraph::num_edges() const
{
  ROSE_ASSERT(valid());

  size_t ret = 0;
  foreach (vertex_type v,  vertices())
    ret += out_degree(v);

  return ret;
}

std::pair<
  DefUseGraph::edge_iterator,
  DefUseGraph::edge_iterator
  >
DefUseGraph::edges() const
{
  ROSE_ASSERT(valid() && impl_->heap);

  BOOST_AUTO(p, impl_->heap->p_edges.get());

  if (!p) {
    p = new std::list<edge_type>;

    foreach (vertex_type v, vertices())
      boost::copy(out_edges(v), std::back_inserter(*p));

    impl_->heap->p_edges.reset(p);
  }

  return std::make_pair(p->begin(), p->end());
}

bool
DefUseGraph::has_edges() const
{
  ROSE_ASSERT(valid());

  foreach (vertex_type v, vertices())
    if (has_out_edges(v))
      return true;
  return false;
}

DefUseGraph::indices_type
DefUseGraph::indices() const
{
  ROSE_ASSERT(valid());

  BOOST_AUTO(alloc, impl_->heap->p_indices.get());
  if (!alloc) {
    alloc =  new boost::unordered_map<vertex_type, int>;
    impl_->heap->p_indices.reset(alloc);
  }

  indices_type indices(*alloc);

  int index = 0;
  foreach (vertex_type v, vertices())
    indices[v] = index++;
  return indices;
}

// -- Advanced graph queries --

DefUseGraph::vertex_type
DefUseGraph::vertex(const SgStatement *statement) const
{
  ROSE_ASSERT(valid());
  if (statement)
    foreach (vertex_type v, vertices())
      if (v && v.is_def() && v.statement() == statement)
        return v;

  return null_vertex();
}

std::list<DefUseGraph::vertex_type>
DefUseGraph::vertices(const SgStatement *statement) const
{
  ROSE_ASSERT(valid());
  std::list<vertex_type> ret;
  if (statement)
    foreach (vertex_type v, vertices())
      if (v && v.statement() == statement)
        ret.push_back(v);
  return ret;
}

DefUseGraph::in_edges_type
DefUseGraph::in_edges(const SgStatement *statement) const
{
  ROSE_ASSERT(valid());
  in_edges_type r;

  if (statement)
    foreach (vertex_type v, vertices())
      if (v.statement() == statement && has_in_edges(v))
#if BOOST_VERSION >= 104600
        r = boost::join(r, in_edges(v));
#else
        boost::copy(in_edges(v), std::back_inserter(r));
#endif // BOOST_VERSION
  return r;
}

DefUseGraph::out_edges_type
DefUseGraph::out_edges(const SgStatement *statement) const
{
  ROSE_ASSERT(valid());
  out_edges_type r;

  if (statement)
    foreach (vertex_type v, vertices())
      if (v.statement() == statement && has_out_edges(v))
#if BOOST_VERSION >= 104600
        r = boost::join(r, out_edges(v));
#else
        boost::copy(out_edges(v), std::back_inserter(r));
#endif // BOOST_VERSION

  return r;
}

bool
DefUseGraph::has_in_edges(const SgStatement *statement) const
{
  ROSE_ASSERT(valid());

  if (statement)
    foreach (vertex_type v, vertices())
      if (v.statement() == statement && has_in_edges(v))
        return true;
   return false;
}

bool
DefUseGraph::has_out_edges(const SgStatement *statement) const
{
  ROSE_ASSERT(valid());

  if (statement)
    foreach (vertex_type v, vertices())
      if (v.statement() == statement && has_out_edges(v))
        return true;
   return false;
}

size_t
DefUseGraph::in_degree(const SgStatement *statement) const
{
  ROSE_ASSERT(valid());

  size_t ret = 0;
  if (statement)
    foreach (vertex_type v, vertices())
      if (v.statement() == statement && has_in_edges(v))
        ret += in_degree(v);
  return ret;
}

size_t
DefUseGraph::out_degree(const SgStatement *statement) const
{
  ROSE_ASSERT(valid());

  size_t ret = 0;
  if (statement)
    foreach (vertex_type v, vertices())
      if (v.statement() == statement && has_out_edges(v))
        ret += out_degree(v);
  return ret;
}

// -- Conversions --

DefUseGraph::adjacency_list_type
DefUseGraph::to_boost_adjacency_list() const
{
  ROSE_ASSERT(valid());
  adjacency_list_type g(boost::num_vertices(*this));

  BOOST_AUTO(indices, boost::get(boost::vertex_index_t(), *this));

  foreach (vertex_type v, boost::vertices(*this)) {
    g[indices[v]].reset(v.get());
    foreach (edge_type e, boost::out_edges(v, *this)) {
      boost::graph_traits<adjacency_list_type>::edge_descriptor ed;
      bool succ;
      boost::tie(ed, succ) =
        boost::add_edge(indices[v], indices[boost::target(e, *this)], g);
      if (succ)
        g[ed].reset(e.get());
    }
  }
  return g;
}

// EOF
