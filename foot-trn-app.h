#ifndef FOOT_TRN_APPLICATION_H
#define FOOT_TRN_APPLICATION_H
#include "utilities.h"
#include "ns3/socket.h"
#include "ns3/application.h"

#include <vector>

using namespace ns3;
namespace ns3
{
    // Handles connections and methods of a single transmitter
    class FootTrnApplication : public Application
    {
        private:
            virtual void StartApplication ();  
            virtual void StopApplication ();
            void SendPacket (Ptr<Socket> socket, uint32_t nodeIndex);
            void ReadIncoming (Ptr<Socket> socket);
            Point m_trnLocation;
            Ptr<Socket> m_socket;
            Ipv6Address m_address;
            uint16_t m_port;
            std::vector<Ptr<Socket>> m_playerList;

        public:
            FootTrnApplication ();
            ~FootTrnApplication();
            void Setup(Inet6SocketAddress sinkAddress, Point trnCoords);
            void ConfigurePlayerConnection (Inet6SocketAddress playerAddress);
            void TrackPlayerLocation (uint32_t nodeIndex);
            static TypeId GetTypeId ();
    };
    
} // namespace ns3

#endif