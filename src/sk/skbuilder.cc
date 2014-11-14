// skbuilder.cc
// 9/12/2012 jichi

#include "sk/skbuilder.h"
#include "sk/skbuilder_p.h"
#include "sk/sknode.h"
#include "xt/xt.h"
//#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/unordered/unordered_set.hpp>

//#define SK_DEBUG "skbuilder"
#include "sk/skdebug.h"
#include <iostream>
//#define delete

#ifdef __clang__
# pragma clang diagnostic ignored "-Wlogical-op-parentheses" // add parenthesis to || &&
#endif // __clang__

#define foreach BOOST_FOREACH

typedef boost::tuple<int, int, int> tuple_iii;

inline tuple_iii &operator+=(tuple_iii &x, const tuple_iii &y)
{
  x.get<0>() += y.get<0>();
  x.get<1>() += y.get<1>();
  x.get<2>() += y.get<2>();
  return x;
}

namespace { // anonymous

  inline SkExpression *_makeexp_sub_one(SkExpression *e)
  {
    SK_ASSERT(e);
    SkNode *parent = e->parent();
    SkExpression *ret = new SkBinaryOperation(Sk::O_Sub, e, new SkValue(1), e->source());
    ret->setParent(parent);
    return ret;
  }

  inline SkExpression *_makeexp_add_one(SkExpression *e)
  {
    SK_ASSERT(e);
    SkNode *parent = e->parent();
    SkExpression *ret = new SkBinaryOperation(Sk::O_Add, e, new SkValue(1), e->source());
    ret->setParent(parent);
    return ret;
  }

} // anonymous namespace

SK_BEGIN_NAMESPACE

// - Global -

SkStatementList SkBuilder::detail::collectBlockStatements(SkBlock *input)
{
  SkStatementList ret;
  if (!input)
    return ret;
  foreach (SkStatement *it, input->statements())
    if (SkBlock *block = sknode_cast<SkBlock *>(it)) {
      SkStatementList l = collectBlockStatements(block);
      if (!l.empty())
        ret.splice(ret.end(), l);
    } else
      ret.push_back(it);
  return ret;
}

SkGlobal *SkBuilder::fromGlobal(SgNode *input)
{
  if (!input)
    return nullptr;
  SK_DPRINT("enter: sage class = " << input->class_name());
  switch (input->variantT()) {
  case V_SgSourceFile:
    SK_DPRINT("leave: source file");
    return fromGlobal(::isSgSourceFile(input)->get_globalScope());
  case V_SgGlobal:
    {
      SkBlock *b = new SkBlock(input);
      foreach (SgNode *it, ::isSgGlobal(input)->get_declarations())
        if (!::isSgFunctionDeclaration(it))
          b->append(sknode_cast<SkStatement *>(fromDeclaration(it, nullptr)));
      SkGlobal *ret = new SkGlobal(input);
      foreach (SkStatement *it, detail::collectBlockStatements(b))
        if (!::sknode_cast<SkFunctionDeclaration *>(it))
          ret->append(it);
      delete b;
      if (ret->isEmpty()) {
        delete ret;
        ret = nullptr;
      }
      return ret;
    } SK_ASSERT(0);
  default:
    std::cerr << "skbuilder:" << __FUNCTION__ << ": "
        << "unhandled sage class = " << input->class_name()
        << std::endl;
    SK_DPRINT("leave: ret = null");
    return nullptr;
  }
  SK_ASSERT(0);
}

SkFunctionDefinition *SkBuilder::fromFunction(SgNode *input, const Option *opt)
{
  SgFunctionDefinition *src = ::isSgFunctionDefinition(input);
  if (!src)
    return nullptr;
  SK_DPRINT("enter: sage class = " << input->class_name() << ", opt = " << opt);

  SkNode *ret = fromDeclaration(src, opt);
  if (ret && ret->classType() != Sk::C_FunctionDefinition) {
    SK_DPRINT("delete invalid decl");
    delete ret;
    ret = nullptr;
  }

  if (opt && opt->danglingSlices && !opt->danglingSlices->empty())
    std::cerr << "skbuilder:" << __FUNCTION__ << ": warning: there are dangling slices" << std::endl;
  SK_DPRINT("leave: ret = " << ret);
  return static_cast<SkFunctionDefinition *>(ret);
}

// - Declarations -
// SgInitializedNamePtrList: a list of variable names
SkStatement *SkBuilder::detail::fromDeclarationList(const SgInitializedNamePtrList &l, SgNode *src, const Option *opt)
{
  SK_DPRINT("enter: size = " << l.size() << ", opt = " << opt);
  switch (l.size()) {
  case 0: SK_DPRINT("leave: empty decl list"); return nullptr;
  case 1: SK_DPRINT("leave: var decl"); return fromDeclaration(l.front(), opt);
  default:
    {
      SkBlock *ret = new SkBlock(src);
      foreach (SgInitializedName *it, l)
        if (SkStatement *decl = fromDeclaration(it, opt))
          ret->append(decl);
      SK_DPRINT("leave: var decl list");
      return ret;
    }
  }
  SK_ASSERT(0);
}

// JG 10/12/2012:
// SgExpressionPtrList: a list of expressions
// An expression != a statement. Check this example in C language:
//    expression: x = 1
//    statement:  x = 1;
SkStatement *SkBuilder::detail::fromDeclarationList(const SgExpressionPtrList &l, SgNode *src, const Option *opt)
{
  SK_DPRINT("enter: size = " << l.size() << ", opt = " << opt);
  switch (l.size()) {
  case 0: SK_DPRINT("leave: empty decl list"); return nullptr;
  case 1: SK_DPRINT("leave: var decl"); return fromDeclaration(l.front(), opt);
  default:
    {
      SkBlock *ret = new SkBlock(src);
      foreach (SgExpression *it, l)
        if (SkStatement *decl = fromDeclaration(it, opt))
          ret->append(decl);
      SK_DPRINT("leave: var decl list");
      return ret;
    }
  }
  SK_ASSERT(0);
}

SkStatement *SkBuilder::detail::fromDeclarationList(const SgDeclarationStatementPtrList &l, SgNode *src, const Option *opt)
{
  SK_DPRINT("enter: size = " << l.size() << ", opt = " << opt);
  switch (l.size()) {
  case 0: SK_DPRINT("leave: empty decl list"); return nullptr;
  case 1: SK_DPRINT("leave: var decl"); return fromDeclaration(l.front(), opt);
  default:
    {
      SkBlock *ret = new SkBlock(src);
      foreach (SgDeclarationStatement *it, l)
        if (SkStatement *decl = fromDeclaration(it, opt))
          ret->append(decl);
      SK_DPRINT("leave: var decl list");
      return ret;
    }
  }
  SK_ASSERT(0);
}

