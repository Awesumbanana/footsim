#include "ns3/animation-interface.h"
#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/config.h"
#include "foot-udp-app.h"
#include "foot-trn-app.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h"
#include "ns3/network-module.h"
#include "ns3/energy-module.h"
#include "ns3/internet-module.h"
#include "ns3/lr-wpan-module.h"
#include "ns3/sixlowpan-module.h"
#include "ns3/ns2-mobility-helper.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/yans-wifi-helper.h"

#include <fstream>
#include <cstdlib>
#include <iostream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("FootSimulation");


// static void
// SensorLocationsOutput() {
//     std::ofstream f;
//     f.open("example-output.txt", std::ios::out | std::ios::app);
//     f << Simulator::Now().GetSeconds() << " " // time [s]
//       << params.txMob->GetPosition().x << " " << params.txMob->GetPosition().y << " "
//       << params.rxMob->GetPosition().x << " " << params.rxMob->GetPosition().y << " "
//       << cond->GetLosCondition() << " "                  // channel state
//       << 10 * log10(Sum(*rxPsd) / Sum(*noisePsd)) << " " // SNR [dB]
//       << -propagationGainDb << std::endl;                // pathloss [dB]
//     f.close();
// }

struct sensorValues
{
  double prevSigStr;
  double currSigStr;
};

