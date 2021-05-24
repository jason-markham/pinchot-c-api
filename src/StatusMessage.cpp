/**
 * Copyright (c) JoeScan Inc. All Rights Reserved.
 *
 * Licensed under the BSD 3 Clause License. See LICENSE.txt in the project
 * root for license information.
 */

#include "StatusMessage.hpp"
#include "Enums.hpp"
#include "TcpSerializationHelpers.hpp"
#include "VersionParser.hpp"
#include "joescan_pinchot.h"

using namespace joescan;

StatusMessage::StatusMessage()
{
  packet.header.magic = kResponseMagic;
  packet.header.size = kMinStatusMessageSize;
  packet.header.type = UdpPacketType::Status;
  packet.global_time = 0;
}

StatusMessage::StatusMessage(uint32_t scan_head_ip, uint32_t serial_number,
                             uint32_t max_scan_rate, VersionInformation version)
  : StatusMessage()
{
  packet.scan_head_ip = scan_head_ip;
  packet.serial_number = serial_number;
  packet.max_scan_rate = max_scan_rate;
  packet.version = version;
  packet.global_time = 0;
}

StatusMessage::StatusMessage(uint8_t *bytes, uint32_t num_bytes)
{
  if ((num_bytes < kMinStatusMessageSize) ||
      (num_bytes > kMaxStatusMessageSize)) {
    throw std::runtime_error("Invalid number of status bytes");
  }

  uint8_t *idx_p = bytes;

  // InfoHeader
  idx_p += ExtractFromNetworkBuffer(packet.header.magic, idx_p);
  idx_p += ExtractFromNetworkBuffer(packet.header.size, idx_p);
  idx_p += ExtractFromNetworkBuffer(packet.header.type, idx_p);
  ValidatePacketHeader(packet.header);

  // VersionInformation
  idx_p += VersionParser::Deserialize(packet.version, idx_p);
  ValidatePacketVersion(packet.version);

  // Static Data
  idx_p += ExtractFromNetworkBuffer(packet.serial_number, idx_p);
  idx_p += ExtractFromNetworkBuffer(packet.max_scan_rate, idx_p);
  idx_p += ExtractFromNetworkBuffer(packet.scan_head_ip, idx_p);
  idx_p += ExtractFromNetworkBuffer(packet.client_ip, idx_p);
  idx_p += ExtractFromNetworkBuffer(packet.client_port, idx_p);
  idx_p += ExtractFromNetworkBuffer(packet.scan_sync_id, idx_p);
  idx_p += ExtractFromNetworkBuffer(packet.global_time, idx_p);
  idx_p += ExtractFromNetworkBuffer(packet.num_packets_sent, idx_p);
  idx_p += ExtractFromNetworkBuffer(packet.num_profiles_sent, idx_p);
  idx_p += ExtractFromNetworkBuffer(packet.valid_encoders, idx_p);
  idx_p += ExtractFromNetworkBuffer(packet.valid_cameras, idx_p);
  idx_p += ExtractFromNetworkBuffer(packet.reserved_0, idx_p);
  idx_p += ExtractFromNetworkBuffer(packet.reserved_1, idx_p);
  idx_p += ExtractFromNetworkBuffer(packet.reserved_2, idx_p);
  idx_p += ExtractFromNetworkBuffer(packet.reserved_3, idx_p);
  idx_p += ExtractFromNetworkBuffer(packet.reserved_4, idx_p);
  idx_p += ExtractFromNetworkBuffer(packet.reserved_5, idx_p);
  idx_p += ExtractFromNetworkBuffer(packet.reserved_6, idx_p);
  idx_p += ExtractFromNetworkBuffer(packet.reserved_7, idx_p);
  ValidatePacketData(packet);

  // Variable Data
  for (int i = 0; i < packet.valid_encoders; i++) {
    idx_p += ExtractFromNetworkBuffer(packet.encoders[i], idx_p);
  }

  for (int i = 0; i < packet.valid_cameras; i++) {
    idx_p += ExtractFromNetworkBuffer(packet.pixels_in_window[i], idx_p);
  }

  for (int i = 0; i < packet.valid_cameras; i++) {
    idx_p += ExtractFromNetworkBuffer(packet.camera_temp[i], idx_p);
  }

  if (idx_p != (bytes + packet.header.size)) {
    throw std::runtime_error("Failed to extract the status message");
  }
}

