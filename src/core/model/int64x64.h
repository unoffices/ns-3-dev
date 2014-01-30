#ifndef INT64X64_H
#define INT64X64_H

#include "ns3/core-config.h"

// Order is important here, as it determines which implementation
// will generate doxygen API docs.  This order mimics the
// selection logic in wscript, so we generate docs from the
// implementation actually chosen by the configuration.
#if defined (INT64X64_USE_128) && !defined (PYTHON_SCAN)
#include "int64x64-128.h"
#elif defined (INT64X64_USE_CAIRO) && !defined (PYTHON_SCAN)
#include "int64x64-cairo.h"
#elif defined (INT64X64_USE_DOUBLE) || defined (PYTHON_SCAN)
#include "int64x64-double.h"
#endif

#include <iostream>

namespace ns3 {

/**
 * \ingroup highprec
 * Addition operator.
 */
inline
int64x64_t operator + (const int64x64_t & lhs, const int64x64_t & rhs)
{
  int64x64_t tmp = lhs;
  tmp += rhs;
  return tmp;
}
/**
 * \ingroup highprec
 * Subtraction operator.
 */
inline
int64x64_t operator - (const int64x64_t & lhs, const int64x64_t & rhs)
{
  int64x64_t tmp = lhs;
  tmp -= rhs;
  return tmp;
}
/**
 * \ingroup highprec
 * Multiplication operator.
 */
inline
int64x64_t operator * (const int64x64_t & lhs, const int64x64_t & rhs)
{
  int64x64_t tmp = lhs;
  tmp *= rhs;
  return tmp;
}
/**
 * \ingroup highprec
 * Division operator.
 */
inline
int64x64_t operator / (const int64x64_t & lhs, const int64x64_t & rhs)
{
  int64x64_t tmp = lhs;
  tmp /= rhs;
  return tmp;
}

/**
 * \ingroup highprec
 * Output streamer for int64x64_t
 *
 * Values are printed with the following format flags
 * (independent of the the stream flags):
 *   - `showpos`
 *   - `left`
 *
 * The stream `width` is ignored.  If `floatfield` is set,
 * `precision` decimal places are printed.  If `floatfield` is not set,
 * all digits of the fractional part are printed, up to the
 * representation limit of 20 digits; trailing zeros are omitted.
 */
std::ostream &operator << (std::ostream &os, const int64x64_t &val);
/**
 * \ingroup highprec
 * Input streamer for int64x64_t
 */
std::istream &operator >> (std::istream &is, int64x64_t &val);

/**
 * \ingroup highprec
 * Absolute value.
 */
inline int64x64_t Abs (const int64x64_t &value)
{
  return (value < 0) ? -value : value;
}

/**
 * \ingroup highprec
 * Minimum.
 *
 * \return The smaller of the arguments.
 */
inline int64x64_t Min (const int64x64_t &a, const int64x64_t &b)
{
  return (a < b) ? a : b;
}
/**
 * \ingroup highprec
 * Maximum.
 *
 * \return The larger of the arguments.
 */
inline int64x64_t Max (const int64x64_t &a, const int64x64_t &b)
{
  return (a > b) ? a : b;
}

} // namespace ns3

#endif /* INT64X64_H */
