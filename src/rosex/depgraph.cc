// depgraph.cc
// 6/17/2011 jichi

#include "rosex/rosex.h"
#include "rosex/slice_p.h"
#include "rosex/depgraph.h"
#include "rosex/depgraph_p.h"
#include <GraphDotOutput.h>
#include <DepInfo.h>
//#include <sstream>
#include <boost/foreach.hpp>
#include <boost/range.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/typeof/typeof.hpp>
#include <algorithm>

#ifdef __clang__
# pragma GCC diagnostic ignored "-Wparentheses"
# pragma GCC diagnostic ignored "-Wchar-subscripts"
# pragma GCC diagnostic ignored "-Wsign-compare"
# pragma GCC diagnostic ignored "-Wunused-function"
#elif defined(__GNUC__)
# pragma GCC diagnostic ignored "-Wparentheses" // assignment as a condition
#endif // __clang__

#include <boost/version.hpp>
#if BOOST_VERSION >= 104600
# include <boost/range/algorithm.hpp>
# include <boost/range/adaptors.hpp>
#else

namespace boost {

  namespace range {
    template <class SinglePassRange, class OutputIterator>
    inline OutputIterator
    copy(const SinglePassRange &rng, OutputIterator out)
    { return std::copy(boost::begin(rng), boost::end(rng), out); }
  } // namespace range
  using range::copy;

} // namespace boost

#endif // BOOST_VERSION

#define foreach BOOST_FOREACH

namespace { static const DepInfo NO_DEP_INFO; } // used as null depinfo
namespace { static const DepRel NO_DEP_REL; } // used as null deprel

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

} // namespace aonymous

// - Construction -

struct DepGraphPrivateData
{
  boost::scoped_ptr<
      std::list<DepGraph::edge_type>
  > p_edges;
  boost::scoped_ptr<
      boost::unordered_map<DepGraph::vertex_type, int>
  > p_indices;
};

DepGraphPrivate::DepGraphPrivate(const DepGraphOption *option)
  : data(new DepGraphPrivateData)
  , option(option)
{}

DepGraphPrivate::~DepGraphPrivate()
{ delete data; }

// - Graph vertices and edges -

std::string DepGraphVertex::toString() const
{ return d_? d_->toString() : std::string(); }

std::string DepGraphEdge::toString() const
{ return d_? d_->toString() : std::string(); }

SgNode *DepGraphVertex::statement() const
{ return d_ ? d_->statement() : nullptr; }

int DepGraphVertex::level() const
{ return d_ ? d_->level() : 0; }

bool DepGraphVertex::isLoop() const
{ return d_ && d_->isLoop(); }

//bool
//DepGraphVertex::isLoop() const
//{ return d_ && d_->is_loop(); }

const DepInfo &DepGraphVertex::depInfo() const
{ return d_? d_->loopMap() : NO_DEP_INFO; }

const DepInfo &DepGraphEdge::depInfo() const
{ return d_? d_->GetInfo() : NO_DEP_INFO; }

//const DepRel &DepGraphEdge::depRel(int source, int target) const
//{ return d_? depInfo().Entry(source, target) : NO_DEP_REL; }

DepType DepGraphEdge::depType() const
{ return d_ ? depInfo().GetDepType() : DEPTYPE_NONE; }

SgNode *DepGraphEdge::sourceExpression() const
{ return d_ ? slice::astnode_cast<SgNode *>(depInfo().SrcRef()) : nullptr; }

SgNode *DepGraphEdge::targetExpression() const
{ return d_ ? slice::astnode_cast<SgNode *>(depInfo().SnkRef()) : nullptr; }

// - Graph iterators -

bool DepGraphVertexIterator::valid() const
{ return !Base::ReachEnd() && Base::Current(); }

DepGraphVertexIterator::value_type DepGraphVertexIterator::current() const
{ return valid() ? value_type(Base::Current()) : value_type(); }

void DepGraphVertexIterator::assign(const Base &b)
{ Base::operator=(b); }

void DepGraphVertexIterator::advance()
{
  if (valid()) {
    Base::Advance();
    value_ = current();
  }
}

bool DepGraphVertexIterator::equal(const Self &that) const
{
  return (!valid() && !that.valid())
      || Base::operator==(that);
}

bool DepGraphEdgeIterator::valid() const
{ return !Base::ReachEnd() && Base::Current(); }

DepGraphEdgeIterator::value_type DepGraphEdgeIterator::current() const
{ return valid() ? value_type(Base::Current()) : value_type(); }

void DepGraphEdgeIterator::assign(const Base &b)
{ Base::operator=(b); }

void DepGraphEdgeIterator::advance()
{
  if (valid()) {
    Base::Advance();
    value_ = current();
  }
}

bool DepGraphEdgeIterator::equal(const Self &that) const
{
  return (!valid() && !that.valid())
      || Base::operator==(that);
}

