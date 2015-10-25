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
#include "ns3/config-store-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/mobility-building-info.h"
#include "ns3/buildings-propagation-loss-model.h"
#include "ns3/building.h"
#include "ns3/buildings-helper.h"
#include "ns3/buildings-module.h"
#include "ns3/log.h"
#include "iomanip"
#include "ios"
#include "string"
#include "vector"


using namespace ns3;

bool AreOverlapping (Box a, Box b)
{
  return !((a.xMin > b.xMax) || (b.xMin > a.xMax) || (a.yMin > b.yMax) || (b.yMin > a.yMax));
}


class FemtocellBlockAllocator
{
public:
  FemtocellBlockAllocator (Box area, uint32_t nApartmentsX, uint32_t nFloors);
  void Create (uint32_t n);
  void Create ();

private:
  bool OverlapsWithAnyPrevious (Box);
  Box m_area;
  uint32_t m_nApartmentsX;
  uint32_t m_nFloors;
  std::list<Box> m_previousBlocks;
  double m_xSize;
  double m_ySize;
  Ptr<UniformRandomVariable> m_xMinVar;
  Ptr<UniformRandomVariable> m_yMinVar;

};

FemtocellBlockAllocator::FemtocellBlockAllocator (Box area, uint32_t nApartmentsX, uint32_t nFloors)
  : m_area (area),
    m_nApartmentsX (nApartmentsX),
    m_nFloors (nFloors),
    m_xSize (nApartmentsX*10 + 20),
    m_ySize (70)
{
  m_xMinVar = CreateObject<UniformRandomVariable> ();
  m_xMinVar->SetAttribute ("Min", DoubleValue (area.xMin));
  m_xMinVar->SetAttribute ("Max", DoubleValue (area.xMax - m_xSize));
  m_yMinVar = CreateObject<UniformRandomVariable> ();
  m_yMinVar->SetAttribute ("Min", DoubleValue (area.yMin));
  m_yMinVar->SetAttribute ("Max", DoubleValue (area.yMax - m_ySize));
}

void
FemtocellBlockAllocator::Create (uint32_t n)
{
  for (uint32_t i = 0; i < n; ++i)
  {
      Create ();
  }
}

void
FemtocellBlockAllocator::Create ()
{
  Box box;
  uint32_t attempt = 0;
  do
  {
      NS_ASSERT_MSG (attempt < 100, "Too many failed attemtps to position apartment block. Too many blocks? Too small area?");
      box.xMin = m_xMinVar->GetValue ();
      box.xMax = box.xMin + m_xSize;
      box.yMin = m_yMinVar->GetValue ();
      box.yMax = box.yMin + m_ySize;
      ++attempt;
  }
  while (OverlapsWithAnyPrevious (box));

  m_previousBlocks.push_back (box);
  Ptr<GridBuildingAllocator>  gridBuildingAllocator;
  gridBuildingAllocator = CreateObject<GridBuildingAllocator> ();
  gridBuildingAllocator->SetAttribute ("GridWidth", UintegerValue (1));
  gridBuildingAllocator->SetAttribute ("LengthX", DoubleValue (10*m_nApartmentsX));
  gridBuildingAllocator->SetAttribute ("LengthY", DoubleValue (10*2));
  gridBuildingAllocator->SetAttribute ("DeltaX", DoubleValue (10));
  gridBuildingAllocator->SetAttribute ("DeltaY", DoubleValue (10));
  gridBuildingAllocator->SetAttribute ("Height", DoubleValue (3*m_nFloors));
  gridBuildingAllocator->SetBuildingAttribute ("NRoomsX", UintegerValue (m_nApartmentsX));
  gridBuildingAllocator->SetBuildingAttribute ("NRoomsY", UintegerValue (2));
  gridBuildingAllocator->SetBuildingAttribute ("NFloors", UintegerValue (m_nFloors));
  gridBuildingAllocator->SetAttribute ("MinX", DoubleValue (box.xMin + 10));
  gridBuildingAllocator->SetAttribute ("MinY", DoubleValue (box.yMin + 10));

  gridBuildingAllocator->Create (2);
}

bool
FemtocellBlockAllocator::OverlapsWithAnyPrevious (Box box)
{
  for (std::list<Box>::iterator it = m_previousBlocks.begin (); it != m_previousBlocks.end (); ++it)
  {
      if (AreOverlapping (*it, box))
      {
          return true;
      }
  }
  return false;
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
                      << "\" at "<< pos.x << "," << pos.y << " left font \"Helvetica,4\" textcolor rgb \"grey\" front point pt 1 ps 0.3 lc rgb \"grey\" offset 0,0"
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
                      << " left font \"Helvetica,4\" textcolor rgb \"white\" front  point pt 2 ps 0.3 lc rgb \"white\" offset 0,0"
                      << std::endl;
          }
      }
  }
}

