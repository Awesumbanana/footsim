#pragma once

#include "ns3/header.h"

class PacketDataHeader : public ns3::Header
{
    public:
        PacketDataHeader() {}
        virtual ~PacketDataHeader() {}

        void SetPacketType(int type) { m_packetType = type; }
        int GetPacketType() const { return m_packetType; }

        void SetXCoord(double x) { m_xCoord = x; }
        double GetXCoord() const { return m_xCoord; }

        void SetYCoord(double y) { m_yCoord = y; }
        double GetYCoord() const { return m_yCoord; }

        void SetBatteryLevel(double level) { m_batteryLevel = level; }
        double GetBatteryLevel() const { return m_batteryLevel; }

        // NS3 Header methods
        virtual void Serialize(ns3::Buffer::Iterator start) const;
        virtual uint32_t Deserialize(ns3::Buffer::Iterator start);
        virtual uint32_t GetSerializedSize() const;
        virtual void Print(std::ostream &os) const;

        // Needed for NS3 TypeId system
        static ns3::TypeId GetTypeId(void);
        virtual ns3::TypeId GetInstanceTypeId(void) const;

    private:
        int m_packetType;
        double m_xCoord;
        double m_yCoord;
        double m_batteryLevel;
};

