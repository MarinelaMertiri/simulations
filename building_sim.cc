#include "ns3/lte-helper.h"
#include "ns3/epc-helper.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lte-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/config-store.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/mobility-building-info.h"
#include "ns3/buildings-propagation-loss-model.h"
#include "ns3/building.h"
#include "ns3/buildings-helper.h"
#include "ns3/buildings-module.h"
#include "ns3/log.h"

using namespace ns3;

void createBuilding(Ptr<Building> build)
{
	double x_min = 0.0;
	double x_max = 50.0;
	double y_min = 0.0;
	double y_max = 50.0;
	double z_min = 0.0;
	double z_max = 10.0;

	//build = CreateObject<Building>();
	build->SetBoundaries(Box(x_min, x_max, y_min, y_max, z_min, z_max));
	build->SetBuildingType(Building::Residential);
	build->SetExtWallsType(Building::ConcreteWithWindows);
	build->SetNFloors(1);
	build->SetNRoomsX(2);
	build->SetNRoomsY(2);
}

void
PrintGnuplottableBuildingListToFile (std::string filename)
{
  std::ofstream outFile;
  outFile.open (filename.c_str (), std::ios_base::out | std::ios_base::trunc);
  if (!outFile.is_open ())
  {
      return;
  }
  uint32_t index = 0;
  for (BuildingList::Iterator it = BuildingList::Begin (); it != BuildingList::End (); ++it)
  {
      ++index;
      Box box = (*it)->GetBoundaries ();
      outFile << "set object " << index
              << " rect from " << box.xMin  << "," << box.yMin
              << " to "   << box.xMax  << "," << box.yMax
              << " front fs empty "
              << std::endl;
  }
}

void
PrintGnuplottableUeListToFile (std::string filename)
{
  std::ofstream outFile;
  outFile.open (filename.c_str (), std::ios_base::out | std::ios_base::trunc);
  if (!outFile.is_open ())
  {
      return;
  }
  for (NodeList::Iterator it = NodeList::Begin (); it != NodeList::End (); ++it)
  {
      Ptr<Node> node = *it;
      int nDevs = node->GetNDevices ();
      for (int j = 0; j < nDevs; j++)
      {
          Ptr<LteUeNetDevice> uedev = node->GetDevice (j)->GetObject <LteUeNetDevice> ();
          if (uedev)
          {
              Vector pos = node->GetObject<MobilityModel> ()->GetPosition ();
              outFile << "set label \"" << uedev->GetImsi ()
                      << "\" at "<< pos.x << "," << pos.y << " left font \"Helvetica,8\" textcolor rgb \"grey\" front point pt 1 ps 0.3 lc rgb \"grey\" offset 0,0"
                      << std::endl;
          }
      }
  }
}

void
PrintGnuplottableEnbListToFile (std::string filename)
{
  std::ofstream outFile;
  outFile.open (filename.c_str (), std::ios_base::out | std::ios_base::trunc);
  if (!outFile.is_open ())
  {
      return;
  }
  for (NodeList::Iterator it = NodeList::Begin (); it != NodeList::End (); ++it)
  {
      Ptr<Node> node = *it;
      int nDevs = node->GetNDevices ();
      for (int j = 0; j < nDevs; j++)
      {
          Ptr<LteEnbNetDevice> enbdev = node->GetDevice (j)->GetObject <LteEnbNetDevice> ();
          if (enbdev)
          {
              Vector pos = node->GetObject<MobilityModel> ()->GetPosition ();
              outFile << "set label \"" << enbdev->GetCellId ()
                      << "\" at "<< pos.x << "," << pos.y
                      << " left font \"Helvetica,8\" textcolor rgb \"white\" front  point pt 2 ps 0.3 lc rgb \"white\" offset 0,0"
                      << std::endl;
          }
      }
  }
}



