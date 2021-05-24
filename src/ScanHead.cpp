/**
 * Copyright (c) JoeScan Inc. All Rights Reserved.
 *
 * Licensed under the BSD 3 Clause License. See LICENSE.txt in the project
 * root for license information.
 */

#include "httplib.hpp"
#include "json.hpp"
#include "NetworkInterface.hpp"
#include "ScanHead.hpp"
#include <iostream>

using namespace joescan;

ScanHead::ScanHead(ScanManager &manager, uint32_t serial_number, uint32_t id)
  : m_scan_manager(manager),
    m_window(30.0, -30.0, -30.0, 30.0),
    m_format(JS_DATA_FORMAT_XY_FULL_LM_FULL),
    m_circ_buffer(kMaxCircularBufferSize),
    m_profile_ptr(nullptr),
    m_serial_number(serial_number),
    m_ip_address(0),
    m_id(id),
    m_fd(0),
    m_port(0),
    m_active_count(0),
    m_packets_received(0),
    m_packets_received_for_profile(0),
    m_complete_profiles_received(0),
    m_expected_packets_received(0),
    m_expected_profiles_received(0),
    m_last_profile_source(0),
    m_last_profile_timestamp(0),
    m_is_data_available_condition_enabled(false)
{
  m_packet_buf = new uint8_t[kMaxPacketSize];
  m_packet_buf_len = kMaxPacketSize;

  // default configuration
  m_config.scan_offset_us = 0;
  m_config.camera_exposure_time_min_us = 10000;
  m_config.camera_exposure_time_def_us = 500000;
  m_config.camera_exposure_time_max_us = 1000000;
  m_config.laser_on_time_min_us = 100;
  m_config.laser_on_time_def_us = 500;
  m_config.laser_on_time_max_us = 1000;
  m_config.laser_detection_threshold = 120;
  m_config.saturation_threshold = 800;
  m_config.saturation_percentage = 30;

  {
    net_iface iface = NetworkInterface::InitRecvSocket(INADDR_ANY, 0);
    m_fd = iface.sockfd;
    m_port = iface.port;
  }

  std::thread receive_thread(&ScanHead::ReceiveMain, this);
  m_receiver = std::move(receive_thread);
}

ScanHead::~ScanHead()
{
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_active_count = -1;
  }

  NetworkInterface::CloseSocket(m_fd);

  m_thread_sync.notify_all();
  m_receiver.join();

  delete[] m_packet_buf;
}

jsScanHeadType ScanHead::GetProductType() const
{
  return m_product_type;
}

uint32_t ScanHead::GetSerialNumber() const
{
  return m_serial_number;
}

uint32_t ScanHead::GetId() const
{
  return m_id;
}

uint32_t ScanHead::GetNumberCameras()
{
  return m_status.GetValidCameras();
}

uint32_t ScanHead::GetIpAddress() const
{
  return m_ip_address;
}

int ScanHead::GetReceivePort() const
{
  return m_port;
}

void ScanHead::ReceiveStart()
{
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_profile_ptr = nullptr;
    m_packets_received = 0;
    m_complete_profiles_received = 0;
    m_last_profile_source = 0;
    m_last_profile_timestamp = 0;
    m_active_count = 1;
    m_is_data_available_condition_enabled = true;
    // reset circular buffer holding profile data
    m_circ_buffer.clear();
  }

  m_thread_sync.notify_all();
}

void ScanHead::ReceiveStop()
{
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_active_count = 0;
  }

  m_thread_sync.notify_all();
}

uint32_t ScanHead::AvailableProfiles()
{
  return static_cast<uint32_t>(m_circ_buffer.size());
}

uint32_t ScanHead::WaitUntilAvailableProfiles(uint32_t count,
                                              uint32_t timeout_us)
{
  std::chrono::duration<uint32_t, std::micro> timeout(timeout_us);
  std::unique_lock<std::mutex> lock(m_mutex);
  m_thread_sync.wait_for(
    lock, timeout, [this, count] { return m_circ_buffer.size() >= count; });
  return static_cast<uint32_t>(m_circ_buffer.size());
}

