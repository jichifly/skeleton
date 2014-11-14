// depgraph_p.cc
// 6/17/2011 jichi
// See: rose/src/midend/programTransformation/loopProcessing/depGraph/DepGraphBuild.C

#include "rosex/rosex.h"
#include "rosex/slice_p.h"
#include "rosex/depgraph.h"
#include "rosex/depgraph_p.h"
#include "rosex/depgraphopt_p.h"
#include <DepGraphBuild.h>
#include <boost/foreach.hpp>
#include <boost/unordered_set.hpp>
#include <stdexcept>
#include <iostream>

#ifdef __clang__
# pragma GCC diagnostic ignored "-Wparentheses"
# pragma GCC diagnostic ignored "-Wchar-subscripts"
# pragma GCC diagnostic ignored "-Wsign-compare"
# pragma GCC diagnostic ignored "-Wunused-function"
#endif // __clang__

#include <boost/version.hpp>

#include <boost/exception/diagnostic_information.hpp>
#if BOOST_VERSION < 104600
#  define BOOST_THROW_EXCEPTION(_e)   throw(_e)
#endif // BOOST_VERSION

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

// - Build -


// -- Graph constructions implementation --

namespace { // anonymous, AST traversal

  // Filter top level statements in the AST
  // Note: if/else statement is not filtered to be consistent with Dr.Yi's implementation.
  class StatementFilter : protected AstTopDownProcessing<bool>
  {
    typedef StatementFilter Self;
    typedef AstTopDownProcessing<bool> Base;

    //SgNode *root_;
    SgStatementPtrList stmts_;

  public:
    explicit StatementFilter(SgNode *root)
    { if (root) traverse(root, true); }

  const SgStatementPtrList &statements() const { return stmts_; }

  protected:
    bool evaluateInheritedAttribute(SgNode *stmt, bool traverse) override;
  };

  bool StatementFilter::evaluateInheritedAttribute(SgNode *stmt, bool traverse)
  {
    // jichi 7/18/2011: The following logic acts exactly the same as traversing
    // Dr. Yi's LoopTree created by LoopTreeBuild.
    // But I don't think the while/dowhile/switchcase/if/ifelse cases are correctly handled.
    // Initializer in the declaration statement are ignored
    //
    #define APPEND(_to, _from) boost::copy((_from), std::back_inserter(_to))

    ROSE_ASSERT(stmt);
    if (traverse)
      if (::isSgStatement(stmt) || ::isSgFunctionDefinition(stmt)) {

        if (::isSgDeclarationStatement(stmt)) {
          //stmts_.push_back(::isSgStatement(stmt));
          return true;
        }

        SgStatementPtrList l, l2;
        bool skip_stmt = false;
        switch (stmt->variantT()) {
        case V_SgBasicBlock:
          return true;

        case V_SgFunctionDefinition:
          skip_stmt = true;
          l = Self(::isSgFunctionDefinition(stmt)->get_body()).statements();
          break;

        case V_SgForStatement:
          l = Self(::isSgForStatement(stmt)->get_loop_body()).statements();
          break;

        case V_SgFortranDo:
          l = Self(::isSgFortranDo(stmt)->get_body()).statements();
          break;

        case V_SgWhileStmt:
          l = Self(::isSgWhileStmt(stmt)->get_body()).statements();
          break;
        case V_SgDoWhileStmt:
          l = Self(::isSgDoWhileStmt(stmt)->get_body()).statements();
          break;

        case V_SgSwitchStatement:
          l = Self(::isSgSwitchStatement(stmt)->get_body()).statements();
          break;

        case V_SgCaseOptionStmt:
          l = Self(::isSgCaseOptionStmt(stmt)->get_body()).statements();
          break;

        case V_SgIfStmt:
          {
            SgIfStmt *p = ::isSgIfStmt(stmt);
            l = Self(p->get_true_body()).statements();
            l2 = Self(p->get_false_body()).statements();
          } break;

        default: ;
        }

        if (!skip_stmt) {
          ROSE_ASSERT(::isSgStatement(stmt));
          stmts_.push_back(static_cast<SgStatement *>(stmt));
        }

        if (!l.empty())
          APPEND(stmts_, l);
        if (!l2.empty())
          APPEND(stmts_, l2);
        //if (!l3.empty())
        //    APPEND(stmts_, l3);

        traverse = false;
      }

    return traverse;
    #undef APPEND
  }

} // anonymous namespace

namespace { // anonymous, graph builder implementation

