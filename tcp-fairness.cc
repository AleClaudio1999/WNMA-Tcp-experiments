#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TcpFairness");

int
main (int argc, char *argv[])
{
  std::string tcpType = "BBR";
  CommandLine cmd;
  cmd.AddValue ("tcp", "BBR or CUBIC", tcpType);
  cmd.Parse (argc, argv);

  if (tcpType == "BBR")
    {
      Config::SetDefault ("ns3::TcpL4Protocol::SocketType",
                          TypeIdValue (TcpBbr::GetTypeId ()));
    }
  else
    {
      Config::SetDefault ("ns3::TcpL4Protocol::SocketType",
                          TypeIdValue (TcpCubic::GetTypeId ()));
    }

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
  Ipv4InterfaceContainer receiverIf = ip.Assign (d3);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  uint16_t port1 = 50000;
  uint16_t port2 = 50001;

  BulkSendHelper source1 ("ns3::TcpSocketFactory",
                          InetSocketAddress (receiverIf.GetAddress (1), port1));
  source1.SetAttribute ("MaxBytes", UintegerValue (0));

  BulkSendHelper source2 ("ns3::TcpSocketFactory",
                          InetSocketAddress (receiverIf.GetAddress (1), port2));
  source2.SetAttribute ("MaxBytes", UintegerValue (0));

  ApplicationContainer apps;
  apps.Add (source1.Install (senders.Get (0)));
  apps.Add (source2.Install (senders.Get (1)));

  PacketSinkHelper sink1 ("ns3::TcpSocketFactory",
                          InetSocketAddress (Ipv4Address::GetAny (), port1));
  PacketSinkHelper sink2 ("ns3::TcpSocketFactory",
                          InetSocketAddress (Ipv4Address::GetAny (), port2));

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

  std::map<FlowId, FlowMonitor::FlowStats> stats =
    monitor->GetFlowStats ();

  double sum = 0;
  double sumSq = 0;
  int n = 0;

  for (auto const &flow : stats)
    {
      double throughput =
        flow.second.rxBytes * 8.0 / (60.0 - 1.0) / 1e6;

      std::cout << "Flow " << flow.first
                << " Throughput: " << throughput << " Mbps" << std::endl;

      sum += throughput;
      sumSq += throughput * throughput;
      n++;
    }

  double fairness = (sum * sum) / (n * sumSq);

  std::cout << "Jain Fairness Index = " << fairness << std::endl;

  Simulator::Destroy ();
  return 0;
}
