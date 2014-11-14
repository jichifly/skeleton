#ifndef _ROSEX_SYMBOLIC_H
#define _ROSEX_SYMBOLIC_H

// symbolic.h
// 10/22/2012 jichi
// Symbolic value.
//
// Example:
// - source: a * 2 + b[i][j]
// - symbolic value: ('a' * '2') + 'c'['i']['j']

#include <string>
#include <boost/algorithm/string.hpp>

#define SYMBOLIC_QUOTE  "'"
#define SYMBOLIC_QUOTE_CH  '\''

class SgNode;

///  Simple symbolic value of the ROSE expressions, represented by string
namespace SymbolicValue {

  // - Construct -

  ///  Construct a symbolic string from the rose source AST
  std::string fromSource(const SgNode *source);

  ///  Quote the symbol
  inline void quote(std::string &var)
  { var = SYMBOLIC_QUOTE + var + SYMBOLIC_QUOTE; }

  ///  Return a quoted variable
  inline std::string quoted(const std::string &var)
  { return SYMBOLIC_QUOTE + var + SYMBOLIC_QUOTE; }

  ///  Remove quotes
  inline void unquote(std::string &symbol)
  { boost::erase_all(symbol, SYMBOLIC_QUOTE); }

  ///  Return the value without quotes
  inline std::string unquoted(const std::string &symbol)
  {
    std::string ret = symbol;
    boost::erase_all(ret, SYMBOLIC_QUOTE);
    return ret;
  }

  // - Query -

  //inline bool isRef(const std::string &symbol)
  //{
  //  return symbol.size() > 2 &&
  //      symbol[0] == SYMBOLIC_QUOTE_CH && symbol[symbol.size()-1] == SYMBOLIC_QUOTE_CH &&
  //      symbol.find(SYMBOLIC_QUOTE_CH, 1) == symbol.size()-1;
  //}

  ///  Return if the symbol is an array access
  inline bool isArrayRef(const std::string &symbol)
  { return symbol.find('[') != std::string::npos; }

  // - Update -

  ///  Replace old symbol with the new one in the target
  inline void replace(std::string &target, const std::string &oldSymbol, const std::string &newSymbol)
  { boost::replace_all(target, oldSymbol, newSymbol); }

} // namespace SymbolicValue

#endif // _ROSEX_SYMBOLIC