static ns3::GlobalValue g_nBlocks ("nBlocks",
                                   "Number of femtocell blocks",
                                   ns3::UintegerValue (1),
                                   ns3::MakeUintegerChecker<uint32_t> ());
static ns3::GlobalValue g_nApartmentsX ("nApartmentsX",
                                        "Number of apartments along the X axis in a femtocell block",
                                        ns3::UintegerValue (10),
                                        ns3::MakeUintegerChecker<uint32_t> ());
static ns3::GlobalValue g_nFloors ("nFloors",
                                   "Number of floors",
                                   ns3::UintegerValue (1),
                                   ns3::MakeUintegerChecker<uint32_t> ());
static ns3::GlobalValue g_nMacroEnbSites ("nMacroEnbSites",
                                          "How many macro sites there are",
                                          ns3::UintegerValue (3),
                                          ns3::MakeUintegerChecker<uint32_t> ());
static ns3::GlobalValue g_nMacroEnbSitesX ("nMacroEnbSitesX",
                                           "(minimum) number of sites along the X-axis of the hex grid",
                                           ns3::UintegerValue (1),
                                           ns3::MakeUintegerChecker<uint32_t> ());
static ns3::GlobalValue g_interSiteDistance ("interSiteDistance",
                                             "min distance between two nearby macro cell sites",
                                             ns3::DoubleValue (500),
                                             ns3::MakeDoubleChecker<double> ());
static ns3::GlobalValue g_areaMarginFactor ("areaMarginFactor",
                                            "how much the UE area extends outside the macrocell grid, "
                                            "expressed as fraction of the interSiteDistance",
                                            ns3::DoubleValue (0.5),
                                            ns3::MakeDoubleChecker<double> ());

static ns3::GlobalValue g_homeEnbDeploymentRatio ("homeEnbDeploymentRatio",
                                                  "The HeNB deployment ratio as per 3GPP R4-092042",
                                                  ns3::DoubleValue (0.2),
                                                  ns3::MakeDoubleChecker<double> ());
static ns3::GlobalValue g_homeEnbActivationRatio ("homeEnbActivationRatio",
                                                  "The HeNB activation ratio as per 3GPP R4-092042",
                                                  ns3::DoubleValue (0.5),
                                                  ns3::MakeDoubleChecker<double> ());

static ns3::GlobalValue g_macroEnbTxPowerDbm ("macroEnbTxPowerDbm",
                                              "TX power [dBm] used by macro eNBs",
                                              ns3::DoubleValue (46.0),
                                              ns3::MakeDoubleChecker<double> ());
static ns3::GlobalValue g_homeEnbTxPowerDbm ("homeEnbTxPowerDbm",
                                             "TX power [dBm] used by HeNBs",
                                             ns3::DoubleValue (20.0),
                                             ns3::MakeDoubleChecker<double> ());
static ns3::GlobalValue g_macroEnbDlEarfcn ("macroEnbDlEarfcn",
                                            "DL EARFCN used by macro eNBs",
                                            ns3::UintegerValue (100),
                                            ns3::MakeUintegerChecker<uint16_t> ());
static ns3::GlobalValue g_homeEnbDlEarfcn ("homeEnbDlEarfcn",
                                           "DL EARFCN used by HeNBs",
                                           ns3::UintegerValue (100),
                                           ns3::MakeUintegerChecker<uint16_t> ());

static ns3::GlobalValue g_outdoorUeMinSpeed ("outdoorUeMinSpeed",
                                             "Minimum speed value of macor UE with random waypoint model [m/s].",
                                             ns3::DoubleValue (0.0),
                                             ns3::MakeDoubleChecker<double> ());
static ns3::GlobalValue g_outdoorUeMaxSpeed ("outdoorUeMaxSpeed",
                                             "Maximum speed value of macor UE with random waypoint model [m/s].",
                                             ns3::DoubleValue (0.0),
                                             ns3::MakeDoubleChecker<double> ());

