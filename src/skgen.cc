// main.cc
// 9/11/2012 jichi
//
// Parse the input source files, and print the generated skeleton.

#include "sk/sknode.h"
//#include "sk/skprofile.h"
#include "sk/skbuilder.h"
#include "sk/skquery.h"
#include "rosex/dataflowtable.h"
#include "rosex/tac.h"
#include "rosex/depgraph.h"
#include "rosex/defusegraph.h"
#include "rosex/rosex.h"

//#include "opt/dep.h"
//#include "opt/inline.h"
//#include "opt/mpiinline.h"
//#include "opt/simplify.h"

#include "xt/xt.h"

#include <ExtractFunctionArguments.h>

#include <boost/algorithm/string/predicate.hpp> // for boost::ends_with
#include <boost/filesystem.hpp>
#include <boost/date_time.hpp>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iostream>
#include <inttypes.h>

#define DEBUG "main"
#include "xt/xdebug.h"

#define foreach BOOST_FOREACH

//#define ENABLE_ASSEMBLY
#define ENABLE_DEPGRAPH
#define ENABLE_DEFUSEGRAPH
#define ENABLE_DATAFLOW
#define ENABLE_TAC
//#define ENABLE_SIDEEFFECT

#define DEFUSE_DELIM "__"

#define GLOBAL_SKELETON "_global"

namespace bf = boost::filesystem;

namespace { // anonymous

  // See: http://stackoverflow.com/questions/865668/parse-command-line-arguments
  inline bool args_has_opt(char **begin, char **end, const std::string &option)
  { return std::find(begin, end, option) != end; }

//  inline std::string current_timestamp()
//  {
//#define STR(v)  boost::lexical_cast<std::string>(v)
//    namespace bp = boost::posix_time;
//    bp::ptime now = bp::microsec_clock::local_time();
//    //bp::date_type d = now.date();
//    bp::time_duration t = now.time_of_day();
//    return STR(t.hours()) + ":" + STR(t.seconds()) + ":" + STR(t.minutes());
//#undef STR
//  }

  /***
   *  \brief  Return if the input reference is the one we would like to keep
   *  \param  input  AST
   *  \param  br  consider branches
   *  \param  loop  consider  loops
   *
   *  Requirements:
   *  - Dataflow
   *    - One of its ancestor is an array reference
   *    - One of its ancestor is a function call
   *  - Control flow
   *    - Top-level expression is part of loops or branches
   */
  bool IsCriticalReference(const SgNode *input, bool br, bool loop)
  {
    if (!input)
      return false;
    switch (input->variantT()) {
    case V_SgIfStmt: case V_SgSwitchStatement:
    case V_SgCaseOptionStmt: case V_SgDefaultOptionStmt:
      if (br)
        return true;
      else
        break;
    case V_SgForStatement: case V_SgFortranDo:
    case V_SgWhileStmt: case V_SgDoWhileStmt:
      if (loop)
        return true;
      else
        break;
    default: ; // prevent branch warning
    }
    SgExpression *e = ::isSgExpression(const_cast<SgNode *>(input));
    if (!e)
      if (const SgExprStatement *s = ::isSgExprStatement(input))
        e = s->get_expression();

    if (!e)
      return false;
    if (SageInterface::isArrayReference(e) || isSgFunctionCallExp(e))
      return true;

    //if (e->unparseToString().find("idd") != std::string::npos)
    //  std::cerr << e->unparseToString() <<std::endl;

     // Check if one of its ancestor is an array reference or a function call
    while ((e = ::isSgExpression(e->get_parent())) &&
           !SageInterface::isArrayReference(e) &&
           !::isSgFunctionCallExp(e));
    if (e)
      return true;

    // Get top-most non-expression
    SgNode *p = SageInterface::getEnclosingStatement(const_cast<SgNode *>(input));
    if (!p)
      return false;
    SgNode *q = p->get_parent();
    SgExpression *pe = nullptr;
    if (p->variantT() == V_SgExprStatement)
      pe = ::isSgExprStatement(p)->get_expression();
    while (q)
      switch (q->variantT()) {
      case V_SgIfStmt:
        return p == ::isSgIfStmt(q)->get_conditional();
      case V_SgForStatement:
        return xt::contains(::isSgForStatement(q)->get_init_stmt(), p) ||
               p == ::isSgForStatement(q)->get_test() ||
               pe == ::isSgForStatement(q)->get_increment();

      case V_SgFortranDo:
        return xt::contains(::isSgForStatement(q)->get_init_stmt(), p) ||
               p == ::isSgForStatement(q)->get_test() ||
               pe == ::isSgForStatement(q)->get_increment();

      case V_SgWhileStmt:
        return p == ::isSgWhileStmt(q)->get_condition();
      case V_SgDoWhileStmt:
        return p == ::isSgDoWhileStmt(q)->get_condition();
      case V_SgSwitchStatement:
        return p == ::isSgSwitchStatement(q)->get_item_selector();

      case V_SgExprStatement:
        pe = ::isSgExprStatement(p)->get_expression();
        p = p->get_parent();
        break;
      default:
        return false;
      }

    return false;
  }

