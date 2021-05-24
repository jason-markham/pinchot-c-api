/**
 * Copyright (c) JoeScan Inc. All Rights Reserved.
 *
 * Licensed under the BSD 3 Clause License. See LICENSE.txt in the project
 * root for license information.
 */

#include "joescan_pinchot.h"
#include "NetworkInterface.hpp"
#include "PinchotConstants.hpp"
#include "ProductInfo.hpp"
#include "ScanHead.hpp"
#include "ScanManager.hpp"
#include "VersionCompatibilityException.hpp"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <string>

#define INVALID_DOUBLE(d) (std::isinf((d)) || std::isnan((d)))

using namespace joescan;

static int _network_init_count = 0;

static unsigned int _data_format_to_stride(jsDataFormat fmt)
{
  unsigned int stride = 0;

  switch (fmt) {
    case JS_DATA_FORMAT_XY_FULL_LM_FULL:
    case JS_DATA_FORMAT_XY_FULL:
      stride = 1;
      break;
    case JS_DATA_FORMAT_XY_HALF_LM_HALF:
    case JS_DATA_FORMAT_XY_HALF:
      stride = 2;
      break;
    case JS_DATA_FORMAT_XY_QUARTER_LM_QUARTER:
    case JS_DATA_FORMAT_XY_QUARTER:
      stride = 4;
      break;
  }

  return stride;
}

EXPORTED
void jsGetAPIVersion(const char **version_str)
{
  *version_str = VERSION_FULL;
}

EXPORTED
void jsGetAPISemanticVersion(uint32_t *major, uint32_t *minor, uint32_t *patch)
{
  if (nullptr != major) {
    *major = strtoul(VERSION_MAJOR, nullptr, 10);
  }
  if (nullptr != minor) {
    *minor = strtoul(VERSION_MINOR, nullptr, 10);
  }
  if (nullptr != patch) {
    *patch = strtoul(VERSION_PATCH, nullptr, 10);
  }
}

EXPORTED
void jsGetError(int32_t return_code, const char **error_str)
{
  if (0 <= return_code) {
    *error_str = "none";
  } else {
    switch (return_code) {
      case (JS_ERROR_INTERNAL):
        *error_str = "internal error";
        break;
      case (JS_ERROR_NULL_ARGUMENT):
        *error_str = "null value argument";
        break;
      case (JS_ERROR_INVALID_ARGUMENT):
        *error_str = "invalid argument";
        break;
      case (JS_ERROR_NOT_CONNECTED):
        *error_str = "state not connected";
        break;
      case (JS_ERROR_CONNECTED):
        *error_str = "state connected";
        break;
      case (JS_ERROR_NOT_SCANNING):
        *error_str = "state not scanning";
        break;
      case (JS_ERROR_SCANNING):
        *error_str = "state scanning";
        break;
      case (JS_ERROR_VERSION_COMPATIBILITY):
        *error_str = "versions not compatible";
        break;
      default:
        *error_str = "unknown error";
    }
  }
}

EXPORTED
int32_t jsGetScanHeadCapabilities(jsScanHeadType type,
                                  jsScanHeadCapabilities *capabilities)
{
  int32_t r = 0;

  if (nullptr == capabilities) {
    return JS_ERROR_NULL_ARGUMENT;
  }

  r = GetProductCapabilities(type, capabilities);

  return r;
}

EXPORTED
jsScanSystem jsScanSystemCreate(void)
{
  jsScanSystem scan_system = nullptr;

  try {
    if (0 == _network_init_count) {
      // We need to explicitly initialze the network interface first thing.
      // This is crucial for Windows since it has some extra start up code that
      // should always be done first thing in the application to ensure that
      // networking works.
      // TODO: this could probably be moved...
      NetworkInterface::InitSystem();
      _network_init_count++;
    }

    ScanManager *manager = new ScanManager();
    scan_system = static_cast<jsScanSystem>(manager);
  } catch (std::exception &e) {
    (void)e;
    scan_system = nullptr;
  }

  return scan_system;
}

EXPORTED
void jsScanSystemFree(jsScanSystem scan_system)
{
  if (nullptr == scan_system) {
    return;
  }

  try {
    if (jsScanSystemIsScanning(scan_system)) {
      jsScanSystemStopScanning(scan_system);
    }

    if (jsScanSystemIsConnected(scan_system)) {
      jsScanSystemDisconnect(scan_system);
    }

    ScanManager *manager = static_cast<ScanManager *>(scan_system);
    delete manager;
  } catch (std::exception &e) {
    (void)e;
  }

  try {
    if (0 != _network_init_count) {
      NetworkInterface::FreeSystem();
      _network_init_count--;
    }
  } catch (std::exception &e) {
    (void)e;
  }
}