SkStatement *SkBuilder::fromDeclaration(SgNode *input, const Option *opt)
{
  if (!input)
    return nullptr;
  SK_DPRINT("enter: sage class = " << input->class_name() << ", opt = " << opt);
  switch (input->variantT()) {
  //case V_SgFunctionDeclaration:
  //  SK_DPRINT("leave: ignore pure function declaration");
  //  return nullptr;

  case V_SgImplicitStatement:
    SK_DPRINT("leave: fortran implicit stmt ignored");
    return nullptr;

  case V_SgPragmaDeclaration:
    SK_DPRINT("leave: pragma");
    return fromPragma(input);

  case V_SgUseStatement: // Ignore fortran use module statement
    SK_DPRINT("leave: import statement");
    return new SkImportStatement(::isSgUseStatement(input)->get_name().str(), input);

  case V_SgModuleStatement:
    SK_DPRINT("leave: module");
    return fromDeclaration(::isSgModuleStatement(input)->get_definition(), opt);
  case V_SgClassDefinition:
    SK_DPRINT("leave: class definition");
    return detail::fromDeclarationList(::isSgClassDefinition(input)->getDeclarationList(), input, opt);

  case V_SgFunctionDefinition:
    {
      // This is the function body not including the "declaration" part
      SgFunctionDefinition *g = ::isSgFunctionDefinition(input);
      SkStatement *k = fromDeclaration(g->get_declaration(), opt);
      if (!k || k->classType() != Sk::C_FunctionDeclaration) {
        if (k) {
          SK_DPRINT("trash decl");
          delete k;
        }
        SK_DPRINT("leave: invalid function declaration");
        return nullptr;
      }
      SkFunctionDeclaration *decl = static_cast<SkFunctionDeclaration *>(k);
      SkFunctionDefinition *ret = new SkFunctionDefinition(input);
      ret->setMainEntry(decl->isMainEntry());
      ret->setName(decl->name());
      if (decl->parameterCount())
        ret->setParameters(decl->parameters());
      SK_DPRINT("trash decl");
      delete decl;
      ret->setBody(detail::fromBlock(g->get_body(), opt));
      if (SageInterface::is_Fortran_language()) {
        SkBlock *b = sknode_cast<SkBlock *>(ret->body());
        if (b && !b->isEmpty()) {
          boost::unordered_set<std::string> symbols;
          foreach (SkSymbolDeclaration *d, ret->parameters())
            symbols.insert(d->symbol()->toString());
          SkStatementList &l = b->rstatements();
          SkStatementList::reverse_iterator p = l.rbegin();
          while (p != l.rend()) {
            SkSymbolDeclaration *d = sknode_cast<SkSymbolDeclaration *>(*p);
            if (d && d->symbol()) {
              std::string var = d->symbol()->toString();
              if (symbols.count(var)) {
                // Erase a reverse_interator
                // See: http://stackoverflow.com/questions/1830158/how-to-call-erase-with-a-reverse-iterator
                l.erase(--(p.base()));
                SK_DPRINT("trash decl");
                d->setParent(nullptr);
                delete d;
                continue;
              }
              symbols.insert(var);
            }
            ++p;
          }
        }
      }
      SK_DPRINT("leave: function definition");
      return ret;
    }

  case V_SgFunctionDeclaration:
  case V_SgProcedureHeaderStatement:  // Fortran procedure, inherit functiondeclaration
  case V_SgProgramHeaderStatement:  // Fortran program, inherit functiondeclaration
    {
      SgFunctionDeclaration *g = ::isSgFunctionDeclaration(input);
      SkFunctionDeclaration *ret = new SkFunctionDeclaration(input);
      ret->setName(g->get_name().getString());
      if (SageInterface::is_Fortran_language())
        ret->setMainEntry(input->variantT() == V_SgProgramHeaderStatement);
      else
        ret->setMainEntry(ret->name() == "main");
      if (SgFunctionParameterList *g_params = g->get_parameterList())
        foreach (SgInitializedName *var, g_params->get_args())
          if (SkStatement *decl = fromDeclaration(var, opt)) {
            if (decl->classType() == Sk::C_SymbolDeclaration)
              ret->appendParameter(static_cast<SkSymbolDeclaration *>(decl));
            else {
              SK_DPRINT("trash decl");
              delete decl;
            }
          }
      SK_DPRINT("leave: function declaration");
      return ret;
    } SK_ASSERT(0);
    //return fromDeclaration(::isSgFunctionDeclaration(input)->get_parameterList(), opt);

  case V_SgVariableDefinition:
    SK_DPRINT("leave: var definition");
    return fromDeclaration(::isSgVariableDefinition(input)->get_vardefn(), opt);

  case V_SgVariableDeclaration:
    if (!SageInterface::is_Fortran_language() && opt && opt->criticalReferences) {
      bool critical = false;
      const boost::unordered_set<const SgNode *> &refs = *opt->criticalReferences;
      foreach (SgNode *var, ::isSgVariableDeclaration(input)->get_variables())
        if (rosex::isArrayReference(var) ||
            rosex::isArrayType(::isSgInitializedName(var)->get_type()) ||
            refs.count(input) || refs.count(var)) {
          critical = true;
          break;
        }

      if (!critical) {
        SK_DPRINT("leave: skip redundant initialization ");
        return nullptr;
      }
    }
    SK_DPRINT("leave: var decl");
    return detail::fromDeclarationList(::isSgVariableDeclaration(input)->get_variables(), input, opt);

  case V_SgFunctionParameterList:
    SK_DPRINT("leave: func params");
    return detail::fromDeclarationList(::isSgFunctionParameterList(input)->get_args(), input, opt);
  case V_SgExprListExp:
    SK_DPRINT("leave: expr list");
    return detail::fromDeclarationList(::isSgExprListExp(input)->get_expressions(), input, opt);

  case V_SgAttributeSpecificationStatement:
    SK_DPRINT("leave: attr spec");
    return fromDeclaration(::isSgAttributeSpecificationStatement(input)->get_parameter_list(), opt);

  case V_SgPntrArrRefExp:
  case V_SgAssignOp:
  case V_SgInitializedName:
    SK_DPRINT("leave: var decl");
    return new SkSymbolDeclaration(fromSymbol(input, opt), input);

  case V_SgContainsStatement:
    SK_DPRINT("leave: contains stmt, ignored");
    return nullptr;

  default:
    std::cerr << "skbuilder:" << __FUNCTION__ << ": "
        << "unhandled sage class = " << input->class_name()
        << std::endl;
    SK_DPRINT("leave: ret = null");
    return nullptr;
  }
  SK_ASSERT(0);
}

// - Statements -