  void showHelp()
  {
    std::cout
      << "Usage: skeletonizer [OPTIONS] ... [FILES] ...\n"
      << "Skeletonizer - Code skeleton generator\n"
      << "\n"
      << " -sk:dataflow              Generate dataflow\n"
      << " -sk:tac                   Generate three address code\n"
      //<< " -sk:cov                   Generate coverage hints for branches\n"
      << " -sk:defuse              Ignore def-use analysis\n"
      << " -sk:nodevec               Do not apply devectorization\n"
      << " -sk:nodep                 Ignore data dependence\n"
      << " -sk:nodep:br              Ignore data dependence for branches\n"
      << " -sk:nodep:loop            Ignore data dependence for loops\n"
      << " -sk:nopurge               Do not remove skeleton-unsupported syntax\n"
      //<< " -sk:opt                   Optimize the source code, imply sk:unparse \n"
      //<< " -sk:sideeffect            Enable function side effect interface\n"
      << " -sk:unparse               Generate ROSE processed source code\n"
      << " -c                        Compile the source file\n"
      << " -h                        Show help\n"
      << std::endl;
  }

} // anonymous namespace

int main(int argc, char *argv[])
{
  bool opt_help = args_has_opt(argv, argv + argc, "--help") ||
                  args_has_opt(argv, argv + argc, "-h") ||
                  args_has_opt(argv, argv + argc, "-?");
  if (opt_help) {
    showHelp();
    return 0;
  }

  XD("enter: argc = " << argc);

  XD("parsing project");
  SgProject *project;
  try {
    // Initialize the ROSE frontend
    project = frontend(argc, argv);
  } catch (std::exception &e) {
    XD("exception: " << e.what());
    XD("exit: failed to parse project");
    return -1;
  }
  XD("parsing finished");

  if (project->get_fileList().empty()) {
    XD("exit: empty file list");
    return 0;
  }
  XD("number of files = " << project->get_fileList().size());

  XD("load configuration");

  bf::path appPath = bf::system_complete(argv[0]);
  bf::path confPath = bf::change_extension(appPath, ".conf");
  if (bf::exists(confPath))
    SkNode::setConfiguration(SkConf::fromFile(confPath.string()));

  // Normalize Fortran loops.
  // For the purpose of ROSE implementation, such as dependence analysis and loop boundary checking
  // Before loop normalization:
  //   for (int i = 10; i < 100; i+= 10)
  //     loop_body(i);
  // After loop normalization:
  //   for (int i_norm = 1; i_norm < 10; i_norm += 1)
  //     loop_body(i_norm * 10);
  XD("normalizing loops");
  if (SageInterface::is_Fortran_language())
    foreach (SgNode *it, NodeQuery::querySubTree(project, V_SgFortranDo))
      SageInterface::doLoopNormalization(::isSgFortranDo(it));
  //else
  //  foreach (SgNode *it, NodeQuery::querySubTree(project, V_SgForStatement))
  //    SageInterface::forLoopNormalization(::isSgForStatement(it));
  XD("normalization finished");

  bool opt_dataflow = args_has_opt(argv, argv + argc, "-sk:dataflow"),
       opt_tac = args_has_opt(argv, argv + argc, "-sk:tac"),
       opt_devec = !args_has_opt(argv, argv + argc, "-sk:nodevec"),
       opt_purge = !args_has_opt(argv, argv + argc, "-sk:purge"),
       opt_unparse = args_has_opt(argv, argv + argc, "-sk:unparse");
       //opt_optimize = args_has_opt(argv, argv + argc, "-sk:opt");
  //bool opt_cov = args_has_opt(argv, argv + argc, "-sk:cov");

#ifndef ENABLE_DATAFLOW
  opt_dataflow = false;
#endif // !ENABLE_DATAFLOW
#ifndef ENABLE_TAC
  opt_tac = false;
#endif // !ENABLE_TAC

  if (opt_dataflow)
    XD("-sk:dataflow  enable dataflow analysis");
  if (opt_tac)
    XD("-sk:tac  generate three address code");
  if (opt_devec)
    XD("!-sk:nodevec  convert vectorization to explicit loops");
  //if (opt_optimize) {
  //  XD("!-sk:optimize  apply optimization");
  //  opt_unparse = true;
  //}
  //if (opt_cov)
  //  XD("-sk:cov  generate coverage hints for branches");

  //if (opt_optimize) {
  //  XD("optimizing applications");
  //  if (SgNode *src = rosex::findMainFunction(project)) {
  //    XD("preprocessing functions");
  //    opt::extractFunctionArguments(src);
  //    opt::extractFunctionReturns(src);
  //    opt::inlineFunctions(src);
  //    opt::inlineMpiFunctions(src);
  //    opt::simplifyStatements(src);

  //    XD("applying dependence analysis");
  //    //foreach (SgNode *func, NodeQuery::querySubTree(project, V_SgFunctionDefinition))
  //    opt::dependenceAnalysis(src);
  //  }

  //  XD("unparsing applications");
  //  unparseProject(project);
  //  return 0;
  //}

  int globalVarCount = 0;

  //XD("preprocess source file");
  // Inline everything: https://github.com/rose-compiler/rose/blob/master/tutorial/inlineTransformations.C
  //foreach (SgNode *it, NodeQuery::querySubTree (project,V_SgFunctionCallExp)) {
  //  SgFunctionCallExp *call = isSgFunctionCallExp(it);
  //  bool ok = doInline(call);
  //}

  XD("create global skeleton");
  bool globalExists = bf::exists(GLOBAL_SKELETON ".sk");
  std::fstream gout(GLOBAL_SKELETON ".sk", std::fstream::out|std::fstream::app);
  if (!globalExists)
    gout
      << "// global\n"
      << ": char = " << sizeof(char) << "\n"
      << ": string = " << sizeof(char) << "\n"
      << ": bool = " << sizeof(bool) << "\n"
      << ": int = " << sizeof(int) << "\n"
      << ": float = " << sizeof(float) << "\n"
      << ": double = " << sizeof(double) << "\n"
      << ": complex = " << 2*sizeof(double) << "\n"
      ;

  XD("analyzing functions");
  foreach (SgFile *file, project->get_fileList()) {
    const std::string srcFile = file->getFileName();
    if (SageInterface::is_Fortran_language())
      if (boost::algorithm::ends_with(srcFile, "rmod") ||
          boost::algorithm::ends_with(srcFile, "RMOD")) {
        XD("skip module file: " << srcFile);
        continue;
      }

    XD("process file: " << srcFile);

    bf::path srcPath(srcFile);
    bf::path dfPath = bf::change_extension(srcPath, ".df");
    bf::path tacPath = bf::change_extension(srcPath, ".tac");
    std::string dfFile = dfPath.string();
    std::string tacFile = tacPath.string();

    bf::path skPath = bf::change_extension(srcPath, ".sk");
    std::string skFile = skPath.string();

    //std::ofstream skOut(skFile.c_str());

    std::ofstream dfOut;
    std::ofstream tacOut;
    std::ostringstream skOut;

    if (opt_dataflow)
      dfOut.open(dfFile.c_str());

    if (opt_tac)
      tacOut.open(tacFile.c_str());

    const char *header = "from " GLOBAL_SKELETON " import *\n";
    skOut << header << std::endl;
    if (SkNode *g = SkBuilder::fromGlobal(file))
       skOut << "// global\n"
             << SkBuilder::indent(g->unparse()) << std::endl;

    foreach (SgNode *n, NodeQuery::querySubTree(file, V_SgFunctionDefinition)) {
      SgFunctionDefinition *func = isSgFunctionDefinition(n);
      const std::string func_name = func->get_declaration()->get_name().getString();

      boost::unordered_map<SkSlice *, SkVariable *> slices;
      SkBuilder::Option opt;
      if (opt_devec)
        opt.danglingSlices = &slices;
      // Compute the set of critical references

      boost::unordered_map<const SgNode *, std::string> names; // AST node references we want to preserve
#ifdef ENABLE_DEFUSEGRAPH
      bool opt_defuse = args_has_opt(argv, argv + argc, "-sk:defuse");
      if (opt_defuse) {
        XD("!-sk:nodefuse  apply defuse analysis");
        DefUseGraph g(func);
        if (!g)
          std::cerr << "Failed to build define-use chain graph." << std::endl;
        else {
          //g.dump(); // debug
          //g.toDOT("test.dot");
          boost::unordered_map<std::string, int> counts; // count of names
          foreach (DefUseGraph::vertex_type v, g.rvertices()) // rvertices to invert the iteration order
            if (v.type() == DefUseGraphVertex::def)
              if (SgNode *var = v.reference())
                if (!rosex::isArrayReference(var) && g.has_out_edges(v))
                  if (SgVariableSymbol *symbol = rosex::getVariableSymbol(var)) {
                    std::string name = symbol->get_name().getString();
                    //BOOST_AUTO(p, counts.find(name));
                    //if (p == counts.end())
                    if (!counts.count(name))
                      counts[name] = 0;
                    else {
                      int count = ++counts[name];
                      name.append(DEFUSE_DELIM);
                      name.append(boost::lexical_cast<std::string>(count));
                      names[var] = name;
                      foreach (DefUseGraph::edge_type e, g.out_edges(v)) {
                        DefUseGraph::vertex_type use = g.target(e);
                        ROSE_ASSERT(use.type() == DefUseGraphVertex::use);
                        names[use.reference()] = name;
                      }
                    }
                  }
        }
      }
      XD("def use name size = " << names.size());
      if (!names.empty())
        opt.variableNames = &names;
#endif // ENABLE_DEFUSEGRAPH

      boost::unordered_set<const SgNode *> refs; // AST node references we want to preserve
#ifdef ENABLE_DEPGRAPH
      bool opt_dep = !args_has_opt(argv, argv + argc, "-sk:nodep");
      if (opt_dep) {
        XD("!-sk:nodep  apply dependence analysis");
        bool opt_dep_branch = !args_has_opt(argv, argv + argc, "-sk:nodep:br"),
             opt_dep_loop = !args_has_opt(argv, argv + argc, "-sk:nodep:loop");
        DepGraph g(func);

        if (!g.valid())
          std::cerr << "main:warning: failed to create dependence graph for function" << std::endl;
        else {
          XD("dep graph size = " << g.num_vertices());
          size_t refs_size;
          boost::unordered_set<const SgNode *> stmts; // critical statements
          do {
            refs_size = refs.size();
            foreach (DepGraph::edge_type e, g.edges()) {
              if (e.sourceExpression() && e.targetExpression() && (
                    refs.count(e.targetExpression()) ||
                    stmts.count(g.target(e).statement()) ||
                    IsCriticalReference(e.targetExpression(), opt_dep_branch, opt_dep_loop))) {
                refs.insert(e.sourceExpression());
                stmts.insert(g.source(e).statement());
              }
            }
          } while(refs_size != refs.size());
        }
      }

      XD("dep ref size = " << refs.size());
      //if (!refs.empty())
      opt.criticalReferences = &refs;
#endif // ENABLE_DEPGRAPH

//#ifdef ENABLE_SIDEEFFECT
#if 0
      bool opt_sideeffect = args_has_opt(argv, argv + argc, "-sk:sideffect");
      //opt_sideeffect = true;
      if (opt_sideeffect) {
        XD("-sk:sideeffect  apply function side effect analysis");
        DepGraphOption opt;
        FunctionSideEffectAnnotation funcInfo;
        opt.funcInfo = &funcInfo;
        DepGraph g(func, &opt);

        if (!g.valid())
          std::cerr << "main:warning: failed to create dependence graph for function" << std::endl;
        else {
          XD("dep graph size = " << g.num_vertices());
          g.dump();
          //size_t refs_size;
          //boost::unordered_set<const SgNode *> stmts; // critical statements
          //do {
          //  refs_size = refs.size();
          //  foreach (DepGraph::edge_type e, g.edges()) {
          //    if (e.sourceExpression() && e.targetExpression() && (
          //          refs.count(e.targetExpression()) ||
          //          stmts.count(g.target(e).statement()) ||
          //          IsCriticalReference(e.targetExpression(), opt_dep_branch, opt_dep_loop))) {
          //      refs.insert(e.sourceExpression());
          //      stmts.insert(g.source(e).statement());
          //    }
          //  }
          //} while(refs_size != refs.size());
        }
      }
      XD("dep ref size = " << refs.size());
#endif // ENABLE_SIDEEFFECT

      SkNode *k_func = SkBuilder::fromFunction(func, &opt);
      if (opt_devec && SageInterface::is_Fortran_language())
        k_func = SkBuilder::devectorize(k_func);
      if (!k_func)
        std::cerr << "failed to analyze the source code" << std::endl;
      else {
        XD("main: write skeleton to file: " << skFile);
        std::string sk = k_func->unparse();
        sk = SkBuilder::indent(sk);
        if (opt_purge)
          sk = SkBuilder::purge(sk);
        skOut << "// " << func_name << "()" << std::endl
              << sk << std::endl;

        if (opt_dataflow) {
          XD("main: generating dataflow summary");
          XD("main: write dataflow to file: " << dfFile);

          std::cout << "// dataflow:\n" << std::endl;
          dfOut << "// def " << func_name << "()\n" << std::endl;
          foreach (SkNode *node, SkQuery::find(k_func, Sk::C_BlockBarrier, Sk::PreOrder)) {
            SkBlockBarrier *barrier = static_cast<SkBlockBarrier *>(node);
            DataFlowTable dft;
            dft.initWithStatements(barrier->sourceStatements());
            std::cout << "// " << barrier->name() << std::endl
                      << dft.toString() << std::endl;

            dfOut << "// " << barrier->name() << "\n"
                   << dft.toString() << std::endl;
          }
        }

        if (opt_tac) {
          XD("main: generating three-address-code summary");
          XD("main: write tac to file: " << tacFile);
          std::cout << "// dataflow:\n" << std::endl;
          tacOut << "// def " << func_name << "()\n" << std::endl;
          foreach (SkNode *node, SkQuery::find(k_func, Sk::C_BlockBarrier, Sk::PreOrder)) {
            SkBlockBarrier *barrier = static_cast<SkBlockBarrier *>(node);
            TacTable tac;
            std::string beg = "[" + boost::replace_first_copy(barrier->name(), " ", ":") + "]";
            tac.initWithStatements(barrier->sourceStatements());
            std::cout << beg << "\n"
                      << tac.toString() << std::endl;

            tacOut << beg << "\n"
                   << tac.toString() << std::endl;
          }
        }
      }

#ifdef ENABLE_ASSEMBLY
      if (SkFunctionDefinition *f = sknode_cast<SkFunctionDefinition*>(k_func))
        if (SkNode *body = f->body())
          if (SgNode *stmt = body->source())
            std::cerr << "Assembly follows:" << std::endl
                      << rosex::unparseToAssembly(stmt) << std::endl;
#endif // ENABLE_ASSEMBLY
      delete k_func;
    }

    XD("output skeleton");
    std::string sk = skOut.str();
    {
      std::string srcBaseName = bf::basename(srcFile);
      //gout << "\n"
      //     << "// " << func_name << "()\n"
      //     << "\n";
      std::string unknown = SK_UNKNOWN;
      size_t pos = 0;
      while ((pos = sk.find(unknown.c_str(), pos)) != std::string::npos) {
        std::string var = "_" + srcBaseName + "_" + boost::lexical_cast<std::string>(++globalVarCount);
        sk.replace(pos, unknown.size(), var);
        gout << ": " << var << " = 100\n";
      }
    }
    std::cout << sk;
    std::ofstream f(skFile.c_str()); f << sk;
  }
  XD("traversal finished");

  //backend(project);

  // Output preprocessed source file.
  if (opt_unparse)
    unparseProject(project);

  // Compile to binary
  // JM: why do we need to compile to binary?
  // JG 10/12/2012:
  // This will generate *.mod for source code of Fortran modules.
  // When parsing Fortran application with multiple files, it might need to compile *.mod
  // for Fortran modules, which is requred by Fortran USE statement.
  bool opt_compile = args_has_opt(argv, argv + argc, "-c");
  if (opt_compile)
    backend(project);
  XD("exit: ret = 0");
  return 0;
}

