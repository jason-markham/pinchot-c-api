/**
 * Copyright (c) JoeScan Inc. All Rights Reserved.
 *
 * Licensed under the BSD 3 Clause License. See LICENSE.txt in the project
 * root for license information.
 */

/**
 * @file configure_and_connect.cpp
 * @brief Example showing how to configure and connect to a single JS-50 WX
 * scan head.
 *
 * This example application demonstrates how to configure, connect, and
 * disconnect from a single scan head. For configuring the scan head, functions
 * and data structures from the joescanapi will be introduced and utilized in
 * a friendly manner. Following successful configuration, the application will
 * connect to the scan head, print out its current status, and then finally
 * disconnect.
 */

#include <iostream>
#include <stdexcept>
#include "joescan_pinchot.h"

class ApiError : public std::runtime_error {
 private:
  jsError m_return_code;

 public:
  ApiError(const char* what, int32_t return_code) : std::runtime_error(what)
  {
    if ((0 < return_code) || (JS_ERROR_UNKNOWN > m_return_code)) {
      m_return_code = JS_ERROR_UNKNOWN;
    } else {
      m_return_code = (jsError) return_code;
    }
  }

  jsError return_code() const
  {
    return m_return_code;
  }
};

/**
 * @brief Display the API version to console output for visual confirmation as
 * to the version being used for this example.
 */
void PrintApiVersion()
{
  uint32_t major, minor, patch;
  jsGetAPISemanticVersion(&major, &minor, &patch);
  std::cout << "Joescan API version " << major << "." << minor << "." << patch
            << std::endl;
}

/**
 * @brief Display serial number, type, and firmware version of the scan head
 * to console output.
 *
 * @param c Struct holding the scan head capabilities.
 */
void PrintScanHeadInfo(jsScanHead scan_head)
{
  int32_t r = 0;

  jsScanHeadType type = jsScanHeadGetType(scan_head);
  if (JS_SCAN_HEAD_INVALID_TYPE == type) {
    throw ApiError("invalid type", r);
  }

  switch (type) {
    case (JS_SCAN_HEAD_JS50WX):
      std::cout << "JS-50 WX";
      break;
    case(JS_SCAN_HEAD_JS50WSC):
      std::cout << "JS-50 WSC";
      break;
    case (JS_SCAN_HEAD_JS50X6B30):
      std::cout << "JS-50 X6B30";
      break;
    case (JS_SCAN_HEAD_JS50X6B20):
      std::cout << "JS-50 X6B20";
      break;
    case (JS_SCAN_HEAD_INVALID_TYPE):
    default:
      std::cout << "Invalid";
      (void) type;
  }

  uint32_t serial = jsScanHeadGetSerial(scan_head);
  std::cout << " serial " << serial;

  uint32_t major, minor, patch;
  r = jsScanHeadGetFirmwareVersion(scan_head, &major, &minor, &patch);
  if (0 > r) {
    throw ApiError("failed to get firmware version", r);
  }

  std::cout << " firmware version " << major << "." << minor << "." << patch
            << std::endl;
}

/**
 * @brief Display the capabilities of a scan head to console output.
 *
 * @param c Struct holding the scan head capabilities.
 */
void PrintScanHeadCapabilities(jsScanHeadCapabilities &c)
{
  std::cout << "jsScanHeadCapabilities" << std::endl;
  std::cout << "\tcamera_brightness_bit_depth=" <<
    c.camera_brightness_bit_depth << std::endl;
  std::cout << "\tmax_camera_image_height=" << c.max_camera_image_height <<
    std::endl;
  std::cout << "\tmax_camera_image_width=" << c.max_camera_image_width <<
    std::endl;
  std::cout << "\tmin_scan_period=" << c.min_scan_period_us << std::endl;
  std::cout << "\tnum_cameras=" << c.num_cameras << std::endl;
  std::cout << "\tnum_encoders=" << c.num_encoders << std::endl;
  std::cout << "\tnum_lasers=" << c.num_lasers << std::endl;
}

/**
 * @brief Prints the contents of a `jsScanHeadStatus` data type to standard out.
 *
 * @param stat Reference to scan head status to print.
 */
void PrintScanHeadStatus(jsScanHeadStatus &stat)
{
  std::cout << "jsScanHeadStatus" << std::endl;
  std::cout << "\tglobal_time_ns=" << stat.global_time_ns << std::endl;
  std::cout << "\tnum_encoder_values=" << stat.num_encoder_values << std::endl;

  std::cout << "\tencoder_values=";
  for (uint32_t n = 0; n < stat.num_encoder_values; n++) {
    std::cout << stat.encoder_values[n];
    if (n != (stat.num_encoder_values - 1)) {
      std::cout << ",";
    } else {
      std::cout << std::endl;
    }
  }

  std::cout << "\tcamera_a_pixels_in_window="
            << stat.camera_a_pixels_in_window << std::endl;
  std::cout << "\tcamera_a_temp=" << stat.camera_a_temp << std::endl;


  std::cout << "\tcamera_b_pixels_in_window="
            << stat.camera_b_pixels_in_window << std::endl;
  std::cout << "\tcamera_b_temp=" << stat.camera_b_temp << std::endl;

  std::cout << "\tnum_profiles_sent=" << stat.num_profiles_sent << std::endl;
}

