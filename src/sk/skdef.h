#ifndef SKDEF_H
#define SKDEF_H

// skdef.h
// 9/15/2012 jichi
// Helper macros.

#define SK_UNUSED(x) (void)(x)  // Suppress compiler warnings of unused variable

#define SK_NOP  SK_UNUSED(0)  // Dummy computation

// Disable the copy operations, same as boost::noncopyable and Q_DISABLE_COPY
#define SK_DISABLE_COPY(Class) \
  Class(const Class &); \
  Class &operator=(const Class &);

#ifndef SK_NO_DEBUG
#  include <cassert>
#  define SK_ASSERT(cond) assert(cond)
#else
#  define SK_ASSERT(cond) SK_NOP
#endif // !SK_NO_DEBUG

// Redefine SK_BEGIN_NAMESPACE/SK_END_NAMESPACE to turn on custom namespace.
#ifndef SK_BEGIN_NAMESPACE
#  define SK_BEGIN_NAMESPACE
#endif // SK_BEGIN_NAMESPACE
#ifndef SK_END_NAMESPACE
#  define SK_END_NAMESPACE
#endif // SK_END_NAMESPACE

#define SK_FORWARD_DECLARE_CLASS(name)  SK_BEGIN_NAMESPACE class  name; SK_END_NAMESPACE
#define SK_FORWARD_DECLARE_STRUCT(name) SK_BEGIN_NAMESPACE struct name; SK_END_NAMESPACE

#include "xt/c++11.h"

#endif // SKDEF_H
