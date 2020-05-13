/**
 * Copyright (c) JoeScan Inc. All Rights Reserved.
 *
 * Licensed under the BSD 3 Clause License. See LICENSE.txt in the project
 * root for license information.
 */

#ifndef JSCANAPI_SCAN_MANAGER_H
#define JSCANAPI_SCAN_MANAGER_H

#include "AlignmentParams.hpp"
#include "Profile.hpp"
#include "ScanHeadReceiver.hpp"
#include "ScanHeadSender.hpp"

#include "joescan_pinchot.h"

#include <map>
#include <string>

namespace joescan {
class ScanHead;

class ScanManager {
 public:
  /**
   * @brief Creates a new scan manager object;
   */
  ScanManager();

  /**
   * @brief Destructor for the `ScanManager` object.
   */
  ~ScanManager();

  /**
   * @brief Creates a `ScanHead` object used to receive scan data.
   *
   * @param serial_number The serial number of the scan head.
   * @return A shared pointer to an object representing the scan head.
   */
  ScanHead* CreateScanner(std::string serial_number);

  /**
   * @brief Creates a `ScanHead` object used to receive scan data.
   *
   * @param serial_number The serial number of the scan head.
   * @param id The ID to associate with the scan head.
   * @return A shared pointer to an object representing the scan head.
   */
  ScanHead* CreateScanner(std::string serial_number, uint32_t id);

  /**
   * @brief Gets a `ScanHead` object used to receive scan data.
   *
   * @param serial_number The serial number of the scan head to get.
   * @param A shared pointer to an object representing the scan head.
   */
  ScanHead* GetScanner(std::string serial_number);

  /**
   * @brief Gets a `ScanHead` object used to receive scan data.
   *
   * @param id The ID of the scan head to get.
   * @param A shared pointer to an object representing the scan head.
   */
  ScanHead* GetScanner(uint32_t id);

  /**
   * @brief Removes a `ScanHead` object from use.
   *
   * @param serial_number The serial number of the scan head to remove.
   */
  void RemoveScanner(std::string serial_number);

  /**
   * @brief Removes a `ScanHead` object from use.
   *
   * @param scanHead A shared pointer of the scan head object to remove.
   */
  void RemoveScanner(ScanHead* scanHead);

  /**
   * @brief Removes all created `ScanHead` objects from use.
   */
  void RemoveAllScanners();

  /**
   * @brief Returns the total number of `ScanHead` objects associated with
   * the `ScanManager`.
   *
   * @return Total number of `ScanHeads`.
   */
  uint32_t GetNumberScanners();

  /**
   * @brief Attempts to connect to all `ScanHead` objects that were previously
   * created using `CreateScanner` call.
   *
   * @param timeout_s Maximum time to allow for connection.
   *
   * @return A vector to all scan heads that successfully connected.
   */
  std::map<std::string, ScanHead*> Connect(uint32_t timeout_s);

  /**
   * @brief Disconnects all `ScanHead` objects that were previously connected
   * from calling `Connect`.
   */
  void Disconnect();

  /**
   * @brief Starts scanning on all `ScanHead` objects that were connected
   * using the `Connect` function.
   */
  void StartScanning();

  /**
   * @brief Starts scanning on a single `ScanHead` object that was connected
   * using the `Connect` function.
   *
   * @note This will place the entire scan manager into a "scanning" state.
   * Stop scanning will need to be called before scanning on additional scan
   * heads.
   */
  void StartScanning(ScanHead* scan_head);

  /**
   * @brief Stop scanning on all `ScanHead` objects that were told to scan
   * through the `StartScanning` function.
   */
  void StopScanning();

  /**
   * @brief Sets the rate at which new data is sent from the scan head.
   *
   * @param rate_hz The frequency of new data in hertz.
   */
  void SetScanRate(double rate_hz);

  /**
   * @brief Gets the rate at which new data is sent from the scan head.
   *
   * @return The configured frequency of new data in hertz.
   */
  double GetScanRate() const;

  /**
   * @brief Configures the type of data and its resolution to be returned
   * from the scan head when performing a scan.
   *
   * @param format The format the data will be presented in
   */
  void SetRequestedDataFormat(jsDataFormat format);

  /**
   * @brief Boolean state function used to determine if the `ScanManager` has
   * connected to the `ScanHead` objects.
   *
   * @return Boolean `true` if connected, `false` if disconnected.
   */
  inline bool IsConnected() const;

  /**
   * @brief Boolean state function used to determine if the `ScanManager` and
   * `ScanHead` objects are actively scanning.
   *
   * @return Boolean `true if scanning, `false` otherwise.
   */
  inline bool IsScanning() const;

 private:
  enum SystemState { Disconnected, Connected, Scanning };

  std::map<std::string, ScanHead*> BroadcastConnect(uint32_t timeout_s);
  void FillVersionInformation(VersionInformation& vi);

  std::map<std::string, ScanHeadReceiver*> receivers_by_serial;
  std::map<std::string, ScanHeadShared*> shares_by_serial;
  std::map<std::string, ScanHead*> scanners_by_serial;
  std::map<uint32_t, ScanHead*> scanners_by_id;
  ScanHeadSender sender;

  uint8_t session_id = 1;
  const double kScanRateHzMax = 10000;
  const double kScanRateHzMin = 0.02;
  double scan_rate_hz_current_max = kScanRateHzMax;
  double scan_rate_hz = 0.0;

  SystemState state = SystemState::Disconnected;
};

inline bool ScanManager::IsConnected() const
{
  return state == SystemState::Connected;
}

inline bool ScanManager::IsScanning() const
{
  return state == SystemState::Scanning;
}
} // namespace joescan

#endif // JSCANAPI_SCAN_MANAGER_H
