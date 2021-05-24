/**
 * Copyright (c) JoeScan Inc. All Rights Reserved.
 *
 * Licensed under the BSD 3 Clause License. See LICENSE.txt in the project
 * root for license information.
 */

#include "ScanManager.hpp"
#include "ScanHead.hpp"

#include "BroadcastConnectMessage.hpp"
#include "DisconnectMessage.hpp"
#include "ImageRequestMessage.hpp"
#include "NetworkInterface.hpp"
#include "NetworkTypes.hpp"
#include "Profile.hpp"
#include "ScanRequestMessage.hpp"
#include "SetWindowMessage.hpp"
#include "StatusMessage.hpp"
#include "VersionCompatibilityException.hpp"
#include "VersionParser.hpp"

#include <cmath>
#include <cstring>
#include <ctime>
#include <sstream>

using namespace joescan;

ScanManager::ScanManager()
{
  session_id = 1;
}

ScanManager::~ScanManager()
{
  for (auto const &pair : scanners_by_serial) {
    ScanHead *scan_head = pair.second;
    delete scan_head;
  }

  sender.Shutdown();
  RemoveAllScanners();
}

ScanHead *ScanManager::CreateScanner(uint32_t serial_number, uint32_t id)
{
  ScanHead *scanner = nullptr;

  if (IsScanning()) {
    std::string error_msg = "Can not add scanner while scanning.";
    throw std::runtime_error(error_msg);
  }

  if (scanners_by_serial.find(serial_number) != scanners_by_serial.end()) {
    std::string error_msg = std::to_string(serial_number) + " already managed.";
    throw std::runtime_error(error_msg);
  }

  if (scanners_by_id.find(id) != scanners_by_id.end()) {
    std::string error_msg = "Id is already assigned to another scanner.";
    throw std::runtime_error(error_msg);
  }

  scanner = new ScanHead(*this, serial_number, id);
  scanners_by_serial[serial_number] = scanner;
  scanners_by_id[id] = scanner;

  return scanner;
}

ScanHead *ScanManager::GetScanHeadBySerial(uint32_t serial_number)
{
  auto scanner = scanners_by_serial.find(serial_number);

  if (scanner == scanners_by_serial.end()) {
    std::string error_msg = "Scanner is not managed.";
    throw std::runtime_error(error_msg);
  }

  return scanner->second;
}

ScanHead *ScanManager::GetScanHeadById(uint32_t id)
{
  auto scanner = scanners_by_id.find(id);
  if (scanner == scanners_by_id.end()) {
    std::string error_msg = "Scanner is not managed.";
    throw std::runtime_error(error_msg);
  }

  return scanner->second;
}

void ScanManager::RemoveScanner(uint32_t serial_number)
{
  if (IsScanning()) {
    std::string error_msg = "Can not remove scanner while scanning";
    throw std::runtime_error(error_msg);
  }

  auto scanner = scanners_by_serial.find(serial_number);
  if (scanner != scanners_by_serial.end()) {
    uint32_t id = scanner->second->GetId();
    scanners_by_serial.erase(serial_number);
    if (scanners_by_id.find(id) != scanners_by_id.end()) {
      scanners_by_id.erase(id);
    } else {
      std::string error_msg = "Scanner ID was not found.";
      throw std::runtime_error(error_msg);
    }
  } else {
    std::string error_msg = "Scanner is not managed.";
    throw std::runtime_error(error_msg);
  }
}

void ScanManager::RemoveScanner(ScanHead *scanner)
{
  if (scanner != nullptr) {
    RemoveScanner(scanner->GetSerialNumber());
  } else {
    throw std::runtime_error("Null pointer passed to RemoveScanner");
  }
}

void ScanManager::RemoveAllScanners()
{
  if (IsScanning()) {
    std::string error_msg = "Can not remove scanners while scanning";
    throw std::runtime_error(error_msg);
  }

  scanners_by_serial.clear();
  scanners_by_id.clear();
}

