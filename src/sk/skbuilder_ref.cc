// skbuilder_ref.cc
// 9/17/2012 jichi
// Reference counting implementation

#include "sk/skbuilder.h"
#include "sk/skbuilder_p.h"
#include "sk/sknode.h"
#include "rosex/rosex.h"
#include <boost/unordered/unordered_set.hpp>

#define foreach BOOST_FOREACH

//#define SK_DEBUG "skbuilder_ref"
#include "sk/skdebug.h"

SK_BEGIN_NAMESPACE

bool SkBuilder::detail::isLoadStoreRef(const SgNode *input)
{
  if (!input || !input->get_parent())
    return false;
  if (isStoreRef(input) && ::isSgCompoundAssignOp(input->get_parent()))
    return true;
  switch (input->get_parent()->variantT()) {
  case V_SgPlusPlusOp:
  case V_SgMinusOp:
    return true;
  default:
    return false;
  }
  ROSE_ASSERT(0);
}

bool SkBuilder::detail::isStoreRef(const SgNode *input)
{
  if (!input)
    return false;
  SgExpression *lhs;
  return SageInterface::isAssignmentStatement(input->get_parent(), &lhs)
      && lhs == input;
}

SkStatementList SkBuilder::refOf(SgNode *input, const Option *opt)
{
  SkStatementList ret;
  if (!input)
    return ret;
  SK_DPRINT("enter: sage class = " << input->class_name());

  boost::unordered_set<SgNode *> set;

  SgNodePtrList l = NodeQuery::querySubTree(input, V_SgPntrArrRefExp);

  foreach (SgNode *e, NodeQuery::querySubTree(input, V_SgVarRefExp))
    if (rosex::isArrayReference(e))
      l.push_back(e);

  foreach (SgNode *e, l)
    if (set.count(e->get_parent()))
      set.insert(e);
    else if (!set.count(e)) {
      set.insert(e);
      if (SkExpression *k = fromExpression(e, opt)) {
        if (SkReference *k_r = sknode_cast<SkReference *>(k)) {
          // use push_front instead of not push_back!
          if (SageInterface::is_Fortran_language())
            if (SkSymbol *s = k_r->symbol())
              if (SkType *t = s->type())
                if (t->classType() == Sk::C_ArrayType) {
                  SkArrayReference *a = new SkArrayReference(s, k_r->source());
                  foreach (SkExpression *e, sknode_cast<SkArrayType *>(t)->dimensions())
                    a->appendIndex(new SkSlice(new SkValue(1), e, new SkValue(1), e->source()));
                  delete k_r;
                  k_r = a;
                }

          if (detail::isLoadStoreRef(e)) {
            ret.push_front(new SkStoreStatement(k_r, e));
            ret.push_front(new SkLoadStatement(k_r->clone(), e));
          } else if (detail::isStoreRef(e))
            ret.push_front(new SkStoreStatement(k_r, e));
          else
            ret.push_front(new SkLoadStatement(k_r, e));
        } else
          delete k;
      }
    }
  SK_DPRINT("leave: ret size = " << ret.size());
  return ret;
}

SK_END_NAMESPACE
