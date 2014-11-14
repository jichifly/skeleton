// skbuilder_new.cc
// 9/12/2012 jichi

#include "sk/skbuilder_p.h"
#include "sk/sknode.h"
#include <boost/lexical_cast.hpp>

#define SK_DEBUG "skbuilder_new"
#include "sk/skdebug.h"
//#include <iostream>

#define foreach BOOST_FOREACH

SK_BEGIN_NAMESPACE

SkVariable *SkBuilder::detail::createTempScalar(Sk::ScalarType type)
{
  static int count = 0;
  count++;
  std::string name = "_" + boost::lexical_cast<std::string>(count);
  return createScalar(name, type);
}

SkVariable *SkBuilder::detail::createScalar(const std::string &name, Sk::ScalarType type, SgNode *src)
{ return new SkVariable(name, new SkScalarType(type), src); }

SkStatement *SkBuilder::detail::createAssignment(SkReference *var, SkExpression *value, SgNode *src)
{ return new SkExpressionStatement(new SkBinaryOperation(Sk::O_Assign, var, value, src), src); }

SkFunctionCall *SkBuilder::detail::createFunctionCall(const std::string &name, SgNode *src)
{ return new SkFunctionCall(new SkFunction(name), src); }

SkFunctionCall *SkBuilder::detail::createFunctionCall(const std::string &name, SkExpression *arg1, SgNode *src)
{
  SkFunctionCall *ret = new SkFunctionCall(new SkFunction(name), src);
  ret->appendArgument(arg1);
  return ret;
}

SK_END_NAMESPACE