SkStatement *SkBuilder::fromStatement(SgNode *input, const Option *opt)
{
  if (!input)
    return nullptr;
  SK_DPRINT("enter: sage class = " << input->class_name() << ", opt = " << opt);

  switch (input->variantT()) {
  case V_SgNullStatement:
    SK_DPRINT("leave: null statement");
    return new SkNullStatement(input);

  case V_SgBasicBlock:
    SK_DPRINT("leave: basic block");
    return detail::fromBlock(input, opt);

  case V_SgSwitchStatement:
    {
      SgSwitchStatement *g = ::isSgSwitchStatement(input);
      SkSwitchStatement *ret = new SkSwitchStatement(input);
      if (SgExprStatement *g_cond = ::isSgExprStatement(g->get_item_selector()))
        ret->setCondition(fromExpression(g_cond->get_expression(), opt));
      ret->setBody(detail::fromBlock(g->get_body(), opt));
      SK_DPRINT("leave: switch statement");
      return ret;
    } SK_ASSERT(0);

  case V_SgCaseOptionStmt:
    {
      SgCaseOptionStmt *g = ::isSgCaseOptionStmt(input);
      SkCaseStatement *ret = new SkCaseStatement(input);
      ret->setCondition(fromExpression(g->get_key(), opt));
      ret->setBody(detail::fromBlock(g->get_body(), opt));
      SK_DPRINT("leave: case statement");
      return ret;
    } SK_ASSERT(0);

  case V_SgDefaultOptionStmt:
    SK_DPRINT("leave: case statement");
    return new SkDefaultCaseStatement(fromStatement(::isSgDefaultOptionStmt(input)->get_body(), opt), input);

  case V_SgIfStmt:
    {
      SgIfStmt *g = ::isSgIfStmt(input);
      SkIfStatement *ret = new SkIfStatement(input);
      if (SgExprStatement *g_cond = ::isSgExprStatement(g->get_conditional()))
        ret->setCondition(fromExpression(g_cond->get_expression(), opt));
      ret->setTrueBody(detail::fromBlock(g->get_true_body(), opt));
      if (SgStatement *g_else = g->get_false_body())
        ret->setFalseBody(detail::fromBlock(g_else, opt));

      SK_DPRINT("leave: if statement");
      return ret;
    } SK_ASSERT(0);

  case V_SgWhileStmt:
    {
      SgWhileStmt *g = ::isSgWhileStmt(input);
      SkWhileStatement *ret = new SkWhileStatement(input);
      if (SgExprStatement *g_cond = ::isSgExprStatement(g->get_condition()))
        ret->setCondition(fromExpression(g_cond->get_expression(), opt));
      ret->setBody(detail::fromBlock(g->get_body(), opt));

      SK_DPRINT("leave: while statement");
      return ret;
    } SK_ASSERT(0);;

  case V_SgDoWhileStmt:
    {
      SgDoWhileStmt *g = ::isSgDoWhileStmt(input);
      SkDoWhileStatement *ret = new SkDoWhileStatement(input);
      if (SgExprStatement *g_cond = ::isSgExprStatement(g->get_condition()))
        ret->setCondition(fromExpression(g_cond->get_expression(), opt));
      ret->setBody(detail::fromBlock(g->get_body(), opt));

      SK_DPRINT("leave: do while statement");
      return ret;
    } SK_ASSERT(0);;

  case V_SgFortranDo:
  case V_SgForStatement:
    {
      SkForStatement *ret = new SkForStatement(input);
      bool forever = false;

      SgInitializedName *ivar;
      SgExpression *lb, *ub, *step;
      SgStatement *body = nullptr;
      if (rosex::isCanonicalLoop(input, &ivar, &lb, &ub, &step, &body)) {
        if (ivar) {
          SkVariable *k_var = new SkVariable(ivar);
          k_var->setName(ivar->get_name().getString());
          k_var->setType(fromType(ivar->get_type(), opt));
          ret->setVariable(k_var);
        }
        ret->setStep(fromExpression(step, opt));
        ret->setStart(fromExpression(lb, opt));
        SkExpression *e = fromExpression(ub, opt);
        if (SageInterface::is_Fortran_language())
          e = _makeexp_add_one(e);
        ret->setStop(e);
      } else
        switch (input->variantT()) {
        case V_SgForStatement:
          body = ::isSgForStatement(input)->get_loop_body(); break;
        case V_SgFortranDo:
          forever = true;
          body = ::isSgFortranDo(input)->get_body(); break;
        default: SK_ASSERT(0);
        }

      if (body)
        ret->setBody(detail::fromBlock(body, opt));

      SK_DPRINT("leave: for statement");
      if (!forever)
        return ret;
      else {
        SkForeverStatement *r = new SkForeverStatement(ret->source());
        r->setBody(ret->body());
        delete ret;
        return r;
      }
    } SK_ASSERT(0);

  case V_SgAllocateStatement:
    if (SgExprListExp *refs = ::isSgAllocateStatement(input)->get_expr_list()) {
      SkBlock *ret = new SkBlock(input);
      foreach (SgExpression *e, refs->get_expressions())
        ret->append(new SkExpressionStatement(
          new SkAllocateExpression(
            fromExpression(e, opt), input),
          input));
      SK_DPRINT("leave: allocate stmt");
      return ret;
    }
    return nullptr;

  case V_SgDeallocateStatement:
    if (SgExprListExp *refs = ::isSgDeallocateStatement(input)->get_expr_list()) {
      SkBlock *ret = new SkBlock(input);
      foreach (SgExpression *e, refs->get_expressions())
        ret->append(new SkExpressionStatement(
          new SkDeallocateExpression(
            fromExpression(e, opt), input),
          input));
      SK_DPRINT("leave: allocate stmt");
      return ret;
    }
    return nullptr;

  case V_SgExprStatement:
    if (opt && opt->criticalReferences) {
      const boost::unordered_set<const SgNode *> &refs = *opt->criticalReferences;
      SgExpression *lhs, *rhs;
      if (SageInterface::isAssignmentStatement(input, &lhs, &rhs) &&
          !::isSgAggregateInitializer(rhs) &&
          //!rosex::isArrayType(lhs->get_type()) &&
          //!rosex::isFortranArraySubscript(lhs) &&
          (
            rosex::isArrayReference(lhs) ||
            !refs.count(input) && !refs.count(lhs))
          ) {
        SK_DPRINT("leave: skip redundant assignment ");
        return nullptr;
      }
    }
    if (SageInterface::is_Fortran_language()) {
      SkExpressionStatement *expstmt = new SkExpressionStatement(
        fromExpression(::isSgExprStatement(input)->get_expression(), opt),
        input
      );
      if (SkFunctionCall *call = sknode_cast<SkFunctionCall *>(expstmt->expression())) {
        std::list<SkList *> lists;
        foreach (SkExpression *e, call->arguments())
          if (SkList *l = sknode_cast<SkList *>(e))
            lists.push_back(l);

        if (!lists.empty()) {
          SkBlock *ret = new SkBlock(input);
          foreach (SkList *l, lists) {
            SkSymbol *var = detail::createTempScalar();
            SkReference *ref = new SkReference(var, var->source());
            SkStatement *stmt = detail::createAssignment(ref, l, input);
            ret->append(stmt);
            call->replaceArgument(l, ref->clone());
          }
          ret->append(expstmt);
          SK_DPRINT("leave: fortran funcrion call with slices");
          return ret;
        }
      }
      SK_DPRINT("leave: expr stmt");
      return expstmt;
    } else { // C/C++
      SgExpression *lhs, *rhs;
      if (SageInterface::isAssignmentStatement(input, &lhs, &rhs))
        foreach (SgNode *it, NodeQuery::querySubTree(rhs, V_SgFunctionCallExp))
          if (SgFunctionCallExp *f = ::isSgFunctionCallExp(it))
            if (SageInterface::get_name(f) == "function_call_function_ref_malloc")
              if (SkExpression *k_e = fromExpression(lhs, opt)) {
                if (SkReference *k_ref = sknode_cast<SkReference *>(k_e))
                  if (SgExprListExp *l = f->get_args())
                    foreach (SgExpression *size, l->get_expressions()) {
                      SkArrayReference *k_a = new SkArrayReference(k_ref->symbol(), k_ref->source());
                      delete k_e;
                      std::string type = lhs->get_type()->unparseToString();
                      boost::erase_all(type, "*");
                      boost::erase_all(type, "unsigned");
                      boost::erase_all(type, " ");
                      SkExpression *k_typesize =  new SkReference(detail::createScalar(type, Sk::S_Integer));
                      SkExpression *k_size = fromExpression(size, opt);
                      SkBinaryOperation *dim = new SkBinaryOperation(Sk::O_Div, k_size, k_typesize);
                      k_a->appendIndex(dim);
                      SkAllocateExpression *k_alloc = new SkAllocateExpression(k_a, input);
                      SK_DPRINT("leave: malloc stmt");
                      return new SkExpressionStatement(k_alloc, input);
                    }

                delete k_e;
              }
      SK_DPRINT("leave: expr stmt");
      return new SkExpressionStatement(
        fromExpression(::isSgExprStatement(input)->get_expression(), opt),
        input
      );
    } SK_ASSERT(0);

  case V_SgPragmaDeclaration:
    SK_DPRINT("leave: pragma declaration");
    return fromDeclaration(input, opt);
  case V_SgVariableDeclaration:
    SK_DPRINT("leave: variable declaration");
    return fromDeclaration(input, opt);
  case V_SgVariableDefinition:
    SK_DPRINT("leave: variable definition");
    return fromDeclaration(input, opt);
  case V_SgImplicitStatement:
    SK_DPRINT("leave: implicit stmt");
    return fromDeclaration(input, opt);
  case V_SgAttributeSpecificationStatement:
    SK_DPRINT("leave: attribute spec");
    return fromDeclaration(input, opt);

  case V_SgBreakStmt:
    SK_DPRINT("leave: break stmt");
    return new SkBreakStatement(input);
  case V_SgContinueStmt:
    SK_DPRINT("leave: continue stmt");
    return new SkContinueStatement(input);
  case V_SgReturnStmt:
    SK_DPRINT("leave: return stmt");
    return new SkReturnStatement(fromExpression(::isSgReturnStmt(input)->get_expression(), opt), input);

  default:
    std::cerr << "skbuilder:fromStatement: ignored stmt class: " << input->class_name();
    SK_DPRINT("leave: comp");
    // JM: why would there be another SkComputeStatement? Another one in fromBlock
    // JG 10/12/2012: It's just implementation detail.
    // The fromBlock function should not be invoked from outside.
    // The fromBlock function is just used to implement fromStatement
    // JM: None of the above answer my question...
    // JG: Here's an example where a comp statement might need out of a compount statment
    // Source code:
    //   if (some_cond)
    //     a = b + c;   // This is not a compount statement, but still we should compute comp for it.
    // Note: the above example is different from the following:
    //   if (some_cond)
    //   { a = b + c; } // This is a compount statement
    {
      int c, ip, fp;

      boost::tie(c, ip, fp) = compOf(input);
      if (ip && fp) {
        SkBlock *ret = new SkBlock(input);
        ret->append(new SkFixedPointStatement(ip, input));
        ret->append(new SkFloatingPointStatement(fp, input));
        return ret;
      } else if (ip && !fp)
        return new SkFixedPointStatement(ip, input);
      else if (!ip && fp)
        return new SkFloatingPointStatement(fp, input);
      else
        return nullptr;
    }
  }

  SK_ASSERT(0);
}

