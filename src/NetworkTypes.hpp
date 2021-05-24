/**
 * Copyright (c) JoeScan Inc. All Rights Reserved.
 *
 * Licensed under the BSD 3 Clause License. See LICENSE.txt in the project
 * root for license information.
 */

#ifndef JOESCAN_NETWORK_TYPES_H
#define JOESCAN_NETWORK_TYPES_H

#include <cstdint>
#include <string>
#include <vector>

#ifdef __linux__
#include <arpa/inet.h>
#else
#include <WinSock2.h>
#endif

namespace joescan {
/// The number of bits per byte.
static const int kBitsPerByte = 8;

/**
 * The maximum payload of an ethernet frame is 1500 bytes; since we want to
 * limit our datagrams to be conatined in a single ethernet frame, we split
 * all data into datagrams with a maximum of 1500 octets/bytes. Reserve 32
 * bytes for the IP & UDP headers.
 */
static const int kMaxFramePayload = 1468;

/// The port used to send commands to the server running on the scan head.
static const uint16_t kScanServerPort = 12346;
/// Identifier for Status message from scan server.
static const uint16_t kResponseMagic = 0xFACE;
/// Identifier for Data Packet message from scan server.
static const uint16_t kDataMagic = 0xFACD;
/// Identifier for Command messages from client.
static const uint16_t kCommandMagic = kResponseMagic; // Why share value?

using Datagram = std::vector<uint8_t>;

// The DataType is a bit field, it has the flag set for all data types
// present to be sent.
enum DataType : uint16_t {
  Brightness = 0x1,
  XYData = 0x2,
  Width = 0x4,
  SecondMoment = 0x8,
  Subpixel = 0x10,
  Image = 0x20,
  // others here, this is extensible
};

static int GetSizeFor(DataType data_type)
{
  switch (data_type) {
    case XYData: {
      return 2 * sizeof(uint16_t);
    }
    case Width:
    case SecondMoment:
    case Subpixel: {
      return sizeof(uint16_t);
    }
    case Brightness:
    case Image:
    default:
      return sizeof(uint8_t);
  }
}

inline DataType operator|(DataType original, DataType additionalType)
{
  return static_cast<DataType>(static_cast<uint16_t>(original) |
                               static_cast<uint16_t>(additionalType));
}

/**
 * This is the fixed size header for each datagram. The bytes have been
 * packed to word size, so that we can serialize this without resorting to
 * changing the compiler struct packing. All elements are in network byte
 * order, so all elements larger than 1 byte must be converted with
 * hton/ntoh.
 */
#pragma pack(push, 1)
struct DatagramHeader {           // size  byte offset
  uint16_t magic;                 // 2      0
  uint16_t exposure_time_us;      // 2      2
  uint8_t scan_head_id;           // 1      4
  uint8_t camera_id;              // 1      5
  uint8_t laser_id;               // 1      6
  uint8_t flags;                  // 1      7
  uint64_t timestamp_ns;          // 8      8
  uint16_t laser_on_time_us;      // 2     16
  uint16_t data_type;             // 2     18
  uint16_t data_length;           // 2     20
  uint8_t number_encoders;        // 1     22
  uint8_t DEPRECATED_DO_NOT_USE;  // 1     23
  uint32_t datagram_position;     // 4     24
  uint32_t number_datagrams;      // 4     28
  uint16_t start_column;          // 2     32
  uint16_t end_column;            // 2     34
};                                // total 36
#pragma pack(pop)

/**
 * This is the header for any packet that is _not_ a profile
 * or image data packet. This header should never change.
 */
#pragma pack(push, 1)
struct InfoHeader {
  InfoHeader() = default;
  InfoHeader(uint8_t *buf)
  {
    magic = htons(*reinterpret_cast<uint16_t *>(&buf[0]));
    size = buf[2];
    type = buf[3];
  }

  uint16_t magic;
  uint8_t size;
  uint8_t type;
};
#pragma pack(pop)

} // namespace joescan

#endif
