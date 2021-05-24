/**
 * Copyright (c) JoeScan Inc. All Rights Reserved.
 *
 * Licensed under the BSD 3 Clause License. See LICENSE.txt in the project
 * root for license information.
 */

#include "ScanRequestMessage.hpp"
#include "DataFormats.hpp"
#include "TcpSerializationHelpers.hpp"
#include "NetworkIncludes.hpp"
#include <algorithm>

using namespace joescan;

ScanRequest::ScanRequest(jsDataFormat format, uint32_t client_ip,
                         int client_port, int scan_head_id, uint32_t interval,
                         uint32_t scanCount,
                         const jsScanHeadConfiguration &config)
{
  m_client_ip = client_ip;
  m_client_port = client_port;
  m_scan_head_id = scan_head_id;
  m_camera_id = 0; // TODO: If these become useful, don't hardcode.
  m_laser_id = 0;  // TODO: If these become useful, don't hardcode.
  m_flags = 0;    // TODO: If these become useful, don't hardcode.
  m_magic = kCommandMagic;
  m_laser_exposure_min_us = config.laser_on_time_min_us;
  m_laser_exposure_def_us = config.laser_on_time_def_us;
  m_laser_exposure_max_us = config.laser_on_time_max_us;
  m_camera_exposure_min_us = config.camera_exposure_time_min_us;
  m_camera_exposure_def_us = config.camera_exposure_time_def_us;
  m_camera_exposure_max_us = config.camera_exposure_time_max_us;
  m_laser_detection_threshold = config.laser_detection_threshold;
  m_saturation_threshold = config.saturation_threshold;
  m_saturation_percentage = config.saturation_percentage;
  // TODO: what to do with this?
  m_average_intensity = 50;
  m_scan_interval_us = interval;
  m_scan_offset_us = config.scan_offset_us;
  // If the caller requested a specific number of scans, obey them, otherwise
  // let's hope they didn't mean to scan for a whole week.
  m_number_of_scans = (scanCount == 0) ? 1000000 : scanCount;
  m_start_col = 0;
  m_end_col = 1455;

  m_data_types = DataFormats::GetDataType(format);
  m_steps = DataFormats::GetStep(format);
}

