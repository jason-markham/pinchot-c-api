/**
 * Copyright (c) JoeScan Inc. All Rights Reserved.
 *
 * Licensed under the BSD 3 Clause License. See LICENSE.txt in the project
 * root for license information.
 */

#ifndef JOESCAN_SCAN_HEAD_H
#define JOESCAN_SCAN_HEAD_H

#include "ScanHeadTemperatures.hpp"
#include "ScanManager.hpp"
#include "ScanWindow.hpp"

#include "boost/circular_buffer.hpp"
#include "joescan_pinchot.h"

#include <string>
#include <vector>

namespace joescan {

class ScanHead {
 public:
  /**
   * Initializes a `ScanHead` object.
   *
   * @param manager Reference to scan manager.
   * @param serial_number The serial number of the scan head to create.
   * @param id The unique identifier to associate with the scan head.
   */
  ScanHead(ScanManager &manager, uint32_t serial_number, uint32_t id);
  ~ScanHead();

  /**
   * Gets the scan head product type.
   *
   * @return Enum value representing scan head type.
   */
  jsScanHeadType GetProductType() const;

  /**
   * Gets the serial number of the scan head.
   *
   * @return String representation of the serial number.
   */
  uint32_t GetSerialNumber() const;

  /**
   * Gets the ID of the scan head.
   *
   * @return Numeric ID of the scan head.
   */
  uint32_t GetId() const;

  /**
   * Gets the number of cameras available on the scan head.
   *
   * @return Number of valid cameras on the scan head.
   */
  uint32_t GetNumberCameras();

  /** Gets the binary representation of the IP address of the scan head.
   * Note, this can be converted to a string representation by using the
   * `inet_ntop` function.
   *
   * @return The binary representation of the scan head's IP address.
   */
  uint32_t GetIpAddress() const;

  /**
   * Gets the port used to receive UDP data from the scan head.
   *
   * @return The port number.
   */
  int GetReceivePort() const;

  /**
   * Causes the receive port to become active and begin listening for UDP
   * messages from the scan head.
   */
  void ReceiveStart(uint32_t scan_interval_us=0);

  /**
   * Stops receiving UDP messages from the receive port.
   */
  void ReceiveStop();

  /**
   * Returns the number of profiles that are available to be read from calling
   * the `GetProfiles` function.
   *
   * @return The number of profiles able to be read.
   */
  uint32_t AvailableProfiles();

  /**
   * Blocks until the number of profiles requested are available to be read.
   *
   * @param count The desired number of profiles to wait for.
   * @param timeout_us The max time to wait for in microseconds.
   * @return The number of profiles able to be read.
   */
  uint32_t WaitUntilAvailableProfiles(uint32_t count, uint32_t timeout_us);

  /**
   * Obtains up to the number of scanning profiles requested from the scan
   * head. Note, if the total number of profiles returned from the scan
   * head is less than what is requested, only the actual number of profiles
   * available will be returned in the vector.
   *
   * @param count The maximum number of profiles to return.
   * @return Vector holding references to profile data.
   */
  std::vector<std::shared_ptr<Profile>> GetProfiles(uint32_t count);

  /**
   * Empties the circular buffer used to store received profiles from the
   * scan head.
   */
  void ClearProfiles();

  /**
   * Obtains the last reported status message from a scan head. Note, status
   * messages are only sent by the scan head when not actively scanning.
   *
   * @return The last reported status message.
   */
  StatusMessage GetStatusMessage();

  /**
   * Clears out the last reported status message from a scan head.
   */
  void ClearStatusMessage();

  /**
   * Gets the scan manager that owns this scan head.
   *
   * @return Reference to `ScanManager` object.
   */
  ScanManager &GetScanManager();

  /**
   * Sets the alignment settings for the scan head.
   *
   * @param alignment The alignment settings.
   */
  void SetAlignment(jsCamera camera, AlignmentParams &alignment);

  /**
   * Gets the alignment settings for the scan head.
   *
   * @return The alignment settings.
   */
  AlignmentParams &GetAlignment(jsCamera camera);

  /**
   * Configures a scan head according to the specified parameters.
   *
   * @param config A reference to the configuration parameters.
   */
  void SetConfiguration(jsScanHeadConfiguration &cfg);

  /**
   * Gets the current configuration of the scan head.
   *
   * @return The configuration of the scan head.
   */
  jsScanHeadConfiguration GetConfiguration() const;

  /**
   * Sets the format of the data being presented to the end user
   *
   * @param format The format the data will be presented in
   */
  void SetDataFormat(jsDataFormat format);

  /**
   * Gets the format of the data being presented to the end user
   *
   * @return The format the data will be presented in
   */
  jsDataFormat GetDataFormat() const;

  /**
   * Gets the temperature readings for the scan head.
   *
   * @return Temperature readings.
   */
  ScanHeadTemperatures GetTemperatures();

  /**
   * Sets the window to be used for scanning with the scan head.
   *
   * @param window The scan window.
   */
  void SetWindow(ScanWindow &window);

  /**
   * Gets the currently configured scan window.
   *
   * @return The scan window.
   */
  ScanWindow &GetWindow();

 private:
  static const int kMaxCircularBufferSize = JS_SCAN_HEAD_PROFILES_MAX;
  // The JS-50 theoretical max packet size is 8k plus header, in reality the
  // max size is 1456 * 4 + header. Using 6k.
  static const int kMaxPacketSize = 6144;
  // JS-50 in image mode will have 4 rows of 1456 pixels for each packet.
  static const int kImageDataSize = 4 * 1456;
  // Port used to access REST interface
  static const uint32_t kRESTport = 8080;

  static const uint32_t kMaxAverageIntensity = 255;
  static const uint32_t kMaxSaturationPercentage = 100;
  static const uint32_t kMaxSaturationThreshold = 1023;
  static const uint32_t kMaxLaserDetectionThreshold = 1023;
  static const uint32_t kMinLaserOnTimeUsec = 15;
  static const uint32_t kMaxLaserOnTimeUsec = 650000;
  static const uint32_t kMinCameraExposureUsec = 15;
  static const uint32_t kMaxCameraExposureUsec = 2000000;

  void PushProfile(std::shared_ptr<Profile> profile);
  void PushStatus(StatusMessage status);
  void ProcessPacket(DataPacket &packet);
  void ReceiveMain();

  ScanManager &m_scan_manager;
  AlignmentParams m_alignment[JS_CAMERA_MAX];
  ScanWindow m_window;
  StatusMessage m_status;
  jsScanHeadConfiguration m_config;
  jsDataFormat m_format;
  jsScanHeadType m_product_type;

  boost::circular_buffer<std::shared_ptr<Profile>> m_circ_buffer;
  std::shared_ptr<Profile> m_profile_ptr;
  std::condition_variable m_thread_sync;
  std::mutex m_mutex;
  std::thread m_receiver;

  uint32_t m_serial_number;
  uint32_t m_ip_address;
  uint32_t m_id;
  SOCKET m_fd;
  int m_port;
  int32_t m_active_count;
  uint32_t m_scan_interval_us;
  uint8_t *m_packet_buf;
  uint32_t m_packet_buf_len;
  uint64_t m_packets_received;
  uint32_t m_packets_received_for_profile;
  uint64_t m_complete_profiles_received;
  uint64_t m_expected_packets_received;
  uint64_t m_expected_profiles_received;
  uint32_t m_last_profile_source;
  uint64_t m_last_profile_timestamp;
  bool m_is_data_available_condition_enabled;
};
} // namespace joescan

#endif // JOESCAN_SCAN_HEAD_H
