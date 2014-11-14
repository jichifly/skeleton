#ifndef SKBUILDER_P_H
#define SKBUILDER_P_H

// skbuilder_p.h
// 9/12/2012 jichi
// Internal header to implement SkBuilder.

#include "sk/skglobal.h"
#include "sk/skbuilder.h"
#include "rosex/rosex.h"

SK_BEGIN_NAMESPACE

namespace SkBuilder { namespace detail {

  ///  \internal
  inline Sk::ScalarType scalarTypeFromSageVariant(VariantT type)
  {
    switch (type) {
    case V_SgTypeShort: case V_SgTypeSignedShort: case V_SgTypeUnsignedShort:
    case V_SgTypeInt: case V_SgTypeSignedInt: case V_SgTypeUnsignedInt:
    case V_SgTypeLong: case V_SgTypeSignedLong: case V_SgTypeUnsignedLong:
    case V_SgTypeLongLong: case V_SgTypeSignedLongLong: case V_SgTypeUnsignedLongLong:
      return Sk::S_Integer;

    case V_SgTypeChar: case V_SgTypeSignedChar: case V_SgTypeUnsignedChar:
      return Sk::S_Character;

    case V_SgTypeFloat: case V_SgTypeDouble: case V_SgTypeLongDouble:
      return Sk::S_Real;

    case V_SgTypeComplex: case V_SgTypeImaginary: // TODO: complex value
      return Sk::S_Complex;

    case V_SgTypeBool:    return Sk::S_Bool;
    case V_SgTypeString:  return Sk::S_String;
    default: return Sk::S_Null;
    }
  }

  ///  \internal
  inline Sk::OperatorType operatorTypeFromSageVariant(VariantT type)
  {
    switch (type) {
    case V_SgNotOp:       return Sk::O_Not;
    case V_SgMinusOp:     return Sk::O_Neg;
    case V_SgPlusPlusOp:  return Sk::O_PlusPlus;
    case V_SgMinusMinusOp:  return Sk::O_MinusMinus;
    case V_SgPlusAssignOp: return Sk::O_AddAssign;
    case V_SgMinusAssignOp:  return Sk::O_SubAssign;
    case V_SgMultAssignOp:  return Sk::O_MulAssign;
    case V_SgDivAssignOp:  return Sk::O_DivAssign;
    case V_SgModAssignOp:  return Sk::O_ModAssign;
    case V_SgAddOp:       return Sk::O_Add;
    case V_SgExponentiationOp: return Sk::O_Exp;
    case V_SgSubtractOp:  return Sk::O_Sub;
    case V_SgMultiplyOp:  return Sk::O_Mul;
    case V_SgDivideOp:    return Sk::O_Div;
    case V_SgModOp:       return Sk::O_Mod;
    case V_SgLessThanOp:  return Sk::O_Les;
    case V_SgLessOrEqualOp: return Sk::O_Leq;
    case V_SgGreaterThanOp: return Sk::O_Grt;
    case V_SgGreaterOrEqualOp: return Sk::O_Geq;
    case V_SgEqualityOp:  return Sk::O_Equ;
    case V_SgNotEqualOp:  return Sk::O_Neq;
    case V_SgAndOp:       return Sk::O_And;
    case V_SgOrOp:        return Sk::O_Or;
    case V_SgBitAndOp:    return Sk::O_BitAnd;
    case V_SgBitXorOp:    return Sk::O_BitXor;
    case V_SgBitOrOp:     return Sk::O_BitOr;
    case V_SgAssignOp:    return Sk::O_Assign;
    case V_SgPntrArrRefExp: return Sk::O_Subscript;
    case V_SgCastExp:     return Sk::O_Cast;
    case V_SgAddressOfOp: return Sk::O_Address;
    default:              return Sk::O_Nop;
    }
  }

  ///  \internal  Return if the input expression is a reference being modified
  bool isStoreRef(const SgNode *src);
  bool isLoadStoreRef(const SgNode *src);

  ///  Is is a ++ operation?
  bool isIncrement(const SgNode *src);

  ///  Combine all blocks in src into a single block
  SkStatementList collectBlockStatements(SkBlock *block);

  /**
   *  \internal  Return either a SkBlock, or a single statement within the block.
   *
   *  JG 10/14/2012: Make a specifical case for single statement block --- likely it IS a single statement(declaration) in the source code but ROSE presensent it as a block
   */
  SkStatement *fromBlock(SgNode *src, const Option *opt = nullptr);

  //  \internal  For each node in the input list, invoke forDeclaration
  SkStatement *fromDeclarationList(const SgInitializedNamePtrList &l, SgNode *src = nullptr, const Option *opt = nullptr);

  //  \internal  For each node in the input list, invoke forDeclaration
  SkStatement *fromDeclarationList(const SgExpressionPtrList &l, SgNode *src = nullptr, const Option *opt = nullptr);

  //  \internal  For each node in the input list, invoke forDeclaration
  SkStatement *fromDeclarationList(const SgDeclarationStatementPtrList &l, SgNode *src = nullptr, const Option *opt = nullptr);

  ///  \internal  For each node in the input list, invoke forDeclaration
  //template <typename T>
  //inline SkStatement *fromDeclarationList(const T &l, SgNode *src = nullptr, const Option *opt = nullptr)
  //{
  //  switch (l.size()) {
  //  case 0: return nullptr;
  //  case 1: return fromDeclaration(l.front(), opt);
  //  default:
  //    {
  //      SkBlock *ret = new SkBlock(src);
  //      BOOST_FOREACH (auto e, l)
  //        if (SkStatement *decl = fromDeclaration(e, opt))
  //          ret->append(decl);
  //      return ret;
  //    }
  //  } ROSE_ASSERT(0);
  //}

  ///  Convert vectorization in ld/st to loops.
  SkNode *devectorizeLoadStore(SkNode *ldst_stmt);

  ///  Convert vectorization in if-condition to loops.
  void devectorizeBranchCondition(SkNode *if_stmt);

  ///  Create a unique temporary scalar variable reference.
  SkVariable *createTempScalar(Sk::ScalarType type = Sk::S_Null);
  SkVariable *createScalar(const std::string &name, Sk::ScalarType type = Sk::S_Null, SgNode *src = nullptr);
  SkStatement *createAssignment(SkReference *var, SkExpression *value, SgNode *src = nullptr);
  SkFunctionCall *createFunctionCall(const std::string &name, SgNode *src = nullptr);
  SkFunctionCall *createFunctionCall(const std::string &name, SkExpression *arg1, SgNode *src = nullptr);

} } // namespace SkBuilder::detail

SK_END_NAMESPACE

#endif // SKBUILDER_P_H
