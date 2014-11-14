// dataflowtable.cc
// 10/16/2012 jichi

#include "rosex/dataflowtable.h"
#include "rosex/rosex.h"
#include "rosex/symbolic.h"
#include <boost/lexical_cast.hpp>
#include <boost/unordered_map.hpp>

//#define DEBUG "dataflowtable"
#include "xt/xdebug.h"
#include <iostream>

#ifdef __clang__
# pragma clang diagnostic ignored "-Wparentheses" // assignment as a condition
#elif defined(__GNUC__)
# pragma GCC diagnostic ignored "-Wparentheses" // assignment as a condition
#endif // __clang__

#define foreach BOOST_FOREACH // TODO: move to implementation file

#define DFT_TEMP_PREFIX   "$" // prefix of temp variable names
#define DFT_ALIAS_PREFIX  "$" // prefix of renamed variable names

// - Private data -

class DataFlowTablePrivate
{
  typedef DataFlowTable Q;
  typedef Q::Reference Reference;
  typedef Q::ReferenceList ReferenceList;
  typedef Q::Entry Entry;
  typedef Q::EntryList EntryList;

  EntryList ssa_;

  ReferenceList loads_, stores_;

  // Map of renamed variables
  typedef boost::unordered_map<std::string, std::string> alias_map;
  alias_map aliases_;

public:
  const EntryList &ssa() const { return ssa_; }

  void clear()
  {
    ssa_.clear();
    loads_.clear();
    stores_.clear();
    aliases_.clear();
  }

  // Apply the symbol renaming
  void updateEntry(Entry &e)
  {
    if (!aliases_.empty() && !e.operands.empty())
      BOOST_FOREACH (const alias_map::value_type &pair, aliases_)
        BOOST_FOREACH (Reference &ref, e.operands)
          SymbolicValue::replace(ref.symbol, pair.first, pair.second);
  }

  // Update the reference of lvalue expression
  void updateLRef(Reference &ref)
  {
    // Alias renaming
    int count = 0;
    {
      std::string orig_symbol;
      foreach (const Reference &r, stores_) {
        const std::string &old_symbol = r.symbol;
        if (old_symbol == ref.symbol) {
          if (orig_symbol.empty())
            orig_symbol = old_symbol;
          ref.symbol = createSymbolAlias(orig_symbol, ++count);
          if (!aliases_.count(old_symbol)) {
            aliases_[old_symbol] = ref.symbol;
            //stores_.push_back(ref);
            break;
          }
        }
      }

      foreach (const Entry &e, ssa_) {
        const std::string &old_symbol = e.result.symbol;
        if (old_symbol == ref.symbol) {
          if (orig_symbol.empty())
            orig_symbol = old_symbol;
          ref.symbol = createSymbolAlias(orig_symbol, ++count);
          if (!aliases_.count(old_symbol)) {
            aliases_[old_symbol] = ref.symbol;
            break;
          }
        }
      }
    }

    // Store
    if (!count && SymbolicValue::isArrayRef(ref.symbol)) { // This is the first assignment to this array
      stores_.push_back(ref);
      std::string new_symbol = createSymbolAlias(ref.symbol, ++count);
      aliases_[ref.symbol] = new_symbol;
      ref.symbol = new_symbol;
    }
  }

  // Update the array reference of rvalue expression
  void updateLoadRef(Reference &ref)
  {
    if (!aliases_.count(ref.symbol)) {
      loads_.push_back(ref);
      std::string new_symbol = createSymbolAlias(ref.symbol, 1);
      aliases_[ref.symbol] = new_symbol;
    }
  }

  static std::string createSymbolAlias(const std::string &symbol, int id)
  { return symbol + DFT_ALIAS_PREFIX + boost::lexical_cast<std::string>(id); }

  void fromBlock(SgNode *input)
  {
    XD("enter: sage class = " << input->class_name());
    if (SgBasicBlock *g = ::isSgBasicBlock(input))
      foreach (SgStatement *stmt, g->get_statements())
        fromStatement(stmt);
    XD("leave");
  }

  void fromStatement(SgNode *input)
  {
    if (input)
      switch (input->variantT()) {
      case V_SgVariableDeclaration:
        foreach (SgInitializedName *var, ::isSgVariableDeclaration(input)->get_variables())
          fromAssignment(var, var->get_initializer());
        break;

      default:
        {
          SgExpression *lhs, *rhs;
          if (SageInterface::isAssignmentStatement(input, &lhs, &rhs))
            fromAssignment(lhs, rhs, input);
        }
      }
  }