std::vector<std::shared_ptr<Profile>> ScanHead::GetProfiles(uint32_t count)
{
  std::vector<std::shared_ptr<Profile>> profiles;
  std::shared_ptr<Profile> profile = nullptr;
  std::lock_guard<std::mutex> lock(m_mutex);

  while (!m_circ_buffer.empty() && (0 < count)) {
    profile = m_circ_buffer.front();
    m_circ_buffer.pop_front();

    profiles.push_back(profile);
    count--;
  }

  return profiles;
}

void ScanHead::ClearProfiles()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_circ_buffer.clear();
}

StatusMessage ScanHead::GetStatusMessage()
{
  return m_status;
}

void ScanHead::ClearStatusMessage()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_status = StatusMessage();
}

ScanManager &ScanHead::GetScanManager()
{
  return m_scan_manager;
}

void ScanHead::SetAlignment(jsCamera camera, AlignmentParams &alignment)
{
  if (camera >= JS_CAMERA_MAX) {
    throw std::exception();
  }

  m_alignment[camera] = alignment;
}

AlignmentParams &ScanHead::GetAlignment(jsCamera camera)
{
  if (camera >= JS_CAMERA_MAX) {
    throw std::exception();
  }

  return m_alignment[camera];
}

void ScanHead::SetConfiguration(jsScanHeadConfiguration &cfg)
{
  if ((cfg.camera_exposure_time_max_us > kMaxCameraExposureUsec) ||
      (cfg.camera_exposure_time_min_us < kMinCameraExposureUsec) ||
      (cfg.camera_exposure_time_max_us < cfg.camera_exposure_time_def_us) ||
      (cfg.camera_exposure_time_max_us < cfg.camera_exposure_time_min_us) ||
      (cfg.camera_exposure_time_def_us < cfg.camera_exposure_time_min_us)) {
    throw std::range_error("out of range");
  }

  if ((cfg.laser_on_time_max_us > kMaxLaserOnTimeUsec) ||
      ((0 != cfg.laser_on_time_min_us) &&
       (cfg.laser_on_time_min_us < kMinLaserOnTimeUsec)) ||
      (cfg.laser_on_time_max_us < cfg.laser_on_time_def_us) ||
      (cfg.laser_on_time_max_us < cfg.laser_on_time_min_us) ||
      (cfg.laser_on_time_def_us < cfg.laser_on_time_min_us)) {
    throw std::range_error("out of range");
  }

  if (cfg.laser_detection_threshold > kMaxLaserDetectionThreshold) {
    throw std::range_error("out of range");
  }

  if (cfg.saturation_threshold > kMaxSaturationThreshold) {
    throw std::range_error("out of range");
  }

  if (cfg.saturation_percentage > kMaxSaturationPercentage) {
    throw std::range_error("out of range");
  }

  m_config = cfg;
}

jsScanHeadConfiguration ScanHead::GetConfiguration() const
{
  return m_config;
}

void ScanHead::SetDataFormat(jsDataFormat format)
{
  m_format = format;
}

jsDataFormat ScanHead::GetDataFormat() const
{
  return m_format;
}

ScanHeadTemperatures ScanHead::GetTemperatures()
{
  std::string ip_str = std::to_string((m_ip_address >> 24) & 0xFF) + "." +
                       std::to_string((m_ip_address >> 16) & 0xFF) + "." +
                       std::to_string((m_ip_address >> 8) & 0xFF) + "." +
                       std::to_string((m_ip_address >> 0) & 0xFF);

  httplib::Client cli(ip_str, kRESTport);
  ScanHeadTemperatures t;

  memset(&t, 0, sizeof(ScanHeadTemperatures));

  const char *ep = "/sensors/temperature";
  httplib::Headers hdr = {{"Content-type", "application/json"}};
  std::shared_ptr<httplib::Response> res = cli.Get(ep, hdr);
  if (!res) {
    return t;
  }

  auto status = GetStatusMessage();
  uint8_t num_cameras = status.GetValidCameras();

  try {
    nlohmann::json json = nlohmann::json::parse(res->body);

    for (int i = 0; i < num_cameras; ++i) {
      t.camera_temp_c[i] = json["camera"][i];
    }
    t.mainboard_temp_c = json["mainboard"];
    t.mainboard_humidity = json["mainboardHumidity"];
  } catch (std::exception &e) {
    // failed to parse
    (void)e;
  }

  return t;
}

