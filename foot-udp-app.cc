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
        for (int i = 0; i < m_playerList.size(); ++i) {
            indices[i] = i;
        }

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
            bool isPositive = isPositiveRSSD(RSSDValues[i], delta);
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
            double distance = zone.first;

            // Calculate the intersection with the current zone
            std::vector<Point> intersections = circleIntersection(possibleZoneCenter, possibleZoneRadius, m_playerList[i].coord, RSSDValues[i]);
            if (!intersections.empty()) {
                possibleZoneCenter = intersections[0];
                possibleZoneRadius = 0.0;
            }
        }

        // Use the possible zone center as the estimated location
        Point estimatedLocation = possibleZoneCenter;

        return estimatedLocation;
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