// EOF

/*
void defuse(SgProject *project)
{
  DFAnalysis *dfa = new DefUseAnalysis(project);
  bool debug = false;
  dfa->run(debug);

  // Call the Def-Use Analysis
  // Output def use analysis results into a dot file
  //dfa->dfaToDOT();

  // Find all variable references
  foreach (SgNode *node, NodeQuery::querySubTree(project, V_SgVarRefExp)) {
    SgVarRefExp *varref = isSgVarRefExp(node);
    SgInitializedName *initname = isSgInitializedName(varref->get_symbol()->get_declaration());
    std::string varname = initname->get_qualified_name().str();

    std::vector<SgNode *> defs = dfa->getDefFor(varref, initname);
    // Each variable reference must have a definition somewhere
    if (SageInterface::is_Fortran_language())
      ROSE_ASSERT(!defs.empty());

    foreach (SgNode *def, defs) {
      std::cerr<< varname << " <= " << def->unparseToString() << std::endl;
      SgStatement *def_stmt = SageInterface::getEnclosingStatement(def);
      ROSE_ASSERT(def_stmt);
    }
  }
}

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

  struct Reference
  {
    SgNode *source;
    std::string id;
  };

  struct Entry
  {
    SgNode *source;

    Reference *result;
    std::list<Reference *> operands;

    int operation() const { return source ? source->variantT() : 0; }
  };

class DataFlowTablePrivate
{
  std::list<Entry *> table_;
public:

  bool build(SgNode *root)
  {
    foreach (SgNode *block, NodeQuery::querySubTree(root, V_SgBasicBlock))
      foreach (SgStatement *stmt, ::isSgBasicBlock(block)->get_statements())
        if (SageInterface::isAssignmentStatement(stmt, &lhs, &rhs))
          fromAssignment(stmt);
    return true;
  }

  bool fromAssignment(SgNode *stmt)
  {
     SgExpression *lhs, *rhs;
     if (!SageInterface::isAssignmentStatement(stmt, &lhs, &rhs))
       return false;
     ROSE_ASSERT(lhs);
     ROSE_ASSERT(rhs);

     Entry *e = new Entry();

     std::string target = lhs->unparseToString();
     std::set<std::string> refs;
     foreach (SgNode *r, NodeQuery::querySubTree(rhs, V_SgVarRefExp)) {
        if (rosex::isArrayType(::isSgVarRefExp(r)->get_type()))
          r = get_array_ref(r, stmt);
         else if (ref_in_array(r, stmt))
          continue;
         ROSE_ASSERT(r);
         refs.insert(r->unparseToString());
       }

        std::string source;
       foreach (const std::string &r, refs)
        source.append(r).push_back(',');

        std::string inst = target + " = " + source;
        ret += inst + "\n";
     }
  }
};

class DataFlowTablePrivate;
class DataFlowTable
{
  typedef DataFlowTablePrivate D;
  D *d_;

public:
  explicit DataFlowTable(SgNode *root = nullptr)
  {
    d_ = new D;
    build(root);
  }

  ~DataFlowTable() { delete d_; }


  bool build(SgNode *root)
  { return d_->build(root); }

  std::string toString() const
  {
    return std::string();
  }
};

std::string unparseDataflow(SgNode *project)
{
  std::string ret;
  //foreach (SgNode *e, NodeQuery::querySubTree(project, V_SgExpression)) {
  //  std::cerr<<e->unparseToString()<<std::endl;
  //}

  SgExpression *lhs, *rhs;
  foreach (SgNode *block, NodeQuery::querySubTree(project, V_SgBasicBlock))
    foreach (SgStatement *stmt, ::isSgBasicBlock(block)->get_statements())
      if (SageInterface::isAssignmentStatement(stmt, &lhs, &rhs)) {
        std::string target = lhs->unparseToString();

        std::set<std::string> refs;
        foreach (SgNode *r, NodeQuery::querySubTree(rhs, V_SgVarRefExp)) {
          if (rosex::isArrayType(::isSgVarRefExp(r)->get_type()))
            r = get_array_ref(r, stmt);
          else if (ref_in_array(r, stmt))
            continue;
          ROSE_ASSERT(r);
          refs.insert(r->unparseToString());
        }

        std::string source;
        foreach (const std::string &r, refs)
          source.append(r).push_back(',');

        std::string inst = target + " = " + source;
        ret += inst + "\n";
      }
  return ret;
}
*/