EXPORTED
jsScanHead jsScanSystemCreateScanHead(jsScanSystem scan_system, uint32_t serial,
                                      uint32_t id)
{
  jsScanHead scan_head = nullptr;

  if (nullptr == scan_system) {
    return nullptr;
  }

  try {
    ScanManager *manager = static_cast<ScanManager *>(scan_system);
    if (false == manager->IsConnected()) {
      ScanHead *s = manager->CreateScanner(serial, id);
      scan_head = static_cast<jsScanHead>(s);
    }
  } catch (std::exception &e) {
    (void)e;
    scan_head = nullptr;
  }

  return scan_head;
}

EXPORTED
jsScanHead jsScanSystemGetScanHeadById(jsScanSystem scan_system, uint32_t id)
{
  jsScanHead scan_head = nullptr;

  if (nullptr == scan_system) {
    return nullptr;
  }

  try {
    ScanManager *manager = static_cast<ScanManager *>(scan_system);
    ScanHead *s = manager->GetScanHeadById(id);
    scan_head = static_cast<jsScanHead>(s);
  } catch (std::exception &e) {
    (void)e;
    scan_head = nullptr;
  }

  return scan_head;
}

EXPORTED
jsScanHead jsScanSystemGetScanHeadBySerial(jsScanSystem scan_system,
                                           uint32_t serial)
{
  jsScanHead scan_head = nullptr;

  if (nullptr == scan_system) {
    return nullptr;
  }

  try {
    ScanManager *manager = static_cast<ScanManager *>(scan_system);
    ScanHead *s = manager->GetScanHeadBySerial(serial);
    scan_head = static_cast<jsScanHead>(s);
  } catch (std::exception &e) {
    (void)e;
    scan_head = nullptr;
  }

  return scan_head;
}

EXPORTED
int32_t jsScanSystemGetNumberScanHeads(jsScanSystem scan_system)
{
  int32_t r = 0;

  if (nullptr == scan_system) {
    return JS_ERROR_NULL_ARGUMENT;
  }

  try {
    ScanManager *manager = static_cast<ScanManager *>(scan_system);
    uint32_t sz = manager->GetNumberScanners();
    r = static_cast<int32_t>(sz);
  } catch (std::exception &e) {
    (void)e;
    r = JS_ERROR_INTERNAL;
  }

  return r;
}

#if 0
// NOTE: These functions were determined not be necessary for the customer API
// at this point. Can be added back into the API in the future.
/**
 * @brief Removes a scan head from being managed by a given `jsScanSystem`.
 *
 * @param scan_system Reference to system that owns the scan head.
 * @param scan_head Reference to scan head to be removed.
 * @return `0` on success, negative value on error.
 */
EXPORTED
int32_t jsScanSystemRemoveScanHead(jsScanSystem scan_system,
  jsScanHead scan_head);

/**
 * @brief Removes a scan head from being managed by a given `jsScanSystem`.
 *
 * @param scan_system Reference to system that owns the scan head.
 * @param id The id of the scan head to remove.
 * @return `0` on success, negative value on error.
 */
EXPORTED
int32_t jsScanSystemRemoveScanHeadById(jsScanSystem scan_system, uint32_t id);

/**
 * @brief Removes a scan head from being managed by a given `jsScanSystem`.
 *
 * @param scan_system Reference to system that owns the scan head.
 * @param serial The serial number of the scan head to remove.
 * @return `0` on success, negative value on error.
 */
EXPORTED
int32_t jsScanSystemRemoveScanHeadBySerial(jsScanSystem scan_system,
  uint32_t serial);

/**
 * @brief Removes all scan heads being managed by a given `jsScanSystem`.
 *
 * @param scan_system Reference to system of scan heads.
 * @return `0` on success, negative value on error.
 */
EXPORTED
int32_t jsScanSystemRemoveAllScanHeads(jsScanSystem scan_system);


EXPORTED
int32_t jsScanSystemRemoveScanHeadById(jsScanSystem scan_system, uint32_t id)
{
  int32_t r = 0;

  if (nullptr == scan_system) {
    return JS_ERROR_NULL_ARGUMENT;
  }

  try {
    ScanManager *manager = static_cast<ScanManager*>(scan_system);
    ScanHead *s = manager->GetScanHead(id);
    if (nullptr == s) {
      r = JS_ERROR_INVALID_ARGUMENT;
    }
    else {
      manager->RemoveScanner(s);
    }
  } catch (std::exception &e) {
    (void) e;
    r = JS_ERROR_INTERNAL;
  }

  return r;
}