  void fromAssignment(SgNode *lhs, SgNode *rhs, SgNode *src = nullptr)
  {
    if (!lhs || !rhs)
      return;
    XD("enter");
    Reference value = fromExpression(rhs); // Parsing rhs must go before parsing lhs
    const size_t valueEnd = ssa_.size();

    //Reference result = Reference::createSymbol(lhs);
    Reference result = fromExpression(lhs);
    updateLRef(result);
    if (value.isTemporary()) {
      ROSE_ASSERT(valueEnd > 0 && valueEnd <= ssa_.size());
      EntryList::iterator p = ssa_.begin();
      std::advance(p, valueEnd -1);
      p->result = result;
    } else {
      Entry e(src);
      e.result = result;
      e.operands.push_back(value);
      updateEntry(e);

      ROSE_ASSERT(valueEnd <= ssa_.size());
      EntryList::iterator p = ssa_.begin();
      std::advance(p, valueEnd);
      ssa_.insert(p, e);
    }
    XD("leave");
  }

  Reference fromExpression(SgNode *input)
  {
    if (!input)
      return Reference();

    XD("enter: sage class = " << input->class_name());
    switch (input->variantT()) {
    case V_SgInitializedName:
    case V_SgVarRefExp:
      XD("leave: variable");
      return Reference::createSymbol(input);

    case V_SgInitializer:
      XD("leave: initialier");
      return fromExpression(::isSgInitializer(input)->get_originalExpressionTree());
    case V_SgAssignInitializer:
      XD("leave: assign initializer");
      return fromExpression(::isSgAssignInitializer(input)->get_operand());

    case V_SgPntrArrRefExp:
      {
        SgPntrArrRefExp *arr = ::isSgPntrArrRefExp(input);
        std::string symbol;
        SgExpression *lhs = arr->get_lhs_operand();

        if (SgPntrArrRefExp *base = ::isSgPntrArrRefExp(lhs)) {
          ROSE_ASSERT(!SageInterface::is_Fortran_language());
          do  {
            std::string index = "[" + fromExpression(base->get_rhs_operand()).symbol + "]";
            symbol = index + symbol;
            lhs = base->get_lhs_operand();
          } while (base = ::isSgPntrArrRefExp(lhs));
        }

        symbol = fromExpression(lhs).symbol + symbol;

        SgExpression *rhs = arr->get_rhs_operand();
        if (SgExprListExp *list = ::isSgExprListExp(rhs)) {
          ROSE_ASSERT(SageInterface::is_Fortran_language());
          std::string indices;
          foreach (SgExpression *e, list->get_expressions()) {
            std::string index = "[" + fromExpression(e).symbol + "]";
            if (SageInterface::is_Fortran_language())
              indices = index + indices;
            else
              indices.append(index);
          }
          symbol.append(indices);
        } else
          symbol.append("[")
                .append(fromExpression(rhs).symbol)
                .push_back(']');

        Reference ret(symbol, input);
        updateLoadRef(ret);
        XD("leave: pointer");
        return ret;
      } ROSE_ASSERT(0);

    case V_SgSubscriptExpression:
      {
        SgSubscriptExpression *g = ::isSgSubscriptExpression(input);
        std::string symbol;
        if (!::isSgNullExpression(g->get_lowerBound()))
          symbol.append(fromExpression(g->get_lowerBound()).symbol);
        if (!::isSgNullExpression(g->get_upperBound()))
          symbol.append(":").append(fromExpression(g->get_upperBound()).symbol);
        if (!::isSgNullExpression(g->get_stride()) &&
            g->get_stride()->unparseToString() != "1")
          symbol.append(":").append(fromExpression(g->get_stride()).symbol);
        Reference ret(symbol, input);
        XD("leave: array subscript");
        return ret;
      } ROSE_ASSERT(0);

    case V_SgNullExpression:
      XD("leave: null exp");
      break;

      // FIXME: type cast expression is ignored
    case V_SgCastExp:
      XD("leave: cast exp");
      return fromExpression(::isSgCastExp(input)->get_operand());

    default:
      if (::isSgValueExp(input)) {
        XD("leave: value exp");
        return Reference::createSymbol(input);
      }
      if (SgUnaryOp *g = ::isSgUnaryOp(input)) {
        Entry e(input);
        e.result = Reference::createTemp(input);
        e.operands.push_back(fromExpression(g->get_operand()));
        updateEntry(e);
        ssa_.push_back(e);
        XD("leave: unary op");
        return e.result;
      }
      if (SgBinaryOp *g = ::isSgBinaryOp(input)) {
        Entry e(input);
        e.result = Reference::createTemp(input);
        e.operands.push_back(fromExpression(g->get_lhs_operand()));
        e.operands.push_back(fromExpression(g->get_rhs_operand()));
        updateEntry(e);
        ssa_.push_back(e);

        XD("leave: binary op");
        return e.result;
      }
      std::cerr << "dataflowtable::fromExpression: warning: unhandled sage class: " << input->class_name() << std::endl;
    }
    XD("leave: unhandled");
    return Reference(input);
  }

