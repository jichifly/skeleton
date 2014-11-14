#ifndef SKDEBUG_H
#define SKDEBUG_H

// skdebug.h
// 10/16/2011 jichi

#if defined(SK_DEBUG) && !defined(SK_NO_DEBUG)
#  include <iostream>
#  define SK_DPRINT(_msg)    std::cerr << SK_DEBUG << ":" << __FUNCTION__ << ": " \
                                       << _msg << std::endl

#else
#  define SK_DPRINT(_dummy)  (void)0

  //#ifdef _MSC_VER
  //# pragma warning (disable:4390) // C4390: empty controlled statement found: is this the intent?
  //#endif // _MSC_VER

  //#ifdef __GNUC__
  //# pragma GCC diagnostic ignored "-Wempty-body" // empty body in an if or else statement
  //#endif // __GNUC__

#endif // SKDEBUG && !SK_NO_DEBUG

#endif // SKDEBUG_H