ScanRequest::ScanRequest(const Datagram &datagram)
{
  if (datagram.size() < 76) {
    // throw std::exception();
  }
  int index = 0;

  m_magic = ntohs(*reinterpret_cast<const uint16_t *>(&datagram[index]));
  if (m_magic != kCommandMagic) {
    throw std::exception();
  }
  index += sizeof(uint16_t);

  uint8_t size = datagram[index++];

  m_request_type = UdpPacketType::_from_integral(datagram[index++]);

  m_client_ip =
    ntohl(*(reinterpret_cast<const uint32_t *>(&datagram[index])));
  index += sizeof(uint32_t);
  m_client_port = ntohs(*(reinterpret_cast<const uint16_t *>(&datagram[index])));
  index += sizeof(uint16_t);
  m_request_sequence = datagram[index++];

  m_scan_head_id = datagram[index++];
  m_camera_id = datagram[index++];
  m_laser_id = datagram[index++];
  DEPRECATED_DO_NOT_USE = datagram[index++];
  m_flags = datagram[index++];

  m_laser_exposure_min_us =
    ntohl(*(reinterpret_cast<const uint32_t *>(&datagram[index])));
  index += sizeof(uint32_t);
  m_laser_exposure_def_us =
    ntohl(*(reinterpret_cast<const uint32_t *>(&datagram[index])));
  index += sizeof(uint32_t);
  m_laser_exposure_max_us =
    ntohl(*(reinterpret_cast<const uint32_t *>(&datagram[index])));
  index += sizeof(uint32_t);

  m_camera_exposure_min_us =
    ntohl(*(reinterpret_cast<const uint32_t *>(&datagram[index])));
  index += sizeof(uint32_t);
  m_camera_exposure_def_us =
    ntohl(*(reinterpret_cast<const uint32_t *>(&datagram[index])));
  index += sizeof(uint32_t);
  m_camera_exposure_max_us =
    ntohl(*(reinterpret_cast<const uint32_t *>(&datagram[index])));
  index += sizeof(uint32_t);

  m_laser_detection_threshold =
    ntohl(*(reinterpret_cast<const uint32_t *>(&datagram[index])));
  index += sizeof(uint32_t);
  m_saturation_threshold =
    ntohl(*(reinterpret_cast<const uint32_t *>(&datagram[index])));
  index += sizeof(uint32_t);
  m_saturation_percentage =
    ntohl(*(reinterpret_cast<const uint32_t *>(&datagram[index])));
  index += sizeof(uint32_t);
  m_average_intensity =
    ntohl(*(reinterpret_cast<const uint32_t *>(&datagram[index])));
  index += sizeof(uint32_t);

  m_scan_interval_us = ntohl(*(reinterpret_cast<const uint32_t *>(&datagram[index])));
  index += sizeof(uint32_t);
  m_scan_offset_us = ntohl(*(reinterpret_cast<const uint32_t *>(&datagram[index])));
  index += sizeof(uint32_t);
  m_number_of_scans =
    ntohl(*(reinterpret_cast<const uint32_t *>(&datagram[index])));
  index += sizeof(uint32_t);

  m_data_types = ntohs(*(reinterpret_cast<const uint16_t *>(&datagram[index])));
  index += sizeof(uint16_t);
  m_start_col = ntohs(*(reinterpret_cast<const uint16_t *>(&datagram[index])));
  index += sizeof(uint16_t);
  m_end_col = ntohs(*(reinterpret_cast<const uint16_t *>(&datagram[index])));
  index += sizeof(uint16_t);

  for (int i = 1; i <= m_data_types; i <<= 1) {
    if (i & m_data_types) {
      uint16_t stepVal =
        ntohs(*(reinterpret_cast<const uint16_t *>(&datagram[index])));
      m_steps.push_back(stepVal);
      index += sizeof(uint16_t);
    }
  }

  if (size != Length()) {
    // throw std::exception();
  }
}

ScanRequest ScanRequest::Deserialize(const Datagram &datagram)
{
  return ScanRequest(datagram);
}

Datagram ScanRequest::Serialize(uint8_t request_sequence)
{
  std::vector<uint8_t> scanRequestPacket;
  scanRequestPacket.reserve(Length());

  size_t index = 0;
  index += SerializeIntegralToBytes(scanRequestPacket, &kCommandMagic);

  scanRequestPacket.push_back(Length());
  scanRequestPacket.push_back(m_request_type);
  index += 2;

  index += SerializeIntegralToBytes(scanRequestPacket, &m_client_ip);
  index += SerializeIntegralToBytes(scanRequestPacket, &m_client_port);

  scanRequestPacket.push_back(m_request_sequence);
  scanRequestPacket.push_back(m_scan_head_id);
  // TODO: This isn't needed now, but may be useful on multi-camera systems in
  // the future
  scanRequestPacket.push_back(m_camera_id);
  // TODO: This also isn't needed now.
  scanRequestPacket.push_back(m_laser_id);
  // deprecated exposure setting
  scanRequestPacket.push_back(DEPRECATED_DO_NOT_USE);
  scanRequestPacket.push_back(m_flags);
  index += 6;

  index += SerializeIntegralToBytes(scanRequestPacket, &m_laser_exposure_min_us);
  index += SerializeIntegralToBytes(scanRequestPacket, &m_laser_exposure_def_us);
  index += SerializeIntegralToBytes(scanRequestPacket, &m_laser_exposure_max_us);

  index += SerializeIntegralToBytes(scanRequestPacket, &m_camera_exposure_min_us);
  index += SerializeIntegralToBytes(scanRequestPacket, &m_camera_exposure_def_us);
  index += SerializeIntegralToBytes(scanRequestPacket, &m_camera_exposure_max_us);

  index +=
    SerializeIntegralToBytes(scanRequestPacket, &m_laser_detection_threshold);
  index += SerializeIntegralToBytes(scanRequestPacket, &m_saturation_threshold);
  index += SerializeIntegralToBytes(scanRequestPacket, &m_saturation_percentage);
  index += SerializeIntegralToBytes(scanRequestPacket, &m_average_intensity);

  index += SerializeIntegralToBytes(scanRequestPacket, &m_scan_interval_us);
  index += SerializeIntegralToBytes(scanRequestPacket, &m_scan_offset_us);
  index += SerializeIntegralToBytes(scanRequestPacket, &m_number_of_scans);

  index += SerializeIntegralToBytes(scanRequestPacket, &m_data_types);
  index += SerializeIntegralToBytes(scanRequestPacket, &m_start_col);
  index += SerializeIntegralToBytes(scanRequestPacket, &m_end_col);

  // for each type, uint16 for step
  for (const auto &step : m_steps) {
    index += SerializeIntegralToBytes(scanRequestPacket, &step);
  }

  // static_assert(index == Length());

  return scanRequestPacket;
}

