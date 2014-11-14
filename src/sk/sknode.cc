// sknode.h
// 10/16/2012 jichi

#include "sk/sknode.h"
#include "sk/skquery.h"
#include "rosex/rosex.h"
#include <boost/foreach.hpp>

#define foreach BOOST_FOREACH

SK_BEGIN_NAMESPACE

// - Static fields -

int SkBlockBarrier::s_count_ = 0;

SkConf SkNode::conf_;

// - Unparse -

std::string SkNode::unparseFunctionCalls(const SkNode *parent)
{
  std::string ret;
  foreach (const SkNode *n, SkQuery::find(parent, Sk::C_FunctionCall, Sk::PostOrder)) {
    const SkFunctionCall *call = static_cast<const SkFunctionCall *>(n);
    if (!call->needsCall() && call->function() &&
        call->function()->name() != "len")
        //call->function()->name() != "sizeof")
      ret.append("call ").append(call->unparse()).push_back('\n');
  }
  return ret;
}

// - ROSE -

std::list<SgNode *> SkBlockBarrier::sourceStatements() const
{
  std::list<SgNode *> ret;
  if (!isEmpty())
    if (SgBasicBlock *g = ::isSgBasicBlock(source()))
      foreach (SgNode *stmt, g->get_statements())
        if (stmt == endSource())
          break;
        else if (!ret.empty() || stmt == beginSource())
          ret.push_back(stmt);
  return ret;
}

//std::string SkNode::unparseComment(const std::string &comment)
//{
//  if (SageInterface::is_Fortran_language())
//    return "! " + comment + "\n";
//  else
//    return "// " + comment + "\n";
//}

int SkNode::columnNumber() const
{
  if (source_)
    if (Sg_File_Info *fi = source_->get_file_info())
      return fi->get_col();
  return 0;
}

int SkNode::lineNumber() const
{
  if (source_)
    if (Sg_File_Info *fi = source_->get_file_info())
      return fi->get_line();
  return 0;
}

std::string SkNode::fileName() const
{
  if (source_)
    if (Sg_File_Info *fi = source_->get_file_info())
      return fi->get_filenameString();
  return std::string();
}

SK_END_NAMESPACE
