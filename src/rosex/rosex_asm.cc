// rose_asm.cc
// 9/25/2012 jichi

#include "rosex/rosex.h"
#include "rosex/rosex_p.h"
#include <boost/foreach.hpp>
#include <boost/unordered/unordered_set.hpp>
#include <fstream>
#include <cstdio>   // for tmpnam()
#include <cstdlib>  // for system()

#define foreach BOOST_FOREACH

#define DEBUG "rosex_asm"
#include "xt/xdebug.h"

// - Outline -

#define BEGIN_C       "int main() {\n"
#define END_C         "\nreturn 0;\n}"
#define BEGIN_FORTRAN   "\nprogram main\n"
#define END_FORTRAN   "\nend"

std::string rosex::outlineStatement(const SgNode *input)
{
  std::string ret;
  if (!input)
    return ret;

  bool fortran = SageInterface::is_Fortran_language();
  ret = fortran ? BEGIN_FORTRAN : BEGIN_C;

  //SageInterface::copyStatement() // TODO: copy ast

  boost::unordered_set<SgSymbol *>symbols;
  foreach (SgNode *var, NodeQuery::querySubTree(const_cast<SgNode *>(input), V_SgVarRefExp))
      symbols.insert(::isSgVarRefExp(var)->get_symbol());
  foreach (SgSymbol *sym, symbols) {
    SgNode *decl = SageBuilder::buildVariableDeclaration(sym->get_name(), sym->get_type());
    decl->set_parent(sym->get_parent());
    ret.append(decl->unparseToString())
       .push_back('\n');
    delete decl;
  }

  ret.append(const_cast<SgNode *>(input)->unparseToCompleteString());
  ret.append(fortran ? END_FORTRAN : END_C);
  return ret;
}

// - ASM -

std::string rosex::unparseToAssembly(const SgNode *input)
{
  std::string ret;
  if (!input)
    return ret;
  const SgNode *stmt = ::isSgStatement(input);
  if (!stmt)
    stmt = getParentStatement(input);
  if (stmt)
    ret = detail::unparseStatementToAssembly(stmt);
  return ret;
}

std::string rosex::detail::unparseStatementToAssembly(const SgNode *input)
{
  std::string ret;
  std::string src = outlineStatement(input);
  if (src.empty())
    return ret;

  bool fortran = SageInterface::is_Fortran_language();

  std::string base = ::tmpnam(nullptr);
  std::string asm_file = base + ".s";
  std::string src_file = base + (fortran ? ".f90" : ".cpp");
  {
    std::ofstream fout(src_file.c_str());
    fout << src;
  }

  const char *cxx_compiler = "g++",
                       *fortran_compiler = "gfortran";

  std::string cmd = fortran ? fortran_compiler : cxx_compiler;
  cmd.append(" -S \"")
     .append(src_file)
     .append("\" -o \"")
     .append(asm_file)
     .push_back('"');

  int err = ::system(cmd.c_str());
  if (!err) {
    std::ifstream fin(asm_file.c_str());
    //ret = std::string((std::istreambuf_iterator<char>(fin)),
    //                                     std::istreambuf_iterator<char>());
    if (fin.good())
      ret = detail::extractAssemblyBody(fin);
  }
  ::remove(src_file.c_str());
  ::remove(asm_file.c_str());
  return ret;
}

std::string rosex::detail::extractAssemblyBody(std::ifstream &fin)
{
  std::string ret;
  if (fin.bad())
    return ret;

  bool yes = false;
  std::string line;
  while (std::getline(fin, line)) {
    if (!yes && line == "Ltmp1:")
      yes = true;
    if (yes) {
      if (line == "\tmovl\t$0, -8(%rbp)" || line == "Leh_func_end1:")
        yes = false;
      else if (yes)
        ret.append(line)
           .push_back('\n');
    }
  }
  return ret;
}

// EOF
