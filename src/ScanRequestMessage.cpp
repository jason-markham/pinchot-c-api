/**
 * Copyright (c) JoeScan Inc. All Rights Reserved.
 *
 * Licensed under the BSD 3 Clause License. See LICENSE.txt in the project
 * root for license information.
 */

#include "ScanRequestMessage.hpp"
#include <algorithm>
#include "DataFormats.hpp"
#include "TcpSerializationHelpers.hpp"

#ifdef __linux__
#include <arpa/inet.h>
#else
#include <winsock2.h>
#endif

using namespace joescan;

ScanRequest::ScanRequest(jsDataFormat format, uint32_t clientAddress,
                         int clientPort, int scanHeadId, uint32_t interval,
                         uint32_t scanCount,
                         const ScanHeadConfiguration &config)
{
  this->clientAddress = clientAddress;
  this->clientPort = clientPort;
  this->scanHeadId = scanHeadId;
  this->cameraId = 0; // TODO: If these become useful, don't hardcode.
  this->laserId = 0;  // TODO: If these become useful, don't hardcode.
  this->flags = 0;    // TODO: If these become useful, don't hardcode.
  minimumLaserExposure = static_cast<uint32_t>(config.MinLaserOn() * 1000.0);
  defaultLaserExposure =
    static_cast<uint32_t>(config.DefaultLaserOn() * 1000.0);
  maximumLaserExposure = static_cast<uint32_t>(config.MaxLaserOn() * 1000.0);
  minimumCameraExposure = static_cast<uint32_t>(config.MinExposure() * 1000.0);
  defaultCameraExposure =
    static_cast<uint32_t>(config.DefaultExposure() * 1000.0);
  maximumCameraExposure = static_cast<uint32_t>(config.MaxExposure() * 1000.0);
  laserDetectionThreshold = config.GetLaserDetectionThreshold();
  saturationThreshold = config.GetSaturationThreshold();
  saturationPercent = config.SaturatedPercentage();
  averageImageIntensity = config.AverageIntensity();
  scanInterval = interval;
  scanOffset = static_cast<uint32_t>(config.GetScanOffset());
  // If the caller requested a specific number of scans, obey them, otherwise
  // let's hope they didn't mean to scan for a whole week.
  numberOfScans = (scanCount == 0) ? 1000000 : scanCount;
  startCol = 0;
  endCol = 1455;

  dataTypes = DataFormats::GetDataType(format);
  steps = DataFormats::GetStep(format);
}

