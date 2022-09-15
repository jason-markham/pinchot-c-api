/**
 * Copyright (c) JoeScan Inc. All Rights Reserved.
 *
 * Licensed under the BSD 3 Clause License. See LICENSE.txt in the project
 * root for license information.
 */

#include "AlignmentParams.hpp"
#include <cmath>
#include <stdexcept>

using namespace joescan;

AlignmentParams::AlignmentParams(double camera_to_mill_scale, double roll,
                                 double shift_x, double shift_y,
                                 jsCableOrientation cable) :
                                 m_cable(cable),
                                 m_roll(roll),
                                 m_camera_to_mill_scale(camera_to_mill_scale)
{
  SetShiftX(shift_x);
  SetShiftY(shift_y);
  CalculateTransform();
}

jsCableOrientation AlignmentParams::GetCableOrientation() const
{
  return m_cable;
}

double AlignmentParams::GetRoll() const
{
  return m_roll;
}

double AlignmentParams::GetShiftX() const
{
  return m_shift_x;
}

double AlignmentParams::GetShiftY() const
{
  return m_shift_y;
}

void AlignmentParams::SetCableOrientation(jsCableOrientation cable)
{
  if ((JS_CABLE_ORIENTATION_DOWNSTREAM == cable) ||
      (JS_CABLE_ORIENTATION_UPSTREAM == cable)) {
    m_cable = cable;
    CalculateTransform();
  }
}

void AlignmentParams::SetRoll(double roll)
{
  m_roll = roll;
  CalculateTransform();
}

void AlignmentParams::SetShiftX(double shift_x)
{
  m_shift_x = shift_x;
  m_shift_x_1000 = shift_x * 1000.0;
}

void AlignmentParams::SetShiftY(double shift_y)
{
  m_shift_y = shift_y;
  m_shift_y_1000 = shift_y * 1000.0;
}

void AlignmentParams::CalculateTransform()
{
  const double rho = (std::atan(1) * 4) / 180.0; // pi / 180
  const double yaw = (JS_CABLE_ORIENTATION_DOWNSTREAM == m_cable) ? 0.0 : 180.0;
  const double sin_roll = std::sin(m_roll * rho);
  const double cos_roll = std::cos(m_roll * rho);
  const double cos_yaw = std::cos(yaw * rho);
  const double sin_neg_roll = std::sin(-1.0 * m_roll * rho);
  const double cos_neg_roll = std::cos(-1.0 * m_roll * rho);
  const double cos_neg_yaw = std::cos(-1.0 * yaw * rho);

  m_camera_to_mill_xx = cos_yaw * cos_roll * m_camera_to_mill_scale;
  m_camera_to_mill_xy = sin_roll * m_camera_to_mill_scale;
  m_camera_to_mill_yx = cos_yaw * sin_roll * m_camera_to_mill_scale;
  m_camera_to_mill_yy = cos_roll * m_camera_to_mill_scale;
  m_mill_to_camera_xx = cos_neg_yaw * cos_neg_roll / m_camera_to_mill_scale;
  m_mill_to_camera_xy = cos_neg_yaw * sin_neg_roll / m_camera_to_mill_scale;
  m_mill_to_camera_yx = sin_neg_roll / m_camera_to_mill_scale;
  m_mill_to_camera_yy = cos_neg_roll / m_camera_to_mill_scale;
}
