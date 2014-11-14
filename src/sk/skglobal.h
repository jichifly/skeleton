#ifndef SKGLOBAL_H
#define SKGLOBAL_H

// skglobal.h
// 9/12/2012 jichi
// Introduce Sk namespace for global invariants and types.
// The purpose of this header is similar to QtGlobal from Qt4.

#include "sk/skdef.h"
#include <cstring>  // typedef size_t
#include <list>

#ifdef __GNUC__
# pragma GCC diagnostic ignored "-Wreturn-type"
#endif // __GNUC__

// - Types -
//
SK_BEGIN_NAMESPACE

///  Defines global invariants for the Skeleton AST
namespace Sk {

  ///  Order for traversing a skeleton AST
  enum TraversalOrder {
    AnyOrder = 0,
    PreOrder,
    PostOrder
  };

  ///  Skeleton AST node type
  enum ClassType {
      C_Null = 0,
      C_Node,
      C_Global,
      C_Literal,
      C_Number,
      C_Bool,
      C_Integer,
      C_Real,
      C_Complex,
      C_Character,
      C_String,
      C_Type,
      C_ScalarType,
      C_ArrayType,
      C_Symbol,
      C_Variable,
      C_Function,
      C_Expression,
      //C_NullExpression,
      C_List,
      C_Value,
      C_Reference,
      C_ArrayReference,
      C_Operation,
      C_UnaryOperation,
      C_BinaryOperation,
      //C_TernaryOperation,
      C_AllocateExpression,
      C_DeallocateExpression,
      C_FunctionCall,
      C_Slice,
      C_Statement,
      C_NullStatement,
      C_ReferenceStatement,
      C_ExpressionStatement,
      C_ForStatement,
      C_ForAllStatement,
      C_ForeverStatement,
      C_WhileStatement,
      C_DoWhileStatement,
      C_IfStatement,
      C_SwitchStatement,
      C_CaseStatement,
      C_DefaultCaseStatement,
      C_BreakStatement,
      C_ContinueStatement,
      C_ReturnStatement,
      C_BranchStatement,
      C_ComputeStatement,
      C_FixedPointStatement,
      C_FloatingPointStatement,
      C_LoadStatement,
      C_StoreStatement,
      C_ImportStatement,
      C_Pragma,
      C_Block,
      C_BlockBarrier,
      //C_DeclarationStatement,
      C_Declaration,
      C_SymbolDeclaration,
      C_FunctionDeclaration,
      C_FunctionDefinition
  };

  ///  Expression type of the skeleton language.
  enum ScalarType {
      S_Null = 0,
      S_Bool,
      S_Integer,
      S_Real,
      S_Complex,
      S_Character,
      S_String
  };

  ///  Return the name of ScalarType.
  inline const char *scalarTypeName(ScalarType t)
  {
      switch (t) {
      case S_Bool: return "bool";
      case S_Integer: return "int";
      case S_Real: return "float";
      case S_Complex: return "complex";
      case S_Character: return "char";
      case S_String: return "string";
      case S_Null: return "";
      //default: SK_ASSERT(0); return nullptr;
      }
  }

  ///  Operators
  enum OperatorType {
      O_Nop = 0,
      O_Not, // unary
      O_Pos,
      O_Neg,
      O_Address,
      O_BitNeg,
      O_Cast,
      O_PlusPlus,
      O_MinusMinus,
      O_AddAssign, // binary, assignment
      O_SubAssign,
      O_MulAssign,
      O_DivAssign,
      O_ModAssign,
      O_Add, // binary, arithmetic
      O_Sub,
      O_Mul,
      O_Div,
      O_Exp,
      O_Mod,
      O_Equ, // binary, comparison
      O_Neq,
      O_Leq,
      O_Les,
      O_Geq,
      O_Grt,
      O_And, // binary, logical
      O_Or,
      O_BitAnd, // binary, logical
      O_BitOr,
      O_BitXor,
      O_Assign, // assignment, logical
      O_Subscript // pointer, logical
  };

