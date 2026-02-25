// ./ns3 run "scratch/tcp-starbucks --tcp=BBR"
// ./ns3 run "scratch/tcp-starbucks --tcp=CUBIC"

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TcpStarbucks");

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

  NodeContainer sender, router, receiver;
  sender.Create (1);
  router.Create (1);
  receiver.Create (1);

  InternetStackHelper stack;
  stack.InstallAll ();

  PointToPointHelper access;
  access.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
  access.SetChannelAttribute ("Delay", StringValue ("2ms"));

  PointToPointHelper bottleneck;
  bottleneck.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  bottleneck.SetChannelAttribute ("Delay", StringValue ("30ms"));
  bottleneck.SetQueue ("ns3::DropTailQueue",
                       "MaxSize", StringValue ("1000p")); // BUFFERBLOAT

  NetDeviceContainer d1 = access.Install (sender.Get (0), router.Get (0));
  NetDeviceContainer d2 = bottleneck.Install (router.Get (0), receiver.Get (0));

  Ipv4AddressHelper ip;
  ip.SetBase ("10.1.1.0", "255.255.255.0");
  ip.Assign (d1);

  ip.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer recvIf = ip.Assign (d2);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  uint16_t port = 50000;

  BulkSendHelper source ("ns3::TcpSocketFactory",
                         InetSocketAddress (recvIf.GetAddress (1), port));
  source.SetAttribute ("MaxBytes", UintegerValue (0));

  PacketSinkHelper sink ("ns3::TcpSocketFactory",
                         InetSocketAddress (Ipv4Address::GetAny (), port));

  ApplicationContainer apps;
  apps.Add (source.Install (sender.Get (0)));
  apps.Add (sink.Install (receiver.Get (0)));

  apps.Start (Seconds (1.0));
  apps.Stop (Seconds (60.0));

  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll ();

  Simulator::Stop (Seconds (60.0));
  Simulator::Run ();

  monitor->CheckForLostPackets ();

  auto stats = monitor->GetFlowStats ();

  for (auto const &flow : stats)
    {
      double throughput =
        flow.second.rxBytes * 8.0 / (60.0 - 1.0) / 1e6;

      double avgRtt =
        flow.second.delaySum.GetSeconds () /
        flow.second.rxPackets;

      std::cout << "Throughput = " << throughput << " Mbps" << std::endl;
      std::cout << "Average RTT = " << avgRtt * 1000 << " ms" << std::endl;
    }

  Simulator::Destroy ();
  return 0;
}
