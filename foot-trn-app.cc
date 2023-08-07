#include "ns3/log.h"
#include "foot-trn-app.h"
#include "ns3/internet-module.h"
#include "ns3/simulator.h"


namespace ns3
{
    NS_LOG_COMPONENT_DEFINE("FootTrnApplication");
    NS_OBJECT_ENSURE_REGISTERED(FootTrnApplication);

    TypeId FootTrnApplication::GetTypeId()
    {
        static TypeId tid = TypeId("ns3::FootTrnApplication")
            .AddConstructor<FootTrnApplication>()
            .SetParent<Application>();
        return tid;
    }

    FootTrnApplication::FootTrnApplication () {}    
    FootTrnApplication::~FootTrnApplication () {}

    void FootTrnApplication::Setup(Inet6SocketAddress sinkAddress)
    {
        NS_LOG_INFO(GetNode());
        m_socket = Socket::CreateSocket(GetNode(), ns3::UdpSocketFactory::GetTypeId());
        m_socket->Bind(sinkAddress);
    }

    void FootTrnApplication::ConfigurePlayerConnection (Inet6SocketAddress playerAddress)
    {
        Ptr<Socket> playerSocket = Socket::CreateSocket(GetNode(), ns3::UdpSocketFactory::GetTypeId());
        playerSocket->Connect(playerAddress);
        m_playerList.push_back(playerSocket);
    }

    void FootTrnApplication::ReadIncoming (Ptr<Socket> socket)
    {
        Ptr<Packet> packet;
        Address from;
        while((packet = socket->RecvFrom(from))) {
            uint32_t size = packet->GetSize();
            uint8_t* buffer = new uint8_t[size];
            packet->CopyData(buffer, size);

            PacketData incomingData;
            std::memcpy(&incomingData, buffer, sizeof(PacketData));
            switch (incomingData.packetType)
            {
                case LOCATION_RESPONSE:
                    // Write response coordinates to file
                    break;
            }
        }
    }

    void FootTrnApplication::SendPacket (Ptr<Socket> socket, uint32_t nodeIndex)
    {
        Ptr<Packet> outgoingPacket;
        int result = socket->Send(outgoingPacket);

        if (result < 0) 
        {
            std::cout << "Error getting packet from player " << nodeIndex << std::endl;
        }

    }

    // Function that actually retrieves player locations. Called from the main simulation code.
    void FootTrnApplication::TrackPlayerLocation (uint32_t playerIndex) {

        std::cout << "Testing" << std::endl;
        if (playerIndex >= m_playerList.size()) {
            playerIndex = 0;
        }

        if (!m_playerList.empty()) {
            Ptr<Socket> playerSocket = m_playerList[playerIndex];

            FootTrnApplication::SendPacket(playerSocket, playerIndex);

            Simulator::Schedule(Seconds(1), &FootTrnApplication::TrackPlayerLocation, this, playerIndex + 1);
        }
    }

    void FootTrnApplication::StartApplication()
    {
        m_socket->SetRecvCallback(MakeCallback (&FootTrnApplication::ReadIncoming, this));
    }

    void FootTrnApplication::StopApplication()
    {
        m_socket->Close();
    }
} // namespace ns3