void ScanRequest::SetDataTypesAndSteps(DataType types,
                                       std::vector<uint16_t> steps)
{
  unsigned int num_types = 0;
  for (int i = 1; i <= types; i <<= 1) {
    if (i & types) {
      num_types += 1;
    }
  }

  if (num_types == steps.size()) {
    m_steps = steps;
    m_data_types = types;
  }
}

void ScanRequest::SetLaserExposure(uint32_t min, uint32_t def, uint32_t max)
{
  if (min <= def && def <= max) {
    m_laser_exposure_min_us = min;
    m_laser_exposure_def_us = def;
    m_laser_exposure_max_us = max;
  }
}

void ScanRequest::SetCameraExposure(uint32_t min, uint32_t def, uint32_t max)
{
  if (min <= def && def <= max) {
    m_camera_exposure_min_us = min;
    m_camera_exposure_def_us = def;
    m_camera_exposure_max_us = max;
  }
}

bool ScanRequest::operator==(const ScanRequest &other) const
{
  bool same = true;
  if (m_magic != other.m_magic || m_request_type != other.m_request_type ||
      m_scan_head_id != other.m_scan_head_id || m_camera_id != other.m_camera_id ||
      m_laser_id != other.m_laser_id ||
      m_laser_exposure_min_us != other.m_laser_exposure_min_us ||
      m_laser_exposure_def_us != other.m_laser_exposure_def_us ||
      m_laser_exposure_max_us != other.m_laser_exposure_max_us ||
      m_camera_exposure_min_us != other.m_camera_exposure_min_us ||
      m_camera_exposure_def_us != other.m_camera_exposure_def_us ||
      m_camera_exposure_max_us != other.m_camera_exposure_max_us ||
      m_laser_detection_threshold != other.m_laser_detection_threshold ||
      m_saturation_threshold != other.m_saturation_threshold ||
      m_saturation_percentage != other.m_saturation_percentage ||
      m_average_intensity != other.m_average_intensity ||
      m_scan_interval_us != other.m_scan_interval_us || m_scan_offset_us != other.m_scan_offset_us ||
      m_number_of_scans != other.m_number_of_scans ||
      m_client_ip != other.m_client_ip || m_client_port != other.m_client_port ||
      m_flags != other.m_flags || m_request_sequence != other.m_request_sequence ||
      m_data_types != other.m_data_types || m_start_col != other.m_start_col ||
      m_end_col != other.m_end_col || m_steps.size() != other.m_steps.size()) {
    same = false;
  }

  if (same) {
    for (unsigned int i = 0; i < m_steps.size(); i++) {
      if (m_steps[i] != other.m_steps[i]) {
        same = false;
      }
    }
  }

  return same;
}

bool ScanRequest::operator!=(const ScanRequest &other) const
{
  return !(*this == other);
}
