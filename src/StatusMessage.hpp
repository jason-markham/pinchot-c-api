/**
 * Copyright (c) JoeScan Inc. All Rights Reserved.
 *
 * Licensed under the BSD 3 Clause License. See LICENSE.txt in the project
 * root for license information.
 */

#ifndef JOESCAN_STATUS_MESSAGE_H
#define JOESCAN_STATUS_MESSAGE_H

#include <array>
#include <vector>

#include "NetworkTypes.hpp"
#include "VersionInformation.hpp"

namespace joescan {

class StatusMessage {
 public:
  /**
   * Initialize the status message.
   *
   * @param scan_head_ip The ip address of the scan head.
   * @param serial_number The serial number of the scan head.
   * @param max_scan_rate The maximum scan rate of the scan head.
   * @param version The version information of the scan head.
   */
  StatusMessage(uint32_t scan_head_ip, uint32_t serial_number,
                uint32_t max_scan_rate, VersionInformation version);

  /**
   * Initialize the status message using raw bytes stored in an array.
   *
   * @param bytes Raw bytes stored in array.
   * @param num_bytes The total number of bytes in the array.
   */
  StatusMessage(uint8_t *bytes, uint32_t num_bytes);

  StatusMessage();
  ~StatusMessage() = default;

  /**
   * Takes a status message and converts it to a raw binary format.
   *
   * @return A vector holding the binary form of the status message.
   */
  std::vector<uint8_t> Serialize() const;

  /**
   * Sets the client connection info.
   *
   * @param ip The client's IP address.
   * @param port The port number used by the client.
   */
  void SetClientAddressInfo(uint32_t ip, uint16_t port);

  /**
   * Sets the global time as reported by the FPGA in nanoseconds.
   *
   * @param global_time The time in nanoseconds.
   */
  void SetGlobalTime(uint64_t global_time);

  /**
   * Sets encoder values for the status message.
   *
   * @param encoders Vector of encoder values.
   */
  void SetEncoders(std::vector<int64_t> encoders);

  /**
   * Sets total number of pixels in the camera window for the status message.
   *
   * @param camera The camera's ID.
   * @param pixels The number of pixels in the camera's window.
   */
  void SetPixelsInWindow(int camera, int32_t pixels);

  /**
   * Sets the scan sync ID paired with the scan head for the status message.
   *
   * @param id The scan sync ID.
   */
  void SetScanSyncId(uint16_t id);

  /**
   * Stores the temperature for a given camera in the status message.
   *
   * @param camera The camera's ID.
   * @param temp The reported temperature in Celsius.
   */
  void SetCameraTemperature(int camera, int32_t temp);

  /**
   * Stores the maximum scan rate supported by the scan head in the status
   * message.
   *
   * @param scan_rate The max scan rate in hertz.
   */
  void SetMaxScanRate(uint32_t scan_rate);

  /**
   * Stores the number of packets sent by the scan head to client in the
   * status message.
   *
   * @param num_packets_sent The total number of UDP packets sent.
   */
  void SetNumPacketsSent(uint32_t num_packets_sent);

  /**
   * Stores the number of profiles sent by the scan head to client in the
   * status message.
   *
   * @param num_profiles_sent The total number of profiles sent.
   */
  void SetNumProfilesSent(uint32_t num_profiles_sent);

  /**
   * Stores the number of valid encoders
   *
   * @param num_valid The total number of valid encoders
   */
  void SetValidEncoders(uint8_t num_valid);

  /**
   * Stores the number of valid cameras
   *
   * @param num_valid The total number of valid cameras
   */
  void SetValidCameras(uint8_t num_valid);

  /**
   * Gets the version information of the scan head.
   *
   * @returns The scan head version information.
   */
  VersionInformation GetVersionInformation() const;

  /**
   * Obtains the binary form of the scan head's IP address. Note, this can
   * be converted to a string representation by using the `inet_ntop`
   * function.
   *
   * @return The binary representation of the scan head IP.
   */
  uint32_t GetScanHeadIp() const;

  /**
   * The serial number of the scan head.
   *
   * @return The scan head serial number.
   */
  uint32_t GetSerialNumber() const;