EXPORTED
int32_t jsScanSystemRemoveScanHeadBySerial(jsScanSystem scan_system,
  uint32_t serial)
{
  int32_t r = 0;

  if (nullptr == scan_system) {
    return JS_ERROR_NULL_ARGUMENT;
  }

  try {
    ScanManager *manager = static_cast<ScanManager*>(scan_system);
    manager->RemoveScanner(serial);
  } catch (std::exception &e) {
    (void) e;
    r = JS_ERROR_INTERNAL;
  }

  return r;
}

EXPORTED
int32_t jsScanSystemRemoveAllScanHeads(jsScanSystem scan_system)
{
  int32_t r;

  if (nullptr == scan_system) {
    return JS_ERROR_NULL_ARGUMENT;
  }

  try {
    ScanManager *manager = static_cast<ScanManager*>(scan_system);
    manager->RemoveAllScanners();
    r = 0;
  } catch (std::exception &e) {
    (void) e;
    r = JS_ERROR_INTERNAL;
  }

  return r;
}
#endif

EXPORTED
int32_t jsScanSystemConnect(jsScanSystem scan_system, int32_t timeout_s)
{
  int32_t r = 0;

  if (nullptr == scan_system) {
    return JS_ERROR_NULL_ARGUMENT;
  }

  try {
    ScanManager *manager = static_cast<ScanManager *>(scan_system);
    auto connected = manager->Connect(timeout_s);
    r = static_cast<int32_t>(connected.size());
  } catch (const VersionCompatibilityException &) {
    r = JS_ERROR_VERSION_COMPATIBILITY;
  } catch (std::exception &e) {
    (void)e;
    r = JS_ERROR_INTERNAL;
  }

  return r;
}

EXPORTED
int32_t jsScanSystemDisconnect(jsScanSystem scan_system)
{
  int32_t r = 0;

  if (nullptr == scan_system) {
    return JS_ERROR_NULL_ARGUMENT;
  }

  try {
    ScanManager *manager = static_cast<ScanManager *>(scan_system);
    manager->Disconnect();
  } catch (std::exception &e) {
    (void)e;
    r = JS_ERROR_INTERNAL;
  }

  return r;
}

EXPORTED
bool jsScanSystemIsConnected(jsScanSystem scan_system)
{
  bool is_connected = false;

  if (nullptr == scan_system) {
    return false;
  }

  try {
    ScanManager *manager = static_cast<ScanManager *>(scan_system);
    is_connected = manager->IsConnected();
  } catch (std::exception &e) {
    (void)e;
    is_connected = false;
  }

  return is_connected;
}

EXPORTED
double jsScanSystemGetMaxScanRate(jsScanSystem scan_system)
{
  double rate_hz = 0.0;

  if (nullptr == scan_system) {
    return rate_hz;
  }

  if (false == jsScanSystemIsConnected(scan_system)) {
    return kPinchotConstantMaxScanRate;
  }

  try {
    ScanManager *manager = static_cast<ScanManager *>(scan_system);
    rate_hz = manager->GetMaxScanRate();
  } catch (std::exception &e) {
    (void)e;
    rate_hz = -1.0;
  }

  return rate_hz;
}

EXPORTED
int32_t jsScanSystemStartScanning(jsScanSystem scan_system, double rate_hz,
                                  jsDataFormat fmt)
{
  ScanManager *manager = static_cast<ScanManager *>(scan_system);
  int32_t r = 0;

  if (nullptr == scan_system) {
    return JS_ERROR_NULL_ARGUMENT;
  } else if (INVALID_DOUBLE(rate_hz)) {
    return JS_ERROR_INVALID_ARGUMENT;
  } else if (false == jsScanSystemIsConnected(scan_system)) {
    return JS_ERROR_NOT_CONNECTED;
  }

  switch (fmt) {
  case (JS_DATA_FORMAT_XY_FULL_LM_FULL):
  case (JS_DATA_FORMAT_XY_HALF_LM_HALF):
  case (JS_DATA_FORMAT_XY_QUARTER_LM_QUARTER):
  case (JS_DATA_FORMAT_XY_FULL):
  case (JS_DATA_FORMAT_XY_HALF):
  case (JS_DATA_FORMAT_XY_QUARTER):
    break;
  default:
    return JS_ERROR_INVALID_ARGUMENT;
  }

  try {
    double rate_hz_max = manager->GetMaxScanRate();
    if (rate_hz > rate_hz_max) {
      r = JS_ERROR_INVALID_ARGUMENT;
    } else {
      manager->SetScanRate(rate_hz);
      manager->SetRequestedDataFormat(fmt);
      manager->StartScanning();
    }
  } catch (std::exception &e) {
    (void)e;
    r = JS_ERROR_INTERNAL;
  }

  return r;
}

