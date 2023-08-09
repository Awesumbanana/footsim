#include "ns3/log.h"
#include "ns3/config.h"
#include "foot-udp-app.h"
#include "packet-data-header.h"
#include "ns3/simulator.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-phy.h"

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
        // for (Transmitter trn : m_transmitters) {
            
        // }
    }

    // Creates a socket to listen to packets from a player. Created for all players. 
    void FootUdpApplication::AddPlayer (Inet6SocketAddress playerAddress)
    {
        TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
        Ptr<Socket> playerSocket = Socket::CreateSocket(GetNode(), tid);
        playerSocket->Connect(playerAddress);
        Neighbor n(0.0, 100.0, 0.0, playerSocket, Point(0.0, 0.0));
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

    // This function returns the score of a neighbor based on weights for attributes
    double FootUdpApplication::ComputeScore(const Neighbor& player)
    {
        // More possible factors such as role of players
        const double distanceWeight = 0.5;
        const double batteryWeight = 0.3;
        const double signalWeight = 0.2;

        return distanceWeight / player.distanceFromMe    
            + batteryWeight * player.batteryLevel
            + signalWeight * player.oldSignalStrength;
    }

    std::vector<Neighbor> FootUdpApplication::GetBestNeighbors () {
        std::vector<Neighbor> neighbors;
        int numPlayers = 5;
        
        std::vector<int> indices(m_playerList.size());
        std::iota(indices.begin(), indices.end(), 0);
        // for (int i = 0; i < m_playerList.size(); ++i) {
        //     indices[i] = i;
        // }

        auto comp = [this](int a, int b) { return FootUdpApplication::ComputeScore(m_playerList[a]) > FootUdpApplication::ComputeScore(m_playerList[b]); };

        // Partially sort the vector to get the top numPlayers elements
        std::nth_element(indices.begin(), indices.begin() + numPlayers, indices.end(), comp);

        // Sort the top numPlayers elements to maintain their relative order
        std::sort(indices.begin(), indices.begin() + numPlayers, comp);
        indices.resize(numPlayers);

        for (int i : indices) {
            neighbors.push_back(m_playerList[i]);
        }
        return neighbors;
    }

    // Function to check if the RSSD value is positive
    bool isPositiveRSSD(double RSSD, double delta) {
        return RSSD > delta;
    }

    // Function to calculate the intersection of circles (possible and marginal zones)
    std::vector<Point> circleIntersection(Point center1, double radius1, Point center2, double radius2) {
        // Calculate the distance between the two centers
        double distance = sqrt((center2.x - center1.x) * (center2.x - center1.x) +
                            (center2.y - center1.y) * (center2.y - center1.y));

        // Check if the circles intersect
        if (distance > radius1 + radius2 || distance < fabs(radius1 - radius2)) {
            // No intersection
            return {};
        }

        // Calculate the intersection points
        double a = (radius1 * radius1 - radius2 * radius2 + distance * distance) / (2 * distance);
        double h = sqrt(radius1 * radius1 - a * a);
        double x2 = center1.x + a * (center2.x - center1.x) / distance;
        double y2 = center1.y + a * (center2.y - center1.y) / distance;

        double intersectionX1 = x2 + h * (center2.y - center1.y) / distance;
        double intersectionY1 = y2 - h * (center2.x - center1.x) / distance;
        double intersectionX2 = x2 - h * (center2.y - center1.y) / distance;
        double intersectionY2 = y2 + h * (center2.x - center1.x) / distance;

        std::vector<Point> intersections;
        intersections.push_back({intersectionX1, intersectionY1});
        intersections.push_back({intersectionX2, intersectionY2});

        return intersections;
    }

    Point FootUdpApplication::GetLocation () {
        std::vector<Neighbor> closePlayers = GetBestNeighbors();
        Point currentLocation(10, 10);

        int numNodes = m_playerList.size();

        double delta = 0.5;
        // Create a vector of pairs (radius, index) to store the radius of each zone and its corresponding index
        std::vector<std::pair<double, int>> zoneRadii;
        for (int i = 0; i < numNodes; ++i) {
            bool isPositive = isPositiveRSSD(0.0, delta);
            if (isPositive) {
                double distance = sqrt((m_playerList[i].coord.x - m_playerList[0].coord.x) * (m_playerList[i].coord.x - m_playerList[0].coord.x) +
                                    (m_playerList[i].coord.y - m_playerList[0].coord.y) * (m_playerList[i].coord.y - m_playerList[0].coord.y));
                zoneRadii.push_back({distance, i});
            }
        }

        // Sort the zones in ascending order of the zone radius
        std::sort(zoneRadii.begin(), zoneRadii.end());

        // Calculate the possible zone by intersecting the smallest zones first
        Point possibleZoneCenter = m_playerList[0].coord;
        double possibleZoneRadius = 0.0;

        for (const auto& zone : zoneRadii) {
            int i = zone.second;
            // double distance = zone.first;

            // Calculate the intersection with the current zone
            std::vector<Point> intersections = circleIntersection(possibleZoneCenter, possibleZoneRadius, m_playerList[i].coord, 0.0);
            if (!intersections.empty()) {
                possibleZoneCenter = intersections[0];
                possibleZoneRadius = 0.0;
            }
        }

        // Use the possible zone center as the estimated location
        Point estimatedLocation = possibleZoneCenter;

        return estimatedLocation;
    }
    
    // Get the location of the player and send back to sink
    void FootUdpApplication::HandleRead (Ptr<Socket> socket) {
        Ptr<Packet> packet;
        Address from;
        while(packet = socket->RecvFrom(from)) {
            // uint32_t size = packet->GetSize();
            // uint8_t buffer[size];
            // packet->CopyData(buffer, size);
            // PacketData incomingData;
            // std::memcpy(&incomingData, buffer, sizeof(PacketData));

            PacketDataHeader header;
            packet->RemoveHeader(header);

            switch (header.GetPacketType())
            {
                case LOCATION_REQUEST:
                {
                    NS_LOG_INFO("Location request");
                    std::vector<Neighbor> closePlayers = GetBestNeighbors();
                    break;
                }
                case INFO_REQUEST:
                {
                    PacketData playerInformation(INFO_RESPONSE, m_currentPosition.x, m_currentPosition.y, m_batteryLevel);
                    uint8_t infResBuffer[sizeof(PacketData)];
                    std::memcpy(infResBuffer, &playerInformation, sizeof(PacketData));
                    Ptr<Packet> responsePacket = Create<Packet>(infResBuffer, sizeof(PacketData));
                    socket->SendTo(responsePacket, 0, from);
                    break;
                }
                case INFO_RESPONSE:
                {

                    break;
                }
            }

            Point playerLocation = GetLocation();
            uint8_t sendBuffer[sizeof(Point)];
            std::memcpy(sendBuffer, &playerLocation, sizeof(Point));
            Ptr<Packet> response = Create<Packet>(sendBuffer, sizeof(Point));
            socket->SendTo(response, 0, from);
        }
    }

    void FootUdpApplication::SniffRx (
        std::string context,
        Ptr<const Packet> packet,
        uint16_t channelFreqMhz,
        WifiTxVector txVector,
        MpduInfo aMpdu,
        SignalNoiseDbm signalNoise,
        uint16_t staId) 
    {
        // double rssi = signalNoise.signal;
        Ptr<Packet> copyPacket = packet->Copy();
        PacketDataHeader header;
        copyPacket->RemoveHeader(header);
        uint32_t size = copyPacket->GetSize();
        NS_LOG_INFO("Sniffer");
        if (size != sizeof(PacketData)) {
            NS_LOG_INFO("Not trn track packet");
        }
        uint8_t buffer[size];
        packet->CopyData(buffer, size);
        PacketData incomingData;
        std::memcpy(&incomingData, buffer, sizeof(PacketData));
        NS_LOG_INFO(context);
    }

    void FootUdpApplication::SetInitialPosition () {
        // double A = 2 * (pos2.x - pos1.x);
        // double B = 2 * (pos2.y - pos1.y);
        // double C = 2 * (pos3.x - pos1.x);
        // double D = 2 * (pos3.y - pos1.y);

        // double E = d1 * d1 - d2 * d2 - pos1.x * pos1.x - pos1.y * pos1.y + pos2.x * pos2.x + pos2.y * pos2.y;
        // double F = d1 * d1 - d3 * d3 - pos1.x * pos1.x - pos1.y * pos1.y + pos3.x * pos3.x + pos3.y * pos3.y;

        // // Calculating estimated position (x, y)
        // double x = (E - F * B / D) / (A - C * B / D);
        // double y = (E - A * x) / B;

        // // Set the estimated position (assuming z = 0)
        // m_currentPosition  = Point(x, y);
    }

    void FootUdpApplication::StartApplication () {
        // FootUdpApplication::SetInitialPosition();
        Ptr<WifiNetDevice> device = GetNode()->GetDevice(0)->GetObject<WifiNetDevice>();
        Ptr<WifiPhy> phy = device->GetPhy();
        // NS_LOG_INFO("RSSI");
        // NS_LOG_INFO(phy->GetRxSensitivity());
        m_socket->SetRecvCallback(MakeCallback(&FootUdpApplication::HandleRead, this));
        std::string node_id = std::to_string (GetNode ()->GetId ());
        Config::Connect("/NodeList/*/DeviceList/0/Phy/MonitorSnifferRx", MakeCallback(&FootUdpApplication::SniffRx, this));
    }

    void FootUdpApplication::StopApplication () {
        m_socket->SetRecvCallback(MakeNullCallback<void, ns3::Ptr<ns3::Socket>>());
        m_socket->Close();
    }
} // namespace ns3