  /**
   * The scan sync ID the scan head is paired with.
   *
   * @return The scan sync ID.
   */
  uint16_t GetScanSyncId() const;

  /**
   * Obtains the global time in nanoseconds as reported from the FPGA.
   *
   * @return The global time in nanoseconds.
   */
  uint64_t GetGlobalTime() const;

  /**
   * Obtains all encoder values for the given status message.
   *
   * @return Vector of encoder values.
   */
  std::vector<int64_t> GetEncoders(void) const;

  /**
   * The binary form of the IP address of the client connected to the scan
   * head. Note, this can be converted to a string representation by using
   * the `inet_ntop` function.
   *
   * @return The binary representation of the binary IP.
   */
  uint32_t GetClientIp() const;

  /**
   * The port number used to connect to the client.
   *
   * @return The client's connection port number.
   */
  uint16_t GetClientPort() const;

  /**
   * Gets the number of valid cameras for a given scan head.
   *
   * @return Number of valid cameras
   */
  uint8_t GetValidCameras() const;

  /**
   * The total number of pixels within the camera's window.
   *
   * @return The number of pixels in window.
   */
  int32_t GetPixelsInWindow(int camera) const;

  /**
   * The current temperature reported by a given camera in Celsius.
   *
   * @param camera The camera ID.
   * @return The temperature in Celcius.
   */
  int32_t GetCameraTemperature(int camera) const;

  /**
   * The reported max scan rate for a scan head based on its current
   * configuration.
   *
   * @return The max scan rate in hertz.
   */
  uint32_t GetMaxScanRate() const;

  /**
   * The number of UDP packets sent from the Scan Head to the client during
   * the last scan period.
   *
   * @return The total number of UDP packets sent.
   */
  uint32_t GetNumPacketsSent() const;

  /**
   * The number of profiles sent from the scan head to the client during the
   * last scan period.
   *
   * @return The total number of profiles sent.
   */
  uint32_t GetNumProfilesSent() const;

 private:
  static const int kMaxEncoders = 3;
  static const int kMaxCameras = 2;

#pragma pack(push, 1)
  struct StatusMessagePacket {
    InfoHeader header;
    VersionInformation version;

    uint32_t serial_number = 0;
    uint32_t max_scan_rate = 0;
    uint32_t scan_head_ip = 0;
    uint32_t client_ip = 0;
    uint16_t client_port = 0;
    uint16_t scan_sync_id = 0;
    uint64_t global_time = 0;
    uint32_t num_packets_sent = 0;
    uint32_t num_profiles_sent = 0;
    uint8_t valid_encoders = 0;
    uint8_t valid_cameras = 0;
    // reserved for future use
    uint32_t reserved_0 = 0xFFFFFFFF;
    uint32_t reserved_1 = 0xFFFFFFFF;
    uint32_t reserved_2 = 0xFFFFFFFF;
    uint32_t reserved_3 = 0xFFFFFFFF;
    uint32_t reserved_4 = 0xFFFFFFFF;
    uint32_t reserved_5 = 0xFFFFFFFF;
    uint32_t reserved_6 = 0xFFFFFFFF;
    uint32_t reserved_7 = 0xFFFFFFFF;

    /**
     * Variable Data
     * These fields are set to the max size they can ever be to aid in in max
     * size calculation. However, not all elements will be valid data. Consult
     * the `valid_XXX` fields to determine how many elements to parse.
     */

    // See `valid_encoders` for number of valid elements
    std::array<uint64_t, kMaxEncoders> encoders = {0};

    // See `valid_cameras` for number of valid elements
    std::array<int32_t, kMaxCameras> pixels_in_window = {0};
    std::array<int32_t, kMaxCameras> camera_temp = {0};
  };
#pragma pack(pop)

  StatusMessagePacket packet;
  static const int kMaxStatusMessageSize = sizeof(StatusMessagePacket);
  static const int kMinStatusMessageSize =
    sizeof(StatusMessagePacket::header) + sizeof(StatusMessagePacket::version);

  static void ValidatePacketHeader(const InfoHeader &hdr);
  static void ValidatePacketData(const StatusMessagePacket &pkt);
  static void ValidatePacketVersion(const VersionInformation &ver);
};
} // namespace joescan

#endif // JOESCAN_STATUS_MESSAGE_H