uint32_t ScanManager::GetNumberScanners()
{
  return static_cast<uint32_t>(scanners_by_serial.size());
}

std::map<uint32_t, ScanHead *> ScanManager::Connect(uint32_t timeout_s)
{
  if (IsConnected()) {
    std::string error_msg = "Already connected.";
    throw std::runtime_error(error_msg);
  }

  if (IsScanning()) {
    std::string error_msg = "Already scanning.";
    throw std::runtime_error(error_msg);
  }

  std::map<uint32_t, ScanHead *> connected;
  if (scanners_by_serial.empty()) {
    return connected;
  }

  for (auto const &pair : scanners_by_serial) {
    ScanHead *scan_head = pair.second;
    scan_head->ReceiveStart();
  }

  session_id++;
  connected = BroadcastConnect(timeout_s);
  if (connected.size() == scanners_by_serial.size()) {
    state = SystemState::Connected;
  }

  if (SystemState::Connected == state) {
    sender.Start();

    for (auto const &pair : scanners_by_serial) {
      ScanHead *sh = pair.second;
      uint32_t ip_addr = sh->GetIpAddress();
      std::vector<WindowConstraint> constraints =
        sh->GetWindow().GetConstraints();

      const int number_of_cameras = sh->GetNumberCameras();

      for (int n = JS_CAMERA_A; n < number_of_cameras; n++) {
        auto msg = SetWindowMessage(n);
        for (auto const &constraint : constraints) {
          Point2D<int32_t> p0, p1;
          int32_t x, y;
          auto a = sh->GetAlignment(static_cast<jsCamera>(n));
          // Note: units are in 1/1000 inch
          // calculate the first point of our window constraint
          x = static_cast<int32_t>(constraint.constraints[0].x);
          y = static_cast<int32_t>(constraint.constraints[0].y);
          // convert the point to the camera's coordinate system
          p0 = a.MillToCamera(x, y);
          // calculate the second point of out window constraint
          x = static_cast<int32_t>(constraint.constraints[1].x);
          y = static_cast<int32_t>(constraint.constraints[1].y);
          // convert the point to the camera's coordinate system
          p1 = a.MillToCamera(x, y);
          // pass constraint points to message to create the constraint
          if (a.GetFlipX()) {
            msg.AddConstraint(p0.x, p0.y, p1.x, p1.y);
          } else {
            msg.AddConstraint(p1.x, p1.y, p0.x, p0.y);
          }
        }
        // send the constraint message to the scan server
        sender.Send(msg.Serialize(), ip_addr);
      }
    }

    // need to allow some time for the windows to be received and applied
    // otherwise we can get back an invalid max scan rate and confuse users
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // clear out all status messages
    for (auto const &pair : scanners_by_serial) {
      ScanHead *sh = pair.second;
      sh->ClearStatusMessage();
    }

    // wait until we get new status messages for each scan head so we can get
    // an accurate max scan rate
    for (auto const &pair : scanners_by_serial) {
      ScanHead *sh = pair.second;
      StatusMessage msg = sh->GetStatusMessage();
      uint64_t timestamp_old = msg.GetGlobalTime();
      uint64_t timestamp_new = timestamp_old;

      while (timestamp_new == timestamp_old) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        msg = sh->GetStatusMessage();
        timestamp_new = msg.GetGlobalTime();
      }
    }
  }

  return connected;
}

void ScanManager::Disconnect()
{
  if (!IsConnected()) {
    std::string error_msg = "Not connected.";
    throw std::runtime_error(error_msg);
  }

  if (IsScanning()) {
    std::string error_msg = "Can not disconnect wile still scanning";
    throw std::runtime_error(error_msg);
  }

  auto message = DisconnectMessage().Serialize();
  for (auto const &pair : scanners_by_serial) {
    ScanHead *scan_head = pair.second;

    sender.Send(message, scan_head->GetIpAddress());
    scan_head->ReceiveStop();
  }
  sender.Stop();

  // slight delay to make sure no new status messages are in route
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  for (auto const &pair : scanners_by_serial) {
    ScanHead *scan_head = pair.second;
    scan_head->ClearStatusMessage();
  }

  state = SystemState::Disconnected;
}

