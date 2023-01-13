/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008,2009 IITP RAS
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Kirill Andreev <andreev@iitp.ru>
 *
 *
 * By default this script creates m_xSize * m_ySize square grid topology with
 * IEEE802.11s stack installed at each node with peering management
 * and HWMP protocol.
 * The side of the square cell is defined by m_step parameter.
 * When topology is created, UDP ping is installed to opposite corners
 * by diagonals. packet size of the UDP ping and interval between two
 * successive packets is configurable.
 * 
 *  
//
//          +--------------------------------------------------------+
//          |                                                        |
//          |              Root Peers ( n nodes)          | 
//          |                                                        |
//          +--------------------------------------------------------+
//                   |       o o o (wireless)       |
//               +--------+                               +--------+
//              | parent |                               | Parent |
//    -----------| peer
             m =2^n nodes |                 -----------| peer m =2^n nodes|
//               ---------                                ---------
//                   |                                        |
//          +----------------+                       +----------------+
//          |    Children     |                       |    Children   |
//          |   Peer 2^m  |                            |   Peer   |
//          |     |                                   |  2^m   |
//          +----------------+                       +----------------+
 *
 *  See also MeshTest::Configure to read more about configurable
 *  parameters.
 */

#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mesh-module.h"
#include "ns3/mobility-module.h"
#include "ns3/mesh-helper.h"

#include <iostream>
#include <sstream>
#include <fstream>
#include "ns3/applications-module.h"
#include "ns3/csma-module.h"
#include "ns3/olsr-helper.h"
#include "ns3/internet-module.h"
#include "ns3/netanim-module.h"
#include "ns3/config-store-module.h"
#include "ns3/flow-monitor-module.h"
#include <iomanip>
#include <string>
#include "ns3/gnuplot.h"


using namespace ns3;

void ThroughputMonitor (FlowMonitorHelper *fmhelper, Ptr<FlowMonitor> flowMon,Gnuplot2dDataset DataSet);
NS_LOG_COMPONENT_DEFINE ("ExistingMesh");

/**
 * \ingroup mesh
 * \brief MeshTest class
 */
class MeshTest
{
public:
  /// Init test
  MeshTest ();
  /**
   * Configure test from command line arguments
   *
   * \param argc command line argument count
   * \param argv command line arguments
   */
  void Configure (int argc, char ** argv);
  /**
   * Run test
   * \returns the test status
   */
  int Run ();

private:
  uint32_t rootNodes ;
  uint32_t parentNodes; 
  uint32_t childrenNodes; 
   uint32_t Nodes ;  
 // int       m_xSize; ///< X size
  //int       m_ySize; ///< Y size
  double    m_step; ///< step
  double    m_randomStart; ///< random start
  double    m_totalTime; ///< total time
  double    m_packetInterval; ///< packet interval
  uint16_t  m_packetSize; ///< packet size
  uint32_t  m_nIfaces; ///< number interfaces
  bool      m_chan; ///< channel
  bool      m_pcap; ///< PCAP
  bool      m_ascii; ///< ASCII
  std::string m_stack; ///< stack
  std::string m_root; ///< root

/// List of network nodes
  //NodeContainer nodes;
  NodeContainer rootPeer;
  NodeContainer parentPeer;
  NodeContainer newParentNodes;
  NodeContainer childrenPeer;
  NodeContainer newChildrenNodes;

/// List of all mesh point devices
  //NetDeviceContainer meshDevices;
  NetDeviceContainer rootDevices;
  NetDeviceContainer parentDevices;
  NetDeviceContainer  childrenDevices;

/// Addresses of interfaces:
  Ipv4InterfaceContainer interfaces;

 /// MeshHelper. Report is not static methods
  MeshHelper mesh;

private:
  /// Create nodes and setup their mobility
  void CreateNodes ();
  /// Install internet m_stack on nodes
  void InstallInternetStack ();
  /// Install applications
  void InstallApplication ();
  /// Print mesh devices diagnostics
  void Report ();
};