EXPORTED
int32_t jsScanSystemStopScanning(jsScanSystem scan_system)
{
  ScanManager *manager = static_cast<ScanManager *>(scan_system);
  int32_t r = 0;

  if (nullptr == scan_system) {
    return JS_ERROR_NULL_ARGUMENT;
  } else if (false == jsScanSystemIsScanning(scan_system)) {
    return JS_ERROR_NOT_SCANNING;
  }

  try {
    manager->StopScanning();
  } catch (std::exception &e) {
    (void)e;
    r = JS_ERROR_INTERNAL;
  }

  return r;
}

EXPORTED
bool jsScanSystemIsScanning(jsScanSystem scan_system)
{
  bool is_scanning = false;

  if (nullptr == scan_system) {
    return false;
  }

  try {
    ScanManager *manager = static_cast<ScanManager *>(scan_system);
    is_scanning = manager->IsScanning();
  } catch (std::exception &e) {
    (void)e;
    is_scanning = false;
  }

  return is_scanning;
}

EXPORTED
jsScanHeadType jsScanHeadGetType(jsScanHead scan_head)
{
  jsScanHeadType type = JS_SCAN_HEAD_INVALID_TYPE;

  if (nullptr == scan_head) {
    return JS_SCAN_HEAD_INVALID_TYPE;
  } else if (false == jsScanHeadIsConnected(scan_head)) {
    return JS_SCAN_HEAD_INVALID_TYPE;
  }

  try {
    ScanHead *sh = static_cast<ScanHead *>(scan_head);
    type = sh->GetProductType();
  } catch (std::exception &e) {
    (void)e;
    type = JS_SCAN_HEAD_INVALID_TYPE;
  }

  return type;
}

EXPORTED
uint32_t jsScanHeadGetId(jsScanHead scan_head)
{
  uint32_t id = 0;

  if (nullptr == scan_head) {
    // make it super obvious that the ID is invalid
    return 0xFFFFFFFF;
  }

  try {
    ScanHead *sh = static_cast<ScanHead *>(scan_head);
    id = sh->GetId();
  } catch (std::exception &e) {
    (void)e;
    id = 0xFFFFFFFF;
  }

  return id;
}

EXPORTED
uint32_t jsScanHeadGetSerial(jsScanHead scan_head)
{
  uint32_t serial = 0;

  if (nullptr == scan_head) {
    // make it super obvious that the serial is invalid
    return 0xFFFFFFFF;
  }

  try {
    ScanHead *sh = static_cast<ScanHead *>(scan_head);
    serial = sh->GetSerialNumber();
  } catch (std::exception &e) {
    (void)e;
    serial = 0xFFFFFFFF;
  }

  return serial;
}

EXPORTED
int32_t jsScanHeadConfigure(jsScanHead scan_head, jsScanHeadConfiguration *cfg)
{
  return jsScanHeadSetConfiguration(scan_head, cfg);
}

EXPORTED
int32_t jsScanHeadSetConfiguration(jsScanHead scan_head,
                                   jsScanHeadConfiguration *cfg)
{
  int32_t r = 0;

  if (nullptr == scan_head) {
    return JS_ERROR_NULL_ARGUMENT;
  } else if (nullptr == cfg) {
    return JS_ERROR_NULL_ARGUMENT;
  }

  try {
    ScanHead *sh = static_cast<ScanHead *>(scan_head);
    ScanManager &manager = sh->GetScanManager();

    if (true == manager.IsScanning()) {
      return JS_ERROR_SCANNING;
    }

    sh->SetConfiguration(*cfg);
  } catch (std::range_error &e) {
    (void)e;
    r = JS_ERROR_INVALID_ARGUMENT;
  } catch (std::exception &e) {
    (void)e;
    r = JS_ERROR_INTERNAL;
  }

  return r;
}

EXPORTED
int32_t jsScanHeadGetConfiguration(jsScanHead scan_head,
                                   jsScanHeadConfiguration *cfg)
{
  int32_t r = 0;

  if (nullptr == scan_head) {
    return JS_ERROR_NULL_ARGUMENT;
  } else if (nullptr == cfg) {
    return JS_ERROR_NULL_ARGUMENT;
  }

  try {
    ScanHead *sh = static_cast<ScanHead *>(scan_head);
    *cfg = sh->GetConfiguration();
  } catch (std::exception &e) {
    (void)e;
    r = JS_ERROR_INTERNAL;
  }

  return r;
}