  class BuildLoopDepGraphEdges : public AstTreeDepGraphBuildImpl
  {
    typedef BuildLoopDepGraphEdges        Self;
    typedef AstTreeDepGraphBuildImpl      Base;

  public:
    typedef GraphAccessInterface::Node    Node;
    typedef GraphAccessInterface::Edge    Edge;
    typedef DepGraphPrivate::Graph        Graph;
    typedef DepGraphPrivate::Vertex       Vertex;

  protected:
    Graph &g;
    GraphAccessWrapTemplate<Node, Edge, Graph> ga;

  public:
    explicit BuildLoopDepGraphEdges(Graph &graph)
      : g(graph), ga(&graph) { }

  private:
    Node *CreateNodeImpl(AstNodePtr start, const DomainCond &c) override
    { ROSE_ASSERT(0); return 0; }

    const GraphAccessInterface* Access() const override
    { return &ga; }

    void CreateEdgeImpl(Node *gn1, Node *gn2, DepInfo info) override
    {
      Vertex *n1 = static_cast<Vertex *>(gn1),
             *n2 = static_cast<Vertex *>(gn2);
      ROSE_ASSERT(n1);
      ROSE_ASSERT(n2);

      if (info.GetDepType() == DEPTYPE_TRANS)
        for (GraphCrossEdgeIterator<Graph> crossIter(&g, n1, n2);
             !crossIter.ReachEnd(); ++crossIter) {
          DepInfoEdge *e = crossIter.Current();
          ROSE_ASSERT(e);
          if (e->GetInfo() == info)
            return;
        }

      g.CreateEdgeFromOrigAst(n1, n2, info);
    }

    DepInfoConstIterator GetDepInfoIteratorImpl(Edge *ge, DepType t) override
    {
      DepInfoEdge *e = static_cast<DepInfoEdge *>(ge);
      ROSE_ASSERT(e);
      return SelectDepType(e->GetInfo(),t) ?
             DepInfoConstIterator(new SingleIterator<DepInfo>(e->GetInfo())) :
             DepInfoConstIterator();
    }

    AstNodePtr GetNodeAst(GraphAccessInterface::Node *gn) override
    {
      Vertex *n = static_cast<Vertex *>(gn);
      ROSE_ASSERT(n && n->statement());
      return slice::astnode_cast<AstNodePtr>(n->statement());
    }

  };

  class BuildLoopDepGraph : public BuildLoopDepGraphEdges
  {
    typedef BuildLoopDepGraph Self;
    typedef BuildLoopDepGraphEdges    Base;

    SgStatementPtrList vertices_;
    SgStatementPtrList::const_iterator current_;

  public:
    BuildLoopDepGraph(SgNode *root, Graph &graph)
      : Base(graph)
    {
      vertices_ = StatementFilter(root).statements();
      current_ = vertices_.begin();

      if (vertices_.empty())
        BOOST_THROW_EXCEPTION(std::runtime_error(
          "warning: no statements found in AST, abort traversing ."
        ));
    }

  private:
    Node *CreateNodeImpl(AstNodePtr start, const DomainCond &c) override
    {
      if (current_ == vertices_.end())
        BOOST_THROW_EXCEPTION(std::runtime_error(
         "warning: found null node while traversing AST statements , abort traversing."
        ));

      SgNode *stmt = *current_++;
      ROSE_ASSERT(stmt);
      return g.CreateNode(stmt, c);
    }
  };

} // anonymous namespace