std::vector<uint8_t> StatusMessage::Serialize() const
{
  std::vector<uint8_t> message;
  message.reserve(kMaxStatusMessageSize);

  ValidatePacketHeader(packet.header);
  ValidatePacketData(packet);

  // InfoHeader
  SerializeIntegralToBytes(message, &packet.header.magic);
  SerializeIntegralToBytes(message, &packet.header.size);
  SerializeIntegralToBytes(message, &packet.header.type);

  // VersionInformation
  VersionParser::Serialize(message, packet.version);

  // Static Data
  SerializeIntegralToBytes(message, &packet.serial_number);
  SerializeIntegralToBytes(message, &packet.max_scan_rate);
  SerializeIntegralToBytes(message, &packet.scan_head_ip);
  SerializeIntegralToBytes(message, &packet.client_ip);
  SerializeIntegralToBytes(message, &packet.client_port);
  SerializeIntegralToBytes(message, &packet.scan_sync_id);
  SerializeIntegralToBytes(message, &packet.global_time);
  SerializeIntegralToBytes(message, &packet.num_packets_sent);
  SerializeIntegralToBytes(message, &packet.num_profiles_sent);
  SerializeIntegralToBytes(message, &packet.valid_encoders);
  SerializeIntegralToBytes(message, &packet.valid_cameras);

  // Reserved for future use
  SerializeIntegralToBytes(message, &packet.reserved_0);
  SerializeIntegralToBytes(message, &packet.reserved_1);
  SerializeIntegralToBytes(message, &packet.reserved_2);
  SerializeIntegralToBytes(message, &packet.reserved_3);
  SerializeIntegralToBytes(message, &packet.reserved_4);
  SerializeIntegralToBytes(message, &packet.reserved_5);
  SerializeIntegralToBytes(message, &packet.reserved_6);
  SerializeIntegralToBytes(message, &packet.reserved_7);

  // Variable Data
  for (int i = 0; i < packet.valid_encoders; i++) {
    SerializeIntegralToBytes(message, &packet.encoders[i]);
  }

  for (int i = 0; i < packet.valid_cameras; i++) {
    SerializeIntegralToBytes(message, &packet.pixels_in_window[i]);
  }

  for (int i = 0; i < packet.valid_cameras; i++) {
    SerializeIntegralToBytes(message, &packet.camera_temp[i]);
  }

  // We don't know the size of the message until this point, so
  // update the size field in the serialized message
  InfoHeader *hdr = reinterpret_cast<InfoHeader *>(message.data());
  hdr->size = static_cast<uint8_t>(message.size());

  return message;
}

void StatusMessage::ValidatePacketHeader(const InfoHeader &hdr)
{
  if (hdr.magic != kResponseMagic) {
    throw std::runtime_error("Invalid magic for status message");
  }

  if ((kMinStatusMessageSize > hdr.size) ||
      (kMaxStatusMessageSize < hdr.size)) {
    throw std::runtime_error("Invalid size for status message");
  }

  if (hdr.type != +UdpPacketType::Status) {
    throw std::runtime_error("Invalid type for status message");
  }
}

void StatusMessage::ValidatePacketData(const StatusMessagePacket &pkt)
{
  if (pkt.valid_encoders > kMaxEncoders) {
    throw std::runtime_error("Invalid number of encoders");
  }

  if (pkt.valid_cameras > kMaxCameras) {
    throw std::runtime_error("Invalid number of cameras");
  }
}

