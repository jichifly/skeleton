// skbuilder_vec.cc
// 9/13/2012 jichi
// Statistics implementation

#include "sk/skbuilder.h"
#include "sk/skbuilder_p.h"
#include "sk/skquery.h"
#include "sk/sknode.h"
#include "rosex/rosex.h"

#define foreach BOOST_FOREACH

//#define SK_DEBUG "skbuilder_vec"
#include "sk/skdebug.h"
#include <iostream>

SK_BEGIN_NAMESPACE

SkNode *SkBuilder::devectorize(SkNode *input)
{
  SkNodeList l = SkQuery::find(input, Sk::C_LoadStatement);
  SkNodeList l2 = SkQuery::find(input, Sk::C_StoreStatement);
  l.splice(l.end(), l2);
  foreach (SkNode *stmt, l)
    if (SkBlock *b = sknode_cast<SkBlock *>(stmt->parent()))
      if (SkNode *res = detail::devectorizeLoadStore(stmt))
        if (res != stmt)
          b->replace(sknode_cast<SkStatement *>(stmt), sknode_cast<SkStatement *>(res));

  //foreach (SkNode *stmt, SkQuery::find(input, Sk::C_IfStatement))
  //  detail::devectorizeBranchCondition(stmt);
  //foreach (SkNode *stmt, SkQuery::find(input, Sk::C_SwitchStatement))
  //  detail::devectorizeBranchCondition(stmt);

  return input;
}

SkNode *SkBuilder::detail::devectorizeLoadStore(SkNode *input)
{
  SkNodeList l;
  foreach (SkNode *n, SkQuery::find(input, Sk::C_Slice))
    if (sknode_cast<SkArrayReference *>(n->parent()))
      l.push_back(n);
  if (l.empty())
    return nullptr;

  SkArrayReference *array = sknode_cast<SkArrayReference *>(l.front()->parent());
  if (!array) {
    SkNode *slice = l.front();
    if (slice->parent())
      std::cerr << "skbuiler_vec::devectorize: error: slice parent is not array: " << slice->parent()->className() << std::endl;
    else
      std::cerr << "skbuiler_vec::devectorize: error: missing slice parent " << slice->parent() << std::endl;
    return nullptr;
  }

  SkForStatement *outer = nullptr,
                 *inner = nullptr;
  foreach (SkNode *it, l) {
    SkSlice *slice = static_cast<SkSlice *>(it);

    SkForStatement *f = new SkForStatement(input->source());
    SkVariable *var = SkBuilder::detail::createTempScalar(Sk::S_Integer);
    f->setVariable(var);
    f->setStart(slice->start());
    f->setStop(slice->stop());
    f->setStep(slice->step());
    array->replaceIndex(slice, new SkReference(var->clone()));
    delete slice;

    if (!outer) {
      outer = f;
      inner = f;
    } else {
      inner->setBody(f);
      inner = f;
    }
  }
  inner->setKeyword(SK_KW_FORALL);
  if (outer) {
    SkBlock *b = new SkBlock(input->source());
    inner->setBody(b);
    b->append(new SkBlockBarrier);
    b->append(static_cast<SkStatement *>(input));
  }
  return outer;
}

void SkBuilder::detail::devectorizeBranchCondition(SkNode *input)
{
  // FIXME: Ignore C_Slice in type declaration
  if (!input)
    return;
  switch (input->classType()) {
  case Sk::C_IfStatement:
    {
      SkIfStatement *stmt = static_cast<SkIfStatement *>(input);

      if (SkQuery::contains(stmt->condition(), Sk::C_Slice)) {
        SkNode *cond = stmt->condition();
        stmt->setCondition(new SkValue(cond->source()));
        delete cond;
      }
    } break;
  case Sk::C_SwitchStatement:
    {
      SkSwitchStatement *stmt = static_cast<SkSwitchStatement *>(input);
      if (SkQuery::contains(stmt->condition(), Sk::C_Slice)) {
        SkNode *cond = stmt->condition();
        stmt->setCondition(new SkValue(cond->source()));
        delete cond;
      }
    } break;
  default: ;
  }
}

SK_END_NAMESPACE
