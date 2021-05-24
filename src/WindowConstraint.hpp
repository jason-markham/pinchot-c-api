/**
 * Copyright (c) JoeScan Inc. All Rights Reserved.
 *
 * Licensed under the BSD 3 Clause License. See LICENSE.txt in the project
 * root for license information.
 */

#ifndef JOESCAN_WINDOW_CONSTRAINT_H
#define JOESCAN_WINDOW_CONSTRAINT_H

#include <cstdint>
#include "Point2D.hpp"

namespace joescan {

struct WindowConstraint {
  Point2D<int64_t> constraints[2];
  bool Satisfies(Point2D<int64_t> p)
  {
    // Calculate: (p.x - c0.x)(c1.y - c0.y) - (p.y - c0.y)(c1.x - c0.x)
    // Note, evaluating `>= 0` to `true` means the point is included
    // according to this constraint
    int64_t result =
      ((p.x - constraints[0].x) * (constraints[1].y - constraints[0].y)) -
      ((p.y - constraints[0].y) * (constraints[1].x - constraints[0].x));

    return result >= 0;
  }

  WindowConstraint(Point2D<int64_t> p1, Point2D<int64_t> p2)
  {
    constraints[0] = p1;
    constraints[1] = p2;
  }

  WindowConstraint(const WindowConstraint &constraints_)
  {
    for (int i = 0; i < 2; i++) {
      constraints[i] = constraints_.constraints[i];
    }
  }
};

} // namespace joescan

#endif
