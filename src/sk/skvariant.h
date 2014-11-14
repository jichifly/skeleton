#ifndef SKVARIANT_H
#define SKVARIANT_H

// skvariant.h
// 9/12/2012 jichi
// Container of runtime polymorphic type.
// The purpose of this class is similar to QVariant from Qt4.

#include "sk/skdef.h"
#include <boost/lexical_cast.hpp>
#include <string>
#include <cstring>

SK_BEGIN_NAMESPACE

/**
 *  \brief Container of runtime polymorphic type.
 *
 *  The purpose of this class is similar to QVariant from Qt4.
 *
 *  JG 10/12/2012: it is a C++ equivalent of union from C. It does the same thing as QVariant (http://qt-project.org/doc/qt-4.8/QVariant.html)
 *    Equivalent C code follows.
 *    typedef union _variant {
 *      float float_val;
 *      double float_val;
 *      int int_val;
 *      long long_val;
 *      long long llong_val;
 *      void *pointer_val;
 *    } variant;
 */
class SkVariant
{
  typedef SkVariant Self;
  long long value_;

  //enum Type { NoType = 0, IntType, FloatType, PointerType };
  //Type type_;

  // - Construction -
public:
  SkVariant() : value_(0) { }
  SkVariant(bool val) : value_(val ? 1 : 0) { }
  SkVariant(char val) : value_(val) { }
  SkVariant(int val) : value_(val) { }
  SkVariant(float val) { setValue(val); }
  SkVariant(double val) { setValue(val); }
  explicit SkVariant(void *val) { setValue(val); }
  SkVariant(const Self &that) : value_(that.value_) { }

  // - Properties -
public:
  bool toBool() const { return value_; }

  char toChar() const { return value_; }

  int toInt() const { return value_; }

  float toFloat() const
  { float ret; llong2float(&ret, &value_); return ret; }

  double toDouble() const
  { double ret; llong2double(&ret, &value_); return ret; }

  void *toPointer() const
  { void *ret; llong2lp(&ret, &value_); return ret; }

  void setValue(bool val) { value_ = val ? 1 : 0; }

  void setValue(char val) { value_ = val; }

  void setValue(int val) { value_ = val; }

  void setValue(float val) { float2llong(&value_, &val); }

  void setValue(double val) { double2llong(&value_, &val); }

  void setValue(void *val) { lp2llong(&value_, &val); }

  std::string toString() const
  { return boost::lexical_cast<std::string>(value_); }

  // - Operators -
public:
  bool operator==(const Self &that) const { return value_ == that.value_; }
  Self &operator=(const Self &that) { value_ = that.value_; return *this; }

  // - Implementation -
protected:
  static void float2llong(long long *dest, const float *src)
  { ::memcpy(dest, src, sizeof(*src)); }

  static void llong2float(float *dest, const long long *src)
  { ::memcpy(dest, src, sizeof(*dest)); }

  static void double2llong(long long *dest, const double *src)
  { ::memcpy(dest, src, sizeof(*src)); }

  static void llong2double(double *dest, const long long *src)
  { ::memcpy(dest, src, sizeof(*dest)); }

  static void lp2llong(long long *dest, void **src)
  { ::memcpy(dest, src, sizeof(*src)); }

  static void llong2lp(void **dest, const long long *src)
  { ::memcpy(dest, src, sizeof(*dest)); }
};

SK_END_NAMESPACE

#endif // SKVARIANT_H