MeshTest::MeshTest () :
  //m_xSize (3),
  //m_ySize (3),
 // m_step (100.0),
  rootNodes(2),
  parentNodes(5),
  childrenNodes(5),
  m_randomStart (0.1),
  m_totalTime (100.0),
  m_packetInterval (0.1),
  m_packetSize (1024),
  m_nIfaces (1),
  m_chan (true),
  m_pcap (false),
  m_ascii (false),
  m_stack ("ns3::Dot11sStack"),
  m_root ("ff:ff:ff:ff:ff:ff")
{
}


void
MeshTest::Configure (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.AddValue ("rootNodes", "number of root nodes", rootNodes);
  cmd.AddValue ("parentNodes", "number of leaf nodes", parentNodes);
  cmd.AddValue ("childrenNodes", "number of LAN nodes", childrenNodes);
  cmd.AddValue ("Nodes", "total numbers of nodes", Nodes);
  //cmd.AddValue ("x-size", "Number of nodes in a row grid", m_xSize);
  //cmd.AddValue ("y-size", "Number of rows in a grid", m_ySize);
  // cmd.AddValue ("step",   "Size of edge in our grid (meters)", m_step);
  // Avoid starting all mesh nodes at the same time (beacons may collide)
  cmd.AddValue ("start",  "Maximum random start delay for beacon jitter (sec)", m_randomStart);
  cmd.AddValue ("time",  "Simulation time (sec)", m_totalTime);
  cmd.AddValue ("packet-interval",  "Interval between packets in UDP ping (sec)", m_packetInterval);
  cmd.AddValue ("packet-size",  "Size of packets in UDP ping (bytes)", m_packetSize);
  cmd.AddValue ("interfaces", "Number of radio interfaces used by each mesh point", m_nIfaces);
  cmd.AddValue ("channels",   "Use different frequency channels for different interfaces", m_chan);
  cmd.AddValue ("pcap",   "Enable PCAP traces on interfaces", m_pcap);
  cmd.AddValue ("ascii",   "Enable Ascii traces on interfaces", m_ascii);
  cmd.AddValue ("stack",  "Type of protocol stack. ns3::Dot11sStack by default", m_stack);
  cmd.AddValue ("root", "Mac address of root mesh point in HWMP", m_root);

  cmd.Parse (argc, argv);
  //NS_LOG_DEBUG ("Grid:" << m_xSize << "*" << m_ySize);
  NS_LOG_DEBUG ("Simulation time: " << m_totalTime << " s");
  if (m_ascii)
    {
      PacketMetadata::Enable ();
    }
}

void
MeshTest::CreateNodes ()
{ 
  /*
   * Create m_ySize*m_xSize stations to form a grid topology
   */
  //nodes.Create (m_ySize*m_xSize);
//Nodes = rootNodes+parentNodes+childrenNodes;
/////////////////////////////////////////////////////////////////////////// 
  //                                                                       //
  // Construct the rootNodes                                                //
  //                                                                       //
  /////////////////////////////////////////////////////////////////////////// 
   

  rootPeer.Create (rootNodes); 
  // Configure YansWifiChannel
  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  wifiPhy.SetChannel (wifiChannel.Create ());
/*
   * Create mesh helper and set stack installer to it
   * Stack installer creates all needed protocols and install them to
   * mesh point device
   */
  mesh = MeshHelper::Default ();
  if (!Mac48Address (m_root.c_str ()).IsBroadcast ())
    {
      mesh.SetStackInstaller (m_stack, "Root", Mac48AddressValue (Mac48Address (m_root.c_str ())));
    }
  else
    {
      //If root is not set, we do not use "Root" attribute, because it
      //is specified only for 11s
      mesh.SetStackInstaller (m_stack);
    }
  if (m_chan)
    {
      mesh.SetSpreadInterfaceChannels (MeshHelper::SPREAD_CHANNELS);
    }
  else
    {
      mesh.SetSpreadInterfaceChannels (MeshHelper::ZERO_CHANNEL);
    }

  mesh.SetMacType ("RandomStart", TimeValue (Seconds (m_randomStart)));
  // Set number of interfaces - default is single-interface mesh point
  mesh.SetNumberOfInterfaces (m_nIfaces);
  // Install protocols and return container if MeshPointDevices
  rootDevices = mesh.Install (wifiPhy, rootPeer);
  MobilityHelper mobility;
  //MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (20.0),
                                 "MinY", DoubleValue (20.0),
                                 "DeltaX", DoubleValue (20.0),
                                 "DeltaY", DoubleValue (20.0),
                                 "GridWidth", UintegerValue (5),
                                 "LayoutType", StringValue ("RowFirst"));
  mobility.SetMobilityModel ("ns3::RandomDirection2dMobilityModel",
                             "Bounds", RectangleValue (Rectangle (-500, 500, -500, 500)),
                             "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=2]"),
                             "Pause", StringValue ("ns3::ConstantRandomVariable[Constant=0.2]"));
  mobility.Install (rootPeer);


