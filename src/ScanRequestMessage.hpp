/**
 * Copyright (c) JoeScan Inc. All Rights Reserved.
 *
 * Licensed under the BSD 3 Clause License. See LICENSE.txt in the project
 * root for license information.
 */

#ifndef JOESCAN_SCAN_REQUEST_MESSAGE_H
#define JOESCAN_SCAN_REQUEST_MESSAGE_H

#include <chrono>
#include <set>

#include "Enums.hpp"
#include "NetworkTypes.hpp"
#include "joescan_pinchot.h"

namespace joescan {

class ScanRequest {
 public:
  ScanRequest(jsDataFormat format, uint32_t client_ip, int client_port,
              int scan_head_id, uint32_t interval, uint32_t scanCount,
              const jsScanHeadConfiguration &config);
  ScanRequest(const Datagram &datagram);
  ScanRequest() = default;
  ~ScanRequest() = default;

  Datagram Serialize(uint8_t request_sequence);
  static ScanRequest Deserialize(const Datagram &datagram);

  inline int Length() const;

  inline UdpPacketType GetRequestType() const;
  inline uint8_t GetScanHeadId() const;
  inline uint8_t GetCameraId() const;
  inline uint8_t GetLaserId() const;
  inline uint8_t GetFlags() const;
  inline uint8_t GetRequestSequence() const;

  inline uint32_t GetMinimumLaserExposure() const;
  inline uint32_t GetDefaultLaserExposure() const;
  inline uint32_t GetMaximumLaserExposure() const;

  inline uint32_t GetMinimumCameraExposure() const;
  inline uint32_t GetDefaultCameraExposure() const;
  inline uint32_t GetMaximumCameraExposure() const;

  inline uint32_t GetLaserDetectionThreshold() const;
  inline uint32_t GetSaturationThreshold() const;
  inline uint32_t GetSaturationPercent() const;
  inline uint32_t GetAverageImageIntensity() const;

  inline uint32_t GetScanInterval() const;
  inline uint32_t GetScanOffset() const;
  inline uint32_t GetNumberOfScans() const;

  inline uint32_t GetClientAddress() const;
  inline uint16_t GetClientPort() const;

  inline uint16_t GetDataTypes() const;
  inline uint16_t GetStartColumn() const;
  inline uint16_t GetEndColumn() const;
  inline const std::vector<uint16_t> &GetStepValues() const;

  void SetDataTypesAndSteps(DataType types, std::vector<uint16_t> steps);
  void SetLaserExposure(uint32_t min, uint32_t def, uint32_t max);
  void SetCameraExposure(uint32_t min, uint32_t def, uint32_t max);

  bool operator==(const ScanRequest &other) const;
  bool operator!=(const ScanRequest &other) const;

 protected:
  uint16_t m_magic;
  UdpPacketType m_request_type = UdpPacketType::StartScanning;

  uint8_t m_scan_head_id;
  uint8_t m_camera_id;
  uint8_t m_laser_id;

  // deprecated exposure setting, interleaved or simultaneous
  uint8_t DEPRECATED_DO_NOT_USE;

  // Laser exposure, 3 * 4 bytes
  uint32_t m_laser_exposure_min_us;
  uint32_t m_laser_exposure_def_us;
  uint32_t m_laser_exposure_max_us;

  // Camera exposure, 3 * 4 bytes
  uint32_t m_camera_exposure_min_us;
  uint32_t m_camera_exposure_def_us;
  uint32_t m_camera_exposure_max_us;

  // Autoexposure pixel and percentage controls, 4 * 4 bytes
  // Minimum brightness value a pixel must reach for the FPGA to register it as
  // the laser peak
  uint32_t m_laser_detection_threshold;

  // Minimum brightness value a pixel must reach for the FPGA to consider the
  // pixel fully saturated
  uint32_t m_saturation_threshold;

  // Target % of fully saturated pixels within the scan window that the scan
  // autoexposure attempts to reach
  uint32_t m_saturation_percentage;

  // Average pixel brightness target that the image autoexposure attempts to
  // reach
  uint32_t m_average_intensity;

  // Scan start/duration (server/FPGA will determine the next actual viable
  // start time from the interval), 3 * 4 bytes
  // Interval in microseconds between the start of each scan
  uint32_t m_scan_interval_us;
  // Offset in microseconds from the start of the natural scan interval
  // boundary that this device operates (i.e., the trigger phase offset time)
  uint32_t m_scan_offset_us;
  // Total number of scans/images to collect for this command
  uint32_t m_number_of_scans;