EXPORTED
int32_t jsScanHeadSetAlignment(jsScanHead scan_head, double roll_degrees,
                               double shift_x, double shift_y,
                               bool is_cable_downstream)
{
  int32_t r = 0;

  if (nullptr == scan_head) {
    return JS_ERROR_NULL_ARGUMENT;
  } else if (INVALID_DOUBLE(roll_degrees) || INVALID_DOUBLE(shift_x) ||
             INVALID_DOUBLE(shift_y)) {
    return JS_ERROR_INVALID_ARGUMENT;
  } else if (true == jsScanHeadIsConnected(scan_head)) {
    return JS_ERROR_CONNECTED;
  }

  try {
    ScanHead *sh = static_cast<ScanHead *>(scan_head);
    AlignmentParams alignment(roll_degrees, shift_x, shift_y,
                              is_cable_downstream);
    sh->SetAlignment(JS_CAMERA_A, alignment);
    sh->SetAlignment(JS_CAMERA_B, alignment);
  } catch (std::exception &e) {
    (void)e;
    r = JS_ERROR_INTERNAL;
  }

  return r;
}

EXPORTED
int32_t jsScanHeadSetAlignmentCamera(jsScanHead scan_head, jsCamera camera,
                                     double roll_degrees, double shift_x,
                                     double shift_y, bool is_cable_downstream)
{
  int32_t r = 0;

  if (nullptr == scan_head) {
    return JS_ERROR_NULL_ARGUMENT;
  } else if (INVALID_DOUBLE(roll_degrees) || INVALID_DOUBLE(shift_x) ||
             INVALID_DOUBLE(shift_y)) {
    return JS_ERROR_INVALID_ARGUMENT;
  } else if (true == jsScanHeadIsConnected(scan_head)) {
    return JS_ERROR_CONNECTED;
  }

  try {
    ScanHead *sh = static_cast<ScanHead *>(scan_head);
    AlignmentParams alignment(roll_degrees, shift_x, shift_y,
                              is_cable_downstream);

    if (static_cast<uint32_t>(camera) < sh->GetNumberCameras()) {
      sh->SetAlignment(camera, alignment);
    } else {
      r = JS_ERROR_INVALID_ARGUMENT;
    }
  } catch (std::exception &e) {
    (void)e;
    r = JS_ERROR_INTERNAL;
  }

  return r;
}

EXPORTED
int32_t jsScanHeadGetAlignmentCamera(jsScanHead scan_head, jsCamera camera,
                                     double *roll_degrees, double *shift_x,
                                     double *shift_y, bool *is_cable_downstream)
{
  int32_t r = 0;

  if ((nullptr == scan_head) || (nullptr == roll_degrees) ||
      (nullptr == shift_x) || (nullptr == shift_x) || (nullptr == shift_y) ||
      (nullptr == is_cable_downstream)) {
    return JS_ERROR_NULL_ARGUMENT;
  }

  try {
    ScanHead *sh = static_cast<ScanHead *>(scan_head);
    AlignmentParams alignment = sh->GetAlignment(camera);
    *roll_degrees = alignment.GetRoll();
    *shift_x = alignment.GetShiftX();
    *shift_y = alignment.GetShiftY();
    *is_cable_downstream = alignment.GetFlipX();
  } catch (std::exception &e) {
    (void)e;
    r = JS_ERROR_INTERNAL;
  }

  return r;
}

EXPORTED
int32_t jsScanHeadSetWindowRectangular(jsScanHead scan_head, double window_top,
                                       double window_bottom, double window_left,
                                       double window_right)
{
  int32_t r = 0;

  if (nullptr == scan_head) {
    return JS_ERROR_NULL_ARGUMENT;
  } else if (INVALID_DOUBLE(window_top) || INVALID_DOUBLE(window_bottom) ||
             INVALID_DOUBLE(window_left) || INVALID_DOUBLE(window_right)) {
    return JS_ERROR_INVALID_ARGUMENT;
  } else if (true == jsScanHeadIsConnected(scan_head)) {
    return JS_ERROR_CONNECTED;
  }

  try {
    ScanHead *sh = static_cast<ScanHead *>(scan_head);
    ScanWindow window(window_top, window_bottom, window_left, window_right);
    sh->SetWindow(window);
  } catch (std::range_error &e) {
    (void)e;
    r = JS_ERROR_INVALID_ARGUMENT;
  } catch (std::exception &e) {
    (void)e;
    r = JS_ERROR_INTERNAL;
  }

  return r;
}