void ScanHead::SetWindow(ScanWindow &window)
{
  m_window = window;
}

ScanWindow &ScanHead::GetWindow()
{
  return m_window;
}

void ScanHead::PushProfile(std::shared_ptr<Profile> profile)
{
  // private function, assume mutex is already locked
  m_circ_buffer.push_back(profile);
  m_thread_sync.notify_all();
}

void ScanHead::PushStatus(StatusMessage status)
{
  // private function, assume mutex is already locked
  m_ip_address = status.GetScanHeadIp();
  m_status = status;

  uint16_t val = status.GetVersionInformation().product;
  m_product_type = (JS_SCAN_HEAD_JS50WX == val) ? JS_SCAN_HEAD_JS50WX
                                                : (JS_SCAN_HEAD_JS50WSC == val)
                                                    ? JS_SCAN_HEAD_JS50WSC
                                                    : JS_SCAN_HEAD_INVALID_TYPE;

  m_thread_sync.notify_all();
}

void ScanHead::ProcessPacket(DataPacket &packet)
{
  // private function, assume mutex is already locked
  uint32_t source = 0;
  uint64_t timestamp = 0;
  uint32_t raw_len = 0;
  uint8_t *raw = packet.GetRawBytes(&raw_len);
  const uint32_t total_packets = packet.GetNumParts();
  const uint32_t current_packet = packet.GetPartNum();
  const uint16_t datatype_mask = packet.GetContents();

  source = packet.GetSourceId();
  timestamp = packet.GetTimeStamp();

  if ((source != m_last_profile_source) ||
      (timestamp != m_last_profile_timestamp)) {
    if (nullptr != m_profile_ptr) {
      // have a partial profile, push it back despite loss
      m_profile_ptr->SetUDPPacketInfo(m_packets_received_for_profile,
                                      total_packets);

      PushProfile(m_profile_ptr);
    }

    m_last_profile_source = source;
    m_last_profile_timestamp = timestamp;
    m_packets_received_for_profile = 0;

    m_profile_ptr = std::make_shared<Profile>(packet);
  }

  // if Brightness, assume X/Y data is present
  if (datatype_mask & DataType::Brightness) {
    FragmentLayout b_layout = packet.GetFragmentLayout(DataType::Brightness);
    FragmentLayout xy_layout = packet.GetFragmentLayout(DataType::XYData);
    uint8_t *b_src = reinterpret_cast<uint8_t *>(&(raw[b_layout.offset]));
    int16_t *xy_src = reinterpret_cast<int16_t *>(&(raw[xy_layout.offset]));
    const uint32_t start_column = packet.GetStartColumn();
    const jsCamera id = packet.GetCamera();

    // assume step is the same for both layouts
    const uint32_t inc = total_packets * xy_layout.step;
    uint32_t idx = start_column + current_packet * xy_layout.step;

    // assume num_vals is same for both layouts
    for (uint32_t n = 0; n < xy_layout.num_vals; n++) {
      int16_t x_raw = htons(*xy_src++);
      int16_t y_raw = htons(*xy_src++);
      uint8_t brightness = *b_src++;

      if ((JS_PROFILE_DATA_INVALID_XY != x_raw) &&
          (JS_PROFILE_DATA_INVALID_XY != y_raw)) {
        int32_t x = static_cast<int32_t>(x_raw);
        int32_t y = static_cast<int32_t>(y_raw);
        Point2D<int32_t> point = m_alignment[id].CameraToMill(x, y);
        m_profile_ptr->InsertPointAndBrightness(idx, point, brightness);
      }

      idx += inc;
    }
  } else if (datatype_mask & DataType::XYData) {
    FragmentLayout layout = packet.GetFragmentLayout(DataType::XYData);
    int16_t *src = reinterpret_cast<int16_t *>(&(raw[layout.offset]));
    const uint32_t start_column = packet.GetStartColumn();
    const jsCamera id = packet.GetCamera();

    const uint32_t inc = total_packets * layout.step;
    uint32_t idx = start_column + current_packet * layout.step;

    for (unsigned int j = 0; j < layout.num_vals; j++) {
      uint16_t x_raw = htons(*src++);
      uint16_t y_raw = htons(*src++);

      if ((JS_PROFILE_DATA_INVALID_XY != x_raw) &&
          (JS_PROFILE_DATA_INVALID_XY != y_raw)) {
        int32_t x = static_cast<int32_t>(x_raw);
        int32_t y = static_cast<int32_t>(y_raw);
        Point2D<int32_t> point = m_alignment[id].CameraToMill(x, y);
        m_profile_ptr->InsertPoint(idx, point);
      }

      idx += inc;
    }
  } else if (datatype_mask & DataType::Image) {
    // skip subpixel packet
    if ((m_packets_received_for_profile + 1) != total_packets) {
      FragmentLayout layout = packet.GetFragmentLayout(DataType::Image);
      uint32_t len = kImageDataSize;
      uint32_t m = current_packet * len;
      uint32_t n = layout.offset;
      // HACK HACK HACK: need to account for the fact that the scan_server sends
      // back exposure value right shifted by 8 for image mode
      m_profile_ptr->SetExposureTime(packet.GetExposureTime() << 8);
      m_profile_ptr->InsertImageSlice(m, &(raw[n]), len);
    }
  }

#if 0
  // we don't support this
  else if (datatype_mask & DataType::Subpixel) {
    FragmentLayout layout = packet.GetFragmentLayout(DataType::Subpixel);
    uint32_t m = 0;
    uint32_t n = layout.offset;
    uint16_t pixel = 0;

    for (unsigned int j = 0; j < layout.num_vals; j++) {
      pixel = htons(*(reinterpret_cast<uint16_t *>(&(raw[n]))));
      n += sizeof(uint16_t);

      if (kInvalidSubpixel != pixel) {
        m = packet.GetStartColumn();
        m += (j * total_packets + current_packet) * layout.step;

        Point2D point(pixel, m);
        m_profile_ptr->InsertPixelCoordinate(m, point);
      }
    }
  }
#endif

  m_packets_received_for_profile++;
  if (m_packets_received_for_profile == total_packets) {
    // received all packets for the profile
    m_profile_ptr->SetUDPPacketInfo(total_packets, total_packets);
    PushProfile(m_profile_ptr);
    m_profile_ptr = nullptr;
    m_complete_profiles_received++;
  }
}