/////////////////////////////////////////////////////////////////////////// 
  //                                                                       //
  // Construct the Parent Peer                                                //
  //                                                                       //
  /////////////////////////////////////////////////////////////////////////// 


for (uint32_t i = 0; i < rootNodes; ++i)
    {
      NS_LOG_INFO ("Configuring local area network for root node " << i);

      //NodeContainer newLanNodes;
      newParentNodes.Create (parentNodes - 1);
      // Now, create the container with all nodes on this link
      //NodeContainer lan (backbone.Get (i), newLanNodes);
        parentPeer = (rootPeer.Get (i), newParentNodes);
      //
      // Create the CSMA net devices and install them into the nodes in our
      // collection.
      //
      CsmaHelper csma;
      csma.SetChannelAttribute ("DataRate",
                                DataRateValue (DataRate (5000000)));
      csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));
      //NetDeviceContainer lanDevices = csma.Install (lan);
        parentDevices = csma.Install (parentPeer);

       MobilityHelper mobilityLan;
      Ptr<ListPositionAllocator> subnetAlloc =
        CreateObject<ListPositionAllocator> ();
      for (uint32_t j = 0; j < newParentNodes.GetN (); ++j)
        {
          subnetAlloc->Add (Vector (0.0, j * 10 + 10, 0.0));
        }
      mobilityLan.PushReferenceMobilityModel (rootPeer.Get (i));
      mobilityLan.SetPositionAllocator (subnetAlloc);
      mobilityLan.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
      mobilityLan.Install (newParentNodes);
    }


////////////////////////////////////////////////////////////////////////// 
  //                                                                       //
  // Construct the Children Nodes                                        //
  //                                                                       //
  /////////////////////////////////////////////////////////////////////////// 


 for (uint32_t i = 0; i < parentNodes; ++i)
    {
      NS_LOG_INFO ("Configuring the children node " << i);

      newChildrenNodes.Create (childrenNodes - 1);
      childrenPeer = (parentPeer.Get (i), newChildrenNodes);
      CsmaHelper csma;
      csma.SetChannelAttribute ("DataRate", 
                                DataRateValue (DataRate (5000000)));
      csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));
      childrenDevices = csma.Install (childrenPeer);
      //
      MobilityHelper mobilityLan;
      Ptr<ListPositionAllocator> subnetAlloc = 
        CreateObject<ListPositionAllocator> ();
      for (uint32_t j = 0; j < newChildrenNodes.GetN (); ++j)
        {
          subnetAlloc->Add (Vector (0.0, j*20 + 20, 0.0));
        }
      mobilityLan.PushReferenceMobilityModel (parentPeer.Get (i));
      mobilityLan.SetPositionAllocator (subnetAlloc);
      mobilityLan.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
      mobilityLan.Install (newChildrenNodes);
    }
}



//if (m_pcap)
  //  wifiPhy.EnablePcapAll (std::string ("mp-"));
  //if (m_ascii)
    //{
      //AsciiTraceHelper ascii;
      //wifiPhy.EnableAsciiAll (ascii.CreateFileStream ("mesh.tr"));
    //}

