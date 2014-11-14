// symbolic.cc
// 10/22/2012 jichi

#include "rosex/symbolic.h"
#include "rosex/rosex.h"
#include <boost/foreach.hpp>

//#define DEBUG "rosex:symbolic"
#include "xt/xdebug.h"
#include <iostream>

#define foreach BOOST_FOREACH

class SgNode;

std::string SymbolicValue::fromSource(const SgNode *input)
{
  if (!input)
    return std::string();
  XD("enter: sage class = " << input->class_name());
  SgNode *source = const_cast<SgNode *>(input);
  switch (source->variantT()) {
  case V_SgInitializedName:
  case V_SgVarRefExp:
    //return fromSource(::isSgVarRefExp(source)->get_symbol()); // not working orz
    XD("leave: variable");
    return quoted(source->unparseToString());
  //case V_SgVariableSymbol:
  //  return fromSource(::isSgVariableSymbol(source)->get_declaration());
  //case V_SgInitializedName:
  //  return quoted(::isSgInitializedName(source)->get_qualified_name().getString());

  case V_SgInitializer:
    XD("leave: initialier");
    return fromSource(::isSgInitializer(input)->get_originalExpressionTree());
  case V_SgAssignInitializer:
    XD("leave: assign initializer");
    return fromSource(::isSgAssignInitializer(input)->get_operand());

  case V_SgPntrArrRefExp:
    {
      SgPntrArrRefExp *arr = ::isSgPntrArrRefExp(source);
      std::string ret = fromSource(arr->get_lhs_operand());
      SgExpression *rhs = arr->get_rhs_operand();

      if (SgExprListExp *list = ::isSgExprListExp(rhs)) {
        std::string indices;
        foreach (SgExpression *e, list->get_expressions()) {
          std::string index = "[" + fromSource(e) + "]";
          if (SageInterface::is_Fortran_language())
            indices = index + indices;
          else
            indices.append(index);
        }
        ret.append(indices);
      } else
         ret.append("[")
            .append(fromSource(rhs))
            .push_back(']');
      XD("leave: ptr");
      return ret;
    } ROSE_ASSERT(0);

  case V_SgSubscriptExpression:
    {
      SgSubscriptExpression *g = ::isSgSubscriptExpression(source);
      std::string ret;
      if (!::isSgNullExpression(g->get_lowerBound()))
        ret.append(fromSource(g->get_lowerBound()));
      if (!::isSgNullExpression(g->get_upperBound()))
        ret.append(":").append(fromSource(g->get_upperBound()));
      if (!::isSgNullExpression(g->get_stride()) &&
          g->get_stride()->unparseToString() != "1")
        ret.append(":").append(fromSource(g->get_stride()));
      XD("leave: array subscript");
      return ret;
    } ROSE_ASSERT(0);

  case V_SgExprListExp:
    {
      std::string ret;
      foreach (SgExpression *e, ::isSgExprListExp(source)->get_expressions()) {
        if (!ret.empty())
          ret.push_back(',');
        ret.append(fromSource(e));
      }
      XD("leave: expr list");
      return ret;
    } ROSE_ASSERT(0);

  case V_SgNullExpression:
    XD("leave: null exp");
    return std::string();

  default:
    if (::isSgValueExp(source)) {
      XD("leave: value exp");
      return quoted(source->unparseToString());
    }

    if (SgUnaryOp *g = ::isSgUnaryOp(source)) {
      XD("leave: unary op");
      return rosex::unparseOperatorVariantT(g->variantT()) + fromSource(g->get_operand());
    }

    if (SgBinaryOp *g = ::isSgBinaryOp(source)) {
      XD("leave: binary op");
      return "(" +
        fromSource(g->get_lhs_operand()) +
        rosex::unparseOperatorVariantT(g->variantT()) +
        fromSource(g->get_rhs_operand()) +
      ")";
    }

    std::cerr << "rosex::symbolic::fromSource: warning: unhandled sage class: " << source->class_name() << std::endl;
  }
  XD("leave: empty");
  return std::string();
}

// EOF
