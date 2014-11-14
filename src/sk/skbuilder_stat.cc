// skbuilder_stat.cc
// 9/13/2012 jichi
// Statistics implementation

#include "sk/skbuilder.h"
#include "sk/sknode.h"
#include "rosex/rosex.h"

#define foreach BOOST_FOREACH

//#define SK_DEBUG "skbuilder_stat"
#include "sk/skdebug.h"

typedef boost::tuple<int, int, int> tuple_iii;

inline tuple_iii operator+(const tuple_iii &x, const tuple_iii &y)
{
  return tuple_iii(
    x.get<0>() + y.get<0>(),
    x.get<1>() + y.get<1>(),
    x.get<2>() + y.get<2>()
  );
}

inline tuple_iii &operator+=(tuple_iii &x, const tuple_iii &y)
{
  x.get<0>() += y.get<0>();
  x.get<1>() += y.get<1>();
  x.get<2>() += y.get<2>();
  return x;
}

SK_BEGIN_NAMESPACE

// - comp -

tuple_iii SkBuilder::compOf(const SgNode *input)
{
  if (!input)
    return tuple_iii();
  SK_DPRINT("enter: sage class = " << input->class_name());

  switch (input->variantT()) {
  case V_SgSizeOfOp:
    SK_DPRINT("leave: sizeof exp");
    return tuple_iii();

  case V_SgExprStatement:
    SK_DPRINT("leave: expr stmt");
    return compOf(::isSgExprStatement(input)->get_expression());

  case V_SgCastExp:
    SK_DPRINT("leave: cast exp");
    return compOf(::isSgCastExp(input)->get_operand());

  case V_SgExprListExp:
    {
      tuple_iii ret;
      foreach (SgExpression *e, ::isSgExprListExp(input)->get_expressions())
        ret += compOf(e);
      SK_DPRINT("leave: exp list");
      return ret;
    } break;

  case V_SgAllocateStatement:
    return compOf(::isSgAllocateStatement(input)->get_expr_list());

  case V_SgDeallocateStatement:
    return compOf(::isSgDeallocateStatement(input)->get_expr_list());

    // FIXME: Symbolic representation for comp/fp
  case V_SgSubscriptExpression :
    {
      const SgSubscriptExpression *g = ::isSgSubscriptExpression(input);
      SK_DPRINT("leave: subscript exp");
      return compOf(g->get_upperBound()) + compOf(g->get_lowerBound()) + compOf(g->get_stride());
    } break;

  case V_SgNullExpression:
    SK_DPRINT("leave: null exp");
    return tuple_iii();

  case V_SgVarRefExp:
    SK_DPRINT("leave: varref exp");
    return tuple_iii();

  case V_SgInitializedName:
    SK_DPRINT("leave: init name");
    return compOf(::isSgInitializedName(input)->get_initializer());

  case V_SgVariableDeclaration:
    SK_DPRINT("leave: var declaration");
    {
      tuple_iii ret;
      foreach (SgInitializedName *p, ::isSgVariableDeclaration(input)->get_variables())
        ret += compOf(p);
      return ret;
    } SK_ASSERT(0);

  case V_SgVariableDefinition:
    SK_DPRINT("leave: var definition");
    return compOf(::isSgVariableDefinition(input)->get_vardefn());

  //case V_SgAggregateInitializer:
  //case V_SgConstructorInitializer:
  case V_SgInitializer:
    SK_DPRINT("leave: initializer");
    return compOf(::isSgInitializer(input)->get_originalExpressionTree());

  case V_SgAssignInitializer:
    SK_DPRINT("leave: assign initializer");
    return compOf(::isSgAssignInitializer(input)->get_operand());

  case V_SgAggregateInitializer:
    SK_DPRINT("leave: aggregate initializer");
    return compOf(::isSgAggregateInitializer(input)->get_initializers());

  case V_SgFunctionCallExp:
    //std::cerr << "skbuilder:compOf: unhandled function invocation: "
    //          << ::isSgFunctionCallExp(input)->get_function()->unparseToString()
    //          << std::endl;
    SK_DPRINT("leave: func call exp");
    //return tuple_iii(1, 0) + compOf(::isSgFunctionCallExp(input)->get_args());
    return compOf(::isSgFunctionCallExp(input)->get_args());

  case V_SgReturnStmt:
    SK_DPRINT("leave: return stmt");
    return compOf(::isSgReturnStmt(input)->get_expression());

  case V_SgConditionalExp:
    SK_DPRINT("leave: C conditional exp");
    return compOf(::isSgConditionalExp(input)->get_conditional_exp())
      + compOf(::isSgConditionalExp(input)->get_true_exp())
      + compOf(::isSgConditionalExp(input)->get_false_exp());

  default: ;
  }
  if (const SgBinaryOp *g = ::isSgBinaryOp(input))
    switch (g->variantT()) { // See: http://rosecompiler.org/ROSE_HTML_Reference/classSgBinaryOp.html
    case V_SgArrowExp:
    case V_SgArrowStarOp:
    case V_SgDotExp:
    case V_SgDotStarOp:
    case V_SgScopeOp:
    case V_SgMembershipOp:
    case V_SgNonMembershipOp:
      SK_DPRINT("leave: memory access binary op");
      return compOf(g->get_lhs_operand()) + compOf(g->get_rhs_operand());
    case V_SgAssignOp:
    case V_SgCommaOpExp:
    case V_SgPntrArrRefExp:
      SK_DPRINT("leave: trivial binary op");
      return compOf(g->get_lhs_operand()) + compOf(g->get_rhs_operand()) + tuple_iii(1, 1, 0) ;
    default:
      {
        SK_DPRINT("leave: binary op");
        int ip = rosex::isIntegerType(g->get_type()) ? 1 : 0;
        int fp = rosex::isRealType(g->get_type()) || rosex::isComplexType(g->get_type()) ? 1 : 0;
        return tuple_iii(1, ip, fp)
          + compOf(g->get_lhs_operand())
          + compOf(g->get_rhs_operand());
      }
    }

  if (const SgUnaryOp *g = ::isSgUnaryOp(input))
    switch (g->variantT()) {
    case V_SgAssignOp:
    case V_SgAddressOfOp:
    case V_SgCastExp:
    case V_SgConjugateOp:
    case V_SgExpressionRoot:
    case V_SgImagPartOp:
    case V_SgPointerDerefExp:
    case V_SgRealPartOp:
    case V_SgThrowOp:
      SK_DPRINT("leave: cast or memory access unary op");
      return compOf(g->get_operand());
    default:
      {
        SK_DPRINT("leave: unary op");
        int ip = rosex::isIntegerType(g->get_type()) ? 1 : 0;
        int fp = rosex::isRealType(g->get_type()) || rosex::isComplexType(g->get_type()) ? 1 : 0;
        return tuple_iii(1, ip, fp) + compOf(g->get_operand());
      }
    }
  if (::isSgValueExp(input)) {
    SK_DPRINT("leave: value exp");
    return tuple_iii();
  }
  if (::isSgDeclarationStatement(input)) {
    SK_DPRINT("leave: decl stmt");
    return tuple_iii();
  }
  std::cerr << "skbuilder:compOf: unhandled sage class: " << input->class_name() << std::endl;
  SK_DPRINT("leave: unhandled");
  return tuple_iii();
}

// - br -

int SkBuilder::brOf(const SgNode *src)
{
  if (!src)
    return 0;
  SK_DPRINT("enter: sage class = " << src->class_name());
  switch (src->variantT()) {
  case V_SgIfStmt:
    SK_DPRINT("leave: ret = true");
    return 1;
  default:
    SK_DPRINT("leave: ret = false");
    return 0;
  }
  SK_ASSERT(0);
}

SK_END_NAMESPACE