void ScanManager::StartScanning()
{
  double scan_interval_us = (1.0 / scan_rate_hz) * 1e6;

  if (!IsConnected()) {
    std::string error_msg = "Not connected.";
    throw std::runtime_error(error_msg);
  }

  if (IsScanning()) {
    std::string error_msg = "Already scanning.";
    throw std::runtime_error(error_msg);
  }

  // Just create & enqueue scan request messages in the SenderReceiver.
  std::vector<std::pair<uint32_t, Datagram>> requests;
  requests.reserve(scanners_by_serial.size());

  for (auto const &pair : scanners_by_serial) {
    ScanHead *scan_head = pair.second;

    scan_head->ReceiveStart();

    auto fmt = scan_head->GetDataFormat();
    auto port = scan_head->GetReceivePort();
    auto id = scan_head->GetId();
    uint32_t interval = static_cast<uint32_t>(scan_interval_us);
    uint32_t count = 0xFFFFFFFF;
    auto config = scan_head->GetConfiguration();

    ScanRequest request(fmt, 0, port, id, interval, count, config);

    auto ip_addr_and_request =
      std::make_pair(scan_head->GetIpAddress(), request.Serialize(session_id));

    requests.push_back(ip_addr_and_request);
  }

  sender.EnqueueScanRequests(requests);

  state = SystemState::Scanning;
}

void ScanManager::StartScanning(ScanHead *scan_head)
{
  double scan_interval_us = (1.0 / scan_rate_hz) * 1e6;

  if (!IsConnected()) {
    std::string error_msg = "Not connected.";
    throw std::runtime_error(error_msg);
  }

  if (IsScanning()) {
    std::string error_msg = "Already scanning.";
    throw std::runtime_error(error_msg);
  }

  auto pair = scanners_by_id.find(scan_head->GetId());
  if (pair == scanners_by_id.end()) {
    std::string error_msg = "Scanner is not managed.";
    throw std::runtime_error(error_msg);
  }

  std::vector<std::pair<uint32_t, Datagram>> requests;
  requests.reserve(1);

  scan_head->ReceiveStart();

  auto fmt = scan_head->GetDataFormat();
  auto port = scan_head->GetReceivePort();
  auto id = scan_head->GetId();
  uint32_t interval = static_cast<uint32_t>(scan_interval_us);
  uint32_t count = 0xFFFFFFFF;
  auto config = scan_head->GetConfiguration();

  ScanRequest request(fmt, 0, port, id, interval, count, config);

  requests.push_back(
    std::make_pair(scan_head->GetIpAddress(), request.Serialize(session_id)));

  sender.EnqueueScanRequests(requests);

  state = SystemState::Scanning;
}

void ScanManager::StopScanning()
{
  if (!IsScanning()) {
    std::string error_msg = "Not scanning.";
    throw std::runtime_error(error_msg);
  }

  sender.ClearScanRequests();

  state = SystemState::Connected;
}

