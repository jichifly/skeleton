#ifndef ROSEX_P_H
#define ROSEX_P_H

// rose_p.h
// 9/25/2012 jichi

#include "rosex/rosex.h"
#include <fstream>

namespace rosex { namespace detail {

  ///  \internal Unparse the AST statement node to assembly code
  std::string unparseStatementToAssembly(const SgNode *stmt);

  ///  \internal Extract the assembly body code
  std::string extractAssemblyBody(std::ifstream &fin);

} } // namespace rosex

#endif // ROSEX_P_H