int 
main(int argc, char* argv[])
{
    // 11 players in a football team
    LogComponentEnable ("FootSimulation", LOG_LEVEL_INFO);
    LogComponentEnable ("FootTrnApplication", LOG_LEVEL_INFO);
    LogComponentEnable ("FootUdpApplication", LOG_LEVEL_INFO);
    NS_LOG_INFO ("Starting Simulation");

    uint32_t n = 11;
    uint32_t m = 3;

    // Dimensions of the football field are 
    // float xBound = 122; in metres, 1 metre for each goal
    // float yBound = 90; in metres
    // Positions of transmitters
    // t1 = (0,45)
    // t2 = (122, 45)
    // t3 = (61, 0)
    std::vector<Point> trnCoords;

    trnCoords.push_back({0, 45});
    trnCoords.push_back({122, 45});
    trnCoords.push_back({61, 0});
    CommandLine cmd(__FILE__);
    // cmd.Parse(argc, argv);
    // Time::SetResolution(Time::S);

    // Player nodes are first n nodes
    NodeContainer playerNodes;
    playerNodes.Create(n);

    // Using mobility model generated from BonnMotion
    Ns2MobilityHelper ns2 = Ns2MobilityHelper("scratch/SportsSim/testscen.ns_movements");
    ns2.Install();

    NodeContainer sinks;
    sinks.Create(m);

    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(trnCoords[0].x, trnCoords[0].y, 0.0));
    positionAlloc->Add(Vector(trnCoords[1].x, trnCoords[1].y, 0.0));
    positionAlloc->Add(Vector(trnCoords[2].x, trnCoords[2].y, 0.0));
    mobility.SetPositionAllocator(positionAlloc);
    mobility.Install(sinks);

    NodeContainer allNodes = NodeContainer(playerNodes, sinks);

    // Adding point to point connections between the sinks and all the players
    // PointToPointHelper p2p;
    // p2p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    // p2p.SetChannelAttribute("Delay", StringValue("2ms"));

    // NetDeviceContainer sink0Devices;
    // NetDeviceContainer sink1Devicmodule
    // Creating a wifi channel between all the nodes(Sinks+players)
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211ax);

    YansWifiPhyHelper wifiPhy;

    Ptr<YansWifiChannel> wifiChannel = CreateObject<YansWifiChannel>();
    Ptr<LogDistancePropagationLossModel> lossModel =
        CreateObject<LogDistancePropagationLossModel>();
    Ptr<ConstantSpeedPropagationDelayModel> delayModel = CreateObject<ConstantSpeedPropagationDelayModel>();

    wifiChannel->SetPropagationDelayModel(delayModel);
    wifiChannel->SetPropagationLossModel(lossModel);

    // wifiChannel.AddPropagationLoss("ns3::LogDistancePropagationLossModel");
    // wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    
    wifiPhy.SetChannel(wifiChannel);
  
    WifiMacHelper wifiMac;
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager");
    // Set it to adhoc mode
    wifiMac.SetType("ns3::AdhocWifiMac");
    
    NetDeviceContainer devices = wifi.Install(wifiPhy, wifiMac, allNodes);

    // // Setting up LRWPAN for players on the field and transmitters
    // LrWpanHelper lrWpanHelper;
    // NetDeviceContainer lrwpanDevices = lrWpanHelper.Install(devices);

    // lrWpanHelper.CreateAssociatedPan(lrwpanDevices, 10);
    // std::cout << "Created " << lrwpanDeices.GetN() << " devices" << std::endl;
    // std::cout << "There are " << playerNodes.GetN() << " nodes" << std::endl;

    InternetStackHelper internetv6;
    internetv6.SetIpv4StackInstall(false);
    internetv6.Install(allNodes);

    SixLowPanHelper sixLowPanHelper;
    NetDeviceContainer sixLowPanDevices = sixLowPanHelper.Install(devices);

    Ipv6AddressHelper ipv6;
    ipv6.SetBase(Ipv6Address("2001:f008::"), Ipv6Prefix(64));
    Ipv6InterfaceContainer wsnDeviceInterfaces;
    wsnDeviceInterfaces = ipv6.Assign(sixLowPanDevices);
    wsnDeviceInterfaces.SetForwarding(0, true);
    wsnDeviceInterfaces.SetDefaultRouteInAllNodes(0);

    // Common port number for all nodes
    uint16_t port = 50000;

    ApplicationContainer sinkApps;
    ApplicationContainer playerApps;

    // Configuring UDP connection for the sinks->players
    for(uint32_t i = 0; i < m; ++i) {
      Ptr<Node> sinkNode = sinks.Get(i);
      Ptr<FootTrnApplication> app_j = CreateObject<FootTrnApplication>();
      sinkNode->AddApplication(app_j);
      Inet6SocketAddress sinkAddress(wsnDeviceInterfaces.GetAddress(i+n, 0), port);
      app_j->Setup(sinkAddress, trnCoords[i]);
      for (uint32_t j = 0; j < n; ++j) {
        Ptr<Node> wsnNode = playerNodes.Get(j);
        Inet6SocketAddress playerAddress(wsnDeviceInterfaces.GetAddress(j, 1), port);
        app_j->ConfigurePlayerConnection(playerAddress);
        // std::cout << "Created player " << j << " connection for sink " << i << std::endl;
      }
      sinkApps.Add(app_j);
    }
    
    sinkApps.Start(Seconds(0.0));
    sinkApps.Stop(Seconds(15.0));

    // uDP connections player->player and player->sink
    for(uint32_t i = 0; i < n; ++i) {
      Ptr<Node> wsnNode = playerNodes.Get(i); 
      Ptr<FootUdpApplication> app_i = CreateObject<FootUdpApplication>();
      wsnNode->AddApplication(app_i);
      Inet6SocketAddress selfAddress(wsnDeviceInterfaces.GetAddress(i, 1), port);
      app_i->Setup(selfAddress);
      // Player->player  
      for (uint32_t j = 1; j < n; ++j) {
        if (i != j){
          Inet6SocketAddress playerAddress(wsnDeviceInterfaces.GetAddress(j, 1), port);
          app_i->AddPlayer(playerAddress);
          // std::cout << "Created player " << j << " connection for player " << i << std::endl;
        }
      }
      // Player->sink
      for (uint32_t k = 0; k < m; ++k) {
        Inet6SocketAddress trnAddress(wsnDeviceInterfaces.GetAddress(n + k, 1), port);
        app_i->AddTransmitter(trnAddress, trnCoords[k]);
          // std::cout << "Created player " << i << " connection to transmitter " << k << std::endl;
      }
      playerApps.Add(app_i);
    }

    playerApps.Start(Seconds(0.0));
    playerApps.Stop(Seconds(15.0));
    NS_LOG_INFO("Players added");

    // Recursively getting the player locations from a single transmitter. For now. 
    Ptr<Node> transmitter1 = sinks.Get(0);
    Ptr<Application> app = transmitter1->GetApplication(0);
    Ptr<FootTrnApplication> trnApplication = DynamicCast<FootTrnApplication>(app);
    Simulator::Schedule(Seconds(1), &FootTrnApplication::TrackPlayerLocation, trnApplication, 0);

    Simulator::Stop(Seconds(15.0));
    std::cout << "Creating trace XML file" << std::endl;
    AnimationInterface anim("footsim.xml");
    anim.EnablePacketMetadata(true);
    Simulator::Run();

    Simulator::Destroy();

    return 0;
}