// See: rose/midend/programTransform/loopProcessing/depGraph/DepGraphBuild.C
// See: rose/midend/programTransform/loopProcessing/computation/LoopTreeDepComp.C
bool DepGraphPrivate::build(SgNode *root)
{
  if (!root)
    return false;

  try {
    graph.setRoot(root);

    AstInterface *fa = slice::globalAstInterface();

    // Set loop transform interface from \p option
    //AutoBackup_LoopTransformInterface backup_previous_settings;
    // 2/14/2014 jichi: FIXME: lacking way to set  interface
    if (option) {
      typedef Hacked_LoopTransformInterface la;
      la::set_astInterfacePtr(fa);
      la::set_aliasInfo(option->aliasInfo);
      la::set_sideEffectInfo(option->funcInfo);
      la::set_arrayInfo(option->arrayInfo);
      //la::set_stmtSideEffectInterface(option->stmtInfo);
    }

    // Process graph
    BuildLoopDepGraph depImpl(root, graph);
    DepInfoAnal anal(*fa);
    BuildAstTreeDepGraph proc(&depImpl, anal);
    if (!ReadAstTraverse(*fa, slice::astnode_cast<AstNodePtr>(root), proc, AstInterface::PreAndPostOrder))
      return false;

    proc.TranslateCtrlDeps();

    // 4/21/2014: Delete loop node will break LoopDepGraph.
    // But keep loop node will break dump function.
    enum { DeleteLoopNode = 1 };

    if (DeleteLoopNode) {
      boost::unordered_set<Vertex *> loopNodes;
      for (VertexIterator iter = graph.GetNodeIterator(); !iter.ReachEnd(); ++iter)
        if (Vertex *n = iter.Current())
          if (n->isLoop())
            loopNodes.insert(n);

      foreach (Vertex *n, loopNodes)
        graph.DeleteNode(n);
    }

    return true;

  } catch (std::exception &e) {
    graph.setRoot(nullptr);
    std::cerr << boost::diagnostic_information(e) << std::endl;
    return false;
  }
}


// EOF

