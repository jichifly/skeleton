// skquery.cc
// 9/12/2012 jichi

#include "sk/sknode.h"
#include "sk/skquery.h"
#include "xt/xt.h"

//#define SK_DEBUG "skquery"
#include "sk/skdebug.h"
#include <iostream>

#define foreach BOOST_FOREACH

SK_BEGIN_NAMESPACE

bool SkQuery::contains(const SkNode *root, Sk::ClassType cls)
{
  if (root) {
    if (root->classType() == cls)
      return true;
    foreach (SkNode *c, root->children())
      if (contains(c, cls))
        return true;
  }
  return false;
}

SkNodeList SkQuery::find(SkNode *root, Sk::ClassType cls, Sk::TraversalOrder order)
{
  SkNodeList ret;
  if (root) {
    SK_DPRINT("enter: sk class = " << root->className());
    if (order != Sk::PostOrder && root->classType() == cls)
      ret.push_back(root);

    foreach (SkNode *c, root->children())
      xt::append(ret, find(c, cls, order));

    if (order == Sk::PostOrder && root->classType() == cls)
      ret.push_back(root);
    SK_DPRINT("leave: size = " << ret.size());
  }
  return ret;
}

SkConstNodeList SkQuery::find(const SkNode *root, Sk::ClassType cls, Sk::TraversalOrder order)
{
  SkConstNodeList ret;
  if (root) {
    SK_DPRINT("enter: sk class = " << root->className());
    if (order != Sk::PostOrder && root->classType() == cls)
      ret.push_back(root);

    foreach (const SkNode *c, root->children())
      xt::append(ret, find(c, cls, order));

    if (order == Sk::PostOrder && root->classType() == cls)
      ret.push_back(root);
    SK_DPRINT("leave: size = " << ret.size());
  }
  return ret;
}

SkNode *SkQuery::findfirst(SkNode *root, Sk::ClassType cls, Sk::TraversalOrder order)
{
  if (root) {
    SK_DPRINT("enter: sk class = " << root->className());
    if (order != Sk::PostOrder && root->classType() == cls)
      return root;

    foreach (SkNode *c, root->children())
      if (SkNode *p = findfirst(c, cls, order))
        return p;

    if (order == Sk::PostOrder && root->classType() == cls)
      return root;
    SK_DPRINT("leave: size = " << ret.size());
  }
  return nullptr;
}

const SkNode *SkQuery::findfirst(const SkNode *root, Sk::ClassType cls, Sk::TraversalOrder order)
{
  if (root) {
    SK_DPRINT("enter: sk class = " << root->className());
    if (order != Sk::PostOrder && root->classType() == cls)
      return root;

    foreach (const SkNode *c, root->children())
      if (const SkNode *p = findfirst(c, cls, order))
        return p;

    if (order == Sk::PostOrder && root->classType() == cls)
      return root;
    SK_DPRINT("leave: size = " << ret.size());
  }
  return nullptr;
}

SK_END_NAMESPACE