void
MeshTest::InstallInternetStack ()
{
  InternetStackHelper internetStack;
  internetStack.Install (rootPeer);
  Ipv4AddressHelper address;
  //address.SetBase ("10.1.1.0", "255.255.255.0");
  //interfaces = address.Assign (rootDevices);
  address.SetBase ("192.168.0.0", "255.255.255.0");
  interfaces = address.Assign (rootDevices);

 address.SetBase ("172.16.0.0", "255.255.255.0");
 internetStack.Install (newParentNodes);
 interfaces = address.Assign (parentDevices);
 address.NewNetwork ();

 address.SetBase ("10.1.1.0", "255.255.255.0");
 internetStack.Install (newChildrenNodes);
 interfaces = address.Assign (childrenDevices);
 address.NewNetwork ();
}


void
MeshTest::InstallApplication ()
{

  uint16_t port = 9; 
  NS_ASSERT (parentNodes > 1 && childrenNodes > 1);
  Ptr<Node> appSource = NodeList::GetNode (rootNodes);
  uint32_t lastNodeIndex = childrenNodes + parentNodes + rootNodes-1;
  Ptr<Node> appSink = NodeList::GetNode (lastNodeIndex);
 
  Ipv4Address remoteAddr = appSink->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal ();
//UdpServerHelper Server (port3);
  //ApplicationContainer serverApps = Server.Install (rootPeer.Get (0));
  //serverApps.Start (Seconds (0.0));
  //serverApps.Stop (Seconds (m_totalTime));
  UdpTraceClientHelper video (remoteAddr, port,""); //./home/folake/output_trace.txt
 // video.SetAttribute ("MaxPackets", UintegerValue ((uint32_t)(m_totalTime*(1/m_packetInterval))));
// video.SetAttribute ("Interval", TimeValue (Seconds (m_packetInterval)));
  video.SetAttribute ("MaxPacketSize", UintegerValue(m_packetSize));
  ApplicationContainer clientApps = video.Install (appSource);
//clientApps = video.Install (parentPeer.Get (1));
 clientApps.Start (Seconds (0.0));
  clientApps.Stop (Seconds (m_totalTime));

}


int
MeshTest::Run ()
{
  CreateNodes ();
  InstallInternetStack ();
  InstallApplication ();

  //Gnuplot parameters for End-to-End Delay
      
    std::string fileNameWithNoExtension = "MeshSTS-Throughput1_";
    std::string graphicsFileName        = fileNameWithNoExtension + ".png";
    std::string plotFileName            = fileNameWithNoExtension + ".plt";
    std::string plotTitle               = "Flow vs MeshSTS-Throughput1";
    std::string dataTitle               = "MeshSTS-Throughput1";

    // Instantiate the plot and set its title for throughput.
    Gnuplot gnuplot (graphicsFileName);
    gnuplot.SetTitle (plotTitle);

// Instantiate the plot and set its title for throughput.
    //Gnuplot gnuplot1 (graphicsFileName1);
    //gnuplot1.SetTitle (plotTitle);



    // Make the graphics file, which the plot file will be when it
    // is used with Gnuplot, be a PNG file.
    gnuplot.SetTerminal ("png");



    // Set the labels for each axis.
    gnuplot.SetLegend ("Flow", "MeshSTS-Throughput(Kbps)");

     
   Gnuplot2dDataset dataset;
   dataset.SetTitle (dataTitle);
   dataset.SetStyle (Gnuplot2dDataset::LINES_POINTS);


  //flowMonitor declaration
  FlowMonitorHelper fmHelper;
  Ptr<FlowMonitor> allMon = fmHelper.InstallAll();
  // call the flow monitor function
  ThroughputMonitor(&fmHelper, allMon, dataset); 

  Simulator::Schedule (Seconds (m_totalTime), &MeshTest::Report, this);
  Simulator::Stop (Seconds (m_totalTime));
  Simulator::Run ();

  //Gnuplot ...continued
  gnuplot.AddDataset (dataset);
  // Open the plot file.
  std::ofstream plotFile (plotFileName.c_str());
  // Write the plot file.
  gnuplot.GenerateOutput (plotFile);
  // Close the plot file.
  plotFile.close ();

  Simulator::Destroy ();
  return 0;
}