void StatusMessage::ValidatePacketVersion(const VersionInformation &ver)
{
  if (ver.product == JS_SCAN_HEAD_INVALID_TYPE ||
      ((ver.product != JS_SCAN_HEAD_JS50WX) &&
       (ver.product != JS_SCAN_HEAD_JS50WSC))) {
    throw std::runtime_error("Invalid product ID: " +
                             std::to_string(ver.product));
  }

  if (ver.major == 0) {
    throw std::runtime_error("Invalid major number");
  }

  if (ver.commit == 0) {
    throw std::runtime_error("Invalid commit hash");
  }
}

void StatusMessage::SetClientAddressInfo(uint32_t ip, uint16_t port)
{
  packet.client_ip = ip;
  packet.client_port = port;
}

void StatusMessage::SetGlobalTime(uint64_t global_time)
{
  packet.global_time = global_time;
}

void StatusMessage::SetEncoders(std::vector<int64_t> encoders)
{
  if (kMaxEncoders > encoders.size()) {
    for (unsigned int n = 0; n < encoders.size(); n++) {
      packet.encoders[n] = encoders[n];
    }
    packet.valid_encoders = static_cast<uint8_t>(encoders.size());
  }
}

void StatusMessage::SetValidCameras(uint8_t num_valid)
{
  packet.valid_cameras = num_valid;
}

uint8_t StatusMessage::GetValidCameras() const
{
  return packet.valid_cameras;
}

void StatusMessage::SetPixelsInWindow(int camera, int32_t pixels_in_window)
{
  if (camera >= 0 && camera < kMaxCameras) {
    packet.pixels_in_window[camera] = pixels_in_window;
  }
}

void StatusMessage::SetScanSyncId(uint16_t id)
{
  packet.scan_sync_id = id;
}

void StatusMessage::SetCameraTemperature(int camera, int32_t temp)
{
  if (camera >= 0 && camera < kMaxCameras) {
    packet.camera_temp[camera] = temp;
  }
}

void StatusMessage::SetMaxScanRate(uint32_t scan_rate)
{
  packet.max_scan_rate = scan_rate;
}

void StatusMessage::SetNumPacketsSent(uint32_t num_packets_sent)
{
  packet.num_packets_sent = num_packets_sent;
}

void StatusMessage::SetNumProfilesSent(uint32_t num_profiles_sent)
{
  packet.num_profiles_sent = num_profiles_sent;
}

VersionInformation StatusMessage::GetVersionInformation() const
{
  return packet.version;
}

uint32_t StatusMessage::GetScanHeadIp() const
{
  return packet.scan_head_ip;
}

uint32_t StatusMessage::GetSerialNumber() const
{
  return packet.serial_number;
}

uint16_t StatusMessage::GetScanSyncId() const
{
  return packet.scan_sync_id;
}

uint64_t StatusMessage::GetGlobalTime() const
{
  return packet.global_time;
}

std::vector<int64_t> StatusMessage::GetEncoders(void) const
{
  std::vector<int64_t> encoders;

  for (int n = 0; n < packet.valid_encoders; n++) {
    encoders.push_back(packet.encoders[n]);
  }

  return encoders;
}

uint32_t StatusMessage::GetClientIp() const
{
  return packet.client_ip;
}

uint16_t StatusMessage::GetClientPort() const
{
  return packet.client_port;
}

int32_t StatusMessage::GetPixelsInWindow(int camera) const
{
  if (camera >= 0 && camera < kMaxCameras) {
    return packet.pixels_in_window[camera];
  }

  return -1;
}

int32_t StatusMessage::GetCameraTemperature(int camera) const
{
  if (camera >= 0 && camera < kMaxCameras) {
    return packet.camera_temp[camera];
  }

  return -1;
}

uint32_t StatusMessage::GetMaxScanRate() const
{
  return packet.max_scan_rate;
}

uint32_t StatusMessage::GetNumPacketsSent() const
{
  return packet.num_packets_sent;
}

uint32_t StatusMessage::GetNumProfilesSent() const
{
  return packet.num_profiles_sent;
}