EXPORTED
bool jsScanHeadIsConnected(jsScanHead scan_head)
{
  bool is_connected = false;

  if (nullptr == scan_head) {
    return false;
  }

  try {
    ScanHead *sh = static_cast<ScanHead *>(scan_head);
    ScanManager &mgr = sh->GetScanManager();

    StatusMessage message = sh->GetStatusMessage();
    auto timestamp = message.GetGlobalTime();
    if (0 != timestamp) {
      is_connected = true;
    }
  } catch (std::exception &e) {
    (void)e;
    is_connected = false;
  }

  return is_connected;
}

EXPORTED
int32_t jsScanHeadGetProfilesAvailable(jsScanHead scan_head)
{
  int32_t r = 0;

  if (nullptr == scan_head) {
    return JS_ERROR_NULL_ARGUMENT;
  }

  try {
    ScanHead *sh = static_cast<ScanHead *>(scan_head);
    uint32_t count = sh->AvailableProfiles();
    r = static_cast<int32_t>(count);
  } catch (std::exception &e) {
    (void)e;
    r = JS_ERROR_INTERNAL;
  }

  return r;
}

EXPORTED
int32_t jsScanHeadWaitUntilProfilesAvailable(jsScanHead scan_head,
                                             uint32_t count,
                                             uint32_t timeout_us)
{
  int32_t r = 0;

  if (nullptr == scan_head) {
    return JS_ERROR_NULL_ARGUMENT;
  } else if (JS_SCAN_HEAD_PROFILES_MAX < count) {
    count = JS_SCAN_HEAD_PROFILES_MAX;
  }

  try {
    ScanHead *sh = static_cast<ScanHead *>(scan_head);
    r = sh->WaitUntilAvailableProfiles(count, timeout_us);
  } catch (std::exception &e) {
    (void)e;
    r = JS_ERROR_INTERNAL;
  }

  return r;
}

EXPORTED
int32_t jsScanHeadClearProfiles(jsScanHead scan_head)
{
  int32_t r = 0;

  if (nullptr == scan_head) {
    return JS_ERROR_NULL_ARGUMENT;
  }

  try {
    ScanHead *sh = static_cast<ScanHead *>(scan_head);
    sh->ClearProfiles();
  } catch (std::exception &e) {
    (void)e;
    r = JS_ERROR_INTERNAL;
  }

  return r;
}

EXPORTED
int32_t jsScanHeadGetRawProfiles(jsScanHead scan_head, jsRawProfile *profiles,
                                 uint32_t max_profiles)
{
  int32_t r = 0;

  if (nullptr == scan_head) {
    return JS_ERROR_NULL_ARGUMENT;
  } else if (nullptr == profiles) {
    return JS_ERROR_NULL_ARGUMENT;
  }

  try {
    ScanHead *sh = static_cast<ScanHead *>(scan_head);
    // TODO: FKS-219
    // We should retool the internal C++ code to make this whole process less
    // labor intensive. Ideally we could just do a straight memcpy.
    auto p = sh->GetProfiles(static_cast<int>(max_profiles));
    uint32_t total = (max_profiles < static_cast<uint32_t>(p.size()))
                       ? max_profiles
                       : static_cast<uint32_t>(p.size());

    for (uint32_t m = 0; m < total; m++) {
      profiles[m].scan_head_id = p[m]->GetScanHeadId();
      profiles[m].camera = p[m]->GetCamera();
      profiles[m].laser = p[m]->GetLaser();
      profiles[m].timestamp_ns = p[m]->GetTimestamp();
      profiles[m].laser_on_time_us = p[m]->GetLaserOnTime();
      profiles[m].format = sh->GetDataFormat();

      std::pair<uint32_t, uint32_t> pkt_info = p[m]->GetUDPPacketInfo();
      profiles[m].udp_packets_received = pkt_info.first;
      profiles[m].udp_packets_expected = pkt_info.second;

      memset(profiles[m].encoder_values, 0, sizeof(int64_t) * JS_ENCODER_MAX);
      std::vector<int64_t> e = p[m]->GetEncoderValues();
      std::copy(e.begin(), e.end(), profiles[m].encoder_values);
      profiles[m].num_encoder_values = static_cast<uint32_t>(e.size());
      assert(profiles[m].num_encoder_values < JS_ENCODER_MAX);

      auto data = p[m]->Data();
      // TODO: We shouldn't need to do this, but for now check to be safe.
      assert(data.size() == JS_RAW_PROFILE_DATA_LEN);
      std::copy(data.begin(), data.end(), profiles[m].data);
      profiles[m].data_len = static_cast<uint32_t>(data.size());
      profiles[m].data_valid_brightness = p[m]->GetNumberValidBrightness();
      profiles[m].data_valid_xy = p[m]->GetNumberValidGeometry();
    }
    // return number of profiles copied
    r = static_cast<int32_t>(total);
  } catch (std::exception &e) {
    (void)e;
    r = JS_ERROR_INTERNAL;
  }

  return r;
}