void ScanHead::ReceiveMain()
{
  int nfds;
  fd_set rfds;
  struct timeval tv;
  int ret;

  while (0 <= m_active_count) {
    if (0 == m_active_count) {
      std::unique_lock<std::mutex> lock(m_mutex);
      m_is_data_available_condition_enabled = false;
      m_thread_sync.notify_all();
      m_thread_sync.wait(lock);
    } else if (0 < m_active_count) {
      nfds = static_cast<int>(m_fd) + 1;
      FD_ZERO(&rfds);
      FD_SET(m_fd, &rfds);
      tv.tv_sec = 0;
      tv.tv_usec = 50000;

      // Poll for activity on on the file descriptor, timeout if no activity.
      ret = select(nfds, &rfds, NULL, NULL, &tv);
      if (0 < ret) {
        std::unique_lock<std::mutex> lock(m_mutex);
        // Activity indicated, read out data from socket.
        char *buf = reinterpret_cast<char *>(m_packet_buf);
        size_t len = m_packet_buf_len;
        int num_bytes = recv(m_fd, buf, len, 0);

        // Check to make sure we are still running in case recv returns due to
        // its socket fd being closed.
        if (0 < m_active_count) {
          if (static_cast<std::size_t>(num_bytes) < sizeof(DatagramHeader)) {
            // TODO: better error handling!
            // throw std::runtime_error("Short header");
            continue;
          }

          uint16_t magic = (m_packet_buf[0] << 8) | (m_packet_buf[1]);
          if (kDataMagic == magic) {
            m_packets_received++;

            DataPacket packet(m_packet_buf, num_bytes, 0);
            ProcessPacket(packet);
          } else if (kResponseMagic == magic) {
            StatusMessage status = StatusMessage(m_packet_buf, num_bytes);
            m_expected_packets_received = status.GetNumPacketsSent();
            m_expected_profiles_received = status.GetNumProfilesSent();
            PushStatus(status);
          } else {
            // TODO: better error handling!
            // throw std::runtime_error("Unknown magic");
            continue;
          }
        }
      }
    }
  }
}