// - Graph constructions -

bool DepGraph::init(SgNode *root, const option_type *option)
{
  D *d = nullptr;
  if (root) {
    d = new D(option);
    if (!d->build(root)) {
      delete d;
      d = nullptr;
    }
  }
  d_.reset(d);
  return d_;
}

void DepGraph::toDOT(const std::string &filename, const std::string &graphname) const
{
  ROSE_ASSERT(valid());

  GraphDotOutput<typeof(d_->graph)>(d_->graph)
      .writeToDOTFile(filename, graphname);
}

// - Graph properties -

DepGraph::vertex_type DepGraph::find(const SgNode *stmt) const
{
  ROSE_ASSERT(valid());

  if (stmt)
    foreach (const vertex_type &v, vertices())
      if (v && v.statement() == stmt)
        return v;

  return null_vertex();
}

bool DepGraph::contains(vertex_type v) const
{
  ROSE_ASSERT(valid());
  return v && d_->graph.ContainNode(v);
}

bool DepGraph::contains(edge_type e) const
{ return valid() && e && d_->graph.ContainEdge(e); }

bool DepGraph::contains(vertex_type u, vertex_type v) const
{
  ROSE_ASSERT(valid());

  if (u && v)
    foreach (const edge_type &e, out_edges(u))
      if (target(e) == v)
        return true;
  return false;
}

DepGraph::vertex_iterator DepGraph::vertices_begin() const
{
  ROSE_ASSERT(valid());
  return d_->graph.GetNodeIterator();
}

DepGraph::vertex_iterator DepGraph::vertices_end() const
{ return vertex_iterator(); }


DepGraph::in_edge_iterator DepGraph::in_edges_begin(vertex_type v) const
{
  ROSE_ASSERT(valid());
  return v ? in_edge_iterator(d_->graph.GetNodeEdgeIterator(v, GraphAccess::EdgeIn))
           : in_edge_iterator();
}

DepGraph::in_edge_iterator DepGraph::in_edges_end(vertex_type v) const
{ return in_edge_iterator(); }

DepGraph::out_edge_iterator DepGraph::out_edges_begin(vertex_type v) const
{
  ROSE_ASSERT(valid());
  return v ? out_edge_iterator(d_->graph.GetNodeEdgeIterator(v, GraphAccess::EdgeOut))
           : out_edge_iterator();
}

DepGraph::out_edge_iterator DepGraph::out_edges_end(vertex_type v) const
{ return out_edge_iterator(); }

DepGraph::vertex_type DepGraph::source(edge_type e) const
{
  ROSE_ASSERT(valid());
  return e ? vertex_type(d_->graph.GetEdgeEndPoint(e, GraphAccess::EdgeOut))
           : null_vertex();
}

DepGraph::vertex_type DepGraph::target(edge_type e) const
{
  ROSE_ASSERT(valid());
  return e ? vertex_type(d_->graph.GetEdgeEndPoint(e, GraphAccess::EdgeIn))
           : null_vertex();
}

// TODO jichi 6/24/2011: Hack the VirtualGraphCreate to directly get the number
// rather than counting manually.

size_t DepGraph::in_degree(vertex_type v) const
{
  ROSE_ASSERT(valid());
  return count(in_edges(v));
}

size_t DepGraph::out_degree(vertex_type v) const
{
  ROSE_ASSERT(valid());
  return count(out_edges(v));
}

size_t DepGraph::num_vertices() const
{
  ROSE_ASSERT(valid());
  return count(vertices());
}

size_t DepGraph::num_edges() const
{
  ROSE_ASSERT(valid());

  size_t ret = 0;
  foreach (const vertex_type &v,    vertices())
    ret += out_degree(v);

  return ret;
}

std::pair<
  DepGraph::edge_iterator,
  DepGraph::edge_iterator
  >
DepGraph::edges() const
{
  ROSE_ASSERT(valid() && d_->data);

  BOOST_AUTO(p, d_->data->p_edges.get());
  if (!p) {
    p = new typeof(*p);

    foreach (const vertex_type &v, vertices())
      boost::copy(out_edges(v), std::back_inserter(*p));

    d_->data->p_edges.reset(p);
  }

  return std::make_pair(p->begin(), p->end());
}

bool DepGraph::has_edges() const
{
  ROSE_ASSERT(valid());

  foreach (const vertex_type &v, vertices())
    if (has_out_edges(v))
      return true;
  return false;
}

boost::associative_property_map<
  boost::unordered_map<DepGraph::vertex_type, int>
  >