int ScanManager::RequestImages(ScanHead *scan_head,
                               jsScanHeadConfiguration &config)
{
  const uint32_t kTimeoutMs = 10000;
  const uint32_t kPollTimeMs = 10;
  int r = 0;

  if (!IsConnected()) {
    std::string error_msg = "Not connected.";
    throw std::runtime_error(error_msg);
  }

  if (IsScanning()) {
    std::string error_msg = "Already scanning.";
    throw std::runtime_error(error_msg);
  }

  auto pair = scanners_by_id.find(scan_head->GetId());
  if (pair == scanners_by_id.end()) {
    std::string error_msg = "Scanner is not managed.";
    throw std::runtime_error(error_msg);
  }

  scan_head->ReceiveStart();

  double scan_interval_us = 0;
  uint32_t num_cameras = scan_head->GetStatusMessage().GetValidCameras();
  uint32_t camera_exposure_max = config.camera_exposure_time_max_us;
  double rate_hz = 1.0 / (num_cameras * camera_exposure_max * 1e-6);
  // cap the max rate at which images are done
  if (rate_hz > 2.0) {
    rate_hz = 2.0;
  }
  scan_interval_us = (1.0 / rate_hz) * 1e6;

  auto port = scan_head->GetReceivePort();
  auto id = scan_head->GetId();
  uint32_t interval = static_cast<uint32_t>(scan_interval_us);
  ImageRequest request(0, port, id, interval, num_cameras, config);

  //sender.Send(request.Serialize(session_id), scan_head->GetIpAddress());

  std::vector<std::pair<uint32_t, Datagram>> requests;
  requests.push_back(
    std::make_pair(scan_head->GetIpAddress(), request.Serialize(session_id)));
  sender.EnqueueScanRequests(requests);
  state = SystemState::Scanning;

  // capture an image per camera, filter results after
  uint32_t time_ms = 0;
  while ((num_cameras > scan_head->AvailableProfiles()) &&
         (time_ms < kTimeoutMs)) {
    std::this_thread::sleep_for(std::chrono::milliseconds(kPollTimeMs));
    time_ms += kPollTimeMs;
  }

  sender.ClearScanRequests();
  state = SystemState::Connected;

  r = (kTimeoutMs == time_ms) ? -1 : scan_head->AvailableProfiles();
  return r;
}

void ScanManager::SetScanRate(double rate_hz)
{
  double max_rate_hz = GetMaxScanRate();

  if ((rate_hz > kScanRateHzMax) || (rate_hz < kScanRateHzMin)) {
    std::stringstream error_msg;

    error_msg << "scan rate " << rate_hz << " out of range, must be between "
              << kScanRateHzMin << " Hz and " << kScanRateHzMax << "Hz";

    throw std::runtime_error(error_msg.str());
  } else if (rate_hz > max_rate_hz) {
    std::stringstream error_msg;

    error_msg << "scan rate " << rate_hz
              << " exceeds max scan rate allowed by window, must be less than "
              << max_rate_hz << "Hz";

    throw std::runtime_error(error_msg.str());
  }

  scan_rate_hz = rate_hz;
}

double ScanManager::GetScanRate() const
{
  return scan_rate_hz;
}

double ScanManager::GetMaxScanRate()
{
  double max_rate = kPinchotConstantMaxScanRate;

  for (auto const &pair : scanners_by_serial) {
    ScanHead *scan_head = pair.second;

    // determine what the greatest maximum exposure is for all of the scan
    // heads; we need this to determine the maximum scan rate of our system
    auto config = scan_head->GetConfiguration();

    double laser_on_max_freq =
      1000000.0 / static_cast<double>(config.laser_on_time_max_us);
    if (laser_on_max_freq < max_rate) {
      max_rate = laser_on_max_freq;
    }

    double rate_hz = scan_head->GetStatusMessage().GetMaxScanRate();
    if (rate_hz < max_rate) {
      max_rate = rate_hz;
    }
  }

  return max_rate;
}

void ScanManager::SetRequestedDataFormat(jsDataFormat format)
{
  for (auto const &m : scanners_by_id) {
    m.second->SetDataFormat(format);
  }
}

void ScanManager::FillVersionInformation(VersionInformation &vi)
{
  vi.major = std::stoi(VERSION_MAJOR);
  vi.minor = std::stoi(VERSION_MINOR);
  vi.patch = std::stoi(VERSION_PATCH);
  vi.commit = std::stoul(VERSION_COMMIT, nullptr, 16);
}