ScanRequest::ScanRequest(const Datagram &datagram)
{
  if (datagram.size() < 76) {
    // throw std::exception();
  }
  int index = 0;

  magic = ntohs(*reinterpret_cast<const uint16_t *>(&datagram[index]));
  if (magic != kCommandMagic) {
    throw std::exception();
  }
  index += sizeof(uint16_t);

  uint8_t size = datagram[index++];

  requestType = UdpPacketType::_from_integral(datagram[index++]);

  clientAddress =
    ntohl(*(reinterpret_cast<const uint32_t *>(&datagram[index])));
  index += sizeof(uint32_t);
  clientPort = ntohs(*(reinterpret_cast<const uint16_t *>(&datagram[index])));
  index += sizeof(uint16_t);
  requestSequence = datagram[index++];

  scanHeadId = datagram[index++];
  cameraId = datagram[index++];
  laserId = datagram[index++];
  exposureMode = CameraExposureMode::_from_integral(datagram[index++]);
  flags = datagram[index++];

  minimumLaserExposure =
    ntohl(*(reinterpret_cast<const uint32_t *>(&datagram[index])));
  index += sizeof(uint32_t);
  defaultLaserExposure =
    ntohl(*(reinterpret_cast<const uint32_t *>(&datagram[index])));
  index += sizeof(uint32_t);
  maximumLaserExposure =
    ntohl(*(reinterpret_cast<const uint32_t *>(&datagram[index])));
  index += sizeof(uint32_t);

  minimumCameraExposure =
    ntohl(*(reinterpret_cast<const uint32_t *>(&datagram[index])));
  index += sizeof(uint32_t);
  defaultCameraExposure =
    ntohl(*(reinterpret_cast<const uint32_t *>(&datagram[index])));
  index += sizeof(uint32_t);
  maximumCameraExposure =
    ntohl(*(reinterpret_cast<const uint32_t *>(&datagram[index])));
  index += sizeof(uint32_t);

  laserDetectionThreshold =
    ntohl(*(reinterpret_cast<const uint32_t *>(&datagram[index])));
  index += sizeof(uint32_t);
  saturationThreshold =
    ntohl(*(reinterpret_cast<const uint32_t *>(&datagram[index])));
  index += sizeof(uint32_t);
  saturationPercent =
    ntohl(*(reinterpret_cast<const uint32_t *>(&datagram[index])));
  index += sizeof(uint32_t);
  averageImageIntensity =
    ntohl(*(reinterpret_cast<const uint32_t *>(&datagram[index])));
  index += sizeof(uint32_t);

  scanInterval = ntohl(*(reinterpret_cast<const uint32_t *>(&datagram[index])));
  index += sizeof(uint32_t);
  scanOffset = ntohl(*(reinterpret_cast<const uint32_t *>(&datagram[index])));
  index += sizeof(uint32_t);
  numberOfScans =
    ntohl(*(reinterpret_cast<const uint32_t *>(&datagram[index])));
  index += sizeof(uint32_t);

  dataTypes = ntohs(*(reinterpret_cast<const uint16_t *>(&datagram[index])));
  index += sizeof(uint16_t);
  startCol = ntohs(*(reinterpret_cast<const uint16_t *>(&datagram[index])));
  index += sizeof(uint16_t);
  endCol = ntohs(*(reinterpret_cast<const uint16_t *>(&datagram[index])));
  index += sizeof(uint16_t);

  for (int i = 1; i <= dataTypes; i <<= 1) {
    if (i & dataTypes) {
      uint16_t stepVal =
        ntohs(*(reinterpret_cast<const uint16_t *>(&datagram[index])));
      steps.push_back(stepVal);
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

Datagram ScanRequest::Serialize(uint8_t requestSequence)
{
  std::vector<uint8_t> scanRequestPacket;
  scanRequestPacket.reserve(Length());

  int index = 0;
  index += SerializeIntegralToBytes(scanRequestPacket, &kCommandMagic);

  scanRequestPacket.push_back(Length());
  scanRequestPacket.push_back(requestType);
  index += 2;

  index += SerializeIntegralToBytes(scanRequestPacket, &clientAddress);
  index += SerializeIntegralToBytes(scanRequestPacket, &clientPort);

  scanRequestPacket.push_back(requestSequence);
  scanRequestPacket.push_back(scanHeadId);
  // TODO: This isn't needed now, but may be useful on multi-camera systems in
  // the future
  scanRequestPacket.push_back(cameraId);
  // TODO: This also isn't needed now.
  scanRequestPacket.push_back(laserId);
  scanRequestPacket.push_back(exposureMode);
  scanRequestPacket.push_back(flags);
  index += 6;

  index += SerializeIntegralToBytes(scanRequestPacket, &minimumLaserExposure);
  index += SerializeIntegralToBytes(scanRequestPacket, &defaultLaserExposure);
  index += SerializeIntegralToBytes(scanRequestPacket, &maximumLaserExposure);

  index += SerializeIntegralToBytes(scanRequestPacket, &minimumCameraExposure);
  index += SerializeIntegralToBytes(scanRequestPacket, &defaultCameraExposure);
  index += SerializeIntegralToBytes(scanRequestPacket, &maximumCameraExposure);

  index +=
    SerializeIntegralToBytes(scanRequestPacket, &laserDetectionThreshold);
  index += SerializeIntegralToBytes(scanRequestPacket, &saturationThreshold);
  index += SerializeIntegralToBytes(scanRequestPacket, &saturationPercent);
  index += SerializeIntegralToBytes(scanRequestPacket, &averageImageIntensity);

  index += SerializeIntegralToBytes(scanRequestPacket, &scanInterval);
  index += SerializeIntegralToBytes(scanRequestPacket, &scanOffset);
  index += SerializeIntegralToBytes(scanRequestPacket, &numberOfScans);

  index += SerializeIntegralToBytes(scanRequestPacket, &dataTypes);
  index += SerializeIntegralToBytes(scanRequestPacket, &startCol);
  index += SerializeIntegralToBytes(scanRequestPacket, &endCol);

  // for each type, uint16 for step
  for (const auto &step : steps) {
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
    this->steps = steps;
    dataTypes = types;
  }
}

void ScanRequest::SetLaserExposure(uint32_t min, uint32_t def, uint32_t max)
{
  if (min <= def && def <= max) {
    minimumLaserExposure = min;
    defaultLaserExposure = def;
    maximumLaserExposure = max;
  }
}

void ScanRequest::SetCameraExposure(uint32_t min, uint32_t def, uint32_t max)
{
  if (min <= def && def <= max) {
    minimumCameraExposure = min;
    defaultCameraExposure = def;
    maximumCameraExposure = max;
  }
}

bool ScanRequest::operator==(const ScanRequest &other) const
{
  bool same = true;
  if (magic != other.magic || requestType != other.requestType ||
      scanHeadId != other.scanHeadId || cameraId != other.cameraId ||
      laserId != other.laserId || exposureMode != other.exposureMode ||
      minimumLaserExposure != other.minimumLaserExposure ||
      defaultLaserExposure != other.defaultLaserExposure ||
      maximumLaserExposure != other.maximumLaserExposure ||
      minimumCameraExposure != other.minimumCameraExposure ||
      defaultCameraExposure != other.defaultCameraExposure ||
      maximumCameraExposure != other.maximumCameraExposure ||
      laserDetectionThreshold != other.laserDetectionThreshold ||
      saturationThreshold != other.saturationThreshold ||
      saturationPercent != other.saturationPercent ||
      averageImageIntensity != other.averageImageIntensity ||
      scanInterval != other.scanInterval || scanOffset != other.scanOffset ||
      numberOfScans != other.numberOfScans ||
      clientAddress != other.clientAddress || clientPort != other.clientPort ||
      flags != other.flags || requestSequence != other.requestSequence ||
      dataTypes != other.dataTypes || startCol != other.startCol ||
      endCol != other.endCol || steps.size() != other.steps.size()) {
    same = false;
  }

  if (same) {
    for (unsigned int i = 0; i < steps.size(); i++) {
      if (steps[i] != other.steps[i]) {
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
