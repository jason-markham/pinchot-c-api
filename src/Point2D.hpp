/**
 * Copyright (c) JoeScan Inc. All Rights Reserved.
 *
 * Licensed under the BSD 3 Clause License. See LICENSE.txt in the project
 * root for license information.
 */

#ifndef JOESCAN_POINT2D_INCLUDED
#define JOESCAN_POINT2D_INCLUDED

#include <cstdint>

namespace joescan {

/**
 * @brief The `Point2D` class is a template used to mark a particular 2D
 * coordinate. In most cases, we use `int32_t` types to hold the data, but
 * `int64_t` is used some cases where multiplying values can lead to integer
 * overflow.
 */
template <class T = uint32_t>
class Point2D {
 public:
  T x;
  T y;

  Point2D(T x_, T y_) : x(x_), y(y_)
  {
  }
  Point2D() : x(0), y(0)
  {
  }
};

} // namespace joescan

#endif
