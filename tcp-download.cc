// for execution ./ns3 run "scratch/tcp-download --tcp=CUBIC" or ./ns3 run "scratch/tcp-download --tcp=BBR"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TcpDownload");

static Time downloadEnd;

void
RxCallback (Ptr<const Packet> p, const Address &addr)
{
  downloadEnd = Simulator::Now ();
}

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

  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("20ms"));
  p2p.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("50p"));

  NetDeviceContainer d1 = p2p.Install (sender.Get (0), router.Get (0));
  NetDeviceContainer d2 = p2p.Install (router.Get (0), receiver.Get (0));

  Ipv4AddressHelper ip;
  ip.SetBase ("10.1.1.0", "255.255.255.0");
  ip.Assign (d1);

  ip.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer recvIf = ip.Assign (d2);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  uint16_t port = 50000;
  uint64_t fileSize = 50 * 1024 * 1024; // 50 MB

  BulkSendHelper source ("ns3::TcpSocketFactory",
                         InetSocketAddress (recvIf.GetAddress (1), port));
  source.SetAttribute ("MaxBytes", UintegerValue (fileSize));

  PacketSinkHelper sink ("ns3::TcpSocketFactory",
                         InetSocketAddress (Ipv4Address::GetAny (), port));

  ApplicationContainer apps;
  apps.Add (source.Install (sender.Get (0)));
  ApplicationContainer sinkApp = sink.Install (receiver.Get (0));

  Ptr<PacketSink> sinkPtr = DynamicCast<PacketSink> (sinkApp.Get (0));
  sinkPtr->TraceConnectWithoutContext ("Rx", MakeCallback (&RxCallback));

  apps.Start (Seconds (1.0));
  apps.Stop (Seconds (100.0));

  Simulator::Stop (Seconds (100.0));
  Simulator::Run ();

  double duration = (downloadEnd - Seconds (1.0)).GetSeconds ();
  double goodput = (fileSize * 8.0) / duration / 1e6;

  std::cout << "Download completed in " << duration << " s" << std::endl;
  std::cout << "Average goodput = " << goodput << " Mbps" << std::endl;

  Simulator::Destroy ();
  return 0;
}