int main(int argc, char *argv[]) {
	double power = 20; // 20 Fempto cell (Femtocells_Hamalainen2011 ,p 42)
	uint16_t rb = 6;
	uint16_t numberOfUes = 10;
	uint8_t numberOfEnbs = 4;
	double simTime = 100.0;
	std::string schedulerType = "rr";
	bool createRem = false;

	if (argc == 2)
	{
		numberOfUes = atoi(argv[1]);
	}
	else if (argc == 3)
	{
		numberOfUes = atoi(argv[1]);
		rb = atoi(argv[2]);

	}
	else if (argc == 4)
	{
		numberOfUes = atoi(argv[1]);
		rb = atoi(argv[2]);
		schedulerType = argv[3];
	}
	else if (argc == 5)
	{
		numberOfUes = atoi(argv[1]);
		rb = atoi(argv[2]);
		schedulerType = argv[3];
		std::string argv4 = argv[4];

		if (argv4.compare("rem") == 0)
			createRem = true;
	}

	Config::SetDefault("ns3::RadioBearerStatsCalculator::EpochDuration",TimeValue (Seconds (1.0)));
	Config::SetDefault("ns3::RadioEnvironmentMapHelper::StopWhenDone", BooleanValue(true));

	// create building
	Ptr<Building> build = CreateObject<Building>();
	createBuilding(build);


	Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();

	Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper>();
	lteHelper->SetEpcHelper(epcHelper);



	// Set the RBs
	lteHelper->SetEnbDeviceAttribute("DlBandwidth", UintegerValue(rb));
	lteHelper->SetEnbDeviceAttribute("UlBandwidth", UintegerValue(rb));

	// Scheduler

	if (schedulerType.compare("rr") == 0)
	{
		lteHelper->SetSchedulerType ("ns3::RrFfMacScheduler");    // RR scheduler
	}
	else if (schedulerType.compare("pf") == 0)
	{
		lteHelper->SetSchedulerType ("ns3::PfFfMacScheduler");    // PF scheduler
	}
	else if (schedulerType.compare("tdtbfq") == 0)
	{
		lteHelper->SetSchedulerType ("ns3::TdTbfqFfMacScheduler");  // TD-TBFQ scheduler
		lteHelper->SetSchedulerAttribute("DebtLimit", IntegerValue(-530)); // default value -625000 bytes (-5Mb)
		lteHelper->SetSchedulerAttribute("CreditLimit", UintegerValue(530)); // default value 625000 bytes (5Mb)
		lteHelper->SetSchedulerAttribute("TokenPoolSize", UintegerValue(53)); // default value 1 byte
		lteHelper->SetSchedulerAttribute("CreditableThreshold", UintegerValue(0)); // default value 0
	}
	else if (schedulerType.compare("fdtbfq") == 0)
	{
		lteHelper->SetSchedulerType ("ns3::FdTbfqFfMacScheduler");  // TD-TBFQ scheduler
		lteHelper->SetSchedulerAttribute("DebtLimit", IntegerValue(-530)); // default value -625000 bytes (-5Mb)
		lteHelper->SetSchedulerAttribute("CreditLimit", UintegerValue(530)); // default value 625000 bytes (5Mb)
		lteHelper->SetSchedulerAttribute("TokenPoolSize", UintegerValue(53)); // default value 1 byte
		lteHelper->SetSchedulerAttribute("CreditableThreshold", UintegerValue(0)); // default value 0
	}
	else if (schedulerType.compare("tdbet") == 0)
	{
		lteHelper->SetSchedulerType("ns3::TdBetFfMacScheduler");
	}
	else if (schedulerType.compare("fdbet") == 0)
	{
		lteHelper->SetSchedulerType("ns3::FdBetFfMacScheduler");
	}
	else if (schedulerType.compare("fdmt") == 0)
	{
		lteHelper->SetSchedulerType("ns3::FdMtFfMacScheduler");
	}
	else if (schedulerType.compare("tdmt") == 0)
	{
		lteHelper->SetSchedulerType("ns3::TdMtFfMacScheduler");
	}
	else if (schedulerType.compare("tta") == 0)
	{
		lteHelper->SetSchedulerType("ns3::TtaFfMacScheduler");
	}
	else if (schedulerType.compare("pss") == 0)
	{
		lteHelper->SetSchedulerType("ns3::PssFfMacScheduler");
	}
	else
	{
		std::cout << "Wrong scheduler type. Use: rr, pf, tdtbfq, fdtbfq" << "\n";
		return -1;
	}
	NodeContainer enbNodes;
	enbNodes.Create(numberOfEnbs);

	// isotropic antenas
	lteHelper->SetEnbAntennaModelType ("ns3::IsotropicAntennaModel");
	MobilityHelper mobilityEnb;
	mobilityEnb.SetMobilityModel("ns3::ConstantPositionMobilityModel");

	// Set mobility for the enbs.
	Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();

	// center
	positionAlloc->Add(Vector(12.5, 37.5, 3));
	positionAlloc->Add(Vector(37.5, 37.5, 3));
	positionAlloc->Add(Vector(12.5, 12.5, 3));
	positionAlloc->Add(Vector(37.5, 12.5, 3));

	mobilityEnb.SetPositionAllocator(positionAlloc);
	mobilityEnb.Install(enbNodes);
	BuildingsHelper::Install(enbNodes);


	// Install LTE Devices to the nodes

	NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice(enbNodes);
	BuildingsHelper::MakeMobilityModelConsistent();

	// Set the transmitted power from Enb.
	for (uint8_t i = 0; i < 4; i++)
	{
		Ptr<LteEnbPhy> enbPhy = enbLteDevs.Get(i)->GetObject<LteEnbNetDevice> ()->GetPhy ();
		enbPhy->SetTxPower (power);
	}


	// create a remote host
	NodeContainer remotehostContainer;
	remotehostContainer.Create(1);
	Ptr<Node> remoteHost = remotehostContainer.Get(0);
	InternetStackHelper internet;
	internet.Install(remotehostContainer);

	// create internet
	PointToPointHelper p2ph;
	p2ph.SetDeviceAttribute("DataRate", DataRateValue(DataRate("100Gb/s")));
	p2ph.SetDeviceAttribute("Mtu", UintegerValue(1500));
	p2ph.SetChannelAttribute("Delay", TimeValue(Seconds(0.010)));

	Ptr<Node> pgw = epcHelper->GetPgwNode();
	NetDeviceContainer internetDevices = p2ph.Install(pgw, remoteHost);
	Ipv4AddressHelper ipv4h;
	ipv4h.SetBase("1.0.0.0", "255.0.0.0");
	Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign(internetDevices);

	Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress(1);

	Ipv4StaticRoutingHelper ipv4RoutingHelper;
	Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting(remoteHost->GetObject<Ipv4>());
	remoteHostStaticRouting->AddNetworkRouteTo(Ipv4Address("7.0.0.0"), Ipv4Mask("255.0.0.0"), 1);

	// Create Ues
	NodeContainer ueNodes;
	ueNodes.Create(numberOfUes);

	// Mobility for UEs (https://www.nsnam.org/doxygen/main-random-walk_8cc_source.html)
	MobilityHelper mobility;

	Ptr<PositionAllocator> positionAlloc2 = CreateObject<RandomRoomPositionAllocator>();
	mobility.SetPositionAllocator(positionAlloc2);


	// Random mobility for UEs

	mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel", "Mode", StringValue ("Time"),
							   "Time", StringValue ("2s"),
							   "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"),
							   "Bounds", StringValue ("0|50|0|50"));

	mobility.Install(ueNodes);

	NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice(ueNodes);

	// Install the IP stack on the UEs
	internet.Install(ueNodes);
	Ipv4InterfaceContainer ueIpIface;
	ueIpIface = epcHelper->AssignUeIpv4Address(NetDeviceContainer(ueLteDevs));

	// Connect ues with enbs
	lteHelper->Attach(ueLteDevs);


	// routing sta ues
	for (uint32_t i = 0; i < ueNodes.GetN(); i++) {
		Ptr<Node> ueNode = ueNodes.Get(i);
		Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting(ueNode->GetObject<Ipv4>());
		ueStaticRouting->SetDefaultRoute(epcHelper->GetUeDefaultGatewayAddress(), 1);
	}

	Ptr<RadioEnvironmentMapHelper> remHelper;
	if (createRem)
	{
		//Radio Environment Map
		PrintGnuplottableBuildingListToFile ("buildings.txt");
		PrintGnuplottableEnbListToFile ("enbs.txt");
		PrintGnuplottableUeListToFile ("ues.txt");
		remHelper = CreateObject<RadioEnvironmentMapHelper> ();
		remHelper->SetAttribute ("ChannelPath", StringValue ("/ChannelList/0"));
		remHelper->SetAttribute ("OutputFile", StringValue ("rem.out"));
		remHelper->SetAttribute ("XMin", DoubleValue (-20.0));
		remHelper->SetAttribute ("XMax", DoubleValue (100.0));
		remHelper->SetAttribute ("XRes", UintegerValue (800));
		remHelper->SetAttribute ("YMin", DoubleValue (-20.0));
		remHelper->SetAttribute ("YMax", DoubleValue (100.0));
		remHelper->SetAttribute ("YRes", UintegerValue (600));
		remHelper->SetAttribute ("Z", DoubleValue (1.0));
		remHelper->SetAttribute ("UseDataChannel", BooleanValue (true));

		remHelper->Install ();

	}

	// install applications

	for (uint16_t i = 0; i < numberOfUes; i++)
	{
		double interPacketInterval;
		double packetSize = 1024;
		ApplicationContainer clientApps;
		ApplicationContainer serverApps;

		uint16_t otherPort1 = 3000;
		uint16_t otherPort2 = 3001;
		uint32_t fileSize = 3000000; // file size = 3MBytes
		uint32_t maxPackets = fileSize / packetSize + 1;
		double bitRate = 11722;   	 // Bit rate = 11722 kbps
		uint16_t choice = i%10;
		if (choice == 0)
		{
			// download a file from the remote host
			// https://www.swisscom.ch/dam/swisscom/en/res/mobile/mobile-network/netztest-connect-en-2014.pdf (page 4)
			interPacketInterval =  packetSize * 8 / bitRate; // packet size (bits) / bitRate (bits/sec)
			if (interPacketInterval < 1)
				interPacketInterval = 1;
			PacketSinkHelper packetSinkHelper_ue ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), otherPort1));
			PacketSinkHelper packetSinkHelper_remote("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), otherPort2));

			serverApps.Add(packetSinkHelper_ue.Install(ueNodes.Get(i)));
			serverApps.Add (packetSinkHelper_remote.Install (remoteHost));

			UdpClientHelper client_ue(ueIpIface.GetAddress(i), otherPort1);
			client_ue.SetAttribute("MaxPackets", UintegerValue(maxPackets));
			client_ue.SetAttribute("Interval", TimeValue(MilliSeconds(interPacketInterval)));
			client_ue.SetAttribute("PacketSize", UintegerValue(packetSize));
			UdpClientHelper client_remote (remoteHostAddr, otherPort2);
			client_remote.SetAttribute("MaxPackets", UintegerValue(maxPackets));
			client_remote.SetAttribute("Interval", TimeValue(MilliSeconds(interPacketInterval)));
			client_remote.SetAttribute("PacketSize", UintegerValue(packetSize));

			clientApps.Add (client_ue.Install (remoteHost));
			clientApps.Add(client_remote.Install(ueNodes.Get(i)));
			serverApps.Start (Seconds (0.01));
			clientApps.Start(Seconds(0.01));
		}
		else if (choice == 1)
		{
			// transmission from the ue to remote controller (video)
			otherPort1 = 4000;
			otherPort2 = 4010;
			interPacketInterval = packetSize*8 / 657;
			maxPackets = 2000;
			PacketSinkHelper packetSinkHelper_ue("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), otherPort1));
			PacketSinkHelper packetSinkHelper_remote("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), otherPort2));
			serverApps.Add (packetSinkHelper_ue.Install (ueNodes.Get(i)));
			serverApps.Add (packetSinkHelper_remote.Install (remoteHost));
			UdpClientHelper client(ueIpIface.GetAddress(i), otherPort1);
			client.SetAttribute ("Interval", TimeValue (MilliSeconds(interPacketInterval))); // 657 KBps (http://www.theglobeandmail.com/technology/tech-news/how-much-bandwidth-does-streaming-use/article7365916/)
			client.SetAttribute ("MaxPackets", UintegerValue(maxPackets));
			client.SetAttribute("PacketSize", UintegerValue(packetSize));
			UdpClientHelper client_remote(remoteHostAddr, otherPort2);

			client_remote.SetAttribute("MaxPackets", UintegerValue(maxPackets));
			client_remote.SetAttribute("Interval", TimeValue(MilliSeconds(interPacketInterval)));
			client_remote.SetAttribute("PacketSize", UintegerValue(packetSize));
			clientApps.Add (client.Install (remoteHost));
			clientApps.Add(client_remote.Install(ueNodes.Get(i)));
			serverApps.Start (Seconds (0.01));
			clientApps.Start (Seconds (0.01));
		}
		else if (choice == 2)
		{
			// file upload
			otherPort1 = 5000;
			otherPort2 = 5010;
			fileSize = 1000000; // file size = 1MByte
			maxPackets = fileSize / packetSize + 1;
			interPacketInterval = packetSize*8 / 1788; // 1788Kbps
			PacketSinkHelper packetSinkHelper_ue("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), otherPort1));
			PacketSinkHelper packetSinkHelper_remote("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), otherPort2));
			serverApps.Add (packetSinkHelper_ue.Install (ueNodes.Get(i)));
			serverApps.Add (packetSinkHelper_remote.Install(remoteHost));
			UdpClientHelper client(ueIpIface.GetAddress(i), otherPort1);
			client.SetAttribute ("Interval", TimeValue (MilliSeconds(interPacketInterval)));
			client.SetAttribute ("MaxPackets", UintegerValue(maxPackets));
			client.SetAttribute("PacketSize", UintegerValue(packetSize));
			UdpClientHelper client_remote(remoteHostAddr, otherPort2);
			client_remote.SetAttribute("MaxPackets", UintegerValue(maxPackets));
			client_remote.SetAttribute("Interval", TimeValue(MilliSeconds(interPacketInterval)));
			client_remote.SetAttribute("PacketSize", UintegerValue(packetSize));
			clientApps.Add (client.Install (remoteHost));
			clientApps.Add(client_remote.Install(ueNodes.Get(i)));
			serverApps.Start (Seconds (0.01));
			clientApps.Start (Seconds (0.01));
		}
		else if (choice == 3)
		{
			// download file (10 MB)
			otherPort1 = 6000;
			otherPort2 = 6001;
			fileSize = 10000000; // file size = 10 MBytes
			maxPackets = fileSize / packetSize + 1;
			bitRate = 13463;   	// Bit Rate = 13463 kbps
			interPacketInterval =  packetSize * 8 / bitRate; // packet size (bits) / bitRate (bits/sec)
			if (interPacketInterval < 1)
				interPacketInterval = 1;
			PacketSinkHelper packetSinkHelper_ue("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), otherPort1));
			PacketSinkHelper packetSinkHelper_remote("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), otherPort2));
			serverApps.Add(packetSinkHelper_ue.Install(ueNodes.Get(i)));
			serverApps.Add (packetSinkHelper_remote.Install (remoteHost));
			UdpClientHelper client_ue(ueIpIface.GetAddress(i), otherPort1);
			client_ue.SetAttribute("MaxPackets", UintegerValue(maxPackets));
			client_ue.SetAttribute("Interval", TimeValue(MilliSeconds(interPacketInterval)));
			client_ue.SetAttribute("PacketSize", UintegerValue(packetSize));
			UdpClientHelper client_remote(remoteHostAddr, otherPort2);
			client_remote.SetAttribute("MaxPackets", UintegerValue(maxPackets));
			client_remote.SetAttribute("Interval", TimeValue(MilliSeconds(interPacketInterval)));
			client_remote.SetAttribute("PacketSize", UintegerValue(packetSize));

			clientApps.Add (client_ue.Install (remoteHost));
			clientApps.Add(client_remote.Install(ueNodes.Get(i)));
			serverApps.Start (Seconds (0.01));
			clientApps.Start(Seconds(0.01));
		}
		else if (choice == 4)
		{
			// file upload
			otherPort1 = 5020;
			otherPort2 = 5021;
			fileSize = 10000000; // file size = 10MBytes
			maxPackets = fileSize / packetSize + 1;
			bitRate = 1920;   	// Bit rate = 1920 kbps
			interPacketInterval =  packetSize * 8 / bitRate; // packet size (bits) / bitRate (bits/sec)
			PacketSinkHelper packetSinkHelper_ue("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), otherPort1));
			PacketSinkHelper packetSinkHelper_remote("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), otherPort2));
			serverApps.Add (packetSinkHelper_ue.Install (ueNodes.Get(i)));
			serverApps.Add (packetSinkHelper_remote.Install(remoteHost));
			UdpClientHelper client(ueIpIface.GetAddress(i), otherPort1);
			client.SetAttribute ("Interval", TimeValue (MilliSeconds(interPacketInterval)));
			client.SetAttribute ("MaxPackets", UintegerValue(maxPackets));
			client.SetAttribute("PacketSize", UintegerValue(packetSize));
			UdpClientHelper client_remote(remoteHostAddr, otherPort2);
			client_remote.SetAttribute("MaxPackets", UintegerValue(maxPackets));
			client_remote.SetAttribute("Interval", TimeValue(MilliSeconds(interPacketInterval)));
			client_remote.SetAttribute("PacketSize", UintegerValue(packetSize));
			clientApps.Add (client.Install (remoteHost));
			clientApps.Add(client_remote.Install(ueNodes.Get(i)));
			serverApps.Start (Seconds (0.01));
			clientApps.Start (Seconds (0.01));
		}
		else if (choice == 5)
		{
			// http://www.slideshare.net/althafhussain1023/how-to-dimension-user-traffic-in-lte (p 8)
			otherPort1 = 6002;
			otherPort2 = 6003;
			fileSize = 10000000; // file size 10 MBytes
			maxPackets = fileSize / packetSize + 1;
			bitRate = 240;   	// Bit rate = 240 kbps
			interPacketInterval =  packetSize * 8 / bitRate; // packet size (bits) / bitRate (bits/sec)
			if (interPacketInterval < 1)
				interPacketInterval = 1;
			PacketSinkHelper packetSinkHelper_ue("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), otherPort1));
			PacketSinkHelper packetSinkHelper_remote("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), otherPort2));
			serverApps.Add(packetSinkHelper_ue.Install(ueNodes.Get(i)));
			serverApps.Add (packetSinkHelper_remote.Install (remoteHost));
			UdpClientHelper client_ue(ueIpIface.GetAddress(i), otherPort1);
			client_ue.SetAttribute("MaxPackets", UintegerValue(maxPackets));
			client_ue.SetAttribute("Interval", TimeValue(MilliSeconds(interPacketInterval)));
			client_ue.SetAttribute("PacketSize", UintegerValue(packetSize));
			UdpClientHelper client_remote(remoteHostAddr, otherPort2);
			client_remote.SetAttribute("MaxPackets", UintegerValue(maxPackets));
			client_remote.SetAttribute("Interval", TimeValue(MilliSeconds(interPacketInterval)));
			client_remote.SetAttribute("PacketSize", UintegerValue(packetSize));

			clientApps.Add (client_ue.Install (remoteHost));
			clientApps.Add(client_remote.Install(ueNodes.Get(i)));
			serverApps.Start (Seconds (0.01));
			clientApps.Start(Seconds(0.01));
		}
		else if (choice == 6)
		{
			// Stream and download music, smartphone
			// http://www.slideshare.net/althafhussain1023/how-to-dimension-user-traffic-in-lte (p 17)
			otherPort1 = 6004;
			otherPort2 = 6005;
			maxPackets = 233333 / packetSize + 1; // 7 MB/month ==> 0.233333 MB/day
			bitRate = 133.28;   	// Bit rate = 60MB/hour = 1 MB/min = 0.016666 MB/sec = 16,66 KB/sec = 133,28 Kbps
			interPacketInterval =  packetSize * 8 / bitRate; // packet size (bits) / bitRate (bits/sec)
			if (interPacketInterval < 1)
				interPacketInterval = 1;
			PacketSinkHelper packetSinkHelper_ue("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), otherPort1));
			PacketSinkHelper packetSinkHelper_remote("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), otherPort2));
			serverApps.Add(packetSinkHelper_ue.Install(ueNodes.Get(i)));
			serverApps.Add (packetSinkHelper_remote.Install (remoteHost));

			UdpClientHelper client_ue(ueIpIface.GetAddress(i), otherPort1);
			client_ue.SetAttribute("MaxPackets", UintegerValue(maxPackets));
			client_ue.SetAttribute("Interval", TimeValue(MilliSeconds(interPacketInterval)));
			client_ue.SetAttribute("PacketSize", UintegerValue(packetSize));
			UdpClientHelper client_remote(remoteHostAddr, otherPort2);
			client_remote.SetAttribute("MaxPackets", UintegerValue(maxPackets));
			client_remote.SetAttribute("Interval", TimeValue(MilliSeconds(interPacketInterval)));
			client_remote.SetAttribute("PacketSize", UintegerValue(packetSize));

			clientApps.Add (client_ue.Install (remoteHost));
			clientApps.Add(client_remote.Install(ueNodes.Get(i)));
			serverApps.Start (Seconds (0.01));
			clientApps.Start(Seconds(0.01));
		}
		else if (choice == 7)
		{
			// Stream video (4G), smartphone
			// http://www.slideshare.net/althafhussain1023/how-to-dimension-user-traffic-in-lte (p 17)
			otherPort1 = 6006;
			otherPort2 = 6007;
			bitRate = 777.76;   	// Bit Rate = 350MB/hour = 5,83333 MB/min = 0.097222 MB/sec = 97,22 KB/sec = 777,76 Kbps
			interPacketInterval =  packetSize * 8 / bitRate; // packet size (bits) / bitRate (bits/sec)
			if (interPacketInterval < 1)
				interPacketInterval = 1;
			PacketSinkHelper packetSinkHelper_ue("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), otherPort1));
			PacketSinkHelper packetSinkHelper_remote("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), otherPort2));
			serverApps.Add(packetSinkHelper_ue.Install(ueNodes.Get(i)));
			serverApps.Add (packetSinkHelper_remote.Install (remoteHost));
			UdpClientHelper client_ue(ueIpIface.GetAddress(i), otherPort1);
			client_ue.SetAttribute("Interval", TimeValue(MilliSeconds(interPacketInterval)));
			client_ue.SetAttribute("PacketSize", UintegerValue(packetSize));
			UdpClientHelper client_remote(remoteHostAddr, otherPort2);
			client_remote.SetAttribute("Interval", TimeValue(MilliSeconds(interPacketInterval)));
			client_remote.SetAttribute("PacketSize", UintegerValue(packetSize));

			clientApps.Add (client_ue.Install (remoteHost));
			clientApps.Add(client_remote.Install(ueNodes.Get(i)));
			serverApps.Start (Seconds (0.01));
			clientApps.Start(Seconds(0.01));
			serverApps.Stop(Seconds(10.0));
			clientApps.Stop(Seconds(10.0));
		}
		else if (choice == 8)
		{
			// Video calling, Tablet
			// http://www.slideshare.net/althafhussain1023/how-to-dimension-user-traffic-in-lte (p 17)
			otherPort1 = 6008;
			otherPort2 = 6009;
			bitRate = 333.36;   	// Bit Rate = 150MB/hour = 2,5 MB/min = 0.041667 MB/sec = 41,67 KB/sec = 333,36 Kbps
			interPacketInterval =  packetSize * 8 / bitRate; // packet size (bits) / bitRate (birs/sec)
			if (interPacketInterval < 1)
				interPacketInterval = 1;
			PacketSinkHelper packetSinkHelper_ue("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), otherPort1));
			PacketSinkHelper packetSinkHelper_remote("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), otherPort2));
			serverApps.Add(packetSinkHelper_ue.Install(ueNodes.Get(i)));
			serverApps.Add (packetSinkHelper_remote.Install (remoteHost));
			UdpClientHelper client_ue(ueIpIface.GetAddress(i), otherPort1);
			client_ue.SetAttribute("Interval", TimeValue(MilliSeconds(interPacketInterval)));
			client_ue.SetAttribute("PacketSize", UintegerValue(packetSize));
			UdpClientHelper client_remote(remoteHostAddr, otherPort2);
			client_remote.SetAttribute("Interval", TimeValue(MilliSeconds(interPacketInterval)));
			client_remote.SetAttribute("PacketSize", UintegerValue(packetSize));
			clientApps.Add (client_ue.Install (remoteHost));
			clientApps.Add(client_remote.Install(ueNodes.Get(i)));
			serverApps.Start (Seconds (0.01));
			clientApps.Start(Seconds(0.01));
		}
		else if (choice == 9)
		{
			// 4G VoIP, tablets
			// http://www.slideshare.net/althafhussain1023/how-to-dimension-user-traffic-in-lte (p 17)
			otherPort1 = 6030;
			otherPort2 = 6031;
			bitRate = 66.64;   	// Bit Rate = 30MB/hour = 0,5 MB/min = 0.00833 MB/sec = 8,33 KB/sec = 66,64 Kbps
			interPacketInterval =  packetSize * 8 / bitRate; // packet size (bits) / bitRate (bits/sec)
			if (interPacketInterval < 1)
				interPacketInterval = 1;
			PacketSinkHelper packetSinkHelper_ue("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), otherPort1));
			PacketSinkHelper packetSinkHelper_remote("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), otherPort2));
			serverApps.Add(packetSinkHelper_ue.Install(ueNodes.Get(i)));
			serverApps.Add (packetSinkHelper_remote.Install (remoteHost));
			UdpClientHelper client_ue(ueIpIface.GetAddress(i), otherPort1);
			client_ue.SetAttribute("Interval", TimeValue(MilliSeconds(interPacketInterval)));
			client_ue.SetAttribute("PacketSize", UintegerValue(packetSize));
			UdpClientHelper client_remote(remoteHostAddr, otherPort2);
			client_remote.SetAttribute("Interval", TimeValue(MilliSeconds(interPacketInterval)));
			client_remote.SetAttribute("PacketSize", UintegerValue(packetSize));
			clientApps.Add (client_ue.Install (remoteHost));
			clientApps.Add(client_remote.Install(ueNodes.Get(i)));
			serverApps.Start (Seconds (0.01));
			clientApps.Start(Seconds(0.01));
		}
		else
		{
			std::cout << "Choice out o range!\n";
		}
		// create bearers
		Ptr<EpcTft> tft = Create<EpcTft> ();
		EpcTft::PacketFilter dlpf;
		dlpf.localPortStart = otherPort1;
		dlpf.localPortEnd = otherPort1;
		tft->Add (dlpf);
		EpcTft::PacketFilter ulpf;
		ulpf.remotePortStart = otherPort2;
		ulpf.remotePortEnd = otherPort2;
		tft->Add (ulpf);
		GbrQosInformation qos;
		qos.gbrDl = 256000; // Downlink GBR
		qos.gbrUl = 256000; // Uplink GBR
		qos.mbrDl = 256000; // Downlink MBR
		qos.mbrUl = 256000; // Uplink MBR
		EpsBearer bearer (EpsBearer::NGBR_VIDEO_TCP_DEFAULT, qos);
		lteHelper->ActivateDedicatedEpsBearer (ueLteDevs.Get (i), bearer, tft);
	}

	lteHelper->EnablePdcpTraces();

	Simulator::Stop(Seconds(simTime));

	Simulator::Run();

	Simulator::Destroy();

	return 0;


}
