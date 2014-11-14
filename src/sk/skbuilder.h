#ifndef SKBUILDER_H
#define SKBUILDER_H

// skbuilder.h
// 9/12/2012 jichi
// Skeleton AST builder.

#include "sk/skglobal.h"
#include <string>
#include <boost/tuple/tuple.hpp>
#include <boost/unordered/unordered_map.hpp>
#include <boost/unordered/unordered_set.hpp>

class SgNode;

SK_BEGIN_NAMESPACE

/**
 *  \brief  Skeleton AST builder.
 *
 *  JG 10/12/2012: map a Rose AST to the Skeleton AST
 *  It is not a node-to-node map.
 *  It will parse each SgNode in the ROSE AST, but will create the SkNode from its information.
 *  SkNode AST is much simpler than ROSE AST.
 */
namespace SkBuilder {

  ///  Options to control how to build skeleton AST.
  struct Option {
    /**
     *  \brief  Critical variables needed to keep in the generated skeleton.
     *
     *  A critical variable could be a scalar variable reference, or memory
     *  reference (array) in the source AST. When a reference is marked as
     *  critical, all modifications to it will be preserved in the skeleton AST.
     *  - When the set is empty, all references in the source AST will be
     *  considered as non-critical, and might be ignored if it is redundant.
     *  - When the pointer is null, all references in the source AST will be
     *  considered as critical, and all computations with using them will be
     *  preserved.
     *  - When the value is a non-empty set, only the references in the set
     *  are considered as critical.
     */
    boost::unordered_set<const SgNode *> *criticalReferences;

    /**
     *  \brief  New name of the scalar variables.
     *
     *  The purpose of variable renaming is for SSA.
     */
    boost::unordered_map<const SgNode *, std::string> *variableNames;

    /**
     *  If not null, the slices will be converted to loops.
     *  If the list is not empty after building, the slice is lost from the skeleton.
     */
    boost::unordered_map<SkSlice *, SkVariable *> *danglingSlices;

    ///**
    // *  \brief  If the initializer is supported in the declarator.
    // *
    // *  For example, when false:
    // *    :float a = 0
    // *  Will become:
    // *    :float a
    // *    a = 0
    // */
    //bool supportsInitializer;

    Option() : criticalReferences(nullptr), variableNames(nullptr), danglingSlices(nullptr) {}
  };

  //inline SkInteger *createInteger(int value = 0)
  //{ SkInteger *ret = new SkInteger; ret->setValue(value); return ret; }

  //inline SkForStatement *createForStatement(int value)
  //{ SkForStatement *ret = new SkInteger; ret->setValue(value); return ret; }

  ///  Build SkExpression from SgExpression.
  SkExpression *fromExpression(SgNode *src, const Option *opt);

  ///  Build SkSymbole from SgSymbol
  SkSymbol *fromSymbol(SgNode *src, const Option *opt, const SgNode *context=nullptr);

  ///  Build SkStatement from SgStatement.
  SkStatement *fromStatement(SgNode *src, const Option *opt);

  ///  Build SkType from SgType.
  SkType *fromType(SgNode *src, const Option *opt);

  ///  Build SkDeclaration, or a block of SkDeclaration from SgDeclaration.
  SkStatement *fromDeclaration(SgNode *src, const Option *opt);

  ///  Return OpenMP pragma
  SkStatement *fromPragma(SgNode *src);

  ///  Build SkFunctionDefinition from SgFunctionDefinition (only function is accepted).
  SkFunctionDefinition *fromFunction(SgNode *src, const Option *opt);

  ///  Extract global properties.
  SkGlobal *fromGlobal(SgNode *src);

  /**
   *  \param  src  SgStatement or SgExpression
   *  \return  Tuple of total operation count, int and floating-point operation count
   */
  boost::tuple<int, int, int> compOf(const SgNode *src);

  /**
   *  \param  src  SgStatement or SgExpression
   *  \return  Estimated branch count, used by \tt br statement.
   */
  int brOf(const SgNode *src);

  /**
   *  \param  src  SgStatement or SgExpression
   *  \return  A list of ld/st statements
   */
  std::list<SkStatement *> refOf(SgNode *src, const Option *opt);

  ///  Indent the unparsed skeleton code
  std::string indent(const std::string &skeleton);

  ///  Remove the lines with unsupported syntax, such like the '#' operator
  std::string purge(const std::string &skeleton);

  ///  Traverse src and apply devectorization recursively.
  SkNode *devectorize(SkNode *src);

} // namespace SkBuilder

SK_END_NAMESPACE

#endif // SKBUILDER_H
