#ifndef FOOT_UDP_APPLICATION_H
#define FOOT_UDP_APPLICATION_H
#include "ns3/socket.h"
#include "ns3/applications-module.h"

#include <vector>

using namespace ns3;
namespace ns3
{   
    struct Point
    {
        double x;
        double y;
        
        Point(double _x, double _y)
            : x(_x), y(_y) {}

        Point () {}
    };
    
    struct Neighbor
    {
        double signalStrength;
        double batteryLevel;
        Ptr<Socket> playerSocket;
        Point coord;

        Neighbor(double _signalStrength, double _batteryLevel, Ptr<Socket> _playerSocket, Point _coord)
            : signalStrength(_signalStrength), batteryLevel(_batteryLevel), playerSocket(_playerSocket), coord(_coord) {}

        void updateCoord (Point newCoords) {
            coord = newCoords;
        }

        void updateSignalStrength (double signal) {
            signalStrength = signal;
        }

        void updateBatteryLevel (double battery) {
            batteryLevel = battery;
        }

        void updateAll (double signal, double battery, Point newCoords) {
            updateSignalStrength(signal);
            updateBatteryLevel(battery);
            updateCoord(newCoords);
        }
    };
    
    struct Transmitter
    {
        Ptr<Socket> trnSocket;
        Point coords;

        Transmitter(Ptr<Socket> _trnSocket, Point _coords) : trnSocket(_trnSocket), coords(_coords) {}
    };

    class FootUdpApplication : public ns3::Application
    {
        private:
            // Storing other nodes in the network as vectors
            std::vector<Neighbor> m_playerList; 
            std::vector<Transmitter> m_transmitters;
            // UDP connections to other nodes
            Ptr<Socket> m_socket;
            Point m_currentPosition;
            Point m_prevPosition;
            ns3::Address m_peerAddress;
            virtual void StartApplication ();
            virtual void StopApplication ();
            void HandleRead (Ptr<Socket> socket);
            void SendPacket (Ptr<Packet> packet, Ipv6Address destination, uint16_t port);
            std::vector<Neighbor> GetBestNeighbors ();
            void SniffRx (std::string context, double rss);
            Point GetLocation ();

        public:
            FootUdpApplication();
            virtual ~FootUdpApplication();
            static TypeId GetTypeId();
            
            void Setup (Inet6SocketAddress sinkAddress);
            void AddPlayer (Inet6SocketAddress playerAddress);
            void AddTransmitter (Inet6SocketAddress trnAddress, Point trnCoords);
            void SetInitialPosition ();
    };
} // namespace ns3


#endif