int main(int argc, char *argv[]) {
	uint16_t macroEnbBandwidth = 15;

	uint16_t homeEnbBandwidth = 6;
	uint32_t nHomeUes = 10;
	uint32_t nMacroUes = 10;
	std::string schedulerType = "rr";
	bool createRem = false;
	double simTime = 1; // 100

	if (argc == 2)
	{
		nHomeUes = atoi(argv[1]);
		nMacroUes = atoi(argv[1]);
	}
	else if (argc == 3)
	{
		nHomeUes = atoi(argv[1]);
		nMacroUes = atoi(argv[1]);
		schedulerType = argv[2];
	}
	else if (argc == 4)
	{
		nHomeUes = atoi(argv[1]);
		nMacroUes = atoi(argv[1]);
		schedulerType = argv[2];
		std::string argv3 = argv[3];

		if (argv3.compare("rem") == 0)
			createRem = true;
	}

	Config::SetDefault("ns3::RadioBearerStatsCalculator::EpochDuration",TimeValue (Seconds (1.0)));
	// the scenario parameters get their values from the global attributes defined above
	UintegerValue uintegerValue;
	IntegerValue integerValue;
	DoubleValue doubleValue;
	BooleanValue booleanValue;
	StringValue stringValue;
	GlobalValue::GetValueByName ("nBlocks", uintegerValue);
	uint32_t nBlocks = uintegerValue.Get ();
	GlobalValue::GetValueByName ("nApartmentsX", uintegerValue);
	uint32_t nApartmentsX = uintegerValue.Get ();
	GlobalValue::GetValueByName ("nFloors", uintegerValue);
	uint32_t nFloors = uintegerValue.Get ();
	GlobalValue::GetValueByName ("nMacroEnbSites", uintegerValue);
	uint32_t nMacroEnbSites = uintegerValue.Get ();

	GlobalValue::GetValueByName ("nMacroEnbSitesX", uintegerValue);
	uint32_t nMacroEnbSitesX = uintegerValue.Get ();
	GlobalValue::GetValueByName ("interSiteDistance", doubleValue);
	double interSiteDistance = doubleValue.Get ();
	GlobalValue::GetValueByName ("areaMarginFactor", doubleValue);
	double areaMarginFactor = doubleValue.Get ();

	GlobalValue::GetValueByName ("homeEnbDeploymentRatio", doubleValue);
	double homeEnbDeploymentRatio = doubleValue.Get ();
	GlobalValue::GetValueByName ("homeEnbActivationRatio", doubleValue);
	double homeEnbActivationRatio = doubleValue.Get ();

	GlobalValue::GetValueByName ("macroEnbTxPowerDbm", doubleValue);
	double macroEnbTxPowerDbm = doubleValue.Get ();
	GlobalValue::GetValueByName ("homeEnbTxPowerDbm", doubleValue);
	double homeEnbTxPowerDbm = doubleValue.Get ();
	GlobalValue::GetValueByName ("macroEnbDlEarfcn", uintegerValue);
	uint16_t macroEnbDlEarfcn = uintegerValue.Get ();
	GlobalValue::GetValueByName ("homeEnbDlEarfcn", uintegerValue);
	uint16_t homeEnbDlEarfcn = uintegerValue.Get ();

	Box macroUeBox;
	double ueZ = 1.5;
	if (nMacroEnbSites > 0)
	{
	      uint32_t currentSite = nMacroEnbSites -1;
	      uint32_t biRowIndex = (currentSite / (nMacroEnbSitesX + nMacroEnbSitesX + 1));
	      uint32_t biRowRemainder = currentSite % (nMacroEnbSitesX + nMacroEnbSitesX + 1);
	      uint32_t rowIndex = biRowIndex*2 + 1;
	      if (biRowRemainder >= nMacroEnbSitesX)
	      {
	          ++rowIndex;
	      }
	      uint32_t nMacroEnbSitesY = rowIndex;

	      macroUeBox = Box (-areaMarginFactor*interSiteDistance,
	                        (nMacroEnbSitesX + areaMarginFactor)*interSiteDistance,
	                        -areaMarginFactor*interSiteDistance,
	                        (nMacroEnbSitesY -1)*interSiteDistance*sqrt (0.75) + areaMarginFactor*interSiteDistance,
	                        ueZ, ueZ);

	}
	else
	{
	      // still need the box to place femtocell blocks
	      macroUeBox = Box (0, 150, 0, 150, ueZ, ueZ);
	}

	FemtocellBlockAllocator blockAllocator (macroUeBox, nApartmentsX, nFloors);
	blockAllocator.Create (nBlocks);

	uint32_t nHomeEnbs = round (4 * nApartmentsX * nBlocks * nFloors * homeEnbDeploymentRatio * homeEnbActivationRatio);


	NodeContainer homeEnbs;
	homeEnbs.Create (nHomeEnbs);
	NodeContainer macroEnbs;
	macroEnbs.Create (3 * nMacroEnbSites);

	MobilityHelper mobility;
	mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

	Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();

	Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper>();
	lteHelper->SetEpcHelper(epcHelper);



	// Set the RBs
	lteHelper->SetAttribute ("PathlossModel", StringValue ("ns3::HybridBuildingsPropagationLossModel"));
	lteHelper->SetPathlossModelAttribute ("ShadowSigmaExtWalls", DoubleValue (0));
	lteHelper->SetPathlossModelAttribute ("ShadowSigmaOutdoor", DoubleValue (1));
	lteHelper->SetPathlossModelAttribute ("ShadowSigmaIndoor", DoubleValue (1.5));
	// use always LOS model
	lteHelper->SetPathlossModelAttribute ("Los2NlosThr", DoubleValue (1e6));
	lteHelper->SetSpectrumChannelType ("ns3::MultiModelSpectrumChannel");


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
	// Macro eNBs in 3-sector hex grid
	mobility.Install (macroEnbs);
	BuildingsHelper::Install (macroEnbs);
	Ptr<LteHexGridEnbTopologyHelper> lteHexGridEnbTopologyHelper = CreateObject<LteHexGridEnbTopologyHelper> ();
	lteHexGridEnbTopologyHelper->SetLteHelper (lteHelper);
	lteHexGridEnbTopologyHelper->SetAttribute ("InterSiteDistance", DoubleValue (interSiteDistance));
	lteHexGridEnbTopologyHelper->SetAttribute ("MinX", DoubleValue (interSiteDistance/2));
	lteHexGridEnbTopologyHelper->SetAttribute ("GridWidth", UintegerValue (nMacroEnbSitesX));
	Config::SetDefault ("ns3::LteEnbPhy::TxPower", DoubleValue (macroEnbTxPowerDbm));
	lteHelper->SetEnbAntennaModelType ("ns3::ParabolicAntennaModel");
	lteHelper->SetEnbAntennaModelAttribute ("Beamwidth",   DoubleValue (70));
	lteHelper->SetEnbAntennaModelAttribute ("MaxAttenuation",     DoubleValue (20.0));
	lteHelper->SetEnbDeviceAttribute ("DlEarfcn", UintegerValue (macroEnbDlEarfcn));
	lteHelper->SetEnbDeviceAttribute ("UlEarfcn", UintegerValue (macroEnbDlEarfcn + 18000));
	lteHelper->SetEnbDeviceAttribute ("DlBandwidth", UintegerValue (macroEnbBandwidth));
	lteHelper->SetEnbDeviceAttribute ("UlBandwidth", UintegerValue (macroEnbBandwidth));


	NetDeviceContainer macroEnbDevs = lteHexGridEnbTopologyHelper->SetPositionAndInstallEnbDevice (macroEnbs);

	// HomeEnbs randomly indoor

	Ptr<PositionAllocator> positionAlloc = CreateObject<RandomRoomPositionAllocator> ();
	mobility.SetPositionAllocator (positionAlloc);
	mobility.Install (homeEnbs);
	BuildingsHelper::Install (homeEnbs);
	Config::SetDefault ("ns3::LteEnbPhy::TxPower", DoubleValue (homeEnbTxPowerDbm));
	lteHelper->SetEnbAntennaModelType ("ns3::IsotropicAntennaModel");
	lteHelper->SetEnbDeviceAttribute ("DlEarfcn", UintegerValue (homeEnbDlEarfcn));
	lteHelper->SetEnbDeviceAttribute ("UlEarfcn", UintegerValue (homeEnbDlEarfcn + 18000));
	lteHelper->SetEnbDeviceAttribute ("DlBandwidth", UintegerValue (homeEnbBandwidth));
	lteHelper->SetEnbDeviceAttribute ("UlBandwidth", UintegerValue (homeEnbBandwidth));

	NetDeviceContainer homeEnbDevs  = lteHelper->InstallEnbDevice (homeEnbs);

	// this enables handover for macro eNBs
	lteHelper->AddX2Interface (macroEnbs);
	lteHelper->AddX2Interface (homeEnbs);
	for(uint32_t jj = 0; jj<macroEnbs.GetN();jj++)
		for(uint32_t jjj = 0; jjj<homeEnbs.GetN();jjj++)
			lteHelper->AddX2Interface(macroEnbs.Get(jj),homeEnbs.Get(jjj));

	// create a remote host
	Ptr<Node> remoteHost;
	NodeContainer remotehostContainer;
	remotehostContainer.Create(1);
	remoteHost = remotehostContainer.Get(0);
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

	Ipv4Address remoteHostAddr;
	Ipv4StaticRoutingHelper ipv4RoutingHelper;
	Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting(remoteHost->GetObject<Ipv4>());
	remoteHostStaticRouting->AddNetworkRouteTo(Ipv4Address("7.0.0.0"), Ipv4Mask("255.0.0.0"), 1);
	// Create Ues
	NodeContainer homeUes;
	homeUes.Create (nHomeUes);
	NodeContainer macroUes;
	macroUes.Create (nMacroUes);

	// home UEs located in the same apartment in which there are the Home eNBs
	positionAlloc = CreateObject<SameRoomPositionAllocator> (homeEnbs);
	mobility.SetPositionAllocator (positionAlloc);
	mobility.Install (homeUes);
	BuildingsHelper::Install (homeUes);

	NetDeviceContainer homeUeDevs = lteHelper->InstallUeDevice (homeUes);

	// macro Ues

	positionAlloc = CreateObject<RandomBoxPositionAllocator> ();
	Ptr<UniformRandomVariable> xVal = CreateObject<UniformRandomVariable> ();
	xVal->SetAttribute ("Min", DoubleValue (macroUeBox.xMin));
	xVal->SetAttribute ("Max", DoubleValue (macroUeBox.xMax));
	positionAlloc->SetAttribute ("X", PointerValue (xVal));
	Ptr<UniformRandomVariable> yVal = CreateObject<UniformRandomVariable> ();
	yVal->SetAttribute ("Min", DoubleValue (macroUeBox.yMin));
	yVal->SetAttribute ("Max", DoubleValue (macroUeBox.yMax));
	positionAlloc->SetAttribute ("Y", PointerValue (yVal));
	Ptr<UniformRandomVariable> zVal = CreateObject<UniformRandomVariable> ();
	zVal->SetAttribute ("Min", DoubleValue (macroUeBox.zMin));
	zVal->SetAttribute ("Max", DoubleValue (macroUeBox.zMax));
	positionAlloc->SetAttribute ("Z", PointerValue (zVal));
	mobility.SetPositionAllocator (positionAlloc);
	mobility.Install (macroUes);


	BuildingsHelper::Install (macroUes);

	NetDeviceContainer macroUeDevs = lteHelper->InstallUeDevice (macroUes);

	NodeContainer ues;

	Ipv4InterfaceContainer ueIpIfaces;

	NetDeviceContainer ueDevs;




	remoteHostAddr = internetIpIfaces.GetAddress(1);

	// for internetworking purposes, consider together home UEs and macro UEs
	ues.Add (homeUes);
	ues.Add (macroUes);
	ueDevs.Add (homeUeDevs);
	ueDevs.Add (macroUeDevs);

	// Install the IP stack on the UEs
	internet.Install (ues);
	ueIpIfaces = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueDevs));

	// attachment (needs to be done after IP stack configuration)
	// using initial cell selection
	lteHelper->Attach (macroUeDevs);
	lteHelper->Attach (homeUeDevs);

	BuildingsHelper::MakeMobilityModelConsistent ();

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

		remHelper->SetAttribute ("XMin", DoubleValue (macroUeBox.xMin));
		remHelper->SetAttribute ("XMax", DoubleValue (macroUeBox.xMax));
		remHelper->SetAttribute ("XRes", UintegerValue (200));
		remHelper->SetAttribute ("YMin", DoubleValue (macroUeBox.yMin));
		remHelper->SetAttribute ("YMax", DoubleValue (macroUeBox.yMax));
		remHelper->SetAttribute ("YRes", UintegerValue (200));
		remHelper->SetAttribute ("Z", DoubleValue (1.5));
		remHelper->SetAttribute ("UseDataChannel", BooleanValue (true));

		remHelper->Install ();
	}

	// install applications

	for (uint16_t i = 0; i < ues.GetN(); i++)
	{
		Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ues.Get(i)->GetObject<Ipv4> ());
		ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
		double interPacketInterval;
		double packetSize = 1024;
		ApplicationContainer clientApps;
		ApplicationContainer serverApps;

		uint16_t otherPort1 = 3000;
		uint16_t otherPort2 = 3001;
		uint32_t fileSize = 3000000; // file size 3MBytes
		uint32_t maxPackets = fileSize / packetSize + 1;
		double bitRate = 11722;   	 // Bit rate = 11722 kbps
		uint16_t choice = i%10;
		if (choice == 0)
		{
			// download file from remote host
			// https://www.swisscom.ch/dam/swisscom/en/res/mobile/mobile-network/netztest-connect-en-2014.pdf (page 4)
			interPacketInterval =  packetSize * 8 / bitRate; // packet size (bits) / bitRate (bits/sec)
			if (interPacketInterval < 1)
				interPacketInterval = 1;
			PacketSinkHelper packetSinkHelper_ue ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), otherPort1));
			PacketSinkHelper packetSinkHelper_remote("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), otherPort2));

			serverApps.Add(packetSinkHelper_ue.Install(ues.Get(i)));
			serverApps.Add (packetSinkHelper_remote.Install (remoteHost));

			UdpClientHelper client_ue(ueIpIfaces.GetAddress(i), otherPort1);
			client_ue.SetAttribute("MaxPackets", UintegerValue(maxPackets));
			client_ue.SetAttribute("Interval", TimeValue(MilliSeconds(interPacketInterval)));
			client_ue.SetAttribute("PacketSize", UintegerValue(packetSize));
			UdpClientHelper client_remote (remoteHostAddr, otherPort2);
			client_remote.SetAttribute("MaxPackets", UintegerValue(maxPackets));
			client_remote.SetAttribute("Interval", TimeValue(MilliSeconds(interPacketInterval)));
			client_remote.SetAttribute("PacketSize", UintegerValue(packetSize));

			clientApps.Add (client_ue.Install (remoteHost));
			clientApps.Add(client_remote.Install(ues.Get(i)));
			serverApps.Start (Seconds (0.01));
			clientApps.Start(Seconds(0.01));
		}
		else if (choice == 1)
		{
			// transmission from the ue to remote controller (video)
			otherPort1 = 4000;
			otherPort2 = 4010;
			interPacketInterval = packetSize*8 / 657;

			PacketSinkHelper packetSinkHelper_ue("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), otherPort1));
			PacketSinkHelper packetSinkHelper_remote("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), otherPort2));
			serverApps.Add (packetSinkHelper_ue.Install (ues.Get(i)));
			serverApps.Add (packetSinkHelper_remote.Install (remoteHost));
			UdpClientHelper client(ueIpIfaces.GetAddress(i), otherPort1);
			client.SetAttribute ("Interval", TimeValue (MilliSeconds(interPacketInterval))); // 657 KBps (http://www.theglobeandmail.com/technology/tech-news/how-much-bandwidth-does-streaming-use/article7365916/)

			client.SetAttribute("PacketSize", UintegerValue(packetSize));
			UdpClientHelper client_remote(remoteHostAddr, otherPort2);

			client_remote.SetAttribute("Interval", TimeValue(MilliSeconds(interPacketInterval)));
			client_remote.SetAttribute("PacketSize", UintegerValue(packetSize));
			clientApps.Add (client.Install (remoteHost));
			clientApps.Add(client_remote.Install(ues.Get(i)));
			serverApps.Start (Seconds (0.01));
			clientApps.Start (Seconds (0.01));
		}
		else if (choice == 2)
		{
			// file upload
			otherPort1 = 5000;
			otherPort2 = 5010;
			fileSize = 1000000; // file size 1MByte
			maxPackets = fileSize / packetSize + 1;
			interPacketInterval = packetSize*8 / 1788; // 1788Kbps
			PacketSinkHelper packetSinkHelper_ue("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), otherPort1));
			PacketSinkHelper packetSinkHelper_remote("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), otherPort2));
			serverApps.Add (packetSinkHelper_ue.Install (ues.Get(i)));
			serverApps.Add (packetSinkHelper_remote.Install(remoteHost));
			UdpClientHelper client(ueIpIfaces.GetAddress(i), otherPort1);
			client.SetAttribute ("Interval", TimeValue (MilliSeconds(interPacketInterval)));
			client.SetAttribute ("MaxPackets", UintegerValue(maxPackets));
			client.SetAttribute("PacketSize", UintegerValue(packetSize));
			UdpClientHelper client_remote(remoteHostAddr, otherPort2);
			client_remote.SetAttribute("MaxPackets", UintegerValue(maxPackets));
			client_remote.SetAttribute("Interval", TimeValue(MilliSeconds(interPacketInterval)));
			client_remote.SetAttribute("PacketSize", UintegerValue(packetSize));
			clientApps.Add (client.Install (remoteHost));
			clientApps.Add(client_remote.Install(ues.Get(i)));
			serverApps.Start (Seconds (0.01));
			clientApps.Start (Seconds (0.01));
		}
		else if (choice == 3)
		{
			// download file (10 MB)
			otherPort1 = 6000;
			otherPort2 = 6001;
			fileSize = 10000000; // file size 10 MBytes
			maxPackets = fileSize / packetSize + 1;
			bitRate = 13463;   	// Bit rate = 13463 kbps
			interPacketInterval =  packetSize * 8 / bitRate; // packet size (bits) / bitRate (bits/sec)
			if (interPacketInterval < 1)
				interPacketInterval = 1;
			PacketSinkHelper packetSinkHelper_ue("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), otherPort1));
			PacketSinkHelper packetSinkHelper_remote("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), otherPort2));
			serverApps.Add(packetSinkHelper_ue.Install(ues.Get(i)));
			serverApps.Add (packetSinkHelper_remote.Install (remoteHost));
			UdpClientHelper client_ue(ueIpIfaces.GetAddress(i), otherPort1);
			client_ue.SetAttribute("MaxPackets", UintegerValue(maxPackets));
			client_ue.SetAttribute("Interval", TimeValue(MilliSeconds(interPacketInterval)));
			client_ue.SetAttribute("PacketSize", UintegerValue(packetSize));
			UdpClientHelper client_remote(remoteHostAddr, otherPort2);
			client_remote.SetAttribute("MaxPackets", UintegerValue(maxPackets));
			client_remote.SetAttribute("Interval", TimeValue(MilliSeconds(interPacketInterval)));
			client_remote.SetAttribute("PacketSize", UintegerValue(packetSize));

			clientApps.Add (client_ue.Install (remoteHost));
			clientApps.Add(client_remote.Install(ues.Get(i)));
			serverApps.Start (Seconds (0.01));
			clientApps.Start(Seconds(0.01));
		}
		else if (choice == 4)
		{
			// file upload
			otherPort1 = 5020;
			otherPort2 = 5021;
			fileSize = 10000000; // file size 10MBytes
			maxPackets = fileSize / packetSize + 1;
			bitRate = 1920;   	// bit rate = 1920 kbps
			interPacketInterval =  packetSize * 8 / bitRate; // packet size (bits) / bitRate (bits/sec)
			PacketSinkHelper packetSinkHelper_ue("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), otherPort1));
			PacketSinkHelper packetSinkHelper_remote("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), otherPort2));
			serverApps.Add (packetSinkHelper_ue.Install (ues.Get(i)));
			serverApps.Add (packetSinkHelper_remote.Install(remoteHost));
			UdpClientHelper client(ueIpIfaces.GetAddress(i), otherPort1);
			client.SetAttribute ("Interval", TimeValue (MilliSeconds(interPacketInterval)));
			client.SetAttribute ("MaxPackets", UintegerValue(maxPackets));
			client.SetAttribute("PacketSize", UintegerValue(packetSize));
			UdpClientHelper client_remote(remoteHostAddr, otherPort2);
			client_remote.SetAttribute("MaxPackets", UintegerValue(maxPackets));
			client_remote.SetAttribute("Interval", TimeValue(MilliSeconds(interPacketInterval)));
			client_remote.SetAttribute("PacketSize", UintegerValue(packetSize));
			clientApps.Add (client.Install (remoteHost));
			clientApps.Add(client_remote.Install(ues.Get(i)));
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
			bitRate = 240;   	// bit rate = 240 kbps
			interPacketInterval =  packetSize * 8 / bitRate; // packet size (bits) / bitRate (bits/sec)
			if (interPacketInterval < 1)
				interPacketInterval = 1;
			PacketSinkHelper packetSinkHelper_ue("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), otherPort1));
			PacketSinkHelper packetSinkHelper_remote("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), otherPort2));
			serverApps.Add(packetSinkHelper_ue.Install(ues.Get(i)));
			serverApps.Add (packetSinkHelper_remote.Install (remoteHost));
			UdpClientHelper client_ue(ueIpIfaces.GetAddress(i), otherPort1);
			client_ue.SetAttribute("MaxPackets", UintegerValue(maxPackets));
			client_ue.SetAttribute("Interval", TimeValue(MilliSeconds(interPacketInterval)));
			client_ue.SetAttribute("PacketSize", UintegerValue(packetSize));
			UdpClientHelper client_remote(remoteHostAddr, otherPort2);
			client_remote.SetAttribute("MaxPackets", UintegerValue(maxPackets));
			client_remote.SetAttribute("Interval", TimeValue(MilliSeconds(interPacketInterval)));
			client_remote.SetAttribute("PacketSize", UintegerValue(packetSize));

			clientApps.Add (client_ue.Install (remoteHost));
			clientApps.Add(client_remote.Install(ues.Get(i)));
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
			bitRate = 133.28;   	// Bit rate= 60MB/hour = 1 MB/min = 0.016666 MB/sec = 16,66 KB/sec = 133,28 Kbps
			interPacketInterval =  packetSize * 8 / bitRate;
			if (interPacketInterval < 1)
				interPacketInterval = 1;
			PacketSinkHelper packetSinkHelper_ue("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), otherPort1));
			PacketSinkHelper packetSinkHelper_remote("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), otherPort2));
			serverApps.Add(packetSinkHelper_ue.Install(ues.Get(i)));
			serverApps.Add (packetSinkHelper_remote.Install (remoteHost));

			UdpClientHelper client_ue(ueIpIfaces.GetAddress(i), otherPort1);
			client_ue.SetAttribute("MaxPackets", UintegerValue(maxPackets));
			client_ue.SetAttribute("Interval", TimeValue(MilliSeconds(interPacketInterval)));
			client_ue.SetAttribute("PacketSize", UintegerValue(packetSize));
			UdpClientHelper client_remote(remoteHostAddr, otherPort2);
			client_remote.SetAttribute("MaxPackets", UintegerValue(maxPackets));
			client_remote.SetAttribute("Interval", TimeValue(MilliSeconds(interPacketInterval)));
			client_remote.SetAttribute("PacketSize", UintegerValue(packetSize));

			clientApps.Add (client_ue.Install (remoteHost));
			clientApps.Add(client_remote.Install(ues.Get(i)));
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
			serverApps.Add(packetSinkHelper_ue.Install(ues.Get(i)));
			serverApps.Add (packetSinkHelper_remote.Install (remoteHost));
			UdpClientHelper client_ue(ueIpIfaces.GetAddress(i), otherPort1);
			client_ue.SetAttribute("Interval", TimeValue(MilliSeconds(interPacketInterval)));
			client_ue.SetAttribute("PacketSize", UintegerValue(packetSize));
			UdpClientHelper client_remote(remoteHostAddr, otherPort2);
			client_remote.SetAttribute("Interval", TimeValue(MilliSeconds(interPacketInterval)));
			client_remote.SetAttribute("PacketSize", UintegerValue(packetSize));

			clientApps.Add (client_ue.Install (remoteHost));
			clientApps.Add(client_remote.Install(ues.Get(i)));
			serverApps.Start (Seconds (0.01));
			clientApps.Start(Seconds(0.01));
		}
		else if (choice == 8)
		{
			// Video calling, Tablet
			// http://www.slideshare.net/althafhussain1023/how-to-dimension-user-traffic-in-lte (p 17)
			otherPort1 = 6008;
			otherPort2 = 6009;
			bitRate = 333.36;   	// Bit Rate = 150MB/hour = 2,5 MB/min = 0.041667 MB/sec = 41,67 KB/sec = 333,36 Kbps
			interPacketInterval =  packetSize * 8 / bitRate; // packet size (bits) / bitRate (bits/sec)
			if (interPacketInterval < 1)
				interPacketInterval = 1;
			PacketSinkHelper packetSinkHelper_ue("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), otherPort1));
			PacketSinkHelper packetSinkHelper_remote("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), otherPort2));
			serverApps.Add(packetSinkHelper_ue.Install(ues.Get(i)));
			serverApps.Add (packetSinkHelper_remote.Install (remoteHost));
			UdpClientHelper client_ue(ueIpIfaces.GetAddress(i), otherPort1);
			client_ue.SetAttribute("Interval", TimeValue(MilliSeconds(interPacketInterval)));
			client_ue.SetAttribute("PacketSize", UintegerValue(packetSize));
			UdpClientHelper client_remote(remoteHostAddr, otherPort2);
			client_remote.SetAttribute("Interval", TimeValue(MilliSeconds(interPacketInterval)));
			client_remote.SetAttribute("PacketSize", UintegerValue(packetSize));
			clientApps.Add (client_ue.Install (remoteHost));
			clientApps.Add(client_remote.Install(ues.Get(i)));
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
			serverApps.Add(packetSinkHelper_ue.Install(ues.Get(i)));
			serverApps.Add (packetSinkHelper_remote.Install (remoteHost));
			UdpClientHelper client_ue(ueIpIfaces.GetAddress(i), otherPort1);
			client_ue.SetAttribute("Interval", TimeValue(MilliSeconds(interPacketInterval)));
			client_ue.SetAttribute("PacketSize", UintegerValue(packetSize));
			UdpClientHelper client_remote(remoteHostAddr, otherPort2);
			client_remote.SetAttribute("Interval", TimeValue(MilliSeconds(interPacketInterval)));
			client_remote.SetAttribute("PacketSize", UintegerValue(packetSize));
			clientApps.Add (client_ue.Install (remoteHost));
			clientApps.Add(client_remote.Install(ues.Get(i)));
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
		lteHelper->ActivateDedicatedEpsBearer (ueDevs.Get (i), bearer, tft);
	}
	lteHelper->EnablePdcpTraces();



	Simulator::Stop(Seconds(simTime));

	Simulator::Run();

	Simulator::Destroy();

	return 0;


}