int main(int argc, char *argv[])
{
  jsScanSystem scan_system = 0;
  jsScanHead scan_head = 0;
  uint32_t serial_number = 0;
  int32_t r = 0;

  if (2 > argc) {
    std::cout << "Usage: " << argv[0] << " SERIAL" << std::endl;
    return 1;
  }

  // Grab the serial number of the scan head from the command line.
  serial_number = strtoul(argv[1], nullptr, 0);

  try {
    PrintApiVersion();

    // One of the first calls to the API should be to create a scan manager
    // software object. This object will be used to manage groupings of scan
    // heads, telling them when to start and stop scanning.
    scan_system = jsScanSystemCreate(JS_UNITS_INCHES);
    if (0 > scan_system) {
      throw ApiError("failed to create scan system", r);
    }

    // Create a scan head software object for the user's specified serial
    // number and associate it with the scan manager we just created. We'll
    // also assign it a user defined ID that can be used within the application
    // as an optional identifier if prefered over the serial number. Note that
    // at this point, we haven't connected with the physical scan head yet.
    int32_t id = 0;
    scan_head = jsScanSystemCreateScanHead(scan_system, serial_number, id);
    if (0 > scan_head) {
      r = scan_head;
      throw ApiError("failed to create scan head", r);
    }

    // The scan head has been created. We can query it for information and also
    // begin configuring it.
    PrintScanHeadInfo(scan_head);

    jsScanHeadCapabilities cap;
    r = jsScanHeadGetCapabilities(scan_head, &cap);
    if (0 > r) {
      throw ApiError("failed to get capabilities", r);
    }

    PrintScanHeadCapabilities(cap);

    // Many of the settings directly related to the operation of the cameras
    // and lasers can be found in the `jsScanHeadConfiguration` struct. Refer
    // to the API documentation for specific details regarding each field. For
    // this example, we'll use some generic values not specifically set for any
    // particular scenario.
    jsScanHeadConfiguration config;
    config.camera_exposure_time_min_us = 10000;
    config.camera_exposure_time_def_us = 47000;
    config.camera_exposure_time_max_us = 900000;
    config.laser_on_time_min_us = 100;
    config.laser_on_time_def_us = 100;
    config.laser_on_time_max_us = 1000;
    config.laser_detection_threshold = 120;
    config.saturation_threshold = 800;
    config.saturation_percentage = 30;
    r = jsScanHeadSetConfiguration(scan_head, &config);
    if (0 > r) {
      throw ApiError("failed to set scan head configuration", r);
    }

    // Proper window selection can be crucial to successful scanning as it
    // allows users to limit the region of interest for scanning; filtering out
    // other sources of light that could complicate scanning. It is worth
    // noting that there is an inverse relationship with the scan window and
    // the overall scan rate a system can run at. Using larger scan windows
    // will reduce the maximum scan rate of a system, whereas using a smaller
    // scan window will increase the maximum scan rate.
    r = jsScanHeadSetWindowRectangular(scan_head, 30.0, -30.0, -30.0, 30.0);
    if (0 > r) {
      throw ApiError("failed to set window", r);
    }

    // Setting the alignment through the following function can help to
    // correct for any mounting issues with a scan head that could affect
    // the 3D measurement. For this example, we'll assume that the scan head
    // is mounted perfectly such that the laser is pointed directly at the scan
    // target.
    r = jsScanHeadSetAlignment(scan_head, 0.0, 0.0, 0.0);
    if (0 > r) {
      throw ApiError("failed to set alignment", r);
    }

    // We've now successfully configured the scan head. Now comes the time to
    // connect to the physical scanner and transmit the configuration values
    // we previously set up.
    r = jsScanSystemConnect(scan_system, 10);
    if (jsScanSystemGetNumberScanHeads(scan_system) != r) {
      throw ApiError("failed to connect", r);
    }

    // Now that we are connected, we can query the scan head to get it's
    // current status. Note that the status will be updated periodically by the
    // scan head and calling this function multiple times will provide the
    // last reported status of the scan head.
    jsScanHeadStatus status;
    r = jsScanHeadGetStatus(scan_head, &status);
    if (0 > r) {
      throw ApiError("failed to get scan head status", r);
    }

    PrintScanHeadStatus(status);

    // Once connected, this is the point where we could command the scan system
    // to start scanning; obtaining profile data from the scan heads associated
    // with it. This will be the focus of a later example.

    // We've accomplished what we set out to do for this example; now it's time
    // to bring down our system.
    r = jsScanSystemDisconnect(scan_system);
    if (0 > r) {
      throw ApiError("failed to disconnect", r);
    }
  } catch (ApiError &e) {
    std::cout << "ERROR: " << e.what() << std::endl;
    r = 1;

    const char *err_str = nullptr;
    jsError err = e.return_code();
    if (JS_ERROR_NONE != err) {
      jsGetError(err, &err_str);
      std::cout << "jsError (" << err << "): " << err_str << std::endl;
    }
  }

  // Clean up data allocated by the scan manager.
  jsScanSystemFree(scan_system);

  return r;
}
