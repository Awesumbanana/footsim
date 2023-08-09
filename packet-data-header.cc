#include "packet-data-header.h"

// Serialize
void PacketDataHeader::Serialize(ns3::Buffer::Iterator start) const {
  start.WriteHtonU32(m_packetType);
  start.WriteHtonU64(*reinterpret_cast<const uint64_t*>(&m_xCoord));
  start.WriteHtonU64(*reinterpret_cast<const uint64_t*>(&m_yCoord));
  start.WriteHtonU64(*reinterpret_cast<const uint64_t*>(&m_batteryLevel));
}

// Deserialize
uint32_t PacketDataHeader::Deserialize(ns3::Buffer::Iterator start) {
  m_packetType = start.ReadNtohU32();
  m_xCoord = *reinterpret_cast<const double*>(start.ReadNtohU64());
  m_yCoord = *reinterpret_cast<const double*>(start.ReadNtohU64());
  m_batteryLevel = *reinterpret_cast<const double*>(start.ReadNtohU64());
  return GetSerializedSize();
}

// GetSerializedSize
uint32_t PacketDataHeader::GetSerializedSize() const {
  return sizeof(m_packetType) + 3 * sizeof(double);
}

// Print
void PacketDataHeader::Print(std::ostream &os) const {
  os << "PacketType: " << m_packetType << ", X: " << m_xCoord << ", Y: " << m_yCoord << ", Battery: " << m_batteryLevel;
}

// TypeId
ns3::TypeId PacketDataHeader::GetTypeId(void) {
  static ns3::TypeId tid = ns3::TypeId("PacketDataHeader")
    .SetParent<Header>()
    .AddConstructor<PacketDataHeader>();
  return tid;
}

ns3::TypeId PacketDataHeader::GetInstanceTypeId(void) const
{
  return GetTypeId();
}