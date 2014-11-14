#ifndef _ROSEX_ROSE_CONFIG_H
#define _ROSEX_ROSE_CONFIG_H

// rose_config.h
// 9/11/2012 jichi
// Suppress harmless compiler warnings for ROSE 0.9

#ifdef __clang__
#  pragma GCC diagnostic ignored "-Wdeprecated"
#  pragma GCC diagnostic ignored "-Wignored-qualifiers"
#  pragma GCC diagnostic ignored "-Wmismatched-tags"
#  pragma GCC diagnostic ignored "-Woverloaded-virtual"
#  pragma GCC diagnostic ignored "-Wparentheses"
#  pragma GCC diagnostic ignored "-Wunused-parameter"
#  pragma GCC diagnostic ignored "-Wunused-variable"
#  pragma GCC diagnostic ignored "-Wunneeded-internal-declaration"
#elif defined __GNUC__
#  pragma GCC diagnostic ignored "-Wignored-qualifiers"
#  pragma GCC diagnostic ignored "-Wunused-parameter"
#endif // __GNUC__

#include "xt/c++11.h"

#endif // _ROSEX_ROSE_CONFIG_H
