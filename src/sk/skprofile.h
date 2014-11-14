#ifndef SKPROFILE_H
#define SKPROFILE_H

// skprofile.h
// 9/12/2012 jichi
// The profil

#include "sk/skglobal.h"
#include <cstring> // for size_t
#include <sstream>
#include <string>

SK_BEGIN_NAMESPACE

///  Abstract hardware specific information
struct SkProfile
{
  virtual ~SkProfile() { }

  ///  Output the containt. This make the class to be abstract.
  virtual std::string toString() const = 0;

  ///  Unparse as the code skeleton header
  virtual std::string unparse() const { return toString(); }
};

///  Profile model for most hardware
struct SkCommonProfile : SkProfile
{
  size_t int_size, float_size, double_size, complex_size;

  SkCommonProfile() :
    int_size(sizeof(int)),
    float_size(sizeof(float)),
    double_size(sizeof(double)),
    complex_size(2*sizeof(double))
  {}

  virtual std::string toString() const override
  {
    std::ostringstream out;
    out << "int_size = " << int_size << "\n"
        << "float_size = " << float_size << "\n"
        << "double_size = " << double_size << "\n"
        << "complex_size = " << complex_size << "\n";
    return out.str();
  }

  virtual std::string unparse() const override
  {
    //std::ostringstream out;
    //out << ":int = " << int_size << "\n"
    //    << ":float = " << float_size << "\n"
    //    << ":double = " << double_size << "\n";
    //return out.str();
    return "from commons import int, float, double, complex";
  }
};

SK_END_NAMESPACE

#endif // SKNODE_H