std::map<uint32_t, ScanHead *> ScanManager::BroadcastConnect(uint32_t timeout_s)
{
  std::map<uint32_t, ScanHead *> connected;
  std::vector<net_iface> ifaces;

  /////////////////////////////////////////////////////////////////////////////
  // STEP 1: Get all available interfaces.
  /////////////////////////////////////////////////////////////////////////////
  {
    auto ip_addrs = NetworkInterface::GetActiveIpAddresses();
    for (auto const &ip_addr : ip_addrs) {
      try {
        net_iface iface = NetworkInterface::InitBroadcastSocket(ip_addr, 0);
        ifaces.push_back(iface);
      } catch (const std::runtime_error &) {
        // Failed to init socket, continue since there might be other ifaces
      }
    }

    if (ifaces.size() == 0) {
      throw std::runtime_error("No valid broadcast interfaces");
    }
  }

  {
    static const int kConnectPollMs = 500;
    uint64_t time_start = std::time(nullptr);
    int32_t timeout_ms = timeout_s * 1000;
    bool is_connected = false;

    while ((false == is_connected) && (0 < timeout_ms)) {
      if (connected.size() != scanners_by_serial.size()) {
        ///////////////////////////////////////////////////////////////////////
        // STEP 2: Send out BroadcastConnect packet for each scan head.
        ///////////////////////////////////////////////////////////////////////
        // spam each network interface with our connection message
        for (auto const &iface : ifaces) {
          for (auto const &pair : scanners_by_serial) {
            uint32_t serial = pair.first;
            ScanHead *scan_head = pair.second;
            uint32_t scan_id = scan_head->GetId();
            uint32_t ip_addr = iface.ip_addr;
            uint16_t port = scan_head->GetReceivePort();

            // skip sending message to scan heads that are already connected
            if (connected.find(serial) != connected.end()) {
              continue;
            }

            // we want the scan head to connect to the client with these params
            auto msg = BroadcastConnectMessage(ip_addr, port, session_id,
                                               scan_id, serial);
            auto bytes = msg.Serialize();

            SOCKET sock = iface.sockfd;
            const char *src = reinterpret_cast<const char *>(bytes.data());
            const uint32_t len = static_cast<uint32_t>(bytes.size());

            // client will send payload out according to these values
            sockaddr_in addr;
            memset(&addr, 0, sizeof(addr));
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
            addr.sin_port = htons(kScanServerPort);

            int r = sendto(sock, src, len, 0,
                           reinterpret_cast<sockaddr *>(&addr), sizeof(addr));
            if (0 >= r) {
              // failed to send data to interface
              break;
            }

            scan_head->ClearStatusMessage();
          }
        }

        // still waiting for status messages...
        std::this_thread::sleep_for(std::chrono::milliseconds(kConnectPollMs));
        timeout_ms -= kConnectPollMs;
      } else {
        is_connected = true;
      }

      /////////////////////////////////////////////////////////////////////////
      // STEP 3: See which (if any) scan heads responded.
      /////////////////////////////////////////////////////////////////////////
      for (auto const &pair : scanners_by_serial) {
        uint32_t serial = pair.first;
        ScanHead *scan_head = pair.second;
        StatusMessage msg = scan_head->GetStatusMessage();
        uint64_t timestamp = 0;

        // get timestamp where status message was received
        timestamp = scan_head->GetStatusMessage().GetGlobalTime();

        if ((connected.end() == connected.find(serial)) &&
            (timestamp > time_start)) {
          VersionInformation client_version;
          FillVersionInformation(client_version);

          auto scanner_version = msg.GetVersionInformation();
          if (!VersionParser::AreVersionsCompatible(client_version,
                                                    scanner_version)) {
            throw VersionCompatibilityException(client_version,
                                                scanner_version);
          }

          // found an active scan head!
          connected[serial] = scan_head;
        }
      }
    }
  }

  /////////////////////////////////////////////////////////////////////////////
  // STEP 4: Clean up and return.
  /////////////////////////////////////////////////////////////////////////////
  for (auto const &iface : ifaces) {
    NetworkInterface::CloseSocket(iface.sockfd);
  }

  return connected;
}
