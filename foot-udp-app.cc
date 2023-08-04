#include "ns3/log.h"
#include "ns3/config.h"
#include "foot-udp-app.h"
#include "ns3/simulator.h"

namespace ns3
{

    NS_LOG_COMPONENT_DEFINE("FootUdpApplication");
    NS_OBJECT_ENSURE_REGISTERED(FootUdpApplication);

    TypeId FootUdpApplication::GetTypeId()
    {
        static TypeId tid = TypeId("ns3::FootUdpApplication")
            .AddConstructor<FootUdpApplication>()
            .SetParent<Application>();
        return tid;
    }

    FootUdpApplication::FootUdpApplication () {}

    FootUdpApplication::~FootUdpApplication () {}

    // Configures socket for self and listens for packets. Reading is handled in StartApplication
    void FootUdpApplication::Setup(Inet6SocketAddress address)
    {
        TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
        m_socket = Socket::CreateSocket(GetNode(), tid);
        m_socket->Bind(address);

        for (Transmitter trn : m_transmitters) {
            
        }
    }

    // Creates a socket to listen to packets from a player. Created for all players. 
    void FootUdpApplication::AddPlayer (Inet6SocketAddress playerAddress)
    {
        TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
        Ptr<Socket> playerSocket = Socket::CreateSocket(GetNode(), tid);
        playerSocket->Connect(playerAddress);
        Neighbor n(0.0, 100.0, playerSocket, Point(0.0, 0.0));
        m_playerList.push_back(n);
    }

    void FootUdpApplication::AddTransmitter (Inet6SocketAddress trnAddress, Point trnCoords)
    {
        TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
        Ptr<Socket> trnSocket = Socket::CreateSocket(GetNode(), tid);
        trnSocket->Connect(trnAddress);
        Transmitter trn(trnSocket, trnCoords);
        m_transmitters.push_back(trn);
    }

    std::vector<Neighbor> FootUdpApplication::GetBestNeighbors () {
        std::vector<Neighbor> neighbors;
        // -Decision tree part-  
        Neighbor n = m_playerList[0]; //TODO: Temporary, change to actual result
        neighbors.push_back(n);
        return neighbors;
    }

    Point FootUdpApplication::GetLocation () {
        std::vector<Neighbor> closePlayers = GetBestNeighbors();
        Point currentLocation(10, 10);

        return currentLocation;
    }
    
    void FootUdpApplication::HandleRead (Ptr<Socket> socket) {
        Ptr<Packet> packet;
        Address from;
        while(packet = socket->RecvFrom(from)) {
            GetLocation();
        }
    }

    void FootUdpApplication::SniffRx (std::string context, double rss) {
    }

    void FootUdpApplication::SetInitialPosition () {
    }

    void FootUdpApplication::StartApplication () {
        FootUdpApplication::SetInitialPosition();
        m_socket->SetRecvCallback(MakeCallback(&FootUdpApplication::HandleRead, this));
        std::string node_id = std::to_string (GetNode ()->GetId ());
        Config::Connect("/NodeList/*/DeviceList/*/Phy/MonitorSnifferRx", MakeCallback(&FootUdpApplication::SniffRx, this));
    }

    void FootUdpApplication::StopApplication () {
        m_socket->SetRecvCallback(MakeNullCallback<void, ns3::Ptr<ns3::Socket>>());
        m_socket->Close();
    }
} // namespace ns3