  ///  Return operator type name
  inline const char *operatorName(OperatorType t)
  {
      switch (t) {
      case O_Not: return "!";
      case O_Pos: return "(pos)";
      case O_Neg: return "(neg)";
      case O_Address: return "&";
      case O_BitNeg: return "~";
      case O_Cast: return "(cast)";
      case O_PlusPlus: return "++";
      case O_MinusMinus: return "--";
      case O_AddAssign: return "+=";
      case O_SubAssign: return "-=";
      case O_MulAssign: return "*=";
      case O_DivAssign: return "/=";
      case O_ModAssign: return "%=";
      case O_Add: return "+";
      case O_Sub: return "-";
      case O_Mul: return "*";
      case O_Div: return "/";
      case O_Exp: return "**";
      case O_Mod: return "%";
      case O_Equ: return "==";
      case O_Neq: return "!=";
      case O_Leq: return "<=";
      case O_Les: return "<";
      case O_Geq: return ">=";
      case O_Grt: return ">";
      case O_And: return "and";
      case O_Or:  return "or";
      case O_BitAnd: return "&";
      case O_BitOr:  return "|";
      case O_BitXor: return "^";
      case O_Assign: return "=";
      case O_Subscript: return "[]";
      case O_Nop: return "(nop)";
      //default: SK_ASSERT(0); return nullptr;
      }
  }

  ///  Unparse operator to string in Skeleton language syntax
  inline const char *unparseOperator(OperatorType t)
  {
      switch (t) {
      case O_Not: return "not";
      case O_Pos: return "+";
      case O_Neg: return "-";
      case O_Address: return ""; // ignored
      case O_BitNeg: return "~";
      case O_Cast: return ""; // ignored
      case O_PlusPlus: return "++";
      case O_MinusMinus: return "--";
      case O_AddAssign: return "+=";
      case O_SubAssign: return "-=";
      case O_MulAssign: return "*=";
      case O_DivAssign: return "/=";
      case O_ModAssign: return "%=";
      case O_Add: return "+";
      case O_Sub: return "-";
      case O_Mul: return "*";
      case O_Div: return "/";
      case O_Exp: return "**";
      case O_Mod: return "%";
      case O_Equ: return "==";
      case O_Neq: return "!=";
      case O_Leq: return "<=";
      case O_Les: return "<";
      case O_Geq: return ">=";
      case O_Grt: return ">";
      case O_And: return "and";
      case O_Or:  return "or";
      case O_BitAnd: return "&";
      case O_BitOr:  return "|";
      case O_BitXor: return "^";
      case O_Assign: return "=";
      case O_Subscript: return "[]";
      case O_Nop: return "#";
      //default: SK_ASSERT(0); return nullptr;
      }
  }

  ///  Operator associativity
  enum OperatorAssoc { NoAssoc = 0, LeftAssoc, RightAssoc };

  ///  Operator traits
  class Operator
  {
      OperatorType op_;
  public:
      explicit Operator(OperatorType op) : op_(op) { }

      ///  Number of operands
      size_t operands() const
      {
          switch (op_) {
          case O_Nop: return 0;
          case O_Not: case O_Pos: case O_Neg: case O_BitNeg: case O_Cast:
          case O_Address:
          case O_PlusPlus: case O_MinusMinus:
              return 1;
          case O_AddAssign: case O_SubAssign: case O_MulAssign: case O_DivAssign: case O_ModAssign:
          case O_Add: case O_Sub: case O_Mul: case O_Div: case O_Exp: case O_Mod:
          case O_Equ: case O_Neq: case O_Leq: case O_Les: case O_Geq: case O_Grt:
          case O_And: case O_Or:
          case O_BitAnd: case O_BitOr: case O_BitXor:
          case O_Assign:
          case O_Subscript:
              return 2;
          //default: SK_ASSERT(0); return 0;
          }
      }