SkStatement *SkBuilder::detail::fromBlock(SgNode *input, const Option *opt)
{
  SgStatement *src = ::isSgStatement(input);
  if (!src)
    return nullptr;
  SK_DPRINT("enter: sage class = " << input->class_name() << ", opt = " << opt);

  SkBlock *ret = new SkBlock(input);
  ret->setComment(rosex::getComment(input));
  boost::unordered_set<std::string> refs;
  tuple_iii comp;
  int br = 0;

  SkBlockBarrier *barrier = nullptr;
#define BARRIER_BEGIN(begin) \
  if (!barrier) { \
    barrier = new SkBlockBarrier(input); \
    barrier->setBeginSource(begin); \
    ret->append(barrier); \
  }

#define BARRIER_END(end) \
  if (barrier) { \
    barrier->setEndSource(end); \
    if (barrier->isEmpty()) { \
      ret->remove(barrier); \
      delete barrier; \
    } \
    barrier = nullptr; \
  \
    int ip = comp.get<1>(), \
        fp = comp.get<2>(); \
    if (ip) \
      ret->append(new SkFixedPointStatement(ip, input)); \
    if (fp) \
      ret->append(new SkFloatingPointStatement(fp, input)); \
    if (br) \
      ret->append(new SkBranchStatement(br, input)); \
 \
    refs.clear(); \
    comp = tuple_iii(); \
    br = 0; \
  }

  SgStatementPtrList stmts;
  if (SgBasicBlock *b = ::isSgBasicBlock(input))
    stmts = b->get_statements();
  else
    stmts.push_back(src);

  foreach (SgNode *stmt, stmts) { // | boost::adaptors::filtered(SageInterface::isCanonicalForLoop))
    SK_DPRINT("stmt = " << stmt->class_name());
    BARRIER_BEGIN(stmt)

    switch (stmt->variantT()) {
      // Scope
    case V_SgUseStatement: // Ignore fortran use module statement
      ret->append(fromDeclaration(stmt, opt));
      break;

    // JG 10/12/2012: it iterates the statements in the body of the basic block.
    // JG: The function is called upon a "scoped" block, which is a SgBasicBlock (!= Basic block)
    // JG: branches are counted, and they are explored as well. In the end, the skeleton outputs either of them.
    // In C, a basic block is another name for a compound statement.
    //   an expr stmt:                    x = 1;
    //   a basic block (compound stmt):   { x = 1; }
    // - In C, a basic block could appear anywhere, not only as the body of a branch, loop, class.
    //   Such as:  int x; { x = 0; }
    // - In C, the true-branch and false-branch of an if statement are not guranteed to be a basic block.
    //   Such as:  if (true) x = 1;
    case V_SgIfStmt:
    case V_SgWhileStmt:
    case V_SgDoWhileStmt:
    case V_SgSwitchStatement:
      // jichi 10/22/2013: Disabled
      //br++;
    case V_SgCaseOptionStmt:
    case V_SgDefaultOptionStmt:
    case V_SgBasicBlock:
    case V_SgFortranDo:
    case V_SgForStatement:
    case V_SgBreakStmt:
    case V_SgContinueStmt:
    case V_SgReturnStmt:
      BARRIER_END(stmt)
    case V_SgPragmaDeclaration:
      {
        SgNodePtrList l;
        switch (stmt->variantT()) {
        case V_SgIfStmt:
          l.push_back(::isSgIfStmt(stmt)->get_conditional());
          break;
        case V_SgWhileStmt:
          l.push_back(::isSgWhileStmt(stmt)->get_condition());
          break;
        case V_SgDoWhileStmt:
          l.push_back(::isSgDoWhileStmt(stmt)->get_condition());
          break;
        case V_SgSwitchStatement:
          l.push_back(::isSgSwitchStatement(stmt)->get_item_selector());
          break;
        case V_SgFortranDo:
          l.push_back(::isSgFortranDo(stmt)->get_initialization());
          l.push_back(::isSgFortranDo(stmt)->get_bound());
          l.push_back(::isSgFortranDo(stmt)->get_increment());
          break;
        case V_SgForStatement:
          foreach (SgStatement *s, ::isSgForStatement(stmt)->get_init_stmt())
            l.push_back(s);
          l.push_back(::isSgForStatement(stmt)->get_test());
          l.push_back(::isSgForStatement(stmt)->get_increment());
          break;
        default: ;
        }

        boost::unordered_set<std::string> refs;
        foreach (SgNode *n, l)
          foreach (SkStatement *p, refOf(n, opt))
            if (p) {
              std::string ref = p->toString();
              if (!refs.count(ref)) {
                refs.insert(ref);
                ret->append(p);
              } else {
                SK_DPRINT("delete non-critical reference");
                delete p;
              }
            }

        tuple_iii comp;
        foreach (SgNode *n, l)
          comp += compOf(n);

        int ip = comp.get<1>(),
            fp = comp.get<2>();
        if (ip)
          ret->append(new SkFixedPointStatement(ip, stmt));
        if (fp)
          ret->append(new SkFloatingPointStatement(fp, stmt));
      }

      ret->append(fromStatement(stmt, opt));
      if (SageInterface::is_Fortran_language() && stmt->variantT() == V_SgCaseOptionStmt)
        ret->append(new SkBreakStatement);
      break;

    case V_SgExprStatement:
      if (::isSgFunctionCallExp(::isSgExprStatement(stmt)->get_expression()))
        BARRIER_END(stmt)
    case V_SgVariableDeclaration:
    case V_SgVariableDefinition:
    case V_SgAllocateStatement:
    case V_SgDeallocateStatement:
      if (SageInterface::is_Fortran_language() && !NodeQuery::querySubTree(stmt, V_SgSubscriptExpression).empty()) {
        BARRIER_END(stmt)
        BARRIER_BEGIN(stmt)
        BARRIER_END(stmt)
      }

      // JG 10/12/2012: Variable declaration needs to be handled in case of "int a = A[i][j]+b"
      comp += compOf(stmt);
      // Adding references in this statement
      foreach (SkStatement *p, refOf(stmt, opt))
        if(p) {
          std::string ref = p->toString();
          if (!refs.count(ref)) {
            refs.insert(ref);
            //if (opt && opt->danglingSlices && !opt->danglingSlices->empty()) {
            //  BARRIER_END(stmt)
            //
            //  SkForStatement *f = new SkForStatement(input->source());
            //  SkVariable *var = SkBuilder::detail::createTempScalar(Sk::S_Integer);
            //  f->setVariable(var);
            //  f->setStart(slice->start());
            //  f->setStop(slice->stop());
            //  f->setStep(slice->step());
            //  array->replaceIndex(slice, new SkReference(var->clone()));
            //  delete slice;
            //}
            ret->append(p);
          } else {
            SK_DPRINT("delete non-critical reference");
            delete p;
          }
        }

      if (SkStatement *k = fromStatement(stmt, opt)) {
        if (k->classType() == Sk::C_Block) {
          ret->append(static_cast<SkBlock *>(k)->statements());
          SK_DPRINT("trash empty block");
          delete k;
        } else
          ret->append(k);
      }
      break;

    case V_SgImplicitStatement:
    case V_SgAttributeSpecificationStatement:
      {
        // JG 10/12/2012: AttributeSpecification is a specific Fortran declaration for procedure parameters
        SkStatement *k = fromStatement(stmt, opt);
        if (k && k->classType() == Sk::C_Block) {
          ret->append(static_cast<SkBlock *>(k)->statements());
          SK_DPRINT("trash empty block");
          delete k;
        } else if (k)
          ret->append(k);
      }
      break;
    default:
      std::cerr << "skbuilder:detail:fromBlock: ignored stmt class: " << stmt->class_name() << std::endl;
      comp += compOf(stmt);
    }
  }

  BARRIER_END(nullptr)

  if (ret->size() == 1) {
    SkStatement *stmt = ret->statements().front();
    stmt->setParent(nullptr);
    SK_DPRINT("trash empty block");
    delete ret;
    SK_DPRINT("leave: single stmt block");
    return stmt;
  }
  SK_DPRINT("leave");
  return ret;

#undef BARRIER_BEGIN
#undef BARRIER_END
}