/*

// - AST attribute processing -

namespace { // anonymous, AST processing

  class NestingLevelAnnotation : public AstAttribute
  {
      int level_, depth_;

  public:
      NestingLevelAnnotation(int level, int depth)
          : level_(level), depth_(depth) { }

      int level() const { return level_; }
      int depth() const { return depth_; }
  };

  class LoopLevelProcessing
      : protected AstTopDownBottomUpProcessing<int, int>
  {
      int maxLevel_;

  public:
      LoopLevelProcessing() : maxLevel_(0) { }

      void attachLoopNestingAnnotation(SgProject *node) { traverseInputFiles(node, 0); }

      int maxLevel() { return maxLevel_; }

  protected:

      int evaluateInheritedAttribute(SgNode *n, int level)
      {
          if (level > maxLevel_)
              maxLevel_ = level;

          ROSE_ASSERT(n);
          switch(n->variantT()) {
          case V_SgGotoStatement:
              // Warning: goto loops are ignored
              return level;

          case V_SgDoWhileStmt:
          case V_SgForStatement:
          case V_SgWhileStmt:
              return level + 1;

          default:
              return level;
          }
      }

      virtual int defaultSynthesizedAttribute(int)
      { return 0; }

      virtual int evaluateSynthesizedAttribute(SgNode *n, int level, SynthesizedAttributesList l)
      {
          if (level > maxLevel_)
              maxLevel_ = level;

          int depth = 0;
          for (SynthesizedAttributesList::iterator i = l.begin(); i != l.end(); i++)
              if (*i > depth)
                  depth = *i;
          switch(n->variantT()) {
          case V_SgDoWhileStmt:
          case V_SgForStatement:
          case V_SgWhileStmt:
              depth++;
          }
          std::cout<<level<<","<<depth<<std::endl;
          n->addNewAttribute("loopNestingInfo", new NestingLevelAnnotation(level, depth));
          return depth;
      }
  };

} // anonymous namespace

// - Helper functions -

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

namespace { // anonymous, type cast

  // Cast between different vertex pointer representations

  template <typename T, typename S>
  inline T type_cast(S input)
  { return S::__cast_not_defined__(input); } // static_assert(0)

  template <>
  inline SgNode*
  type_cast<SgNode*>(AstNodePtr input)
  { return static_cast<SgNode*>(input.get_ptr()); }

  template <>
  inline AstNodePtr
  type_cast<AstNodePtr>(SgNode *input)
  { return AstNodePtrImpl(input); }

  template <>
  inline SgNode*
  type_cast<SgNode*>(Vertex *input)
  { return input? input->statement() : 0; }

} // anonymous namespace

// - Graph constructions implementation -

namespace { // anonymous, AST traversal

  // Filter top level statements in the AST
  // Note: if/else statement is not filtered to be consistent with Dr.Yi's implementation.
  class StatementFilter : protected AstTopDownProcessing<bool>
  {
      typedef StatementFilter Self;
      typedef AstTopDownProcessing<bool> Base;

      SgNode *root_;
      SgStatementPtrList stmts_;

  public:
      explicit StatementFilter(SgNode *root)
      { if (root) traverse(root, true); }

  const SgStatementPtrList &statements() const { return stmts_; }

  protected:
      virtual bool evaluateInheritedAttribute(SgNode *stmt, bool traverse);
  };

  bool
  StatementFilter::evaluateInheritedAttribute(SgNode *stmt, bool traverse)
  {
      // jichi 7/18/2011: The following logic acts exactly the same as traversing
      // Dr. Yi's LoopTree created by LoopTreeBuild.
      // But I don't think the while/dowhile/if/ifelse cases are correctly handled.
      //
      #define APPEND(_to, _from) boost::copy((_from), std::back_inserter(_to))

      ROSE_ASSERT(stmt);
      if (traverse)
          if (::isSgStatement(stmt) || ::isSgFunctionDefinition(stmt)) {

              if (::isSgDeclarationStatement(stmt))
                  return true;

              SgStatementPtrList l, l2;
              bool skip_stmt = false;
              switch (stmt->variantT()) {
              case V_SgBasicBlock:
                  return true;

              case V_SgForStatement:
                  l = Self(::isSgForStatement(stmt)->get_loop_body()).statements();
                  break;

              //case V_SgIfStmt:
              //    l = Self(::isSgIfStmt(stmt)->get_conditional()).statements();
              //    l2 = Self(::isSgIfStmt(stmt)->get_true_body()).statements();
              //    l3 = Self(::isSgIfStmt(stmt)->get_false_body()).statements();
              //    break;

              case V_SgWhileStmt:
                  l = Self(::isSgWhileStmt(stmt)->get_body()).statements();
                  break;

              case V_SgDoWhileStmt:
                  l = Self(::isSgDoWhileStmt(stmt)->get_body()).statements();
                  l2 = Self(::isSgDoWhileStmt(stmt)->get_condition()).statements();
                  break;

              case V_SgFunctionDefinition:
                  skip_stmt = true;
                  l = Self(::isSgFunctionDefinition(stmt)->get_body()).statements();
                  break;
              }

              if (!skip_stmt) {
                  ROSE_ASSERT(::isSgStatement(stmt));
                  stmts_.push_back(::isSgStatement(stmt));
              }

              if (!l.empty())
                  APPEND(stmts_, l);
              if (!l2.empty())
                  APPEND(stmts_, l2);

              traverse = false;
          }

      return traverse;
      #undef APPEND
  }

} // anonymous namespace
// - Function side effect for dependency analysis -

namespace { // anonymous, ROSE semantic queries


  ///    Return the initializedName for \p input variable.
  SgNode*
  getVariableInitializedName(SgNode *input)
  {
      #define SELF(_var) getVariableInitializedName(_var)
      if (input)
          switch (input->variantT()) {
          case V_SgInitializedName: return input;
          case V_SgVarRefExp:             return SELF(::isSgVarRefExp(input)->get_symbol());
          case V_SgVariableSymbol:    return SELF(::isSgVariableSymbol(input)->get_declaration());
          }

      return 0;
      #undef SELF
  }

  ///    If variables \p x and \p y have the same initializedName (i.e. same name and same scope)
  inline bool
  sameInitialziedName(SgNode *x, SgNode *y)
  { return getVariableInitializedName(x) == getVariableInitializedName(y); }

  SgFunctionDeclaration*
  getFunctionDeclaration(SgNode *input)
  {
#define SELF(_x) getFunctionDeclaration(_x)
      if (input)
          switch (input->variantT()) {
          case V_SgFunctionDeclaration: return ::isSgFunctionDeclaration(input);
          case V_SgFunctionRefExp:            return SELF(::isSgFunctionRefExp(input)->get_symbol());
          case V_SgFunctionCallExp:         return SELF(::isSgFunctionCallExp(input)->get_function());
          case V_SgFunctionSymbol:            return SELF(::isSgFunctionSymbol(input)->get_declaration());
          case V_SgFunctionDefinition:    return SELF(::isSgFunctionDefinition(input)->get_declaration());
          }

      return 0;
#undef SELF
  }

  ///    If \p type is an array, non-const pointer, or non-const reference.
  bool
  isNonConstPointerType(const SgType *type)
  {
      ROSE_ASSERT(type);
      return ::isSgArrayType(const_cast<SgType*>(type))
              || SageInterface::isPointerToNonConstType(const_cast<SgType*>(type))
              || SageInterface::isNonconstReference(const_cast<SgType*>(type));
  }

  ///    If the node is in the global scope
  bool
  isInGlobalScope(const SgNode *input)
  { return SageInterface::getScope(input) == SageInterface::getGlobalScope(input); }

} // anonymous namespace
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
          case V_SgFunctionSymbol:                return SELF(::isSgFunctionSymbol(input)->get_declaration());
          case V_SgFunctionDefinition:        return SELF(::isSgFunctionDefinition(input)->get_body());

          case V_SgFunctionDeclaration: {
                  SgFunctionDeclaration *decl = ::isSgFunctionDeclaration(input);

                  BOOST_FOREACH (SgNode *ref,    SELF(decl->get_definition()))
                      BOOST_FOREACH (SgNode *arg, decl->get_args())
                          if (::sameInitialziedName(arg, ref))
                              ret.push_back(arg); // TODO: add support for returning array reference
              } break;

              // Statements:
          case V_SgBasicBlock: {
                  BOOST_FOREACH (SgStatement *stmt, ::isSgBasicBlock(input)->get_statements())
                      APPEND(ret, SELF(stmt));
              } break;

          case V_SgForInitStatement: {
                  BOOST_FOREACH (SgStatement *stmt, ::isSgForInitStatement(input)->get_init_stmt())
                      APPEND(ret, SELF(stmt));
              } break;

          case V_SgIfStmt: {
                  BOOST_AUTO(stmt, ::isSgIfStmt(input));
                  ret = SELF(stmt->get_conditional());
                  APPEND(ret, SELF(stmt->get_true_body()));
                  APPEND(ret, SELF(stmt->get_false_body()));
              } break;

          case V_SgWhileStmt: {
                  BOOST_AUTO(stmt, ::isSgWhileStmt(input));
                  ret = SELF(stmt->get_condition());
                  APPEND(ret, SELF(stmt->get_body()));
              } break;

          case V_SgDoWhileStmt: {
                  BOOST_AUTO(stmt, ::isSgDoWhileStmt(input));
                  ret = SELF(stmt->get_condition());
                  APPEND(ret, SELF(stmt->get_body()));
              } break;

          case V_SgForStatement: {
                  BOOST_AUTO(stmt, ::isSgForStatement(input));
                  ret = SELF(stmt->get_for_init_stmt());
                  APPEND(ret, SELF(stmt->get_test()));
                  APPEND(ret, SELF(stmt->get_increment()));
                  APPEND(ret, SELF(stmt->get_loop_body()));
              } break;

          case V_SgCaseOptionStmt:    return SELF(::isSgCaseOptionStmt(input)->get_body());
          case V_SgExprStatement:     return SELF(::isSgExprStatement(input)->get_expression());
          case V_SgReturnStmt: return SELF(::isSgReturnStmt(input)->get_expression());

              // Expressions

          case V_SgFunctionRefExp:    return SELF(::isSgFunctionRefExp(input)->get_symbol());
          case V_SgFunctionCallExp: {
                  BOOST_AUTO(call, ::isSgFunctionCallExp(input));
                  ROSE_ASSERT(call->get_args());
                  BOOST_AUTO(called_function, call->get_function());

                  BOOST_AUTO(decl, getFunctionDeclaration(called_function));
                  if (!decl)
                      break;
                  BOOST_AUTO(def, decl->get_definition());
                  BOOST_AUTO(called_args, ::isSgExprListExp(call->get_args())->get_expressions());
                  BOOST_AUTO(decl_args, decl->get_args());

                  // If it is a recursive function call, or found var_args
                  if (boost::find(visited, def) != visited.end() // i.e. there are same function calls in the stack
                          || decl_args.size() != called_args.size())                // args count mismatch for var_args
                      BOOST_FOREACH (SgNode *ref,    called_args)
                          APPEND(ret, SELF(ref));

                  else { // otherwise
                      visited.push_back(def);
                      RETURN all_refs = SELF(def);

                      for (int i = 0; i < std::min(decl_args.size(), called_args.size()); i++) {
                          // Skip scalar parameters for computing write references
                          if (rw == WRITE
                                  && !isNonConstPointerType( decl_args[i]->get_type() ))
                              continue;

                          BOOST_FOREACH (SgNode *ref,    all_refs)
                              if (::sameInitialziedName(decl_args[i], ref))
                                  APPEND(ret, SELF(called_args[i]));
                      }
#if BOOST_VERSION >= 104600
                      APPEND(ret, all_refs | boost::adaptors::filtered(isInGlobalScope));
#else
                      BOOST_FOREACH (SgNode *ref, all_refs) if (isInGlobalScope(ref))
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
              if (::isSgExpression(input))
                  APPEND(ret, NodeQuery::querySubTree(input, V_SgVarRefExp)); // TODO: Array not correctly handled!
              else if (::isSgDeclarationStatement(input))
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