      /**
       *  \brief  Operator precedence, descendant
       *
       *  Consistant with C.
       *  See: http://en.cppreference.com/w/cpp/language/operator_precedence
       */
      size_t prec() const
      {
          switch (op_) {
          case O_Nop: return 0;
          case O_Subscript: return 1;
          case O_PlusPlus: case O_MinusMinus: return 2;
          case O_Not: case O_Address: case O_Pos: case O_Neg: case O_BitNeg: case O_Cast: return 3;
          case O_Mul: case O_Div: case O_Exp: case O_Mod: return 5;
          case O_Add: case O_Sub: return 6;
          case O_Leq: case O_Les: case O_Geq: case O_Grt: return 8;
          case O_Equ: case O_Neq: return 9;
          case O_BitAnd:  return 10;
          case O_BitXor:  return 11;
          case O_BitOr:   return 12;
          case O_And: return 13;
          case O_Or:  return 14;
          case O_Assign:
          case O_AddAssign: case O_SubAssign: case O_MulAssign: case O_DivAssign: case O_ModAssign:
            return 15;
          }
      }

      ///  Operator associativity
      OperatorAssoc assoc() const
      {
          switch (op_) {
          case O_Nop:
          case O_Not: case O_Address: case O_Pos: case O_Neg: case O_BitNeg: case O_Cast:
          case O_Add: case O_Sub: case O_Mul: case O_Div: case O_Exp: case O_Mod:
          case O_AddAssign: case O_SubAssign: case O_MulAssign: case O_DivAssign: case O_ModAssign:
          case O_Equ: case O_Neq: case O_Leq: case O_Les: case O_Geq: case O_Grt:
          case O_And: case O_Or:
          case O_BitAnd: case O_BitOr: case O_BitXor:
          case O_Assign:
          case O_Subscript:
          case O_PlusPlus: case O_MinusMinus: // FIXME
              return LeftAssoc;
          //default: SK_ASSERT(0); return 0;
          }
      }
  };

} // namespace Sk

SK_END_NAMESPACE

// - Forward Declarations -

SK_BEGIN_NAMESPACE
class SkNode;     // Root
class SkGlobal;
class SkLiteral;  // Literals (literal = { value })
class SkNumber;
class SkBool;
class SkInteger;
class SkReal;
class SkComplex; // TODO: not implemented
class SkString;
class SkType;     // Types
class SkScalarType;
class SkArrayType;
class SkSymbol;   // Symbols (symbol = { name, type, value })
class SkVariable;
class SkFunction;
class SkExpression; // Expressions
//class SkNullExpression;
class SkList;
class SkValue;
class SkReference;
class SkArrayReference;
class SkOperation;
class SkUnaryOperation;
class SkBinaryOperation;
class SkAllocateExpression;
class SkDeallocateExpression;
//class SkTernaryOperaton;
class SkFunctionCall;
class SkSlice;
class SkStatement;  // Statements
class SkNullStatement;
class SkReferenceStatement;
class SkExpressionStatement;
class SkForStatement;
class SkForAllStatement;
class SkForeverStatement;
class SkWhileStatement;
class SkDoWhileStatement;
class SkIfStatement;
class SkSwitchStatement;
class SkCaseStatement;
class SkDefaultCaseStatement;
class SkBreakStatement;
class SkContinueStatement;
class SkReturnStatement;
class SkBranchStatement;
class SkComputeStatement;
class SkFixedPointStatement;
class SkFloatingPointStatement;
class SkLoadStatement;
class SkStoreStatement;
class SkImportStatement;
class SkPragma;
class SkBlock;
class SkBlockBarrier;
//class SkDeclarationStatement;
class SkDeclaration;
class SkSymbolDeclaration;
class SkFunctionDeclaration;
class SkFunctionDefinition;
SK_END_NAMESPACE

// - Typedefs -

SK_BEGIN_NAMESPACE
typedef std::list<SkNode *> SkNodeList;   ///< List of node pointers
typedef std::list<const SkNode *> SkConstNodeList;   ///< List of node pointers
typedef std::list<SkStatement *> SkStatementList;   ///< List of statement pointers
typedef std::list<SkExpression *> SkExpressionList; ///< List of expression pointers
typedef std::list<SkSymbolDeclaration *> SkSymbolDeclarationList; ///< List of variable declarations
SK_END_NAMESPACE

#endif // SKGLOBAL_H