void
MeshTest::Report ()
{
  /*unsigned n (0);
  for (NetDeviceContainer::Iterator i = meshDevices.Begin (); i != meshDevices.End (); ++i, ++n)
    {
      std::ostringstream os;
      os << "mp-report-" << n << ".xml";
      std::cerr << "Printing mesh point device #" << n << " diagnostics to " << os.str () << "\n";
      std::ofstream of;
      of.open (os.str ().c_str ());
      if (!of.is_open ())
        {
          std::cerr << "Error: Can't open file " << os.str () << "\n";
          return;
        }
      mesh.Report (*i, of);
      of.close ();
    }*/
}


int
main (int argc, char *argv[])
{
  MeshTest t; 
  t.Configure (argc, argv);
  return t.Run ();
}





void ThroughputMonitor (FlowMonitorHelper *fmhelper, Ptr<FlowMonitor> flowMon,Gnuplot2dDataset DataSet)
	{
    double localThrou=0;
//double meanDelay=0;
  //  double meanJitter=0;
  //double packetLoss=0;
      //double startDelay=0;//startup-delay

		std::map<FlowId, FlowMonitor::FlowStats> flowStats = flowMon->GetFlowStats();
		Ptr<Ipv4FlowClassifier> classing = DynamicCast<Ipv4FlowClassifier> (fmhelper->GetClassifier());
		for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator stats = flowStats.begin (); stats != flowStats.end (); ++stats)
		{
			Ipv4FlowClassifier::FiveTuple fiveTuple = classing->FindFlow (stats->first);
			std::cout<<"Flow ID			: " << stats->first <<" ; "<< fiveTuple.sourceAddress <<" -----> "<<fiveTuple.destinationAddress<<std::endl;
			std::cout<<"Tx Packets = " << stats->second.txPackets<<std::endl;
			std::cout<<"Rx Packets = " << stats->second.rxPackets<<std::endl;
                        std::cout<<"delaySum= "<< stats->second.delaySum <<std::endl;
                        std::cout<<"jitterSum= "<< stats->second.jitterSum <<std::endl;
                        std::cout<<"lostPackets = "<< stats->second.lostPackets<<std::endl;
            std::cout<<"Duration		: "<<(stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstTxPacket.GetSeconds())<<std::endl;
			std::cout<<"Last Received Packet	: "<< stats->second.timeLastRxPacket.GetSeconds()<<" Seconds"<<std::endl;
//std::cout<<"First Received Packet	: "<< stats->second.timeFirstRxPacket.GetSeconds()<<" Seconds"<<std::endl;
			//std::cout<<"Throughput: " << stats->second.rxBytes * 8.0 / (stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstTxPacket.GetSeconds())/1024  << " Kbps"<<std::endl;

               // std::cout<<"meanDelay: " <<(stats->second.delaySum.GetSeconds()) / stats->second.rxPackets  << " s"<<std::endl;
               // std::cout<<"meanJitter: " << (stats->second.jitterSum.GetSeconds())/ ((stats->second.rxPackets)-1) << " s"<<std::endl;
              // std::cout<< "packetLoss:" << stats->second.lostPackets /(stats->second.rxPackets + stats->second.lostPackets) << " %"<<std::endl;
         localThrou=(stats->second.rxBytes * 8.0 / (stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstTxPacket.GetSeconds())/1024/1024);
//meanDelay=((stats->second.delaySum.GetSeconds()) / stats->second.rxPackets );
 //meanJitter=((stats->second.jitterSum.GetSeconds())/ ((stats->second.rxPackets)-1));
      //packetLoss= (stats->second.lostPackets /(stats->second.rxPackets) + (stats->second.lostPackets))/100;
   // startDelay =  (stats->second.timeFirstTxPacket.GetSeconds())/ (stats->second.delaySum.GetSeconds()) ;
			// updata gnuplot data
            DataSet.Add((double)Simulator::Now().GetSeconds(),(double) localThrou);
           
			std::cout<<"---------------------------------------------------------------------------"<<std::endl;
		}
			Simulator::Schedule(Seconds(1),&ThroughputMonitor, fmhelper, flowMon,DataSet);
   //if(flowToXml)
      {
	flowMon->SerializeToXmlFile ("MeshSTS1.xml", true, true);
      }

	}