EXPORTED
int32_t jsScanHeadGetProfiles(jsScanHead scan_head, jsProfile *profiles,
                              uint32_t max_profiles)
{
  int32_t r = 0;

  if (nullptr == scan_head) {
    return JS_ERROR_NULL_ARGUMENT;
  } else if (nullptr == profiles) {
    return JS_ERROR_NULL_ARGUMENT;
  }

  try {
    ScanHead *sh = static_cast<ScanHead *>(scan_head);
    // TODO: FKS-219
    // We should retool the internal C++ code to make this whole process less
    // labor intensive. Ideally we could just do a straight memcpy.
    auto p = sh->GetProfiles(static_cast<int>(max_profiles));
    uint32_t total = (max_profiles < static_cast<uint32_t>(p.size()))
                       ? max_profiles
                       : static_cast<uint32_t>(p.size());

    for (uint32_t m = 0; m < total; m++) {
      profiles[m].scan_head_id = p[m]->GetScanHeadId();
      profiles[m].camera = p[m]->GetCamera();
      profiles[m].laser = p[m]->GetLaser();
      profiles[m].timestamp_ns = p[m]->GetTimestamp();
      profiles[m].laser_on_time_us = p[m]->GetLaserOnTime();
      profiles[m].format = sh->GetDataFormat();

      std::pair<uint32_t, uint32_t> pkt_info = p[m]->GetUDPPacketInfo();
      profiles[m].udp_packets_received = pkt_info.first;
      profiles[m].udp_packets_expected = pkt_info.second;

      memset(profiles[m].encoder_values, 0, sizeof(int64_t) * JS_ENCODER_MAX);
      std::vector<int64_t> e = p[m]->GetEncoderValues();
      std::copy(e.begin(), e.end(), profiles[m].encoder_values);
      profiles[m].num_encoder_values = static_cast<uint32_t>(e.size());
      assert(profiles[m].num_encoder_values < JS_ENCODER_MAX);

      auto data = p[m]->Data();
      unsigned int stride = _data_format_to_stride(profiles[m].format);
      unsigned int p = 0;
      for (unsigned int n = 0; n < data.size(); n += stride) {
        if ((JS_PROFILE_DATA_INVALID_XY != data[n].x) ||
            (JS_PROFILE_DATA_INVALID_XY != data[n].y)) {
          // Note: Only need to check X/Y since we only support data types with
          // X/Y coordinates alone or X/Y coordinates with brightness.
          profiles[m].data[p++] = data[n];
        }
      }
      profiles[m].data_len = p;
    }
    // return number of profiles copied
    r = static_cast<int32_t>(total);
  } catch (std::exception &e) {
    (void)e;
    r = JS_ERROR_INTERNAL;
  }

  return r;
}