// - Types -

SkType *SkBuilder::fromType(SgNode *input, const Option *opt)
{
  if (!input)
    return nullptr;
  SK_DPRINT("enter: sage class = " << input->class_name());
  switch (input->variantT()) {
  case V_SgTypeBool:

  case V_SgTypeFloat: case V_SgTypeDouble: case V_SgTypeLongDouble:

  case V_SgTypeComplex: case V_SgTypeImaginary:

  case V_SgTypeString:

  case V_SgTypeChar: case V_SgTypeSignedChar: case V_SgTypeUnsignedChar: case V_SgTypeWchar:

  case V_SgTypeShort: case V_SgTypeSignedShort: case V_SgTypeUnsignedShort:
  case V_SgTypeInt: case V_SgTypeSignedInt: case V_SgTypeUnsignedInt:
  case V_SgTypeLong: case V_SgTypeSignedLong: case V_SgTypeUnsignedLong:
  case V_SgTypeLongLong: case V_SgTypeSignedLongLong: case V_SgTypeUnsignedLongLong:

  //case V_SgTypeUnknown:
  //case V_SgTypeComplex:
    {
      Sk::ScalarType type = detail::scalarTypeFromSageVariant(input->variantT());
      SK_DPRINT("leave: scalar type");
      return new SkScalarType(type, input);
    } SK_ASSERT(0);
  case V_SgArrayType:
    {
      SgArrayType *g = ::isSgArrayType(input);
      SkType *ret = fromType(g->get_base_type(), opt);
      if (!ret) {
        //SK_ASSERT(0);
        std::cerr << "skbuilder::fromType: failed to build array type" << std::endl;
        SK_DPRINT("leave: array type");
        return nullptr;
      }
      switch (ret->classType()) {
      case Sk::C_ScalarType:
        {
          SkArrayType *array = new SkArrayType(g);
          array->setBaseType(ret);
          ret = array;
        }
      case Sk::C_ArrayType:
        {
          SkArrayType *k = static_cast<SkArrayType *>(ret);
          if (SgExprListExp *dim = g->get_dim_info()) {
            foreach (SgExpression *e, dim->get_expressions())
              if (SageInterface::is_Fortran_language())
                k->prependDimension(fromExpression(e, opt));
              else
                k->appendDimension(fromExpression(e, opt));
          } else {
            if (SageInterface::is_Fortran_language())
              k->prependDimension(fromExpression(g->get_index(), opt));
            else
              k->appendDimension(fromExpression(g->get_index(), opt));
          }
        }
        break;
      default: SK_ASSERT(0);
      }
      ret->setSource(g);
      SK_DPRINT("leave: array type");
      return ret;
    } SK_ASSERT(0);
  case V_SgPointerType:
    {
      // JG 10/12/2012: for C_xxxx, please check Sk::ClassType enum in skglobal.h.
      // JG 10/12/2012:
      // can ROSE determine whether the pointer is pointing to a scalar or an array?
      // for C, the answer is NO. I currently assume the pointer to be an array.
      SgPointerType *g = ::isSgPointerType(input);
      SkType *ret = fromType(g->get_base_type(), opt);
      if (!ret) {
        //SK_ASSERT(0);
        SK_DPRINT("leave: pointer type");
        return nullptr;
      }
      switch (ret->classType()) {
      case Sk::C_ScalarType:
        {
          SkArrayType *array = new SkArrayType(g);
          array->setBaseType(ret);
          ret = array;
        }
      case Sk::C_ArrayType:
        if (!SageInterface::is_Fortran_language())
          static_cast<SkArrayType *>(ret)->appendDimension();
        //else
        //  static_cast<SkArrayType *>(ret)->prependDimension();
        break;
      default: SK_ASSERT(0);
      }
      ret->setSource(g);
      SK_DPRINT("leave: pointer type");
      return ret;
    } SK_ASSERT(0);
  // JM: how do you handle non-array data, like a "Struct" in C, a "Class" in C++, or a "Common block" in Fortran?
  // JG 10/12/2012: Unhandled source code node will be left as a question mark
  default:
    std::cerr << "skbuilder:" << __FUNCTION__ << ": "
              << "unhandled sage type = " << input->class_name()
              << std::endl;
    SK_DPRINT("leave: ret = null");
    return nullptr;
  }
  SK_ASSERT(0);
}

