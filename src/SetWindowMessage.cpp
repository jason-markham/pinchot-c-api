/**
 * Copyright (c) JoeScan Inc. All Rights Reserved.
 *
 * Licensed under the BSD 3 Clause License. See LICENSE.txt in the project
 * root for license information.
 */

#include "SetWindowMessage.hpp"
#include "TcpSerializationHelpers.hpp"
#include "enum.h"

#include <fstream>

using namespace joescan;

SetWindowMessage::SetWindowMessage(int camera)
{
  m_camera = camera;
}

SetWindowMessage SetWindowMessage::Deserialize(std::vector<uint8_t> &message)
{
  int index = 0;
  uint16_t magic = htons(*(reinterpret_cast<uint16_t *>(&(message[index]))));
  index += sizeof(uint16_t);
  if (magic != kCommandMagic) {
    throw std::exception();
  }

  index++; // skip size
  if (message[index++] != +UdpPacketType::SetWindow) {
    throw std::exception();
  }

  SetWindowMessage msg(-1);
  msg.SetCamera(message[index++]);
  index += 3; // skip unused bytes

  while ((index + (4 * sizeof(int32_t))) <= message.size()) {
    int32_t *ptr = reinterpret_cast<int32_t *>(&message[index]);
    // note, units are in 1/1000 inch
    int32_t x1 = htonl(ptr[0]);
    int32_t y1 = htonl(ptr[1]);
    int32_t x2 = htonl(ptr[2]);
    int32_t y2 = htonl(ptr[3]);

    msg.AddConstraint(x1, y1, x2, y2);
    index += 4 * sizeof(int32_t);
  }

  return msg;
}

std::vector<uint8_t> SetWindowMessage::Serialize() const
{
  std::vector<uint8_t> message;
  int constraint_size = 3 * sizeof(int32_t);
  int message_size = 0;
  {
    // TODO: This needs to be better explained/documented. It's not really
    // obvious as to how the `sizeof` calls relate to the message fields.
    size_t sz = (m_constraints.size() * constraint_size) +
                (4 * sizeof(uint8_t)) + sizeof(uint32_t);
    message_size = static_cast<int>(sz);
  }
  message.reserve(message_size);

  size_t index = 0;
  index += SerializeIntegralToBytes(message, &kCommandMagic);
  message.push_back(message_size);
  UdpPacketType type = UdpPacketType::SetWindow;
  message.push_back(type._to_integral());
  message.push_back(m_camera);
  message.push_back(0);
  message.push_back(0);
  message.push_back(0);
  index += 6;

  for (auto &window_constraint : m_constraints) {
    int32_t x, y;
    // note, units are in 1/1000 inch
    x = static_cast<int32_t>(window_constraint.constraints[0].x);
    index += SerializeIntegralToBytes(message, &(x));
    y = static_cast<int32_t>(window_constraint.constraints[0].y);
    index += SerializeIntegralToBytes(message, &(y));
    x = static_cast<int32_t>(window_constraint.constraints[1].x);
    index += SerializeIntegralToBytes(message, &(x));
    y = static_cast<int32_t>(window_constraint.constraints[1].y);
    index += SerializeIntegralToBytes(message, &(y));
  }

  return message;
}

void SetWindowMessage::AddConstraints(
  std::vector<WindowConstraint> &constraints)
{
  for (auto &constraint : constraints) {
    m_constraints.push_back(constraint);
  }
}

bool SetWindowMessage::SatisfiesConstraints(int32_t x, int32_t y)
{
  return SatisfiesConstraints(Point2D<int64_t>(x, y));
}

bool SetWindowMessage::SatisfiesConstraints(Point2D<int64_t> p)
{
  for (auto &constraint : m_constraints) {
    if (!constraint.Satisfies(p)) {
      return false;
    }
  }

  return true;
}

std::vector<WindowConstraint> SetWindowMessage::Constraints() const
{
  return m_constraints;
}
