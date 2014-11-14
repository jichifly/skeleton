#ifndef ROSEX_H
#define ROSEX_H

// rosex.h
// 9/13/2012 jichi

#include "rosex/rose_config.h"
#include <rose.h>
#include <iostream>

namespace rosex {

std::string indent(const std::string &code);

inline bool containsSubTree(const SgNode *node, VariantT type)
{ return !NodeQuery::querySubTree(const_cast<SgNode *>(node), type).empty(); }

inline SgVariableSymbol *getVariableSymbol(SgNode *var)
{
  if (var)
    switch (var->variantT()) {
    case V_SgVariableSymbol:
      return ::isSgVariableSymbol(var);
    case V_SgInitializedName:
      return ::isSgVariableSymbol(::isSgInitializedName(var)->get_symbol_from_symbol_table());
    case V_SgVarRefExp:
      return ::isSgVariableSymbol(::isSgVarRefExp(var)->get_symbol());
    default: ;
    }
  return nullptr;
}

///  Merge isCanonicalForLoop and isCanonicalDoLoop.
inline bool isCanonicalLoop(
  SgNode *loop,
  SgInitializedName **ivar = nullptr,
  SgExpression** lb = nullptr,
  SgExpression** ub = nullptr,
  SgExpression** step = nullptr,
  SgStatement** body = nullptr,
  bool *hasIncrementalIterationSpace = nullptr,
  bool* isInclusiveUpperBound = nullptr)
{
  SgFortranDo *f = ::isSgFortranDo(loop);
  return f ? SageInterface::isCanonicalDoLoop(f, ivar, lb, ub, step, body, hasIncrementalIterationSpace, isInclusiveUpperBound)
     : SageInterface::isCanonicalForLoop(loop, ivar, lb, ub, step, body, hasIncrementalIterationSpace, isInclusiveUpperBound);
}

inline bool isCanonicalLoop(const SgNode *loop)
{ return isCanonicalLoop(const_cast<SgNode *>(loop)); }


///  Return if the input a floating point SgType (float,double,real,...)
inline bool isRealType(const SgNode *src)
{
  if (src)
    switch (src->variantT()) {
    case V_SgTypeFloat:
    case V_SgTypeDouble:
    case V_SgTypeLongDouble:
      return true;
    default: ;
    }

  return false;
}

///  Return if the input a Fortran complex type
inline bool isComplexType(const SgNode *src)
{
  if (src)
    switch (src->variantT()) {
    case V_SgTypeComplex:
    case V_SgTypeImaginary:
      return true;
    default: ;
    }

  return false;
}

///  Return if the input an integer SgType (int,long,short,char,...)
inline bool isIntegerType(const SgNode *src)
{
  if (src)
    switch (src->variantT()) {
    case V_SgTypeChar:
    case V_SgTypeSignedChar:
    case V_SgTypeUnsignedChar:
    case V_SgTypeShort:
    case V_SgTypeSignedShort:
    case V_SgTypeUnsignedShort:
    case V_SgTypeInt:
    case V_SgTypeSignedInt:
    case V_SgTypeUnsignedInt:
    case V_SgTypeLong:
    case V_SgTypeSignedLong:
    case V_SgTypeUnsignedLong:
    case V_SgTypeLongLong:
    case V_SgTypeSignedLongLong:
    case V_SgTypeUnsignedLongLong:
      return true;
    default: ;
    }

  return false;
}

///  Return if the type could be an array
inline bool isArrayType(const SgNode *src)
{
  if (src)
    switch (src->variantT()) {
    case V_SgArrayType:
    case V_SgPointerType: // assuming all pointers are arrays
    case V_SgTypeString:
      return true;
    default: ;
    }

  return false;
}

///  Return if the the expression is an array reference
inline bool isArrayReference(const SgNode *src)
{
  if (src)
    switch (src->variantT()) {
    case V_SgPntrArrRefExp:
      return true;
    default:
      if (const SgExpression *e = ::isSgExpression(src))
        return isArrayType(e->get_type());
    }

  return false;
}

inline bool isFortranArraySubscript(const SgNode *src)
{
  if (src)
    switch (src->variantT()) {
    case V_SgPntrArrRefExp:
    case V_SgVarRefExp:
      return containsSubTree(src, V_SgSubscriptExpression);
    default: ;
    }
  return false;
}

inline const char *unparseOperatorVariantT(VariantT type)
{
  switch (type) {
  case V_SgNotOp:         return "!";
  case V_SgMinusOp:       return "-";
  case V_SgAddOp:         return "+";
  case V_SgSubtractOp:    return "-";
  case V_SgMultiplyOp:    return "*";
  case V_SgDivideOp:      return "/";
  case V_SgExponentiationOp: return "**"; // Fortran only
  case V_SgModOp:         return "%";
  case V_SgLessThanOp:    return "<";
  case V_SgLessOrEqualOp: return "<=";
  case V_SgGreaterThanOp: return ">";
  case V_SgGreaterOrEqualOp: return ">=";
  case V_SgEqualityOp:    return "==";
  case V_SgNotEqualOp:    return "!=";
  case V_SgAndOp:         return "&&";
  case V_SgOrOp:          return "||";
  case V_SgBitAndOp:      return "&";
  case V_SgBitXorOp:      return "^";
  case V_SgBitOrOp:       return "|";
  case V_SgAssignOp:      return "=";
  //case V_SgPntrArrRefExp: return "[]";
  //case V_SgCastExp:       return "()";
  default:                return "";
  }
}

///  Get the first symbol from the declared variable list
inline SgVariableSymbol *getFirstDeclaredVariableSymbol(SgVariableDeclaration *decl)
{
  if (decl) {
    const SgInitializedNamePtrList &vars = decl->get_variables();
    if (!vars.empty())
      if (SgInitializedName *var = vars.front())
        return::isSgVariableSymbol(var->get_symbol_from_symbol_table());
  }
  return nullptr;
}

///  Return if the child is a descendant of the ancestor
inline bool isAncestor(const SgNode *child, const SgNode *parent)
{
  if (child)
    do
      if (child == parent)
        return true;
    while (child = child->get_parent());
  return false;
}
//inline bool isAncestor(const SgNode *child, const SgNode *ancestor)
//{
//  SgNode *parent = nullptr;
//  if (child && ancestor)
//    while ((parent=child->get_parent()) && parent != ancestor);
//  return parent;
//}

///  Return the next statement in the same block
inline SgStatement *getNextStatementInBlock(SgStatement *n, SgBasicBlock *block = nullptr)
{
  if (!n)
    return nullptr;
  if (!block) {
    block = ::isSgBasicBlock(n->get_parent());
    if (!block)
      return nullptr;
  }

  Rose_STL_Container<SgStatement *>::iterator i;
  SgStatementPtrList &l = block->get_statements();
  for (i = l.begin(); i != l.end() && (*i) != n; i++);
  return i == l.end() ? nullptr : *++i;
}

///  Return the previous statement in the same block
inline SgStatement *getPreviousStatementInBlock(SgStatement *n, SgBasicBlock *block = nullptr)
{
  if (!n)
    return nullptr;
  if (!block) {
    block = ::isSgBasicBlock(n->get_parent());
    if (!block)
      return nullptr;
  }

  SgStatementPtrList &l = block->get_statements();
  for (Rose_STL_Container<SgStatement *>::iterator
       i = l.begin(), j = l.end(); i != l.end(); i++)
    if (*i == n)
      return j == l.end() ? nullptr : *j;
  return nullptr;
}

///  Return the most enclosing parent statement
inline SgNode *getParentStatement(SgNode *n)
{
  if (n) while ((n = n->get_parent()) && !::isSgStatement(n)) ;
  return n;
}
///  \override
inline const SgNode *getParentStatement(const SgNode *n)
{
  if (n) while ((n = n->get_parent()) && !::isSgStatement(n)) ;
  return n;
}

///  Return the most enclosing parent statement
inline SgNode *getParentType(SgNode *n)
{
  if (n) while ((n = n->get_parent()) && !::isSgType(n)) ;
  return n;
}
///  \override
inline const SgNode *getParentType(const SgNode *n)
{
  if (n) while ((n = n->get_parent()) && !::isSgType(n)) ;
  return n;
}

/// Extract comments
inline std::string getComment(const SgNode *n)
{
  std::string ret;
  if (AttachedPreprocessingInfoType *c = ::isSgLocatedNode(const_cast<SgNode *>(n))
                                         ->getAttachedPreprocessingInfo())
    for(AttachedPreprocessingInfoType::iterator p = c->begin(); p != c->end(); ++p)
      ret.append((*p)->getString());
  return ret;
}

/// Generate main function
std::string outlineStatement(const SgNode *stmt);

///  Unparse the AST node to assembly code
std::string unparseToAssembly(const SgNode *src);

/// Get the main function
SgNode *findMainFunction(const SgNode *src);

///  Return whether stmt is a function call to name with argcount. Don't forget adding "::" before global functions
inline bool isCallStatementToParticularFunction(const std::string &name, int argcount, SgNode *stmt)
{
  if (SgExprStatement *call = ::isSgExprStatement(stmt))
    if (SgFunctionCallExp *expr = ::isSgFunctionCallExp(call->get_expression()))
      if (SageInterface::isCallToParticularFunction(name, argcount, expr)) // 0 = number of elements
        return true;
  return false;
}

// Replacement of Sage API

/// SageInterface::replaceExpression
void replaceExpression(SgExpression *oldExp, SgExpression *newExp, bool keepOldExp = false);

// Symbolic evaluation

inline long evalLongLong(const SgExpression *e, bool *ok = nullptr)
{
  if (!e) { if (ok) *ok = false; return 0; }
  switch (e->variantT()) {
  case V_SgCharVal: if (ok) *ok = true; return ::isSgCharVal(e)->get_value();
  case V_SgUnsignedCharVal: if (ok) *ok = true; return ::isSgUnsignedCharVal(e)->get_value();
  case V_SgShortVal: if (ok) *ok = true; return ::isSgShortVal(e)->get_value();
  case V_SgIntVal: if (ok) *ok = true; return ::isSgIntVal(e)->get_value();
  case V_SgLongIntVal: if (ok) *ok = true; return ::isSgLongIntVal(e)->get_value();
  case V_SgLongLongIntVal: if (ok) *ok = true; return ::isSgLongLongIntVal(e)->get_value();
  case V_SgUnsignedShortVal: if (ok) *ok = true; return ::isSgUnsignedShortVal(e)->get_value();
  case V_SgUnsignedIntVal: if (ok) *ok = true; return ::isSgUnsignedIntVal(e)->get_value();
  case V_SgUnsignedLongVal: if (ok) *ok = true; return ::isSgUnsignedLongVal(e)->get_value();
  case V_SgUnsignedLongLongIntVal: if (ok) *ok = true; return ::isSgUnsignedLongLongIntVal(e)->get_value();
  case V_SgFloatVal: if (ok) *ok = true; return ::isSgFloatVal(e)->get_value();
  case V_SgDoubleVal: if (ok) *ok = true; return ::isSgDoubleVal(e)->get_value();
  case V_SgLongDoubleVal: if (ok) *ok = true; return ::isSgLongDoubleVal(e)->get_value();
  default:
    std::cerr << "rosex::evalLongLong: error: unhandled type = " << e->class_name() << ", " << e->unparseToString() << std::endl;
    if (ok) *ok = false; return 0;
  }
}

inline long double evalLongDouble(const SgExpression *e, bool *ok = nullptr)
{
  if (!e) { if (ok) *ok = false; return 0; }
  switch (e->variantT()) {
  case V_SgCharVal: if (ok) *ok = true; return ::isSgCharVal(e)->get_value();
  case V_SgUnsignedCharVal: if (ok) *ok = true; return ::isSgUnsignedCharVal(e)->get_value();
  case V_SgShortVal: if (ok) *ok = true; return ::isSgShortVal(e)->get_value();
  case V_SgIntVal: if (ok) *ok = true; return ::isSgIntVal(e)->get_value();
  case V_SgLongIntVal: if (ok) *ok = true; return ::isSgLongIntVal(e)->get_value();
  case V_SgLongLongIntVal: if (ok) *ok = true; return ::isSgLongLongIntVal(e)->get_value();
  case V_SgUnsignedShortVal: if (ok) *ok = true; return ::isSgUnsignedShortVal(e)->get_value();
  case V_SgUnsignedIntVal: if (ok) *ok = true; return ::isSgUnsignedIntVal(e)->get_value();
  case V_SgUnsignedLongVal: if (ok) *ok = true; return ::isSgUnsignedLongVal(e)->get_value();
  case V_SgUnsignedLongLongIntVal: if (ok) *ok = true; return ::isSgUnsignedLongLongIntVal(e)->get_value();
  case V_SgFloatVal: if (ok) *ok = true; return ::isSgFloatVal(e)->get_value();
  case V_SgDoubleVal: if (ok) *ok = true; return ::isSgDoubleVal(e)->get_value();
  case V_SgLongDoubleVal: if (ok) *ok = true; return ::isSgLongDoubleVal(e)->get_value();
  default:
    std::cerr << "rosex::evalLongDouble: error: unhandled type = " << e->class_name() << ", " << e->unparseToString() << std::endl;
    if (ok) *ok = false; return 0;
  }
}

inline long evalLong(const SgExpression *e, bool *ok = nullptr) { return evalLongLong(e, ok); }
inline int evalInt(const SgExpression *e, bool *ok = nullptr) { return evalLongLong(e, ok); }
inline double evalDouble(const SgExpression *e, bool *ok = nullptr) { return evalLongDouble(e, ok); }
inline float evalFloat(const SgExpression *e, bool *ok = nullptr) { return evalLongDouble(e, ok); }

// Comment

//  Return input into comment, and return the parent statement of the comment
inline SgStatement *commentOutStatement(SgStatement *input)
{
  if (!input)
    return nullptr;
  SgStatement *ret = nullptr;
  if (ret = SageInterface::getNextStatement(input)) {
    std::string comment = input->unparseToString();
    SageInterface::attachComment(ret, comment, PreprocessingInfo::before);
  } else if (ret = SageInterface::getPreviousStatement(input)) {
    std::string comment = input->unparseToString();
    SageInterface::attachComment(ret, comment, PreprocessingInfo::after);
  }
  SageInterface::removeStatement(input);
  return ret;
}

// MPI helpers
inline const char *mpiTypeName(const SgNode *type)
{
  ROSE_ASSERT(type);
  switch (type->variantT()) {
  case V_SgTypeChar: return "MPI_CHAR";
  case V_SgTypeUnsignedChar: return "MPI_UNSIGNED_CHAR";
  case V_SgTypeShort: return "MPI_SHORT";
  case V_SgTypeInt: return "MPI_INT";
  case V_SgTypeUnsignedInt: return "MPI_UNSIGNED";
  case V_SgTypeLong: return "MPI_LONG";
  case V_SgTypeUnsignedLong: return "MPI_UNSIGNED_LONG";
  case V_SgTypeLongLong: return "MPI_LONG_LONG";
  case V_SgTypeFloat: return "MPI_FLOAT";
  case V_SgTypeDouble: return "MPI_DOUBLE";
  case V_SgTypeLongDouble: return "MPI_LONG_DOUBLE";
  case V_SgTypeBool: return "MPI_C_BOOL";
  default: return nullptr;
  }
}

} // namespace rosex

#endif // ROSEXT_H