// - Expressions -

SkExpression *SkBuilder::fromExpression(SgNode *input, const Option *opt)
{
  if (!input)
    return nullptr;
  SK_DPRINT("enter: sage class = " << input->class_name());

  switch (input->variantT()) {
  case V_SgNullExpression:
    SK_DPRINT("leave: null exp");
    return nullptr;

  case V_SgShortVal:
    SK_DPRINT("leave: short val");
    return new SkValue(::isSgShortVal(input)->get_value(), input);
  case V_SgUnsignedShortVal:
    SK_DPRINT("leave: ushort val");
    return new SkValue(static_cast<int>(::isSgUnsignedShortVal(input)->get_value()), input);
  case V_SgIntVal:
    SK_DPRINT("leave: int val");
    return new SkValue(::isSgIntVal(input)->get_value(), input);
  case V_SgUnsignedIntVal:
    SK_DPRINT("leave: warning: uint val truncate to int");
    return new SkValue(static_cast<int>(::isSgUnsignedIntVal(input)->get_value()), input);
  case V_SgLongIntVal:
    SK_DPRINT("leave: warning: long val truncate to int");
    return new SkValue(static_cast<int>(::isSgLongIntVal(input)->get_value()), input);
  case V_SgLongLongIntVal:
    SK_DPRINT("leave: warning: llong val truncate to int");
    return new SkValue(static_cast<int>(::isSgLongLongIntVal(input)->get_value()), input);
  case V_SgUnsignedLongVal:
    SK_DPRINT("leave: warning: ulong val truncate to int");
    return new SkValue(static_cast<int>(::isSgUnsignedLongVal(input)->get_value()), input);
  case V_SgUnsignedLongLongIntVal:
    SK_DPRINT("leave: warning: ullong val truncate to int");
    return new SkValue(static_cast<int>(::isSgUnsignedLongLongIntVal(input)->get_value()), input);
  case V_SgFloatVal:
    SK_DPRINT("leave: float val");
    return new SkValue(::isSgFloatVal(input)->get_value(), input);
  case V_SgDoubleVal:
    SK_DPRINT("leave: double val");
    return new SkValue(::isSgDoubleVal(input)->get_value(), input);
  case V_SgLongDoubleVal:
    SK_DPRINT("leave: warning: long double val truncate to double");
    return new SkValue(static_cast<double>(::isSgLongDoubleVal(input)->get_value()), input);
  case V_SgComplexVal: // TODO: complex value
    SK_DPRINT("leave: warning: complex val to zero");
    return new SkValue(0.0, input);
  case V_SgCharVal:
    SK_DPRINT("leave: char val");
    return new SkValue(::isSgCharVal(input)->get_value(), input);
  case V_SgUnsignedCharVal:
    SK_DPRINT("leave: warning: uchar val truncate to char");
    return new SkValue(static_cast<char>(::isSgUnsignedCharVal(input)->get_value()), input);
  case V_SgStringVal:
    SK_DPRINT("leave: string val");
    return new SkValue(::isSgStringVal(input)->get_value(), input);
  case V_SgBoolValExp:
    SK_DPRINT("leave: fortran bool val exp");
    return new SkValue(bool(::isSgBoolValExp(input)->get_value()), input);

  case V_SgSizeOfOp:
    SK_DPRINT("leave: sizeof exp");
    return fromExpression(::isSgSizeOfOp(input)->get_operand_expr(), opt);

  case V_SgVariableSymbol:
  case V_SgVarRefExp:
  case V_SgInitializedName:
    SK_DPRINT("leave: variable symbol");
    return new SkReference(fromSymbol(input, opt), input);

  case V_SgInitializer:
    SK_DPRINT("leave: initialier");
    return fromExpression(::isSgInitializer(input)->get_originalExpressionTree(), opt);
  case V_SgAssignInitializer:
    SK_DPRINT("leave: assign initializer");
    return fromExpression(::isSgAssignInitializer(input)->get_operand(), opt);

  case V_SgAggregateInitializer:
    if (SgExprListExp *l = ::isSgAggregateInitializer(input)->get_initializers())
      foreach (SgExpression *g, l->get_expressions())  {
        SK_DPRINT("leave: aggregate initializer");
        return fromExpression(g, opt);
      }
    std::cerr << "skbuilder::fromExpression: warning: too many aggregate initializers" << std::endl;
    return nullptr;

  case V_SgFunctionCallExp:
    {
      SgFunctionCallExp *g = ::isSgFunctionCallExp(input);
      SkFunctionCall *ret = new SkFunctionCall(input);

      if (SgExprListExp *g_args = g->get_args())
        foreach (SgExpression *e, g_args->get_expressions())
          ret->appendArgument(fromExpression(e, opt));

      SgExpression *g_func = g->get_function();
      SkSymbol *k_func = fromSymbol(g_func, opt);
      if (!k_func && g_func)  {
        std::string name = g_func->unparseToString();
        k_func = new SkFunction(name);
      }
      if (k_func) {
        if (k_func->classType() == Sk::C_Function)
          ret->setFunction(static_cast<SkFunction *>(k_func));
        else {
          SK_DPRINT("trash function");
          delete k_func;
          k_func = nullptr;
        }
      }

      // Replace size with len
      if (SageInterface::is_Fortran_language())
        if (SkFunction *f = ret->function())
          if (f->name() == "size" && ret->argumentCount() == 2) {
            f->setName("len");
            if (SkReference *a = sknode_cast<SkReference *>(ret->arguments().front()))
              if (SkSymbol *s = a->symbol())
                if (SkArrayType *t = sknode_cast<SkArrayType *>(s->type()))
                  if (int dim = t->dimensionCount())
                    if (SkExpression *index = ret->arguments().back())
                      ret->replaceArgument(index,
                          new SkBinaryOperation(Sk::O_Sub, new SkValue(dim), index, index->source()));
          }

      // Replace free with len
      if (!SageInterface::is_Fortran_language())
        if (SkFunction *f = ret->function())
          if (f->name() == "free" && ret->argumentCount() == 1) {
            SkExpression *t = new SkDeallocateExpression(ret->arguments().front(), input);
            delete ret;
            SK_DPRINT("leave: function call free()");
            return t;
          }

      SK_DPRINT("leave: function call");
      return ret;
    } SK_ASSERT(0);

  //case V_SgExprStatement:
  //  SK_DPRINT("leave: expr stmt");
  //  return fromExpression(::isSgExprStatement(input)->get_expression(), opt);

  case V_SgSubscriptExpression:
    {
      // An array index
      SgSubscriptExpression *g = ::isSgSubscriptExpression(input);
      SkSlice *ret = new SkSlice(input);
      ret->setStart(fromExpression(g->get_lowerBound(), opt));
      ret->setStop(fromExpression(g->get_upperBound(), opt));
      ret->setStep(fromExpression(g->get_stride(), opt));

      if (SageInterface::is_Fortran_language()) {
        if (ret->start())
          ret->setStart(::_makeexp_sub_one(ret->start()));
        else
          ret->setStart(new SkValue(0));
        if (!ret->step())
          ret->setStep(new SkValue(1));
        if (ret->stop())
          ret->setStop(::_makeexp_sub_one(ret->stop()));
        else {
          // Get array dimension
          SgExprListExp *dim = ::isSgExprListExp(g->get_parent());
          if (!dim)
            std::cerr << "skbuilder::fromExpression: warning: missing dim to infer slice stop" << std::endl;
          else {
            SgPntrArrRefExp *array = ::isSgPntrArrRefExp(dim->get_parent());
            if (!array)
              std::cerr << "skbuilder::fromExpression: warning: missing array to infer slice stop" << std::endl;
            else {
              SgVarRefExp *var = ::isSgVarRefExp(array->get_lhs_operand());
              if (!var)
                std::cerr << "skbjilder::fromExpression: warning: missing var to infer slice stop" << std::endl;
              else {
                SgArrayType *type = ::isSgArrayType(var->get_type());
                if (!type)
                  std::cerr << "skbuilder::fromExpression: warning: missing type to infer slice stop" << std::endl;
                else {
                  SgExprListExp *decl_dim = type->get_dim_info();
                  if (!decl_dim)
                    std::cerr << "skbuilder::fromExpression: warning: missing decl dim to infer slice stop" << std::endl;
                  else {
                    int count = dim->get_expressions().size();
                    int index = 0;
                    SgExpressionPtrList::iterator it = decl_dim->get_expressions().begin();
                    foreach (SgExpression *e, dim->get_expressions()) {
                      if (e == g) {
                        if (::isSgSubscriptExpression(*it)) {
                          SkFunctionCall *e = new SkFunctionCall;
                          e->setFunction(new SkFunction("len"));
                          e->appendArgument(fromExpression(var, opt));
                          e->appendArgument(new SkValue(count - index -1));
                          ret->setStop(e);
                        } else
                          ret->setStop(fromExpression(*it, opt));
                      }
                      ++it;
                      ++index;
                    }
                  }
                }
              }
            }
          }
        }
      }

      SK_DPRINT("leave: slice expression");
      //if (opt && opt->danglingSlices && !rosex::getParentType(input)) {
      //  SkVariable *var = detail::createTempScalar();
      //  (*opt->danglingSlices)[ret] = var;
      //  return new SkReference(var);
      //}

      return ret;
    } SK_ASSERT(0);

  case V_SgExprListExp:
    {
      SkList * ret = new SkList(input);
      foreach (SgExpression *e, ::isSgExprListExp(input)->get_expressions())
        ret->append(fromExpression(e, opt));
      SK_DPRINT("leave: list expression");
      return ret;
    } SK_ASSERT(0);

  case V_SgPntrArrRefExp:
    {
      SgPntrArrRefExp *g = ::isSgPntrArrRefExp(input);
      if (SkExpression *k_left = fromExpression(g->get_lhs_operand(), opt)) {
        SkArrayReference *ret = nullptr;
        switch (k_left->classType()) {
        case Sk::C_Reference:
          {
            SkReference *k = static_cast<SkReference *>(k_left);
            ret = new SkArrayReference(k->symbol(), input);
            SK_DPRINT("trash array ref");
            delete k;
          } break;
        case Sk::C_ArrayReference:
          ret = static_cast<SkArrayReference *>(k_left);
          break;
        default:
          std::cerr << "skbuilder:" << __FUNCTION__ << ": "
                    << "unhandled pointer subscript: " << g->get_lhs_operand()
                    << std::endl;
        }

        if (ret) {
          SgExpression *g_right = g->get_rhs_operand();
          // Getting array indices
          if (SgExprListExp *l = ::isSgExprListExp(g_right)) {
            foreach (SgExpression *e, l->get_expressions())
              // Converting subscription ordering to C-style
              if (SageInterface::is_Fortran_language()) {
                SkExpression *i = fromExpression(e, opt);
                if (!sknode_cast<SkSlice *>(i))
                  i = ::_makeexp_sub_one(i);
                ret->prependIndex(i);
              }
              else
                ret->appendIndex(fromExpression(e, opt));
          } else {
            if (SageInterface::is_Fortran_language()) {
              SkExpression *i = fromExpression(g_right, opt);
              if (!sknode_cast<SkSlice *>(i))
                i = ::_makeexp_sub_one(i);
              ret->prependIndex(i);
            } else
              ret->appendIndex(fromExpression(g_right, opt));
          }

          SK_DPRINT("leave: [] expression");
          return ret;
        }
      }
    } //break;
  default:

    if (SgBinaryOp *g = ::isSgBinaryOp(input)) {
      SkBinaryOperation *k = new SkBinaryOperation(g);
      k->setOp(detail::operatorTypeFromSageVariant(g->variantT()));
      k->setLeft(fromExpression(g->get_lhs_operand(), opt));
      k->setRight(fromExpression(g->get_rhs_operand(), opt));
      return k;
    }
    if (SgUnaryOp *g = ::isSgUnaryOp(input)) {
      SkUnaryOperation *k = new SkUnaryOperation(g);
      k->setOp(detail::operatorTypeFromSageVariant(g->variantT()));
      k->setOperand(fromExpression(g->get_operand(), opt));
      return k;
    }

    std::cerr << "skbuilder:" << __FUNCTION__ << ": "
              << "unhandled sage exp = " << input->class_name()
              << std::endl;
    //if (input->class_name() == "SgColonShapeExp") {
    //  SgNode *p = input;
    //  while (p = p->get_parent()) {
    //    std::cerr<< p << p->class_name() << " : " << p->unparseToString() << std::endl;
    //  }
    //}
    SK_DPRINT("leave: ret = null");
    return nullptr;
  }
  SK_ASSERT(0);
}

