#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TcpFriendliness");

int
main (int argc, char *argv[])
{
  NodeContainer senders;
  senders.Create (2);
  NodeContainer router;
  router.Create (1);
  NodeContainer receiver;
  receiver.Create (1);

  InternetStackHelper stack;
  stack.InstallAll ();

  PointToPointHelper access;
  access.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
  access.SetChannelAttribute ("Delay", StringValue ("2ms"));

  PointToPointHelper bottleneck;
  bottleneck.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  bottleneck.SetChannelAttribute ("Delay", StringValue ("20ms"));
  bottleneck.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("100p"));

  NetDeviceContainer d1 = access.Install (senders.Get (0), router.Get (0));
  NetDeviceContainer d2 = access.Install (senders.Get (1), router.Get (0));
  NetDeviceContainer d3 = bottleneck.Install (router.Get (0), receiver.Get (0));

  Ipv4AddressHelper ip;
  ip.SetBase ("10.1.1.0", "255.255.255.0");
  ip.Assign (d1);

  ip.SetBase ("10.1.2.0", "255.255.255.0");
  ip.Assign (d2);

  ip.SetBase ("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer recvIf = ip.Assign (d3);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  uint16_t portBbr = 50000;
  uint16_t portCubic = 50001;

  // -------- BBR FLOW (Linux)
  Config::Set ("/NodeList/0/$ns3::TcpL4Protocol/SocketType",
               TypeIdValue (TcpBbr::GetTypeId ()));

  BulkSendHelper bbrSource ("ns3::TcpSocketFactory",
                            InetSocketAddress (recvIf.GetAddress (1), portBbr));
  bbrSource.SetAttribute ("MaxBytes", UintegerValue (0));

  // -------- CUBIC FLOW (Windows)
  Config::Set ("/NodeList/1/$ns3::TcpL4Protocol/SocketType",
               TypeIdValue (TcpCubic::GetTypeId ()));

  BulkSendHelper cubicSource ("ns3::TcpSocketFactory",
                              InetSocketAddress (recvIf.GetAddress (1), portCubic));
  cubicSource.SetAttribute ("MaxBytes", UintegerValue (0));

  ApplicationContainer apps;
  apps.Add (bbrSource.Install (senders.Get (0)));
  apps.Add (cubicSource.Install (senders.Get (1)));

  PacketSinkHelper sink1 ("ns3::TcpSocketFactory",
                          InetSocketAddress (Ipv4Address::GetAny (), portBbr));
  PacketSinkHelper sink2 ("ns3::TcpSocketFactory",
                          InetSocketAddress (Ipv4Address::GetAny (), portCubic));

  apps.Add (sink1.Install (receiver.Get (0)));
  apps.Add (sink2.Install (receiver.Get (0)));

  apps.Start (Seconds (1.0));
  apps.Stop (Seconds (60.0));

  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll ();

  Simulator::Stop (Seconds (60.0));
  Simulator::Run ();

  monitor->CheckForLostPackets ();

  Ptr<Ipv4FlowClassifier> classifier =
    DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
  auto stats = monitor->GetFlowStats ();

  double bbrThroughput = 0;
  double cubicThroughput = 0;

  for (auto const &flow : stats)
    {
      auto t = classifier->FindFlow (flow.first);
      double throughput =
        flow.second.rxBytes * 8.0 / (60.0 - 1.0) / 1e6;

      if (t.destinationPort == portBbr)
        bbrThroughput = throughput;
      else if (t.destinationPort == portCubic)
        cubicThroughput = throughput;
    }

  std::cout << "BBR Throughput   = " << bbrThroughput << " Mbps" << std::endl;
  std::cout << "CUBIC Throughput = " << cubicThroughput << " Mbps" << std::endl;
  std::cout << "BBR/CUBIC Ratio  = "
            << bbrThroughput / cubicThroughput << std::endl;

  Simulator::Destroy ();
  return 0;
}