EXPORTED
int32_t jsScanHeadGetCameraImage(jsScanHead scan_head, jsCamera camera,
                                 bool enable_lasers, jsCameraImage *image)
{
  int32_t r = JS_ERROR_INTERNAL;

  if (nullptr == scan_head) {
    return JS_ERROR_NULL_ARGUMENT;
  } else if (nullptr == image) {
    return JS_ERROR_NULL_ARGUMENT;
  }

  try {
    ScanHead *sh = static_cast<ScanHead *>(scan_head);
    ScanManager &manager = sh->GetScanManager();

    // Only allow image capture if connected and not currently scanning.
    if (false == manager.IsConnected()) {
      r = JS_ERROR_NOT_CONNECTED;
    } else if (true == manager.IsScanning()) {
      r = JS_ERROR_SCANNING;
    } else if (camera >= sh->GetNumberCameras()) {
      r = JS_ERROR_INVALID_ARGUMENT;
    } else {
      jsScanHeadCapabilities capabilities;
      // TODO: need to get the scan head type from somewhere
      jsGetScanHeadCapabilities(JS_SCAN_HEAD_JS50WX, &capabilities);
      auto num_cameras = capabilities.num_cameras;

      // use temporary config to enable/disable lasers for image capture
      jsScanHeadConfiguration user_config = sh->GetConfiguration();
      jsScanHeadConfiguration config = user_config;
      if (false == enable_lasers) {
        config.laser_on_time_max_us = 0;
        config.laser_on_time_def_us = 0;
        config.laser_on_time_min_us = 0;
      } else {
        // make sure laser on time does not exceed camera exposure as it
        // could violate an assumption in the scan server or FPGA
        uint32_t laser_max = config.laser_on_time_max_us;
        uint32_t laser_def = config.laser_on_time_def_us;
        uint32_t laser_min = config.laser_on_time_min_us;

        if (laser_max > config.camera_exposure_time_max_us) {
          laser_max = config.camera_exposure_time_max_us;
        }
        if (laser_def > config.camera_exposure_time_def_us) {
          laser_def = config.camera_exposure_time_def_us;
        }
        if (laser_min > config.camera_exposure_time_min_us) {
          laser_min = config.camera_exposure_time_min_us;
        }

        config.laser_on_time_max_us = laser_max;
        config.laser_on_time_def_us = laser_def;
        config.laser_on_time_min_us = laser_min;
      }

     manager.RequestImages(sh, config);

      // copy the image into the jsCameraImage struct
      auto p = sh->GetProfiles(num_cameras);
      for (unsigned int m = 0; m < p.size(); m++) {
        if (p[m]->GetCamera() == camera) {
          image->scan_head_id = p[m]->GetScanHeadId();
          image->camera = p[m]->GetCamera();
          image->timestamp_ns = p[m]->GetTimestamp();
          image->camera_exposure_time_us = p[m]->GetExposureTime();
          image->laser_on_time_us = p[m]->GetLaserOnTime();

          memset(image->encoder_values, 0, sizeof(int64_t) * JS_ENCODER_MAX);
          std::vector<int64_t> e = p[m]->GetEncoderValues();
          std::copy(e.begin(), e.end(), image->encoder_values);
          image->num_encoder_values = static_cast<uint32_t>(e.size());
          assert(image->num_encoder_values < JS_ENCODER_MAX);

          image->image_height = JS_CAMERA_IMAGE_DATA_MAX_HEIGHT;
          image->image_width = JS_CAMERA_IMAGE_DATA_MAX_WIDTH;

          std::vector<uint8_t> data = p[m]->Image();
          // TODO: we shouldn't need to do this, but for now check to be safe
          assert(data.size() == JS_CAMERA_IMAGE_DATA_LEN);
          std::copy(data.begin(), data.end(), image->data);

          r = 0;
          break;
        } else {
          // not the camera that the user requested
          r = JS_ERROR_INTERNAL;
        }
      }
    }
  } catch (std::exception &e) {
    (void)e;
    r = JS_ERROR_INTERNAL;
  }

  return r;
}

EXPORTED
int32_t jsScanHeadGetStatus(jsScanHead scan_head, jsScanHeadStatus *status)
{
  int32_t r = 0;

  if (nullptr == scan_head) {
    return JS_ERROR_NULL_ARGUMENT;
  } else if (nullptr == status) {
    return JS_ERROR_NULL_ARGUMENT;
  }

  try {
    ScanHead *sh = static_cast<ScanHead *>(scan_head);
    ScanManager &manager = sh->GetScanManager();
    StatusMessage msg = sh->GetStatusMessage();
    ScanHeadTemperatures temps = sh->GetTemperatures();

    if (true == manager.IsScanning()) {
      return JS_ERROR_SCANNING;
    } else if (false == manager.IsConnected()) {
      return JS_ERROR_NOT_CONNECTED;
    }

    status->global_time_ns = msg.GetGlobalTime();
    status->num_profiles_sent = msg.GetNumProfilesSent();

    std::fill(std::begin(status->encoder_values),
              std::begin(status->encoder_values) + JS_ENCODER_MAX, 0);

    std::vector<int64_t> e = msg.GetEncoders();
    std::copy(e.begin(), e.end(), status->encoder_values);
    status->num_encoder_values = static_cast<uint32_t>(e.size());
    assert(status->num_encoder_values < JS_ENCODER_MAX);

    for (int n = 0; n < JS_CAMERA_MAX; n++) {
      status->camera_pixels_in_window[n] = msg.GetPixelsInWindow(n);
      status->camera_temp[n] = static_cast<int32_t>(temps.camera_temp_c[n]);
    }
    status->mainboard_temp = static_cast<int32_t>(temps.mainboard_temp_c);

    VersionInformation ver = msg.GetVersionInformation();
    status->firmware_version_major = ver.major;
    status->firmware_version_minor = ver.minor;
    status->firmware_version_patch = ver.patch;
  } catch (std::exception &e) {
    (void)e;
    r = JS_ERROR_INTERNAL;
  }

  return r;
}
