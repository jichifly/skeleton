#ifndef _XT_TYPEOF_H
#define _XT_TYPEOF_H

// typeof.h
// 9/20/2012 jichi

#if defined(__clang__) || defined(__GNUC__)
#  define typeof __typeof__
#else
#  include <boost/typeof/typeof.hpp>
#endif // __clang__

#endif // _XT_TYPEOF_H
