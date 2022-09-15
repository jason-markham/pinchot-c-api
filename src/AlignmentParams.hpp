/**
 * Copyright (c) JoeScan Inc. All Rights Reserved.
 *
 * Licensed under the BSD 3 Clause License. See LICENSE.txt in the project
 * root for license information.
 */

#ifndef JOESCAN_ALIGNMENT_PARAMS_H
#define JOESCAN_ALIGNMENT_PARAMS_H

#include "Point2D.hpp"
#include "joescan_pinchot.h"

namespace joescan {

class AlignmentParams {
 public:
  /**
   * Initialize the AlinmentParams object for use in coordinate conversion.
   *
   * @param camera_to_mill_scale scale to be applied to transform.
   * @param roll The rotation to be applied along the Z axis.
   * @param shift_x The shift to be applied specified in scan system units.
   * @param shift_y The shift to be applied specified in scan system units.
   * @param cable The cable orientation of the scan head.
   * degrees about the Y axis, placing postive X at former negative X.
   */
  AlignmentParams(double camera_to_mill_scale = 1.0 , double roll = 0.0,
                  double shift_x = 0.0, double shift_y = 0.0,
                  jsCableOrientation cable=JS_CABLE_ORIENTATION_UPSTREAM);

  AlignmentParams(const AlignmentParams &other) = default;

  jsCableOrientation GetCableOrientation() const;

  /**
   * Obtain the applied rotation for use in coordinate conversion.
   *
   * @return The current rotation.
   */
  double GetRoll() const;

  /**
   * Obtain the applied X direction shift for use in coordinate conversion.
   *
   * @return The current X shift expressed in scan system units.
   */
  double GetShiftX() const;

  /**
   * Obtain the applied Y direction shift for use in coordinate conversion.
   *
   * @return The current Y shift expressed in scan system units.
   */
  double GetShiftY() const;

  void SetCableOrientation(jsCableOrientation cable);

  /**
   * For XY profile data, this will rotate the points around the mill
   * coordinate system origin.
   *
   * @param roll The rotation to be applied.
   * @param is_cable_downstream Set to `true` to rotate the coordinate system
   * by 180
   */
  void SetRoll(double roll);

  /**
   * For XY profile data, apply the specified shift to X when converting
   * from camera coordinates to mill coordinates.
   *
   * @param shift_x The shift to be applied specified in inches.
   */
  void SetShiftX(double shift_x);

  /**
   * For XY profile data, apply the specified shift to Y when converting
   * from camera coordinates to mill coordinates.
   *
   * @param roll shift_y The shift to be applied specified in inches.
   */
  void SetShiftY(double shift_y);

  /**
   * Convert XY profile data from camera coordinates to mill coordinates.
   *
   * @param p Geometry X and Y value held in Point2D object.
   * @return Converted geometry point.
   */
  Point2D<int32_t> CameraToMill(Point2D<int32_t> p) const;

  /**
   * Convert XY profile data from camera coordinates to mill coordinates.
   *
   * @param x Geometry X value expressed in 1/1000 scan system units.
   * @param y Geometry Y value expressed in 1/1000 scan system units.
   * @return Converted geometry point.
   */
  inline Point2D<int32_t> CameraToMill(int32_t x, int32_t y) const;

  /**
   * Convert XY profile data from mill coordinates to camera coordinates.
   *
   * @param p Geometry X and Y value held in Point2D object.
   * @return Converted geometry point.
   */
  inline Point2D<int32_t> MillToCamera(Point2D<int32_t> p) const;

  /**
   * Convert XY profile data from mill coordinates to camera coordinates.
   *
   * @param x Geometry X value expressed in 1/1000 scan system units.
   * @param y Geometry Y value expressed in 1/1000 scan system units.
   * @return Converted geometry point.
   */
  inline Point2D<int32_t> MillToCamera(int32_t x, int32_t y) const;

 private:
  void CalculateTransform();

  jsCableOrientation m_cable;
  double m_roll;
  double m_shift_x;
  double m_shift_y;
  double m_shift_x_1000;
  double m_shift_y_1000;
  double m_camera_to_mill_xx;
  double m_camera_to_mill_xy;
  double m_camera_to_mill_yx;
  double m_camera_to_mill_yy;
  double m_mill_to_camera_xx;
  double m_mill_to_camera_xy;
  double m_mill_to_camera_yx;
  double m_mill_to_camera_yy;
  double m_camera_to_mill_scale;
};

/*
 * Note: We inline these functions for performance benefits.
 */
inline Point2D<int32_t> AlignmentParams::CameraToMill(Point2D<int32_t> p) const
{
  return CameraToMill(p.x, p.y);
}

inline Point2D<int32_t> AlignmentParams::CameraToMill(int32_t x,
                                                      int32_t y) const
{
  // static cast int32_t fields to doubles before doing our math
  double xd = static_cast<double>(x);
  double yd = static_cast<double>(y);

  // now calculate the mill values for both X and Y
  double xm = (xd * m_camera_to_mill_xx) - (yd * m_camera_to_mill_xy) +
              m_shift_x_1000;

  double ym = (xd * m_camera_to_mill_yx) + (yd * m_camera_to_mill_yy) +
              m_shift_y_1000;

  // now convert back to int32_t values
  int32_t xi = static_cast<int32_t>(xm);
  int32_t yi = static_cast<int32_t>(ym);

  return Point2D<int32_t>(xi, yi);
}

inline Point2D<int32_t> AlignmentParams::MillToCamera(Point2D<int32_t> p) const
{
  return MillToCamera(p.x, p.y);
}

inline Point2D<int32_t> AlignmentParams::MillToCamera(int32_t x,
                                                      int32_t y) const
{
  // static cast int32_t fields to doubles before doing our math
  double xd = static_cast<double>(x);
  double yd = static_cast<double>(y);

  // now calculate the camera values for both X and Y
  double xc = ((xd - m_shift_x_1000) * m_mill_to_camera_xx) -
              ((yd - m_shift_y_1000) * m_mill_to_camera_xy);

  double yc = ((xd - m_shift_x_1000) * m_mill_to_camera_yx) +
              ((yd - m_shift_y_1000) * m_mill_to_camera_yy);

  // now convert back to int32_t values
  int32_t xi = static_cast<int32_t>(xc);
  int32_t yi = static_cast<int32_t>(yc);

  return Point2D<int32_t>(xi, yi);
}

} // namespace joescan

#endif // JOESCAN_ALIGNMENT_PARAMS_H