DepGraph::indices() const
{
  #define SELF indices
  typedef typeof(SELF()) RETURN;

  ROSE_ASSERT(valid());

  BOOST_AUTO(alloc, d_->data->p_indices.get());
  if (!alloc) {
    alloc = new typeof(*alloc);
    d_->data->p_indices.reset(alloc);
  }

  RETURN indices(*alloc);

  int index = 0;
  foreach (const vertex_type &v, vertices())
    indices[v] = index++;
  return indices;

  #undef SELF
}

// - Conversions -

boost::adjacency_list<
  boost::vecS, boost::vecS, boost::bidirectionalS,
  DepGraph::vertex_type, DepGraph::edge_type
  >
DepGraph::to_boost_adjacency_list() const
{
  #define SELF to_boost_adjacency_list
  typedef typeof(SELF()) RETURN;

  ROSE_ASSERT(valid());
  RETURN g(boost::num_vertices(*this));

  BOOST_AUTO(indices, boost::get(boost::vertex_index_t(), *this));

  foreach (const vertex_type &v, boost::vertices(*this)) {
    g[indices[v]].reset(v.data());
    foreach (const edge_type &e, boost::out_edges(v, *this)) {
      boost::graph_traits<typeof(g)>::edge_descriptor ed;
      bool succ;
      boost::tie(ed, succ) =
        boost::add_edge(indices[v], indices[boost::target(e, *this)], g);
      if (succ)
        g[ed].reset(e.data());
    }
  }
  return g;
  #undef SELF
}

std::string DepGraph::toString() const
{
  std::string ret;
  if (valid()) {
    vertex_type v;
    edge_type e;
    // for each vertex in the dependency graph
    foreach (v, vertices())
      if (out_degree(v) == 0) // if no edges, output vertex only
        ret += v.toString() + " => (none)" + "\n";
      else // otherwise, output source vertex, target vertex, and the edge
        foreach (e, out_edges(v))
          ret += v.toString() + " => " + target(e).toString() + " => " + e.toString() + "\n";
  }
  return ret;
}

// EOF