  // - Unparse -
  std::string unparseSSA() const
  {
    std::string ret;
    foreach (const Entry &e, ssa_)
      ret.append(e.toString())
         .push_back('\n');
    return ret;
  }

  std::string unparseLoads() const
  {
    std::string ret;
    foreach (const Reference &r, loads_) {
      std::string value = SymbolicValue::unquoted(r.symbol);
      std::string entry = value + "$1 = " + value;
      ret.append(entry)
         .push_back('\n');
    }
    return ret;
  }

  std::string findSymbolAlias(const std::string &symbol) const
  {
    std::string ret =  symbol;
    alias_map::const_iterator p = aliases_.find(ret);
    while (p != aliases_.end()) {
      ROSE_ASSERT(ret != p->second);
      ret = p->second;
      p = aliases_.find(ret);
    }
    return ret;
  }

  std::string unparseStores() const
  {
    std::string ret;
    foreach (const Reference &r, stores_) {
      std::string alias = findSymbolAlias(r.symbol);
      SymbolicValue::unquote(alias);
      std::string entry = SymbolicValue::unquoted(r.symbol)  + " = " + alias;
      ret.append(entry)
         .push_back('\n');
    }
    return ret;
  }
};

// - Table entry -

bool DataFlowTable::Reference::isTemporary() const
{ return boost::starts_with(symbol, DFT_TEMP_PREFIX); }

DataFlowTable::Reference DataFlowTable::Reference::createTemp(SgNode *src)
{
  static int id = 0;
  return Reference(DFT_TEMP_PREFIX + boost::lexical_cast<std::string>(++id), src);
}

DataFlowTable::Reference DataFlowTable::Reference::createSymbol(SgNode *src)
{ return Reference(SymbolicValue::fromSource(src), src); }

std::string DataFlowTable::Reference::toString() const
{ return symbol.empty() ? std::string() : SymbolicValue::unquoted(symbol); }

std::string DataFlowTable::Entry::toString() const
{
  std::string ret;
  if (SgExpression *e = ::isSgExpression(result.source))
    ret.append(e->get_type()->unparseToString())
       .push_back(' ');
  ret.append(result.toString())
     .append(" = ");

  std::string op;
  if (source)
    op = rosex::unparseOperatorVariantT(source->variantT());

  if (op.empty()) {
    bool first = true;
    foreach (const Reference &ref, operands) {
      if (first)
        first = false;
      else
        ret.append(", ");
      ret.append(ref.toString());
    }
  } else
    switch (operands.size()) {
    case 0:
      std::cerr << "dataflowtable::entry::toString: warning: missing operands" << std::endl;
      ret.append(op);
      break;
    case 1:
      ret.append(op + " ")
         .append(operands.front().toString());
      break;
    case 2:
      ret.append(operands.front().toString())
         .append(" " + op + " ")
         .append(operands.back().toString());
      break;
    default:
      ret.append(op).push_back('(');
      {
        bool first = true;
        foreach (const Reference &ref, operands) {
          if (first)
            first = false;
          else
            ret.append(", ");
          ret.append(ref.toString());
        }
      }
      ret.push_back(')');
    }
  return ret;
}

// - Constructions -

DataFlowTable::DataFlowTable(SgNode *source)
  : d_(new D)
{ initWithBlock(source); }

DataFlowTable::~DataFlowTable()
{ delete d_; }

void DataFlowTable::clear()
{ d_->clear(); }

bool DataFlowTable::isEmpty() const
{ return d_->ssa().empty(); }

void DataFlowTable::initWithBlock(SgNode *source)
{
  clear();
  d_->fromBlock(source);
}

void DataFlowTable::addStatement(SgNode *stmt)
{ d_->fromStatement(stmt); }

std::string DataFlowTable::toString() const
{
  //return d_->unparseLoads() + "\n" +
  //       d_->unparseSSA() + "\n" +
  //       d_->unparseStores() + "\n";
  return d_->unparseSSA() + "\n";
}

// EOF

/*
SgNode *get_array_ref(SgNode *ref, SgNode *parent = nullptr)
{
  if (!ref)
    return nullptr;

  SgExpression *e = ::isSgExpression(ref);
  if (!e || !rosex::isArrayType(e->get_type()))
    return nullptr;

  SgNode *p = ref->get_parent();
  if (SageInterface::is_Fortran_language())
    return p;
  e = ::isSgExpression(p);
  if (!e || !rosex::isArrayType(e->get_type()))
    return ref;

  if (p == parent)
    return nullptr;

  return get_array_ref(p, parent);
}

bool ref_in_array(SgNode *ref, SgNode *parent = nullptr)
{
  if (!ref)
    return false;
  if (::isSgPntrArrRefExp(ref))
    return true;
  if (ref == parent)
    return false;
  while (ref = ref->get_parent()) {
    if (::isSgPntrArrRefExp(ref))
      return true;
    if (ref == parent)
      break;
  }
  return false;
}
*/
