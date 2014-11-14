// rose.cc
// 9/25/2012 jichi

#include "rosex/rosex.h"
#include "rosex/rosex_p.h"
#include <boost/foreach.hpp>

#define foreach BOOST_FOREACH

#define DEBUG "rosex"
#include "xt/xdebug.h"

// - Queries -

SgNode *rosex::findMainFunction(const SgNode *src)
{
  foreach (SgNode *it, NodeQuery::querySubTree(const_cast<SgNode *>(src), V_SgFunctionDefinition)) {
    SgFunctionDefinition *func = isSgFunctionDefinition(it);
    std::string func_name = func->get_declaration()->get_name().getString();
    if (func_name == "main")
      return it;
  }
  return nullptr;
}

// - SageInterface -

void rosex::replaceExpression(SgExpression *oldExp, SgExpression *newExp, bool keepOldExp)
{
  ROSE_ASSERT(oldExp);
  ROSE_ASSERT(newExp);
  if (oldExp == newExp)
    return;

  if (isSgVarRefExp(newExp))
    newExp->set_need_paren(true); // enclosing new expression with () to be safe

  SgNode* parent = oldExp->get_parent();
  ROSE_ASSERT(parent!=NULL);
  newExp->set_parent(parent);

  // set lvalue when necessary
  if (oldExp->get_lvalue() == true)
    newExp->set_lvalue(true);

  if (isSgExprStatement(parent))
    isSgExprStatement(parent)->set_expression(newExp);
  else if (isSgForStatement(parent)) {
    ROSE_ASSERT(isSgForStatement(parent)->get_increment() == oldExp);
    isSgForStatement(parent)->set_increment(newExp);
    // TODO: any other cases here??
  }
  else if (SgFortranDo *f = isSgFortranDo(parent)) { // jichi: Add Fortran support
    if (f->get_increment() == oldExp)
      f->set_increment(newExp);
    else if (f->get_bound() == oldExp)
      f->set_bound(newExp);
    else if (f->get_initialization() == oldExp)
      f->set_initialization(newExp);
    else {
      XD("Unhandled fortran do expression");
      ROSE_ASSERT(false);
    }
  }
  else if (isSgReturnStmt(parent))
    isSgReturnStmt(parent)->set_expression(newExp);
  else  if (isSgBinaryOp(parent) != NULL){
    if (oldExp==isSgBinaryOp(parent)->get_lhs_operand())
      isSgBinaryOp(parent)->set_lhs_operand(newExp);
    else if (oldExp==isSgBinaryOp(parent)->get_rhs_operand())
      isSgBinaryOp(parent)->set_rhs_operand(newExp);
    else
      ROSE_ASSERT(false);
  } else if (isSgUnaryOp(parent) != NULL) { // unary parent
    if (oldExp==isSgUnaryOp(parent)->get_operand_i())
      isSgUnaryOp(parent)->set_operand_i(newExp);
    else
      ROSE_ASSERT(false);
  } else if (isSgConditionalExp(parent) != NULL) { //SgConditionalExp
    SgConditionalExp *expparent= isSgConditionalExp(parent);//get explicity type parent
    if (oldExp == expparent->get_conditional_exp())
      expparent->set_conditional_exp(newExp);
    else if (oldExp == expparent->get_true_exp())
      expparent->set_true_exp(newExp);
    else if (oldExp == expparent->get_false_exp())
      expparent->set_false_exp(newExp);
    else
      ROSE_ASSERT(false);
  } else if (isSgExprListExp(parent) != NULL) {
    SgExpressionPtrList &explist = isSgExprListExp(parent)->get_expressions();
    for (Rose_STL_Container<SgExpression*>::iterator i = explist.begin();i!=explist.end();i++)
      if (isSgExpression(*i)==oldExp) {
        isSgExprListExp(parent)->replace_expression(oldExp,newExp);
        // break; //replace the first occurrence only??
      }
  } else if (isSgValueExp(parent))
    // For compiler generated code, this could happen.
    // We can just ignore this function call since it will not appear in the final AST.
    return;
  else if (isSgExpression(parent)) {
    int worked = isSgExpression(parent)->replace_expression(oldExp, newExp);
    // ROSE_DEPRECATED_FUNCTION
    ROSE_ASSERT (worked);
  } else if (isSgInitializedName(parent)) {
    SgInitializedName* initializedNameParent = isSgInitializedName(parent);
    if (oldExp == initializedNameParent->get_initializer()) {
       //We can only replace an initializer expression with another initializer expression
      ROSE_ASSERT(isSgInitializer(newExp));
      initializedNameParent->set_initializer(isSgInitializer(newExp));
    } else {
      //What other expressions can be children of an SgInitializedname?
      ROSE_ASSERT(false);
    }
  } else {
    XD("Unhandled parent expression type of SageIII enum value: " << parent->class_name());
    ROSE_ASSERT(false);
  }

  if (!keepOldExp)
    SageInterface::deepDelete(oldExp); // avoid dangling node in memory pool
  else
    oldExp->set_parent(NULL);
}

// EOF