  // Data routing, 4 + 2 bytes
  // IP address of client PC
  uint32_t m_client_ip;
  // Port on which the client is listening for data packets
  uint16_t m_client_port;

  // Multiple scan request packets may be generated during one start/stop scan
  // pair from the API.  All scan requests related to the same API-level scan
  // command are given a common sequence number, so that the scan server can
  // handle any edge case where the user stops & restarts scanning rapidly.  By
  // observing the new request sequence number, the scan server can
  // unambiguously know to treat the last request as the start of something new.

  // Misc stuff, 2 * 1 bytes
  // Currently unused
  uint8_t m_flags;
  // Used to indicate that this request is/isn't connected with
  // prior/subsequent scans
  uint8_t m_request_sequence;

  // Data format, (3 + # of types) * 2 bytes
  // Bitmask of requested data return values
  uint16_t m_data_types;
  // First camera column to return data from
  uint16_t m_start_col;
  // Last camera column to return data from
  uint16_t m_end_col;

  // The step values are ordered by the profile data type, with lowest-value
  // type's skip first, increasing.  So for example, if a request has brightness
  // and width data types, the brightness skip value is first in the vector,
  // followed by the width skip value.
  std::vector<uint16_t> m_steps;

  // Overall: 74 + # of types * 2 bytes (m_steps)
};

inline int ScanRequest::Length() const
{
  size_t sz = 74 + m_steps.size() * 2;
  return static_cast<int>(sz);
}

inline UdpPacketType ScanRequest::GetRequestType() const
{
  return m_request_type;
}

inline uint8_t ScanRequest::GetScanHeadId() const
{
  return m_scan_head_id;
}

inline uint8_t ScanRequest::GetCameraId() const
{
  return m_camera_id;
}

inline uint8_t ScanRequest::GetLaserId() const
{
  return m_laser_id;
}

inline uint8_t ScanRequest::GetFlags() const
{
  return m_flags;
}

inline uint8_t ScanRequest::GetRequestSequence() const
{
  return m_request_sequence;
}

inline uint32_t ScanRequest::GetMinimumLaserExposure() const
{
  return m_laser_exposure_min_us;
}

inline uint32_t ScanRequest::GetDefaultLaserExposure() const
{
  return m_laser_exposure_def_us;
}

inline uint32_t ScanRequest::GetMaximumLaserExposure() const
{
  return m_laser_exposure_max_us;
}

inline uint32_t ScanRequest::GetMinimumCameraExposure() const
{
  return m_camera_exposure_min_us;
}

inline uint32_t ScanRequest::GetDefaultCameraExposure() const
{
  return m_camera_exposure_def_us;
}

inline uint32_t ScanRequest::GetMaximumCameraExposure() const
{
  return m_camera_exposure_max_us;
}

inline uint32_t ScanRequest::GetLaserDetectionThreshold() const
{
  return m_laser_detection_threshold;
}

inline uint32_t ScanRequest::GetSaturationThreshold() const
{
  return m_saturation_threshold;
}

inline uint32_t ScanRequest::GetSaturationPercent() const
{
  return m_saturation_percentage;
}

inline uint32_t ScanRequest::GetAverageImageIntensity() const
{
  return m_average_intensity;
}

inline uint32_t ScanRequest::GetScanInterval() const
{
  return m_scan_interval_us;
}

inline uint32_t ScanRequest::GetScanOffset() const
{
  return m_scan_offset_us;
}

inline uint32_t ScanRequest::GetNumberOfScans() const
{
  return m_number_of_scans;
}

inline uint32_t ScanRequest::GetClientAddress() const
{
  return m_client_ip;
}

inline uint16_t ScanRequest::GetClientPort() const
{
  return m_client_port;
}

inline uint16_t ScanRequest::GetDataTypes() const
{
  return m_data_types;
}

inline uint16_t ScanRequest::GetStartColumn() const
{
  return m_start_col;
}

inline uint16_t ScanRequest::GetEndColumn() const
{
  return m_end_col;
}

inline const std::vector<uint16_t> &ScanRequest::GetStepValues() const
{
  return m_steps;
}

} // namespace joescan

#endif // JOESCAN_SCAN_REQUEST_H
