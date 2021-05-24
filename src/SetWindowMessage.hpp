/**
 * Copyright (c) JoeScan Inc. All Rights Reserved.
 *
 * Licensed under the BSD 3 Clause License. See LICENSE.txt in the project
 * root for license information.
 */

#ifndef JOESCAN_SCAN_WINDOW_MESSAGE_H
#define JOESCAN_SCAN_WINDOW_MESSAGE_H

#include <vector>

#include "Enums.hpp"
#include "WindowConstraint.hpp"

namespace joescan {
class SetWindowMessage {
 public:
  SetWindowMessage(int camera);
  ~SetWindowMessage() = default;

  static SetWindowMessage Deserialize(std::vector<uint8_t> &message);
  std::vector<uint8_t> Serialize() const;

  inline void AddConstraint(int32_t x1, int32_t y1, int32_t x2, int32_t y2);
  inline void AddConstraint(Point2D<int64_t> p1, Point2D<int64_t> p2);
  void AddConstraints(std::vector<WindowConstraint> &constraints);
  inline void SetCamera(uint8_t camera);
  inline uint8_t GetCameraId() const;
  bool SatisfiesConstraints(int32_t x, int32_t y);
  bool SatisfiesConstraints(Point2D<int64_t> p);

  std::vector<WindowConstraint> Constraints() const;

 private:
  std::vector<WindowConstraint> m_constraints;
  uint8_t m_camera = 0;
};

// 1/1000 inch
inline void SetWindowMessage::AddConstraint(int32_t x1, int32_t y1, int32_t x2,
                                            int32_t y2)
{
  Point2D<int64_t> p1(x1, y1);
  Point2D<int64_t> p2(x2, y2);

  m_constraints.push_back(WindowConstraint(p1, p2));
}

// 1/1000 inch
inline void SetWindowMessage::AddConstraint(Point2D<int64_t> p1,
                                            Point2D<int64_t> p2)
{
  m_constraints.push_back(WindowConstraint(p1, p2));
}

inline void SetWindowMessage::SetCamera(uint8_t camera)
{
  m_camera = camera;
}

inline uint8_t SetWindowMessage::GetCameraId() const
{
  return m_camera;
}
} // namespace joescan

#endif // JOESCAN_SCAN_WINDOW_MESSAGE_H
