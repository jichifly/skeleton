#ifndef SKNODE_H
#define SKNODE_H

// sknode.h
// 9/12/2012 jichi
// Skeleton AST nodes.
// See: https://sites.google.com/site/xpectwiki/code-skeleton-1/application-example

#include "sk/skglobal.h"
#include "sk/skconf.h"
#include "sk/skvariant.h"
#include <string>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>

#ifdef __clang__
# pragma GCC diagnostic ignored "-Wdangling-else"
# pragma GCC diagnostic ignored "-Wunused-parameter"
#elif defined __GNUC__
# pragma GCC diagnostic ignored "-Wparentheses"
#endif // __GNUC__
#include <boost/range/algorithm.hpp>

#define SK_UNKNOWN  "$"
#define SK_UNKNOWN_DIMENSION  ":"

class SgNode;

#define _SK_SET_PROPERTY(_prop, _val) \
  { \
    if (_prop && _prop->parent() == this) \
      _prop->setParent(nullptr); \
    _prop = _val;  \
    if (_val) \
      _val->setParent(this); \
  }

#define _SK_SET_LIST_PROPERTY(_prop, _val, _type) \
  { \
    BOOST_FOREACH (Sk##_type *it, _prop) \
      if (it && it->parent() == this) \
        it->setParent(nullptr); \
    _prop = _val; \
    BOOST_FOREACH (Sk##_type *it, _val) \
      if (it) \
        it->setParent(this); \
  }

#define SK_PROPERTY(_getter, _setter, _type) \
  private: \
    Sk##_type *_getter##_; \
  public: \
    Sk##_type *_getter() const { return _getter##_; } \
    void _setter(Sk##_type *val) \
    { _SK_SET_PROPERTY(_getter##_, val) } \
  private:

#define SK_LIST_PROPERTY(_getter, _setter, _type) \
  private: \
    Sk##_type##List _getter##_; \
  public: \
    const Sk##_type##List &_getter() const { return _getter##_; } \
    void _setter(const Sk##_type##List &val) \
    { _SK_SET_LIST_PROPERTY(_getter##_, val, _type) } \
  private:

#define SK_SCALAR_PROPERTY(_getter, _setter, _type) \
  private: \
    _type _getter##_; \
  public: \
    const _type &_getter() const { return _getter##_; } \
    void _setter(const _type &val) \
    { _getter##_ = val; } \
  private:

SK_BEGIN_NAMESPACE

// - Root Nodes -

/**
 *  \brief  An abstract skeleton AST node.
 *
 *  Parent class of all node in the skeleton AST.
 *
 *  Conventions of subclasses:
 *  - All subclasses should override classType and className.
 *  - Cascade deletion.
 *    - When the parent is deleted, this class will be deleted.
 *    - When this class is deleted, all its children will be deleted.
 *  - toString() = 0 for abstract class
 */
class SkNode
{
  SK_DISABLE_COPY(SkNode)  // disable shadow copy
  typedef SkNode Self;

  Self *parent_;
  SkNodeList children_;
  SgNode *source_;  ///< nullable, optional

  std::string comment_;

  static SkConf conf_; // unparse configuration

public:
  virtual ~SkNode()
  {
    if (!children_.empty())
      BOOST_FOREACH (Self *child, children_) {
        SK_ASSERT(child && child->parent() == this);
        delete child;
      }
  }

protected:
  explicit SkNode(SgNode *src = nullptr)
    : parent_(nullptr), source_(src) {}

public:
  ///  Create a new instance. Partially implemented in children.
  virtual Self *clone() const = 0; // This makes SgNode an abstract class

  ///  Return the polymorphic class type.
  virtual Sk::ClassType classType() const { return Sk::C_Node; }

  ///  Return the polymorphic class name.
  virtual const char *className() const { return "SkNode"; }

  ///  Unparse the content to string, maybe not in the Skeleton language syntax.
  virtual std::string toString() const { return className(); }

  ///  Unparse to string in strict Skeleton language syntax.
  virtual std::string unparse() const { return toString(); }

  ///  Unparse to assembly
  std::string unparseToAssembly() const;
  ///  The class is owned by parent. All non-root nodes should have a parent.
  SkNode *parent() const { return parent_; }

  ///  The orignal line number in the source file
  int lineNumber() const;

  ///  The orignal column number in the source file
  int columnNumber() const;

  ///  The name of the source file
  std::string fileName() const;

  void setParent(SkNode *val)
  {
    if (parent_ != val) {
      if (parent_)
        parent_->children_.remove(this);
      if (val)
        val->children_.push_back(this);
      parent_ = val;
    }
  }

  ///  Return all child nodes
  const SkNodeList &children() const { return children_; }

  ///  Nullable. Point to the AST where this skeleton node is generated from.
  SgNode *source() const { return source_; }
  void setSource(SgNode *val) { source_ = val; }

  ///  Comment for the node.
  const std::string &comment() const { return comment_; }
  void setComment(const std::string &val) { comment_ = val; }

  static const SkConf &configuration() { return conf_; }
  static void setConfiguration(const SkConf &val) { conf_ = val; }

protected:
  static std::string unparseComment(const std::string &comment)
  { return "// " + comment + "\n"; }

  ///  Generate line numbers in the comment
  std::string unparseLineComment() const
  {
    int n = lineNumber();
    return n ? unparseComment("@line " + boost::lexical_cast<std::string>(n)) : std::string();
  }

  static std::string unparseBlock(const SkNode *body)
  {

    std::string ret = !body ? "{\n}" :
           blockNeedsParentheses(body) ? "{\n" + body->unparse()  + "\n}" :
           body->unparse();
    return ret;
  }

  static bool blockNeedsParentheses(const SkNode *body)
  { return !body ||  body->classType() != Sk::C_Block; }

  // Unparse functions within the input node. FIXME: lamed design >_<
  static std::string unparseFunctionCalls(const SkNode *parent);
};

/*
 *  Contract of the descendant of SkNode.
 *  All child classes of SkNode should put SK_NODE on the top.
 */
#define SK_NODE(_class, _base) \
  SK_DISABLE_COPY(Sk##_class) \
  private: \
    typedef Sk##_class Self; \
    typedef Sk##_base Base; \
    Self *self() const { return const_cast<Self *>(this); } \
  public: \
    Sk::ClassType classType() const override { return Sk::C_##_class; } \
    const char *className() const override { return "Sk" #_class; } \
  private:

#define SK_ABSTRACT \
  public: \
    Self *clone() const override = 0; \
  private:

///  TODO: Implement in a more efficient way by creating sk metaobject.
template <typename T>
inline T sknode_cast(SkNode *node)
{ return dynamic_cast<T>(node); }

template <typename T>
inline T sknode_cast(const SkNode *node)
{
  // this will cause a compilation error if T is not const
  //register
  T ptr = static_cast<T>(node);
  SK_UNUSED(ptr);
  return sknode_cast<T>(const_cast<SkNode *>(node));
}

// - Literals -

/**
 *  \brief  Abstract literal.
 *
 *  Only numeric literals is implemented at the time.
 *  JG? In general, the granularity of skeleton is basic block; however,
 *    a SkLiteral Node is needed in identifying/expressing critical paths.
 */
class SkLiteral : public SkNode
{
  SK_NODE(Literal, Node)
  SK_ABSTRACT
  SK_SCALAR_PROPERTY(value, setValue, SkVariant)
protected:
  explicit SkLiteral(SgNode *src = nullptr) : Base(src) {}
  explicit SkLiteral(const SkVariant &val, SgNode *src = nullptr)
    : Base(src), value_(val) {}

public:
  virtual Sk::ScalarType type() const = 0;

protected:
  SkVariant &rvalue() { return value_; } ///< Return the reference.
};

///  Bool contant
class SkBool : public SkLiteral
{
  SK_NODE(Bool, Literal)
public:
public:
  explicit SkBool(SgNode *src = nullptr) : Base(src) {}
  explicit SkBool(bool val, SgNode *src = nullptr) : Base(val, src) {}

  Self *clone() const override { return new Self(value(), source()); }

  Sk::ScalarType type() const override { return Sk::S_Bool; }

  ///  Overload with int type.
  bool value() const { return Base::value().toBool(); }
  void setValue(bool val) { Base::rvalue().setValue(val); }

  std::string toString() const override
  { return value() ? "true" : "false"; }

  std::string unparse() const override
  { return value() ? "True" : "False"; }
};

///  Abstract numeric literal.
class SkNumber : public SkLiteral
{
  SK_NODE(Number, Literal)
  SK_ABSTRACT
protected:
  explicit SkNumber(SgNode *src = nullptr) : Base(src) {}
  explicit SkNumber(const SkVariant &val, SgNode *src = nullptr)
    : Base(val, src) {}
};

///  Integer number (int/long/...)
class SkInteger : public SkNumber
{
  SK_NODE(Integer, Number)
public:
  explicit SkInteger(SgNode *src = nullptr) : Base(src) {}
  explicit SkInteger(int val, SgNode *src = nullptr)
    : Base(val, src) {}

  Self *clone() const override { return new Self(value(), source()); }

  Sk::ScalarType type() const override { return Sk::S_Integer; }

  ///  Overload with int type.
  int value() const { return Base::value().toInt(); }
  void setValue(int val) { Base::rvalue().setValue(val); }

  std::string toString() const override
  { return boost::lexical_cast<std::string>(value()); }
};

///  Real number (float/double/...)
class SkReal : public SkNumber
{
  SK_NODE(Real, Number)
public:
  explicit SkReal(SgNode *src = nullptr) : Base(src) {}
  explicit SkReal(double val, SgNode *src = nullptr)
    : Base(val, src) {}

  Self *clone() const override { return new Self(value(), source()); }

  Sk::ScalarType type() const override { return Sk::S_Integer; }

  ///  Overload with int type.
  double value() const { return Base::value().toDouble(); }
  void setValue(double val) { Base::rvalue().setValue(val); }

  std::string toString() const override
  { return boost::lexical_cast<std::string>(value()); }
};

///  Character literal, such as 'c'
class SkCharacter : public SkNumber
{
  SK_NODE(Character, Number)
public:
  explicit SkCharacter(SgNode *src = nullptr) : Base(src) {}
  explicit SkCharacter(char val, SgNode *src = nullptr)
    : Base(val, src) {}

  Self *clone() const override { return new Self(value(), source()); }

  Sk::ScalarType type() const override { return Sk::S_Character; }

  ///  Overload with int type.
  char value() const { return Base::value().toChar(); }
  void setValue(char val) { Base::rvalue().setValue(val); }

  std::string toString() const override
  { return "'" + std::string(1, value()) + "'"; }
};

///  String literal, such as "a string"
class SkString : public SkLiteral
{
  SK_NODE(String, Literal)
  SK_SCALAR_PROPERTY(value, setValue, std::string)
public:
  explicit SkString(SgNode *src = nullptr) : Base(src) {}
  explicit SkString(const std::string &val, SgNode *src = nullptr)
    : Base(src), value_(val) {}

  Self *clone() const override { return new Self(value(), source()); }

  Sk::ScalarType type() const override { return Sk::S_String; }

  std::string toString() const override
  { return "\"" + value_ + "\""; }
};

// - Types -

/**
 *  \brief  Abstract expression type.
 *
 *  Include scalar type and array type.
 */
class SkType : public SkNode
{
  SK_NODE(Type, Node)
  SK_ABSTRACT
protected:
  explicit SkType(SgNode *src = nullptr) : Base(src) {}
};

// - Symbols -

///  Abstract symbol.
class SkSymbol : public SkNode
{
  SK_NODE(Symbol, Node)
  SK_PROPERTY(type, setType, Type)
  SK_PROPERTY(value, setValue, Node)
  SK_SCALAR_PROPERTY(name, setName, std::string)
protected:
  explicit SkSymbol(SgNode *src = nullptr)
    : Base(src), type_(nullptr), value_(nullptr) {}

  SkSymbol(const std::string &name, SkType *type, SgNode *src = nullptr)
    : Base(src), type_(type), value_(nullptr), name_(name)
  { if (type_) type_->setParent(this); }

  SkSymbol(const std::string &name, SkType *type, SkNode *value, SgNode *src = nullptr)
    : Base(src), type_(type), value_(value), name_(name)
  {
    if (type_) type_->setParent(this);
    if (value_) value->setParent(this);
  }

public:
  Self *clone() const override
  { return new Self(name_, type_ ? type_->clone() : nullptr, value_ ? value_->clone() : nullptr, source()); }

  ///  Return type.name
  std::string toString() const override
  {
    std::string ret = type_ ? type_->toString() : std::string(SK_UNKNOWN);
    ret.append(".")
       .append(name_);
    if (value_)
      ret.append(" = ")
         .append(value_->toString());
    return ret;
  }

  ///  Return variable name
  std::string unparse() const override { return name_; }
};

///  The variable symble.
class SkVariable : public SkSymbol
{
  SK_NODE(Variable, Symbol)
public:
  explicit SkVariable(SgNode *src = nullptr) : Base(src) {}
  SkVariable(const std::string &name, SkType *type, SgNode *src = nullptr)
    : Base(name, type, src) {}

  Self *clone() const override
  {
    Self *ret = new Self(name(), type() ? type()->clone() : nullptr, source());
    if (value()) ret->setValue(value()->clone());
    return ret;
  }
};

/**
 *  \brief  The function symble.
 *
 *  Function type is not implemented yet.
 */
class SkFunction : public SkSymbol
{
  SK_NODE(Function, Symbol)
public:
  explicit SkFunction(SgNode *src = nullptr) : Base(src) {}
  explicit SkFunction(const std::string &name, SgNode *src = nullptr)
    : Base(name, nullptr, src) {}

  Self *clone() const override
  {
    Self *ret = new Self(name(), source());
    if (type()) ret->setType(type()->clone());
    if (value()) ret->setValue(value()->clone());
    return ret;
  }
};

// - Expressions -

///  Abstract expression
class SkExpression : public SkNode
{
  SK_NODE(Expression, Node)
  SK_ABSTRACT
protected:
  explicit SkExpression(SgNode *src = nullptr) : Base(src) {}
};

//  Represents an invalid expression. Shouldn't appear in the AST.
//class SkNullExpression : public SkExpression
//{
//  SK_NODE(NullExpression, Expression)
//public:
//  explicit SkNullExpression(SgNode *src = nullptr) : Base(src) {}
//
//  SkNode *value() const override { return nullptr; }
//
//  std::string unparse() const override { return SK_UNKNOWN; }
//};

///  Symbolic reference (lvalue)
class SkReference : public SkExpression
{
  SK_NODE(Reference, Expression)
  SK_PROPERTY(symbol, setSymbol, Symbol)
public:
  explicit SkReference(SgNode *src = nullptr)
    : Base(src), symbol_(nullptr) {}

  explicit SkReference(SkSymbol *symbol, SgNode *src = nullptr)
    : Base(src), symbol_(symbol)
  { if (symbol_) symbol_->setParent(this); }

  Self *clone() const override
  { return new Self(symbol_ ? symbol_->clone() : nullptr, source()); }

  //SkNode *value() const override { return symbol(); }

  std::string toString() const override
  { return symbol_ ? symbol_->toString() : std::string(SK_UNKNOWN); }

  std::string unparse() const override
  { return symbol_ ? symbol_->unparse() : std::string(SK_UNKNOWN); }
};

/**
 *  \brief  Array access expression, such as a(i,j)
 *
 *  Conventions:
 *  - Array subscript is nullabe.
 */
class SkArrayReference : public SkReference
{
  SK_NODE(ArrayReference, Reference)
  SK_LIST_PROPERTY(indices, setIndices, Expression)
public:
  explicit SkArrayReference(SgNode *src = nullptr) : Base(src) {}
  explicit SkArrayReference(SkSymbol *symbol, SgNode *src = nullptr)
    : Base(symbol, src) {}

  Self *clone() const override
  {
    Self *ret = new Self(symbol() ? symbol()->clone() : nullptr, source());
    BOOST_FOREACH (SkExpression *it, indices_)
      ret->appendIndex(it ? it->clone() : nullptr);
    return ret;
  }

  int dimension() const { return indices_.size(); }

  void replaceIndex(SkExpression *from, SkExpression *to)
  {
    SkExpressionList::iterator p = boost::find(indices_, from);
    if (p != indices_.end()) {
      if (*p && (*p)->parent() == this)
        (*p)->setParent(nullptr);
      if (to)
        to->setParent(this);
      *p = to;
    }
  }

  ///  Add a dimension, nullable.
  void prependIndex(SkExpression *val = nullptr)
  {
    indices_.push_front(val);
    if (val)
      val->setParent(this);
  }

  ///  Add a dimension, nullable.
  void appendIndex(SkExpression *val = nullptr)
  {
    indices_.push_back(val);
    if (val)
      val->setParent(this);
  }

  ///  Return type[dim1][dim2]...
  std::string toString() const override
  {
    std::string ret = symbol() ? symbol()->toString() : std::string(SK_UNKNOWN);
    BOOST_FOREACH (const SkExpression *it, indices_)
      ret.append("[")
         .append(it ? it->toString() : std::string(SK_UNKNOWN))
         .push_back(']');
    return ret;
  }

  ///  Return type[dim1][dim2]...
  std::string unparse() const override
  {
    std::string ret = symbol() ? symbol()->unparse() : std::string(SK_UNKNOWN_DIMENSION);
    BOOST_FOREACH (const SkExpression *it, indices_)
      ret.append("[")
         .append(it ? it->unparse() : std::string(SK_UNKNOWN_DIMENSION))
       .push_back(']');
    return ret;
  }
};

///  List initializer
class SkList : public SkExpression
{
  SK_NODE(List, Expression)
  SK_LIST_PROPERTY(values, setValues, Expression)
public:
  explicit SkList(SgNode *src = nullptr)
    : Base(src) {}

  Self *clone() const override
  {
    Self *ret = new Self(source());
    BOOST_FOREACH (SkExpression *it, values_)
      ret->append(it ? it->clone() : nullptr);
    return ret;
  }

  int size() const { return values_.size(); }

  void prepend(SkExpression *value)
  {
    values_.push_front(value);
    if (value)
      value->setParent(this);
  }

  void append(SkExpression *value)
  {
    values_.push_back(value);
    if (value)
      value->setParent(this);
  }
  std::string toString() const override
  {
    std::string ret;
    BOOST_FOREACH(SkExpression *it, values_)
      if (it) {
        if (!ret.empty())
          ret.append(", ");
        ret.append(it->toString());
      }
    return "list(" + ret + ")";
  }

  std::string unparse() const override
  {
    std::string ret;
    BOOST_FOREACH(SkExpression *it, values_)
      if (it) {
        if (!ret.empty())
          ret.append(", ");
        ret.append(it->unparse());
      }
    return needsBrackets() ? "[" + ret + "]" : ret;
  }

protected:
  bool needsBrackets() const
  {
    if (parent())
      switch (parent()->classType()) {
      case Sk::C_CaseStatement:
      //case Sk::C_AllocateExpression:
      //case Sk::C_DeallocateExpression:
        return false;
      default: ;
      }
    return true;
  }
};

///  Value expression (rvalue)
class SkValue : public SkExpression
{
  SK_NODE(Value, Expression)
  SK_PROPERTY(literal, setLiteral, Literal)
public:
  explicit SkValue(SgNode *src = nullptr)
    : Base(src), literal_(nullptr) {}

  explicit SkValue(SkLiteral *literal, SgNode *src = nullptr)
    : Base(src), literal_(literal)
  { if (literal_) literal_->setParent(this); }

  explicit SkValue(bool val, SgNode *src = nullptr)
    : Base(src)
  { (literal_ = new SkBool(val, src))->setParent(this);  }

  explicit SkValue(int val, SgNode *src = nullptr)
    : Base(src)
  { (literal_ = new SkInteger(val, src))->setParent(this);  }

  explicit SkValue(double val, SgNode *src = nullptr)
    : Base(src)
  { (literal_ = new SkReal(val, src))->setParent(this);  }

  explicit SkValue(char val, SgNode *src = nullptr)
    : Base(src)
  { (literal_ = new SkCharacter(val, src))->setParent(this);  }

  explicit SkValue(const std::string &val, SgNode *src = nullptr)
    : Base(src)
  { (literal_ = new SkString(val, src))->setParent(this);  }

  Self *clone() const override
  {  return new Self(literal_ ? literal_->clone() : nullptr, source()); }

  std::string toString() const override
  { return literal_ ? literal_->toString() : std::string(SK_UNKNOWN); }

  std::string unparse() const override
  { return literal_ ? literal_->unparse() : std::string(SK_UNKNOWN); }
};

///  Abstract operation, parent of expressions that have an operator
class SkOperation : public SkExpression
{
  SK_NODE(Operation, Expression)
  SK_ABSTRACT
  SK_SCALAR_PROPERTY(op, setOp, Sk::OperatorType)
protected:
  explicit SkOperation(SgNode *src = nullptr)
    : Base(src), op_(Sk::O_Nop) {}

  explicit SkOperation(Sk::OperatorType op, SgNode *src = nullptr)
    : Base(src), op_(op) {}

protected:
  ///  Suggest parentheses to wrap current expression when unparse
  bool needsParentheses() const
  {
    Self *p;
    return classType() != Sk::C_UnaryOperation &&
        (p = sknode_cast<Self *>(parent())) &&
        Sk::Operator(p->op()).prec() <= Sk::Operator(op()).prec();
  }
};

///  Unary expression
class SkUnaryOperation : public SkOperation
{
  SK_NODE(UnaryOperation, Operation)
  SK_PROPERTY(operand, setOperand, Expression)
public:
  explicit SkUnaryOperation(SgNode *src = nullptr)
    : Base(src), operand_(nullptr) {}

  explicit SkUnaryOperation(Sk::OperatorType op, SgNode *src = nullptr)
    : Base(op, src), operand_(nullptr) {}

  SkUnaryOperation(Sk::OperatorType op, SkExpression *operand, SgNode *src = nullptr)
    : Base(op, src), operand_(operand)
  { if (operand_) operand_->setParent(this); }

  Self *clone() const override
  { return new Self(op(), operand_ ? operand_->clone() : nullptr, source()); }

  std::string toString() const override
  {
    return std::string(Sk::operatorName(op()))
      .append("(")
      .append(operand_ ? operand_->toString() : std::string(SK_UNKNOWN))
      .append(")");
  }

  std::string unparse() const override
  {
    bool par = needsParentheses();
    bool left = Sk::Operator(op()).assoc() == Sk::LeftAssoc;
    std::string ret;
    std::string op_str = Sk::unparseOperator(op());
    if (par)
      ret.push_back('(');
    if (left && !op_str.empty())
      ret.append(op_str)
         .append(" ");
    ret.append(operand_ ? operand_->unparse() : std::string(SK_UNKNOWN));
    if (!left && !op_str.empty())
      ret.append(" ")
         .append(op_str);
    if (par)
      ret.push_back(')');
    return ret;
  }
};

///  Binary expression
class SkBinaryOperation : public SkOperation
{
  SK_NODE(BinaryOperation, Operation)
  SK_PROPERTY(left, setLeft, Expression)
  SK_PROPERTY(right, setRight, Expression)
public:
  explicit SkBinaryOperation(SgNode *src = nullptr)
    : Base(src), left_(nullptr), right_(nullptr) {}

  explicit SkBinaryOperation(Sk::OperatorType op, SgNode *src = nullptr)
    : Base(op, src), left_(nullptr), right_(nullptr) {}

  SkBinaryOperation(Sk::OperatorType op, SkExpression *left, SkExpression *right, SgNode *src = nullptr)
    : Base(op, src), left_(left), right_(right)
  {
    if (left_) left_->setParent(this);
    if (right_) right_->setParent(this);
  }

  Self *clone() const override
  { return new Self(op(), left_ ? left_->clone() : nullptr, right_ ? right_->clone() : nullptr, source()); }

  bool isAssignment() const
  { return op() == Sk::O_Assign; }

  std::string toString() const override
  {
    return std::string(Sk::operatorName(op()))
      .append("(")
      .append(left_ ? left_->toString() : std::string(SK_UNKNOWN))
      .append(", ")
      .append(right_ ? right_->toString() : std::string(SK_UNKNOWN))
      .append(")");
  }

  std::string unparse() const override
  {
    bool par = needsParentheses();
    std::string ret;
    if (par)
      ret.push_back('(');
    ret.append(left_ ? left_->unparse() : std::string(SK_UNKNOWN))
       .append(" ")
       .append(Sk::unparseOperator(op()))
       .append(" ")
       .append(right_ ? right_->unparse() : std::string(SK_UNKNOWN));
    if (par)
      ret.push_back(')');
    return ret;
  }
};

/**
 *  \brief  Represent a function call expression.
 *
 *  Conventions:
 *  - Function parameters being invoked are nullable
 */
#define SK_KW_CALL  "call"
class SkFunctionCall : public SkExpression
{
  SK_NODE(FunctionCall, Expression)
  SK_SCALAR_PROPERTY(keyword, setKeyword, std::string)
  SK_PROPERTY(function, setFunction, Function)
  SK_LIST_PROPERTY(arguments, setArguments, Expression)
public:
  explicit SkFunctionCall(SgNode *src = nullptr)
    : Base(src), keyword_(SK_KW_CALL), function_(nullptr) {}

  explicit SkFunctionCall(SkFunction *func, SgNode *src = nullptr)
    : Base(src), keyword_(SK_KW_CALL), function_(func)
  { if (function_) function_->setParent(this); }

  explicit SkFunctionCall(SkFunction *func, const SkExpressionList args, SgNode *src = nullptr)
    : Base(src), function_(func), arguments_(args)
  {
    if (function_) function_->setParent(this);
    BOOST_FOREACH (SkExpression *it, arguments_) if (it) it->setParent(this);
  }

  Self *clone() const override
  {
    Self *ret = new Self(function_ ? function_->clone() : nullptr, source());
    ret->setKeyword(keyword());
    BOOST_FOREACH (SkExpression *it, arguments_)
      ret->appendArgument(it ? it->clone() : nullptr);
    return ret;
  }

  ///  Number of arguments
  int argumentCount() const { return arguments_.size(); }

  void replaceArgument(SkExpression *from, SkExpression *to)
  {
    SkExpressionList::iterator p = boost::find(arguments_, from);
    if (p != arguments_.end()) {
      if (*p && (*p)->parent() == this)
        (*p)->setParent(nullptr);
      if (to)
        to->setParent(this);
      *p = to;
    }
  }

  ///  This class will take the ownership.
  void appendArgument(SkExpression *e)
  {
    arguments_.push_back(e);
    if (e)
      e->setParent(this);
  }

  void prependArgument(SkExpression *e)
  {
    arguments_.push_front(e);
    if (e)
      e->setParent(this);
  }

  std::string toString() const override
  {
    std::string ret = function_ ? function_->toString() : std::string(SK_UNKNOWN);
    ret.push_back('(');
    bool comma = false;
    BOOST_FOREACH (const SkExpression *it, arguments_) {
      if (comma)
        ret.append(", ");
      else
        comma = true;
      ret.append(it ? it->toString() : std::string(SK_UNKNOWN));
    }
    ret.push_back(')');
    return ret;
  }

  std::string unparse() const override
  {
    std::string ret;
    if (needsCall())
      ret.append(keyword_).append(" ");
    ret.append(function_ ? function_->unparse() : std::string(SK_UNKNOWN))
       .push_back('(');
    bool comma = false;
    BOOST_FOREACH (const SkExpression *it, arguments_) {
      if (comma)
        ret.append(", ");
      else
        comma = true;
      ret.append(it ? it->unparse() : std::string(SK_UNKNOWN));
    }
    ret.push_back(')');
    return ret;
  }

  ///  Return if unparse require a leading "call" keyword
  bool needsCall() const
  { return parent() && parent()->classType() == Sk::C_ExpressionStatement; }
};

///  Fortran allocate statement, or new expression in C
#define SK_KW_ALLOC "alloc"
class SkAllocateExpression : public SkExpression
{
  SK_NODE(AllocateExpression, Expression)
  SK_SCALAR_PROPERTY(keyword, setKeyword, std::string)
  SK_PROPERTY(value, setValue, Expression)
public:
  explicit SkAllocateExpression(SgNode *src = nullptr)
    : Base(src), keyword_(SK_KW_ALLOC), value_(nullptr) {}

  explicit SkAllocateExpression(SkExpression *value, SgNode *src = nullptr)
    : Base(src), keyword_(SK_KW_ALLOC), value_(value)
  { if (value_) value_->setParent(this); }

  Self *clone() const override
  { return new Self(value_ ? value_->clone() : nullptr, source()); }

  std::string toString() const override
  { return keyword_ + " " + (value_ ? value_->toString() : std::string(SK_UNKNOWN)); }

  std::string unparse() const override
  { return keyword_ + " " + (value_ ? value_->unparse() : std::string(SK_UNKNOWN)); }
};

///  Fortran deallocate statement, or delete expression in C
#define SK_KW_DEALLOC   "free"
class SkDeallocateExpression : public SkExpression
{
  SK_NODE(DeallocateExpression, Expression)
  SK_SCALAR_PROPERTY(keyword, setKeyword, std::string)
  SK_PROPERTY(value, setValue, Expression)
public:
  explicit SkDeallocateExpression(SgNode *src = nullptr)
    : Base(src), keyword_(SK_KW_DEALLOC), value_(nullptr) {}

  explicit SkDeallocateExpression(SkExpression *value, SgNode *src = nullptr)
    : Base(src), keyword_(SK_KW_DEALLOC), value_(value)
  { if (value_) value_->setParent(this); }

  Self *clone() const override
  { return new Self(value_ ? value_->clone() : nullptr, source()); }

  std::string toString() const override
  { return keyword_ + " " + (value_ ? value_->toString() : std::string(SK_UNKNOWN)); }

  std::string unparse() const override
  { return keyword_ + " " + (value_ ? value_->unparse() : std::string(SK_UNKNOWN)); }
};

// - Types -

/**
 *  \brief  Scalar type.
 *
 *  Such as int or float.
 */
class SkScalarType : public SkType
{
  SK_NODE(ScalarType, Type)
  SK_SCALAR_PROPERTY(type, setType, Sk::ScalarType)
public:
  explicit SkScalarType(SgNode *src = nullptr)
    : Base(src), type_(Sk::S_Null) {}
  explicit SkScalarType(Sk::ScalarType type, SgNode *src = nullptr)
    : Base(src), type_(type) {}

  Self *clone() const override { return new Self(type_, source()); }

  ///  Return type name
  std::string toString() const override
  { return Sk::scalarTypeName(type_); }
};

/**
 *  \brief  Array type.
 *
 *  Include the base type and the dimension.
 *  Conventions:
 *  - Array dimension is nullabe.
 */
class SkArrayType : public SkType
{
  SK_NODE(ArrayType, Type)
  SK_PROPERTY(baseType, setBaseType, Type)
  SK_LIST_PROPERTY(dimensions, setDimensions, Expression)
public:
  explicit SkArrayType(SgNode *src = nullptr)
    : Base(src), baseType_(nullptr) {}

  explicit SkArrayType(SkType *baseType, SgNode *src = nullptr)
    : Base(src), baseType_(baseType)
  { if (baseType_) baseType_->setParent(this); }

  ///  Number of dimensions
  int dimensionCount() const
  { return dimensions_.size(); }

  Self *clone() const override
  {
    Self *ret = new Self(baseType_ ? baseType_->clone() : nullptr, source());
    BOOST_FOREACH (SkExpression *it, dimensions_)
      ret->appendDimension(it ? it->clone() : nullptr);
    return ret;
  }

  ///  Add an unknown dimension.
  void prependDimension(SkExpression *val = nullptr)
  {
    dimensions_.push_front(val);
    if (val)
      val->setParent(this);
  }

  ///  Add an unknown dimension.
  void appendDimension(SkExpression *val = nullptr)
  {
    dimensions_.push_back(val);
    if (val)
      val->setParent(this);
  }

  ///  Return type[dim1][dim2]...
  std::string toString() const override
  {
    std::string ret = baseType_ ? baseType_->toString() : std::string(SK_UNKNOWN);
    BOOST_FOREACH (const SkExpression *it, dimensions_)
      ret.append("[")
         .append(it ? it->toString() : std::string(SK_UNKNOWN))
         .push_back(']');
    return ret;
  }

  ///  Return type[dim1][dim2]...
  std::string unparse() const override
  {
    std::string ret = baseType_ ? baseType_->unparse() : std::string(SK_UNKNOWN_DIMENSION);
    BOOST_FOREACH (const SkExpression *it, dimensions_)
      ret.append("[")
         .append(it ? it->unparse() : std::string(SK_UNKNOWN_DIMENSION))
         .push_back(']');
    return ret;
  }
};
// - Statements -

///  Abstract statement.
class SkStatement : public SkNode
{
  SK_NODE(Statement, Node)
  SK_ABSTRACT
protected:
  explicit SkStatement(SgNode *src = nullptr) : Base(src) {}
};

///  Null statement, such as ";"
class SkNullStatement : public SkStatement
{
  SK_NODE(NullStatement, Statement)
public:
  explicit SkNullStatement(SgNode *src = nullptr) : Base(src) {}
  Self *clone() const override { return new Self(source()); }

  std::string toString() const override { return "{}"; }
};

///  Expression statement.
class SkExpressionStatement : public SkStatement
{
  SK_NODE(ExpressionStatement, Statement)
  SK_PROPERTY(expression, setExpression, Expression)
public:
  explicit SkExpressionStatement(SgNode *src = nullptr)
    : Base(src), expression_(nullptr) {}

  explicit SkExpressionStatement(SkExpression *expr, SgNode *src = nullptr)
    : Base(src), expression_(expr)
  { if (expression_) expression_->setParent(this); }

  Self *clone() const override
  {  return new Self(expression_ ? expression_->clone() : nullptr, source());  }

  std::string toString() const override
  { return expression_ ? expression_->toString() : std::string(SK_UNKNOWN); }

  std::string unparse() const override
  {
    return expression_ ? unparseFunctionCalls(expression_) + expression_->unparse()
                 : std::string(SK_UNKNOWN);
  }
};

///  Represents the ld statement.
class SkReferenceStatement : public SkStatement
{
  SK_NODE(ReferenceStatement, Statement)
  SK_ABSTRACT
  SK_PROPERTY(reference, setReference, Reference)
protected:
  explicit SkReferenceStatement(SgNode *src = nullptr)
    : Base(src), reference_(nullptr) {}
  explicit SkReferenceStatement(SkReference *ref, SgNode *src = nullptr)
    : Base(src), reference_(ref)
  { if (reference_) reference_->setParent(this); }

  std::string unparseReference() const
  {
    std::string ret;
    if (reference()) {
      ret = reference()->unparse();
      //if (SkSymbol *s = reference()->symbol())
      //  if (SkType *t = s->type())
      //    if (t->classType() == Sk::C_ArrayType)
      //      BOOST_FOREACH (const SkExpression *it, sknode_cast<SkArrayType *>(t)->dimensions())
      //        if (it->classType() == Sk::C_Slice)
      //          ret.append("[")
      //             .append(it ? it->unparse() : std::string(SK_UNKNOWN_DIMENSION))
      //             .push_back(']');
      //        else
      //          ret.append("[0:")
      //             .append(it ? it->unparse() : std::string(SK_UNKNOWN_DIMENSION))
      //             .push_back(']');

    }
    return ret;
  }
};

///  Represents the ld statement.
#define SK_KW_LD    "ld"
class SkLoadStatement : public SkReferenceStatement
{
  SK_NODE(LoadStatement, ReferenceStatement)
  SK_SCALAR_PROPERTY(keyword, setKeyword, std::string)
public:
  explicit SkLoadStatement(SgNode *src = nullptr)
    : Base(src), keyword_(SK_KW_LD) {}
  explicit SkLoadStatement(SkReference *ref, SgNode *src = nullptr)
    : Base(ref, src), keyword_(SK_KW_LD) {}

  Self *clone() const override
  {  return new Self(reference() ? reference()->clone() : nullptr, source());  }

  std::string toString() const override
  { return keyword_ + " " + (reference() ? reference()->toString() : std::string(SK_UNKNOWN)); }

  std::string unparse() const override
  { return keyword_ + " " + unparseReference(); }
};

///  Represents the st statement.
#define SK_KW_ST    "st"
class SkStoreStatement : public SkReferenceStatement
{
  SK_NODE(StoreStatement, ReferenceStatement)
  SK_SCALAR_PROPERTY(keyword, setKeyword, std::string)
public:
  explicit SkStoreStatement(SgNode *src = nullptr)
    : Base(src), keyword_(SK_KW_ST) {}
  explicit SkStoreStatement(SkReference *ref, SgNode *src = nullptr)
    : Base(ref, src), keyword_(SK_KW_ST) {}

  Self *clone() const override
  {  return new Self(reference() ? reference()->clone() : nullptr, source());  }

  std::string toString() const override
  { return keyword_ + " " + (reference() ? reference()->toString() : std::string(SK_UNKNOWN)); }

  std::string unparse() const override
  { return keyword_ + " " + unparseReference(); }
};

///  Abstract class. Represents the comp statement.
class SkComputeStatement : public SkStatement
{
  SK_NODE(ComputeStatement, Statement)
  SK_ABSTRACT
  SK_SCALAR_PROPERTY(keyword, setKeyword, std::string)
  SK_SCALAR_PROPERTY(value, setValue, int)
protected:
  explicit SkComputeStatement(SgNode *src = nullptr)
    : Base(src), value_(0) {}
  explicit SkComputeStatement(int val, SgNode *src = nullptr)
    : Base(src), value_(val) {}

  //std::string toString() const override
  //{ return "comp " + boost::lexical_cast<std::string>(value_); }

  std::string toString() const override
  { return keyword_ + " " + boost::lexical_cast<std::string>(value()); }
};

///  Computation intensity of integer operations
#define SK_KW_XP    "xp"
class SkFixedPointStatement : public SkComputeStatement
{
  SK_NODE(FixedPointStatement, ComputeStatement)
public:
  explicit SkFixedPointStatement(SgNode *src = nullptr)
    : Base(src) { setKeyword(SK_KW_XP); }
  explicit SkFixedPointStatement(int val, SgNode *src = nullptr)
    : Base(val, src) { setKeyword(SK_KW_XP); }

  Self *clone() const override { return new Self(value(), source()); }
};

///  Computation intensity of floating-point operations
#define SK_KW_FP    "fp"
class SkFloatingPointStatement : public SkComputeStatement
{
  SK_NODE(FloatingPointStatement, ComputeStatement)
public:
  explicit SkFloatingPointStatement(SgNode *src = nullptr)
  : Base(src) { setKeyword(SK_KW_FP); }
  explicit SkFloatingPointStatement(int val, SgNode *src = nullptr)
    : Base(val, src) { setKeyword(SK_KW_FP); }

  Self *clone() const override { return new Self(value(), source()); }
};

///  Represents the br statement.
#define SK_KW_BR    "br"
class SkBranchStatement : public SkComputeStatement
{
  SK_NODE(BranchStatement, ComputeStatement)
public:
  explicit SkBranchStatement(SgNode *src = nullptr)
  : Base(src) { setKeyword(SK_KW_BR); }
  explicit SkBranchStatement(int val, SgNode *src = nullptr)
    : Base(val, src) { setKeyword(SK_KW_BR); }

  Self *clone() const override { return new Self(value(), source()); }
};

///  Represent an array subscript, similar to std::slice
class SkSlice : public SkExpression
{
  SK_NODE(Slice, Expression)
  SK_PROPERTY(start, setStart, Expression)
  SK_PROPERTY(stop, setStop, Expression)
  SK_PROPERTY(step, setStep, Expression)
public:
  explicit SkSlice(SgNode *src = nullptr)
    : Base(src), start_(nullptr), stop_(nullptr), step_(nullptr) {}

  SkSlice(SkExpression *start, SkExpression *stop, SkExpression *step, SgNode *src = nullptr)
    : Base(src), start_(start), stop_(stop), step_(step)
  {
    if (start_) start_->setParent(this);
    if (stop_) stop_->setParent(this);
    if (step_) step_->setParent(this);
  }

  Self *clone() const override
  {
    return new Self(
        start_ ? start_->clone() : nullptr,
        stop_ ? stop_->clone() : nullptr,
        step_ ? step_->clone() : nullptr,
        source());
  }

  std::string toString() const override
  {
    std::string ret;
    if (start_)
      ret = start_->toString().append(":");
    else
      ret = ":";
    if (stop_)
      ret.append(stop_->toString());
    else
      ret.append(SK_UNKNOWN);
    if (step_)
      ret.append(":").append(step_->toString());
    return ret;
  }

  std::string unparse() const override
  {
    if (isUnknown())
      return SK_UNKNOWN;
    std::string ret;
    if (start_)
      ret = start_->unparse().append(":");
    else
      ret = ":";
    if (stop_)
      ret.append(stop_->unparse());
    else
      ret.append(SK_UNKNOWN);
    if (step_) {
      std::string t = step_->unparse();
      if (t != "1")
        ret.append(":").append(step_->unparse());
    }
    return ret;
  }

protected:
  bool isUnknown() const
  {
    return parent() &&
        parent()->parent() &&
        parent()->parent()->parent() &&
        parent()->parent()->parent()->classType() == Sk::C_SymbolDeclaration;
  }
};

#define SK_KW_FOREVER   "while (true)"
class SkForeverStatement : public SkStatement
{
  SK_NODE(ForeverStatement, Statement)
  SK_SCALAR_PROPERTY(keyword, setKeyword, std::string)
  SK_PROPERTY(body, setBody, Statement)
public:
  explicit SkForeverStatement(SgNode *src = nullptr)
    : Base(src), keyword_(SK_KW_FOREVER), body_(nullptr) {}

  explicit SkForeverStatement(SkStatement *body, SgNode *src = nullptr)
    : Base(src), keyword_(SK_KW_FOREVER), body_(body) { if (body_) body_->setParent(this); }

  Self *clone() const override
  { return new Self(body_ ? body_->clone() : nullptr, source()); }

  std::string toString() const override
  { return keyword_ + " " + unparseBlock(body_); }
};

///  Represents a while-statement
#define SK_KW_WHILE   "while"
class SkWhileStatement : public SkStatement
{
  SK_NODE(WhileStatement, Statement)
  SK_SCALAR_PROPERTY(keyword, setKeyword, std::string)
  SK_PROPERTY(condition, setCondition, Expression)
  SK_PROPERTY(body, setBody, Statement)
public:
  explicit SkWhileStatement(SgNode *src = nullptr)
    : Base(src), keyword_(SK_KW_WHILE), condition_(nullptr), body_(nullptr) {}

  SkWhileStatement(SkExpression *cond, SkStatement *body, SgNode *src = nullptr)
    : Base(src), keyword_(SK_KW_WHILE), condition_(cond), body_(body)
  {
    if (condition_) condition_->setParent(this);
    if (body_) body_->setParent(this);
  }

  Self *clone() const override
  { return new Self(condition_ ? condition_->clone() : nullptr, body_ ? body_->clone() : nullptr, source()); }

  std::string toString() const override
  {
    return std::string(keyword_)
        .append(" (")
        .append(condition_ ? condition_->toString() : std::string(SK_UNKNOWN))
        .append(") ")
        .append(body_ ? body_->toString() : std::string("{}"));
  }

  std::string unparse() const override
  {
    return unparseFunctionCalls(condition_)
       .append(unparseLineComment())
       .append(keyword_)
       .append(" (")
       .append(condition_ ? condition_->unparse() : std::string(SK_UNKNOWN))
       .append(") ")
       .append(unparseBlock(body_));
  }
};

///  Represents a do-while statement
class SkDoWhileStatement : public SkStatement
{
  SK_NODE(DoWhileStatement, Statement)
  SK_PROPERTY(condition, setCondition, Expression)
  SK_PROPERTY(body, setBody, Statement)
public:
  explicit SkDoWhileStatement(SgNode *src = nullptr)
    : Base(src), condition_(nullptr), body_(nullptr) {}

  SkDoWhileStatement(SkExpression *cond, SkStatement *body, SgNode *src = nullptr)
    : Base(src), condition_(cond), body_(body)
  {
    if (condition_) condition_->setParent(this);
    if (body_) body_->setParent(this);
  }

  Self *clone() const override
  { return new Self(condition_ ? condition_->clone() : nullptr, body_ ? body_->clone() : nullptr, source()); }

  std::string toString() const override
  {
    return std::string("do ")
       .append(body_ ? body_->toString() : std::string("{}"))
       .append("\nwhile (")
       .append(condition_ ? condition_->toString() : std::string(SK_UNKNOWN))
       .append(")");
  }

  std::string unparse() const override
  {
    return unparseFunctionCalls(condition_)
       .append(unparseLineComment())
       .append("do ")
       .append(unparseBlock(body_))
       .append("\nwhile (")
       .append(condition_ ? condition_->unparse() : std::string(SK_UNKNOWN))
       .append(")");
  }
};

// - Pragma -

///  Pragma statement such as OpenMP.
class SkPragma : public SkStatement
{
  SK_NODE(Pragma, Statement)
  SK_SCALAR_PROPERTY(command, setCommand, std::string)
public:
  explicit SkPragma(SgNode *src = nullptr) : Base(src) {}
  explicit SkPragma(const std::string &command, SgNode *src = nullptr)
    : Base(src), command_(command) {}

  Self *clone() const override { return new Self(command_, source()); }

  ///  Return if contains command
  bool empty() const { return command_.empty(); }

  std::string toString() const override
  { return command_; }
};

// - Scope -

/**
 *  \brief  Compound statement.
 *
 *  Conventions:
 *  - Null statement added to body will be skipped.
 *  JG 10/12/2012: Example:
 *    an_skblock->append(nullptr);
 *  The above code will not insert nullptr into the body, but will simply skip the nullptr.
 *  So that, I could asssume all stmt in the body are not null.
 */
class SkBlock : public SkStatement
{
  SK_NODE(Block, Statement)
  //SK_LIST_PROPERTY(statements, setStatements, Statement)
  SkStatementList body_;
public:
  explicit SkBlock(SgNode *src = nullptr)
    : Base(src) {}

  Self *clone() const override
  {
    Self *ret = new Self(source());
    BOOST_FOREACH (SkStatement *it, body_)
      if (it)
        ret->append(it->clone());
    return ret;
  }

  ///  Return the number of the statements.
  int size() const { return body_.size(); }

  ///  Return if contain statements.
  bool isEmpty() const { return body_.empty(); }

  ///  Contained statements.
  const SkStatementList &statements() const { return body_; }
  SkStatementList &rstatements() { return body_; }

  ///  Null statement will be skipped.
  void setStatements(const SkStatementList &l)
  {
    clear();
    append(l);
  }

  ///  Clear the body
  void clear()
  {
    if (!body_.empty()) {
      BOOST_FOREACH (SkStatement *it, body_)
        if (it && it->parent() == this)
          it->setParent(nullptr);
      body_.clear();
    }
  }

  ///  This class will take the ownership.
  void append(SkStatement *stmt)
  {
    if (!stmt)
      return;
    body_.push_back(stmt);
    stmt->setParent(this);
  }

  void replace(SkStatement *from, SkStatement *to)
  {
    SkStatementList::iterator p = boost::find(body_, from);
    if (p != body_.end()) {
      if (*p && (*p)->parent() == this)
        (*p)->setParent(nullptr);
      if (to)
        to->setParent(this);
      *p = to;
    }
  }

  void remove(SkStatement *stmt)
  {
    SkStatementList::iterator p = boost::find(body_, stmt);
    if (p != body_.end()) {
      body_.erase(p);
      if (stmt->parent() == this)
        stmt->setParent(nullptr);
    }
  }

  void append(const SkStatementList &l)
  {
    BOOST_FOREACH (SkStatement *it, l)
      if (it) {
        body_.push_back(it);
        it->setParent(this);
      }
  }

  void prepend(SkStatement *stmt)
  {
    if (!stmt)
      return;
    body_.push_front(stmt);
    stmt->setParent(this);
  }

  void prepend(const SkStatementList &l)
  {
    BOOST_FOREACH (SkStatement *it, l)
      if (it) {
        body_.push_front(it);
        it->setParent(this);
      }
  }

  ///  The removed stmt is not deleted.
  void remove(const SkStatement *stmt)
  { body_.remove(const_cast<SkStatement *>(stmt)); }

  std::string toString() const override
  {
    std::string ret = "{\n";
    BOOST_FOREACH (const SkStatement *it, body_)
      ret.append(it->toString())
         .push_back('\n');
    ret.push_back('}');
    return ret;
  }

  std::string unparse() const override
  {
    bool par = needsParentheses();
    std::string ret = comment();
    if (par)
      ret = "{\n";
    ret += unparseBody();
    if (par)
      ret.push_back('}');
    return ret;
  }

  ///  Return if the unparsed statement will be wrapped with blocks
  bool needsParentheses() const  { return true; }
  //bool needsParentheses() const  { return size() != 1; }

protected:
  ///  Unparse the statements without parentheses
  std::string unparseBody() const
  {
    std::string ret;
    BOOST_FOREACH (const SkStatement *it, body_) {
      std::string t = it->unparse();
      if (!t.empty())
        ret.append(t)
           .push_back('\n');
    }
    return ret;
  }

};

/**
 *  \brief  Barrier for dividing block into basic blocks
 *
 *  The source code of a barrier is the parent SgBasicBlock.
 *  A basic block consists of statements between beginSource and endSource (exclucded)
 *  The beginSource is the first statement of the basic block
 *  The endSource is the first statement after the last statement of the basic block.
 */
class SkBlockBarrier : public SkStatement
{
  SK_NODE(BlockBarrier, Statement)
  static int s_count_;
  int id_;
  SgNode *begin_, *end_; // The starting and ending statement
public:
  explicit SkBlockBarrier(SgNode *src = nullptr)
    : Base(src), id_(++s_count_), begin_(nullptr), end_(nullptr) {}

  SkBlockBarrier(SgNode *beg, SgNode *end, SgNode *src = nullptr)
    : Base(src), id_(++s_count_), begin_(beg), end_(end) {}

  Self *clone() const override
  { return new Self(begin_, end_, source()); }

  ///  Reset the static counter of class instances
  static void resetCounter() { s_count_ = 0; }

  ///  Return the unique id of this block
  int id() const { return id_; }

  ///  The first statement of the basic block
  SgNode *beginSource() const { return begin_; }
  void setBeginSource(SgNode *src) { begin_ = src; }

  ///  The first statement after the last statement of the basic block.
  SgNode *endSource() const { return end_; }
  void setEndSource(SgNode *src) { end_ = src; }

  ///  Return if there are no statements within this basic block
  bool isEmpty() const { return beginSource() == endSource(); }

  ///  Return list of statements within this basic block
  std::list<SgNode *> sourceStatements() const;

  ///  Return the unique name of this block
  std::string name() const
  {
    return configuration().k_blockStatement + " " +
        boost::lexical_cast<std::string>(id_);
  }

  std::string toString() const override
  { return name(); }
};

//class SkForNestStatement : public SkStatemet {};
//class SkForAllStatement : public SkStatemet {};

///  Represents a if-statement
class SkIfStatement : public SkStatement
{
  SK_NODE(IfStatement, Statement)
  SK_PROPERTY(condition, setCondition, Expression)
  SK_PROPERTY(trueBody, setTrueBody, Statement)
  SK_PROPERTY(falseBody, setFalseBody, Statement)
public:
  explicit SkIfStatement(SgNode *src = nullptr)
    : Base(src), condition_(nullptr), trueBody_(nullptr), falseBody_(nullptr) {}

  SkIfStatement(SkExpression *cond, SkStatement *tbody, SkStatement *fbody, SgNode *src = nullptr)
    : Base(src), condition_(cond), trueBody_(tbody), falseBody_(fbody)
  {
    if (condition_) condition_->setParent(this);
    if (trueBody_) trueBody_->setParent(this);
    if (falseBody_) falseBody_->setParent(this);
  }

  Self *clone() const override
  {
    return new Self(
        condition_ ? condition_->clone() : nullptr,
        trueBody_ ? trueBody_->clone() :  nullptr,
        falseBody_ ? falseBody_->clone() : nullptr,
        source());
  }

  std::string toString() const override
  {
    std::string ret = "if (";
    ret.append(condition_ ? condition_->toString() : std::string(SK_UNKNOWN))
       .append(") ")
       .append(trueBody_ ? trueBody_->toString() : std::string("{}"));
    if (falseBody_)
      ret.append("\nelse ")
         .append(falseBody_->toString());
    return ret;
  }

  std::string unparse() const override
  {
    std::string ret = unparseFunctionCalls(condition_)
        .append(unparseLineComment())
        .append("if (")
        .append(condition_ ? condition_->unparse() : std::string(SK_UNKNOWN))
        .append(") ")
        .append(unparseBlock(trueBody_));
    if (falseBody_ && needsElse())
      ret.append("\nelse ")
         .append(unparseBlock(falseBody_));
    return ret;
  }

private:
  bool needsElse() const
  {
    return falseBody_ &&  (falseBody_->classType() != Sk::C_Block ||
      !static_cast<SkBlock *>(falseBody_)->isEmpty());
  }

  //static bool needsParentheses(const SkStatement *body)
  //{
  //  return !body ||  (body->classType() != Sk::C_Block ||
  //    !static_cast<const SkBlock *>(body)->needsParentheses());
  //}
};

///  Represents a if-statement
class SkCaseStatement : public SkStatement
{
  SK_NODE(CaseStatement, Statement)
  SK_PROPERTY(condition, setCondition, Expression)
  SK_PROPERTY(body, setBody, Statement)
public:
  explicit SkCaseStatement(SgNode *src = nullptr)
    : Base(src), condition_(nullptr), body_(nullptr) {}

  explicit SkCaseStatement(SkStatement *body, SgNode *src = nullptr)
    : Base(src), condition_(nullptr), body_(body)
  { if (body_) body_->setParent(this); }

  SkCaseStatement(SkExpression *cond, SkStatement *body, SgNode *src = nullptr)
    : Base(src), condition_(cond), body_(body)
  {
    if (condition_) condition_->setParent(this);
    if (body_) body_->setParent(this);
  }

  Self *clone() const override
  { return new Self(condition_ ? condition_->clone() : nullptr, body_ ? body_->clone() : nullptr, source()); }

  std::string toString() const override
  {
    return std::string("case (")
       .append(condition_ ? condition_->toString() : std::string(SK_UNKNOWN))
       .append(") ")
       .append(body_ ? body_->toString() : std::string("{}"));
  }

  std::string unparse() const override
  {
    return unparseLineComment()
       .append("case (")
       .append(condition_ ? condition_->unparse() : std::string(SK_UNKNOWN))
       .append(") ")
       .append(unparseBlock(body_));
  }
};

///  Represents a if-statement
class SkDefaultCaseStatement : public SkCaseStatement
{
  SK_NODE(DefaultCaseStatement, CaseStatement)
public:
  explicit SkDefaultCaseStatement(SgNode *src = nullptr)
    : Base(src) {}

  explicit SkDefaultCaseStatement(SkStatement *body, SgNode *src = nullptr)
    : Base(body, src) {}

  Self *clone() const override
  {
    Self *ret = new Self(body() ? body()->clone() : nullptr, source());
    if (condition()) ret->setCondition(condition());
    return ret;
  }

  std::string toString() const override
  { return "default " + (body() ? body()->toString() : std::string("{}")); }

  std::string unparse() const override
  {
    return unparseLineComment()
        .append("default ")
        .append(unparseBlock(body()));
  }
};

///  Represents a switch-statement
class SkSwitchStatement : public SkStatement
{
  SK_NODE(SwitchStatement, Statement)
  SK_PROPERTY(condition, setCondition, Expression)
  SK_PROPERTY(body, setBody, Statement)
public:
  explicit SkSwitchStatement(SgNode *src = nullptr)
    : Base(src), condition_(nullptr), body_(nullptr) {}


  SkSwitchStatement(SkExpression *cond, SkStatement *body, SgNode *src = nullptr)
    : Base(src), condition_(cond), body_(body)
  {
    if (condition_) condition_->setParent(this);
    if (body_) body_->setParent(this);
  }

  Self *clone() const override
  { return new Self(condition_ ? condition_->clone() : nullptr, body_ ? body_->clone() : nullptr, source()); }

  std::string toString() const override
  {
    return std::string("switch (")
       .append(condition_ ? condition_->toString() : std::string(SK_UNKNOWN))
       .append(") ")
       .append(body_ ? body_->toString() : std::string("{}"));
  }

  std::string unparse() const override
  {
    return unparseLineComment()
       .append("switch (")
       .append(condition_ ? condition_->unparse() : std::string(SK_UNKNOWN))
       .append(") ")
       .append(unparseBlock(body_));
  }
};

///  Represent a for-statement
#define SK_KW_FOR   "for"
class SkForStatement : public SkStatement
{
  SK_NODE(ForStatement, Statement)
  SK_SCALAR_PROPERTY(keyword, setKeyword, std::string)
  SK_PROPERTY(variable, setVariable, Variable)
  SK_PROPERTY(start, setStart, Expression)
  SK_PROPERTY(stop, setStop, Expression)
  SK_PROPERTY(step, setStep, Expression)
  SK_PROPERTY(body, setBody, Statement)
public:
  explicit SkForStatement(SgNode *src = nullptr)
    : Base(src), keyword_(SK_KW_FOR), variable_(nullptr), start_(nullptr), stop_(nullptr), step_(nullptr), body_(nullptr) {}

  Self *clone() const override
  {
    Self *ret = new Self(source());
    ret->setKeyword(keyword());
    if (variable_) ret->setVariable(variable_->clone());
    if (start_) ret->setStart(start_->clone());
    if (stop_) ret->setStop(stop_->clone());
    if (step_) ret->setStep(step_->clone());
    if (body_) ret->setBody(body_->clone());
    return ret;
  }

  std::string toString() const override
  {
    std::string ret = keyword_ + " ";
    if (variable_)
      ret.append(variable_->toString()).append(" = ");
    if (start_)
      ret.append(start_->toString()).append(":");
    if (stop_)
      ret.append(stop_->toString()).append(":");
    if (step_)
      ret.append(step_->toString());
    if (body_)
      ret.append("\n").append(body_->toString());
    return ret;
  }

  ///  Return if the nested loops are collapsable. Unimplemented and always return false.
  bool collapsable() const
  { return false; }

  std::string unparse() const override
  {
    bool col = collapsable();
    std::string ret = unparseFunctionCalls(start_) + unparseFunctionCalls(stop_) + unparseFunctionCalls(step_);
    ret.append(unparseLineComment());
    if (!(col && hasOuterLoop()))
      ret.append(keyword_).append(" ");
    if (variable_)
      ret.append(variable_->unparse()).append(" = ");
    if (start_)
      ret.append(start_->unparse());
    else
      ret.append(":").append(SK_UNKNOWN);
    if (stop_)
      ret.append(":").append(stop_->unparse());
    else
      ret.append(":").append(SK_UNKNOWN);
    if (step_) {
      std::string t = step_->unparse();
      if (t != "1")
        ret.append(":").append(t);
    }
    ret.append("\n")
       .append(unparseBlock(body_));
    //if (body_) {
    //  bool par = hasInnerLoop() && !col;
    //  if (hasInnerLoop() && col)
    //    ret.append(", ");
    //  else
    //    ret.push_back('\n');
    //  if (par)
    //    ret.append("{\n");
    //  ret.append(body_->unparse());
    //  if (par)
    //    ret.append("\n}");
    //} else
    //  ret.append("{\n}");
    return ret;
  }
private:
  ///  \internal
  bool hasInnerLoop() const
  {
    if (body_)
      switch (body_->classType()) {
      case Sk::C_ForStatement:
      case Sk::C_ForAllStatement:
        return true;
      case Sk::C_Block:
        {
          SkBlock *b = static_cast<SkBlock *>(body_);
          if (b->size() == 1)
            switch (b->statements().front()->classType()) {
            case Sk::C_ForStatement:
            case Sk::C_ForAllStatement:
              return true;
            default: ;
            }
        }
      default: ;
      }
    return false;
  }

  ///  \internal
  bool hasOuterLoop() const
  {
    if (parent())
      switch (parent()->classType()) {
      case Sk::C_ForStatement:
      case Sk::C_ForAllStatement:
        return true;
      case Sk::C_Block:
        {
          SkBlock *b = static_cast<SkBlock *>(parent());
          if (b->size() == 1 && b->parent())
           switch (b->parent()->classType()) {
           case Sk::C_ForStatement:
           case Sk::C_ForAllStatement:
             return true;
           default: ;
           }
        }
      default: ;
      }
    return false;
  }
};

///  forall statement
#define SK_KW_FORALL    "forall"
class SkForAllStatement : public SkForStatement
{
  SK_NODE(ForAllStatement, ForStatement)
public:
  explicit SkForAllStatement(SgNode *src = nullptr)
    : Base(src) { setKeyword(SK_KW_FORALL); }

  Self *clone() const override
  {
    Self *ret = new Self(source());
    ret->setKeyword(keyword());
    if (variable()) ret->setVariable(variable()->clone());
    if (start()) ret->setStart(start()->clone());
    if (stop()) ret->setStop(stop()->clone());
    if (step()) ret->setStep(step()->clone());
    if (body()) ret->setBody(body()->clone());
    return ret;
  }
};

///  A break statement in a loop
class SkBreakStatement : public SkStatement
{
  SK_NODE(BreakStatement, Statement)
public:
  explicit SkBreakStatement(SgNode *src = nullptr) : Base(src) {}
  Self *clone() const override { return new Self(source()); }

  std::string toString() const override { return "break"; }
};

///  A continue statement in a loop
class SkContinueStatement : public SkStatement
{
  SK_NODE(ContinueStatement, Statement)
public:
  explicit SkContinueStatement(SgNode *src = nullptr) : Base(src) {}
  Self *clone() const override { return new Self(source()); }

  std::string toString() const override { return "continue"; }
};

///  A return statement in a function
class SkReturnStatement : public SkStatement
{
  SK_NODE(ReturnStatement, Statement)
  SK_PROPERTY(value, setValue, Expression)
public:
  explicit SkReturnStatement(SgNode *src = nullptr)
    : Base(src), value_(nullptr) {}

  explicit SkReturnStatement(SkExpression *value, SgNode *src = nullptr)
    : Base(src), value_(value)
  { if (value_) value_->setParent(this); }

  Self *clone() const override
  { return new Self(value_ ? value_->clone() : nullptr, source()); }

  std::string toString() const override
  { return value_ ? "return " + value_->toString() : std::string("return"); }

  std::string unparse() const override
  { return "return"; } // ignore return value which is not supported by the skeletonizer
  //{ return value_ ? "return " + value_->unparse() : std::string("return"); }
};

// - Declaration -

///  Abstract declaration statement.
class SkDeclaration : public SkStatement
{
  SK_NODE(Declaration, Statement)
  SK_ABSTRACT
protected:
  explicit SkDeclaration(SgNode *src = nullptr) : Base(src) {}
};

///  Import from other files.
class SkImportStatement : public SkDeclaration
{
  SK_NODE(ImportStatement, Declaration)
  SK_SCALAR_PROPERTY(name, setName, std::string)
public:
  explicit SkImportStatement(SgNode *src = nullptr) : Base(src) {}
  explicit SkImportStatement(const std::string &name, SgNode *src = nullptr)
    : Base(src), name_(name) {}

  Self *clone() const override { return new Self(name_, source()); }

  std::string toString() const override
  { return "from " + name_ + " import *"; }
};

///  Variable declaration statement.
class SkSymbolDeclaration : public SkDeclaration
{
  SK_NODE(SymbolDeclaration, Declaration)
  SK_PROPERTY(symbol, setSymbol, Symbol)
public:
  explicit SkSymbolDeclaration(SgNode *src = nullptr)
    : Base(src), symbol_(nullptr) {}
  explicit SkSymbolDeclaration(SkSymbol *var, SgNode *src = nullptr)
    : Base(src), symbol_(var)
  { if (symbol_) symbol_->setParent(this); }

  Self *clone() const override
  { return new Self(symbol_ ? symbol_->clone() : nullptr, source()); }

  std::string toString() const override
  {
    if (!symbol_)
      return SK_UNKNOWN;
    std::string ret = symbol_->type() ? symbol_->type()->toString() : std::string(SK_UNKNOWN);
    ret.append(" ")
       .append(symbol_->name());
    if (symbol_->value())
      ret.append(" = ")
         .append(symbol_->value()->toString());
    return ret;
  }

  std::string unparse() const override
  {
    if (!symbol_)
      return std::string();
    if (symbol_->type() && Sk::C_ArrayType == symbol_->type()->classType())
      return unparseArrayDeclaration();
    //if (!symbol_->value() && !sknode_cast<const SkFunctionDeclaration *>(parent()))
    //  return std::string();

    std::string ret;
    if (!inFunction())
      ret = ":";

    bool unparseType = symbol_->type() && !symbol_->value();
    if (unparseType)
      ret.append(symbol_->type()->unparse())
         .push_back(' ');
    ret.append(symbol_->name());
    if (symbol_->value())
      ret.append(" = ")
         .append(symbol_->value()->unparse());
    return ret;
  }

private:
  ///  \internal  Unparse declaration for array symbol.
  std::string unparseArrayDeclaration() const override
  {
    SkArrayType *t;
    if (!symbol_ ||
      !(t = sknode_cast<SkArrayType *>(symbol_->type())))
      return ":" SK_UNKNOWN;
    SK_ASSERT(t);
    std::string ret;
    bool unparseType = !inFunction();
    if (unparseType)
      ret = ":";
    if (unparseType)
      if (t->baseType())
        ret.append(t->baseType()->unparse())
           .push_back(' ');
    ret.append(symbol_->name());
    if (unparseType) {
      BOOST_FOREACH (const SkExpression *it, t->dimensions()) {
        ret.push_back('[');
        std::string dim = it ? it->unparse() : std::string();
        if (dim.empty() || dim == SK_UNKNOWN)
          dim = SK_UNKNOWN_DIMENSION;
        ret.append(dim);
        ret.push_back(']');
      }
    }
    // Assignment to arrays are ignored
    //if (symbol_->value()) {
    //  if (inFunction())
    //    ret.append(" = ")
    //       .append(symbol_->value()->unparse());
    //  else {
    //    ret.push_back('\n');
    //    ret.append(symbol_->name());
    //    //BOOST_FOREACH (const SkExpression *val, t->dimensions())
    //    for (size_t i = 0; i < t->dimensions().size(); i++)
    //      ret.append("[:]");
    //    ret.append(" = ")
    //       .append(symbol_->value()->unparse());
    //  }
    //}
    return ret;
  }

  bool inFunction() const
  { return !sknode_cast<SkBlock *>(parent()); }
};

/**
 *  \brief  Function declaration.
 *
 *  In the skeleton language, the function does NOT have a return type.
 */
#define SK_KW_DEF   "def"
class SkFunctionDeclaration : public SkDeclaration
{
  SK_NODE(FunctionDeclaration, Declaration)
  SK_SCALAR_PROPERTY(keyword, setKeyword, std::string)
  SK_LIST_PROPERTY(parameters, setParameters, SymbolDeclaration)
  SK_SCALAR_PROPERTY(name, setName, std::string)
  SK_SCALAR_PROPERTY(isMainEntry, setMainEntry, bool)
public:
  explicit SkFunctionDeclaration(SgNode *src = nullptr)
    : Base(src), keyword_(SK_KW_DEF), isMainEntry_(false) {}

  Self *clone() const override
  {
    Self *ret = new Self(source());
    ret->setKeyword(keyword());
    ret->setName(name_);
    ret->setMainEntry(isMainEntry_);
    BOOST_FOREACH (SkSymbolDeclaration *it, parameters_)
      ret->appendParameter(it ? it->clone() : nullptr);
    return ret;
  }

  //  Function return type.
  //SkType *returnType() const { return return_; }
  //void setReturnType(SkType *val)
  //{ return_ = val; if (val) val->setParent(this); }

  int parameterCount() const { return parameters_.size(); }

  ///  This class will take the ownership.
  void appendParameter(SkSymbolDeclaration *decl)
  {
    parameters_.push_back(decl);
    if (decl)
      decl->setParent(this);
  }

  void prependParameter(SkSymbolDeclaration *decl)
  {
    parameters_.push_front(decl);
    if (decl)
      decl->setParent(this);
  }

  std::string toString() const override
  {
    //if (return_)
    //  ret.append(return_->unparse())
    //     .push_back(' ');
    std::string ret = isMainEntry_ ? std::string("main") :
                      name_.empty() ? std::string(SK_UNKNOWN) :
                      name_;
    ret.push_back('(');
    bool comma = false;
    BOOST_FOREACH (const SkSymbolDeclaration *it, parameters_) {
      if (comma)
        ret.append(", ");
      else
        comma = true;
      ret.append(it ? it->unparse() : std::string(SK_UNKNOWN));
    }
    ret.push_back(')');
    return ret;
  }

  std::string unparse() const override
  {
    std::string ret = keyword_;
    ret.append(" ")
       .append(isMainEntry_ ? std::string("main") :
               name_.empty() ? std::string(SK_UNKNOWN) :
               name_);
    ret.push_back('(');
    bool comma = false;
    BOOST_FOREACH (const SkSymbolDeclaration *it, parameters_) {
      if (comma)
        ret.append(", ");
      else
        comma = true;
      ret.append(it ? it->unparse() : std::string(SK_UNKNOWN));
    }
    ret.push_back(')');
    return ret;
  }
};

/**
 *  \brief  Function declaration.
 *
 *  JG 10/12/2012:
 *  The need for both function definition and declaration in Skeleton:
 *  logically, a function definition also implicit a function declaration.
 *  But SkFunctionDeclaration is never used currently.
 */
class SkFunctionDefinition : public SkFunctionDeclaration
{
  SK_NODE(FunctionDefinition, FunctionDeclaration)
  SK_PROPERTY(body, setBody, Statement)
public:
  explicit SkFunctionDefinition(SgNode *src = nullptr)
    : Base(src), body_(nullptr) {}

  Self *clone() const override
  {
    Self *ret = new Self(source());
    ret->setName(name());
    ret->setMainEntry(isMainEntry());
    BOOST_FOREACH (SkSymbolDeclaration *it, parameters())
      ret->appendParameter(it ? it->clone() : nullptr);
    if (body_) ret->setBody(body_->clone());
    return ret;
  }

  std::string toString() const override
  {
    return Base::toString()
       .append("\n")
       .append(body_ ? body_->toString() : "\n{}");
  }

  std::string unparse() const override
  {
    return Base::unparse()
       .append("\n")
       .append(unparseBlock(body_));
  }

protected:
  //bool needsParentheses() const
  //{
  //  return !(
  //    body_ && body_ ->classType() == Sk::C_Block &&
  //    static_cast<SkBlock *>(body_)->needsParentheses()
  //  );
  //}
};

///  Global properties, such as variable declarations
class SkGlobal : public SkBlock
{
  SK_NODE(Global, Block)
public:
  explicit SkGlobal(SgNode *src = nullptr)
    : Base(src) {}

  Self *clone() const override
  {
    Self *ret = new Self(source());
    BOOST_FOREACH (SkStatement *it, statements())
      if (it)
        ret->append(it->clone());
    return ret;
  }

  std::string unparse() const override
  { return unparseBody(); }
};

SK_END_NAMESPACE

#endif // SKNODE_H