/*
namespace { // anonymous, collect read/write memory side effect

  enum read_or_write { READ = 0, WRITE = 1 };

  // \param    input    AST to collect.
  // \param    visited    Functions left on the STACK!
  // \param    rw    Whether collect read references or write.
  // \throw runtime_error    When failed to analyze the input code.
  // \return references of expressions
  //
  // FIXME: This condition is not solved when recursive function appeared in the parameters
  //         int f(int);
  //         ...
  //         f( f(12345) );
  //         ...
  //
  std::list<SgNode*>
  collectReadOrWriteRefs(SgNode *input, std::list<SgFunctionDefinition*> &visited, read_or_write rw)
      throw(std::runtime_error)
  {
  #define SELF(_x) collectReadOrWriteRefs((_x), visited, rw)
  #define APPEND(_to, _from) boost::copy((_from), std::back_inserter(_to))

      typedef typeof(SELF(input)) RETURN;
      RETURN ret;

      if (input) {

          // Assignment statement
          {
              SgExpression *lhs = 0, *rhs = 0;
              bool readlhs = false;
              if (SageInterface::isAssignmentStatement(input, &lhs, &rhs, &readlhs)) {
                  switch (rw) {
                  case WRITE: return SELF(lhs);

                  case READ:
                      if (rhs) ret = SELF(rhs);
                      if (readlhs) APPEND(ret, SELF(lhs));
                      return ret;
                  }

                  ROSE_ASSERT(0);
              }
          }

          // General contitions
          switch (input->variantT()) {
              // Symbols:
          case V_SgFunctionSymbol:                return SELF(isSgFunctionSymbol(input)->get_declaration());
          case V_SgFunctionDefinition:        return SELF(isSgFunctionDefinition(input)->get_body());

          case V_SgFunctionDeclaration: {
                  SgFunctionDeclaration *decl = isSgFunctionDeclaration(input);

                  foreach (SgNode *ref,    SELF(decl->get_definition()))
                      foreach (SgNode *arg, decl->get_args())
                          if (::sameInitialziedName(arg, ref))
                              ret.push_back(arg); // TODO: add support for returning array reference
              } break;

              // Statements:
          case V_SgBasicBlock: {
                  foreach (SgStatement *stmt, isSgBasicBlock(input)->get_statements())
                      APPEND(ret, SELF(stmt));
              } break;

          case V_SgForInitStatement: {
                  foreach (SgStatement *stmt, isSgForInitStatement(input)->get_init_stmt())
                      APPEND(ret, SELF(stmt));
              } break;

          case V_SgIfStmt: {
                  BOOST_AUTO(stmt, isSgIfStmt(input));
                  ret = SELF(stmt->get_conditional());
                  APPEND(ret, SELF(stmt->get_true_body()));
                  APPEND(ret, SELF(stmt->get_false_body()));
              } break;

          case V_SgWhileStmt: {
                  BOOST_AUTO(stmt, isSgWhileStmt(input));
                  ret = SELF(stmt->get_condition());
                  APPEND(ret, SELF(stmt->get_body()));
              } break;

          case V_SgDoWhileStmt: {
                  BOOST_AUTO(stmt, isSgDoWhileStmt(input));
                  ret = SELF(stmt->get_condition());
                  APPEND(ret, SELF(stmt->get_body()));
              } break;

          case V_SgForStatement: {
                  BOOST_AUTO(stmt, isSgForStatement(input));
                  ret = SELF(stmt->get_for_init_stmt());
                  APPEND(ret, SELF(stmt->get_test()));
                  APPEND(ret, SELF(stmt->get_increment()));
                  APPEND(ret, SELF(stmt->get_loop_body()));
              } break;

          case V_SgCaseOptionStmt:    return SELF(isSgCaseOptionStmt(input)->get_body());
          case V_SgExprStatement:     return SELF(isSgExprStatement(input)->get_expression());
          case V_SgReturnStmt: return SELF(isSgReturnStmt(input)->get_expression());

              // Expressions

          case V_SgFunctionRefExp:    return SELF(isSgFunctionRefExp(input)->get_symbol());
          case V_SgFunctionCallExp: {
                  BOOST_AUTO(call, isSgFunctionCallExp(input));
                  ROSE_ASSERT(call->get_args());
                  BOOST_AUTO(called_function, call->get_function());

                  BOOST_AUTO(decl, getFunctionDeclaration(called_function));
                  if (!decl)
                      break;
                  BOOST_AUTO(def, decl->get_definition());
                  BOOST_AUTO(called_args, isSgExprListExp(call->get_args())->get_expressions());
                  BOOST_AUTO(decl_args, decl->get_args());

                  // If it is a recursive function call, or found var_args
                  if (boost::find(visited, def) != visited.end() // i.e. there are same function calls in the stack
                          || decl_args.size() != called_args.size())                // args count mismatch for var_args
                      foreach (SgNode *ref,    called_args)
                          APPEND(ret, SELF(ref));

                  else { // otherwise
                      visited.push_back(def);
                      RETURN all_refs = SELF(def);

                      for (int i = 0; i < std::min(decl_args.size(), called_args.size()); i++) {
                          // Skip scalar parameters for computing write references
                          if (rw == WRITE
                                  && !isNonConstPointerType( decl_args[i]->get_type() ))
                              continue;

                          foreach (SgNode *ref,    all_refs)
                              if (::sameInitialziedName(decl_args[i], ref))
                                  APPEND(ret, SELF(called_args[i]));
                      }
#if BOOST_VERSION >= 104600
                      APPEND(ret, all_refs | boost::adaptors::filtered(isInGlobalScope));
#else
                      foreach (SgNode *ref, all_refs) if (isInGlobalScope(ref))
                          ret.push_back(ref);
#endif // BOOST_VERSION

                      ROSE_ASSERT(visited.back() == def); // visited is supposed to be a stack
                      visited.pop_back();
                  }
              } break;

          // Ignored:
          case V_SgNullStatement:
          case V_SgBreakStmt:
          case V_SgContinueStmt:
          case V_SgGotoStatement:
              break;

          default:
              // Normal expressions
              if (isSgExpression(input))
                  APPEND(ret, NodeQuery::querySubTree(input, V_SgVarRefExp)); // TODO: Array not correctly handled!
              else if (isSgDeclarationStatement(input))
                  ; // Declaration statement has no side effects
              else {
                  std::cerr << input->class_name() << std::endl
                                      << input << std::endl
                                      << "If assertion failure happened, the input type needed to be handled"
                                      << std::endl;
                  ROSE_ASSERT(0);
              }
          }
      }

      if (ret.size() > 1)
          ret.unique();
      return ret;
  #undef APPEND
  #undef SELF
  }

} // anonymous namespaced

std::pair<
  std::list<SgNode*>,
  bool
  >
rose_detail::collectReadRefs(SgNode *input)
{
  #define SELF collectReadRefs
  typedef typeof(SELF(input)) RETURN;
  try {
      std::list<SgFunctionDefinition*> visited;
      return RETURN(
          ::collectReadOrWriteRefs(input, visited, READ),
          true);

  } catch (std::runtime_error&) {
      return RETURN();
  }
  #undef SELF
}

// Return write references for function \p f, or empty list if no side effect.
std::pair<
  std::list<SgNode*>,
  bool
  >
rose_detail::collectWriteRefs(SgNode *input)
{
  #define SELF collectWriteRefs
  typedef typeof(SELF(input)) RETURN;

  try {
      std::list<SgFunctionDefinition*> visited;
      return RETURN(
          ::collectReadOrWriteRefs(input, visited, WRITE),
          true);

  } catch (std::runtime_error&) {
      return RETURN();
  }
  #undef SELF
}
*/
