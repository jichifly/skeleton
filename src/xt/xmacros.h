#ifndef XMACROS_H
#define XMACROS_H

// xmacros.h
// 10/16/2011 jichi

//inline void xt_noop(void) { }

//template <typename T>
//inline void xUnused(T &x) { (void)x; }

#define X_UNUSED(x) (void)(x)

#define X_NOP   X_UNUSED(0)

#define X_DISABLE_COPY(Class) \
  Class(const Class &); \
  Class &operator=(const Class &);

#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 96)
# define X_LIKELY(expr)     __builtin_expect(!!(expr), true)
# define X_UNLIKELY(expr)   __builtin_expect(!!(expr), false)
#else
# define X_LIKELY(x)   (x)
# define X_UNLIKELY(x) (x)
#endif

// Qt-like Pimp

// Similar to QT_DECLARE_PRIVATE
#define X_DECLARE_PRIVATE(_class) \
  friend class _class; \
  typedef _class D; \
  D *const d_;

// Similar to QT_DECLARE_PUBLIC
#define X_DECLARE_PUBLIC(_class) \
  friend class _class; \
  typedef _class Q; \
  Q *const q_;

// Self and Base

#define X_CLASS(_self) \
  typedef _self Self; \
  Self *self() const { return const_cast<Self *>(this); }

#define X_EXTEND_CLASS(_self, _base) \
  SK_CLASS(_self) \
  typedef _base Base;

#ifndef XT_NO_DEBUG
#  include <cassert>
#  define X_ASSERT(cond) assert(cond)
#else
#  define X_ASSERT(cond) X_NOP
#endif // !XT_NO_DEBUG

#endif // XMACROS_H
