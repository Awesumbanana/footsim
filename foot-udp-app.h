#ifndef FOOT_UDP_APPLICATION_H
#define FOOT_UDP_APPLICATION_H
#include "utilities.h"
#include "ns3/socket.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-tx-vector.h"
#include "ns3/wifi-mpdu-type.h"
#include "ns3/phy-entity.h"

#include <vector>
#include <numeric>

using namespace ns3;
namespace ns3
{   
    struct Neighbor
    {
        double oldSignalStrength;
        double batteryLevel;
        double distanceFromMe;
        Ptr<Socket> playerSocket;
        Point coord;

        Neighbor(double _signalStrength, double _batteryLevel, double _distanceFromMe, Ptr<Socket> _playerSocket, Point _coord)
            : oldSignalStrength(_signalStrength), batteryLevel(_batteryLevel), distanceFromMe(_distanceFromMe), playerSocket(_playerSocket), coord(_coord) {}

        void updateCoord (Point newCoords) {
            coord = newCoords;
        }

        void updateSignalStrength (double signal) {
            oldSignalStrength = signal;
        }

        void updateBatteryLevel (double battery) {
            batteryLevel = battery;
        }

        void updateDistanceFromMe(double distance) {
            distanceFromMe = distance;
        }

        void updateAll (double signal, double battery, double distance, Point newCoords) {
            updateSignalStrength(signal);
            updateBatteryLevel(battery);
            updateCoord(newCoords);
            updateDistanceFromMe(distance);
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
            double m_batteryLevel;
            ns3::Address m_peerAddress;
            virtual void StartApplication ();
            virtual void StopApplication ();
            double ComputeScore(const Neighbor& player);
            void HandleRead (Ptr<Socket> socket);
            void SendPacket (Ptr<Packet> packet, Ipv6Address destination, uint16_t port);
            std::vector<Neighbor> GetBestNeighbors ();
            void SniffRx (std::string context,
                Ptr<const Packet> packet,
                uint16_t channelFreqMhz,
                WifiTxVector txVector,
                MpduInfo aMpdu,
                SignalNoiseDbm signalNoise,
                uint16_t staId);
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