// - Symbols -

SkSymbol *SkBuilder::fromSymbol(SgNode *input, const Option *opt, const SgNode *context)
{
  if (!input)
    return nullptr;
  SK_DPRINT("enter: sage class = " << input->class_name());
  // TO_DISCUSS: In this function, I don't understand why you need recursively call fromSymbol to get some of the symbols...
  // JG 10/12/2012: In the skeleton AST, only have a simple concept of symbol.
  // But in the ROSE AST of the source code, the symbol system is complicated, where each symbol could own a subtree.

  switch (input->variantT()) {
  case V_SgVarRefExp:
    SK_DPRINT("leave: var ref exp");
    return fromSymbol(::isSgVarRefExp(input)->get_symbol(), opt, input);
  case V_SgFunctionRefExp:
    SK_DPRINT("leave: func ref exp");
    return fromSymbol(::isSgFunctionRefExp(input)->get_symbol(), opt);

  case V_SgFunctionSymbol:
    {
      SgFunctionSymbol *g = ::isSgFunctionSymbol(input);
      std::string name = g->get_name().getString();
      //SkType *type = fromType(g->get_type()); // TODO: function type is not implemented
      SK_DPRINT("leave: function symbol");
      return new SkFunction(name, input);
    } SK_ASSERT(0);
  case V_SgVariableSymbol:
    {

      SgVariableSymbol *g = ::isSgVariableSymbol(input);
      std::string name;
      if (opt && opt->variableNames) {
        BOOST_AUTO(p, opt->variableNames->find(context));
        if (p != opt->variableNames->end())
          name = p->second;
      }
      if (name.empty())
        name = g->get_name().getString();
      SkType *type = fromType(g->get_type(), opt);
      SK_DPRINT("leave: var symbol");
      return new SkVariable(name, type, input);
    } SK_ASSERT(0);
  case V_SgInitializedName:
    {
      SgInitializedName *g = ::isSgInitializedName(input);
      SkSymbol *ret = fromSymbol(g->get_symbol_from_symbol_table(), opt, input);
      if (ret)
        ret->setValue(fromExpression(g->get_initializer(), opt));
      SK_DPRINT("leave: initalized name");
      return ret;
    } SK_ASSERT(0);
  case V_SgAssignOp:
    {
      SgAssignOp *g = ::isSgAssignOp(input);
      SkSymbol *ret = fromSymbol(g->get_lhs_operand(), opt);
      SgExpression *g_value = g->get_rhs_operand();
      if (ret && g_value)
        ret->setValue(fromExpression(g_value, opt));
      SK_DPRINT("leave: assign op");
      return ret;
    } SK_ASSERT(0);

  case V_SgPntrArrRefExp:
    {
      SgPntrArrRefExp *g = ::isSgPntrArrRefExp(input);
      if (SgVarRefExp *g_var = ::isSgVarRefExp(g->get_lhs_operand())) {
        SkSymbol *k = fromSymbol(g_var->get_symbol(), opt);
        if (k) {
          SgExpression *g_dim = g->get_rhs_operand();
          if (!g_dim) {
            SK_DPRINT("leave: bare array ref");
            return k;
          }
          SkExpressionList k_dims;
          if (SgExprListExp *l = ::isSgExprListExp(g_dim)) {
            foreach (SgExpression *e, l->get_expressions())
              if (SageInterface::is_Fortran_language())
                k_dims.push_front(fromExpression(e, opt));
              else
                k_dims.push_back(fromExpression(e, opt));
          } else
            k_dims.push_back(fromExpression(g_dim, opt));
          SkType *t = k->type();
          if (!t)
            k->setType(t = new SkArrayType(input));
          SkArrayType *k_type = nullptr;
          switch (t->classType()) {
          case Sk::C_ArrayType:
            k_type = static_cast<SkArrayType *>(t);
            break;
          case Sk::C_ScalarType:
            k_type = new SkArrayType;
            k_type->setSource(t->source());
            k_type->setBaseType(t);
            break;
          default: SK_ASSERT(0);
          }
          if (k_type)
            k_type->setDimensions(k_dims);
          k->setType(k_type);
          SK_DPRINT("leave: array ref exp");
          return k;
        }
      }
    }
    SK_DPRINT("leave: invalid array ref exp");
    return nullptr;
  default:
    std::cerr << "skbuilder:" << __FUNCTION__ << ": "
        << "unhandled sage symbol = " << input->class_name()
        << std::endl;
    SK_DPRINT("leave: ret = null");
    return nullptr;
  }
  SK_ASSERT(0);
}

// - Pragma -

SkStatement *SkBuilder::fromPragma(SgNode *input)
{
  if (!input)
    return nullptr;
  SK_DPRINT("enter: sage class = " << input->class_name());
  switch (input->variantT()) {
  case V_SgPragmaDeclaration:
    SK_DPRINT("leave: pragma decl");
    return new SkPragma(input->unparseToString(), input);
  default:
    std::cerr << "skbuilder:" << __FUNCTION__ << ": "
              << "unhandled sage type = " << input->class_name()
              << std::endl;
    SK_DPRINT("leave: ret = null");
    return nullptr;
  }
  SK_ASSERT(0);
}

SK_END_NAMESPACE
