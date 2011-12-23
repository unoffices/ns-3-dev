/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007,2008,2009 INRIA, UDCAST
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
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
 * The original version of UdpClient is by  Amine Ismail
 * <amine.ismail@sophia.inria.fr> <amine.ismail@udcast.com> 
 * The rest of the code (including modifying UdpClient into
 *  LteRadioBearerTagUdpClient) is by Nicola Baldo <nbaldo@cttc.es> 
 */



#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/test.h"
#include "ns3/epc-helper.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/udp-client-server-helper.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/csma-helper.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/inet-socket-address.h"
#include "ns3/packet-sink.h"
#include <ns3/ipv4-static-routing-helper.h>
#include <ns3/ipv4-static-routing.h>
#include <ns3/ipv4-interface.h>
#include <ns3/mac48-address.h>
#include "ns3/seq-ts-header.h"
#include "ns3/lte-radio-bearer-tag.h"
#include "ns3/arp-cache.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"


namespace ns3 {



NS_LOG_COMPONENT_DEFINE ("EpcTestS1uUplink");

/*
 * A Udp client. Sends UDP packet carrying sequence number and time
 * stamp but also including the LteRadioBearerTag. This tag is normally
 * generated by the LteEnbNetDevice when forwarding packet in the
 * uplink. But in this test we don't have the LteEnbNetDevice, because
 * we test the S1-U interface with simpler devices to make sure it
 * just works.
 * 
 */
class LteRadioBearerTagUdpClient : public Application
{
public:
  static TypeId
  GetTypeId (void);

  LteRadioBearerTagUdpClient ();
  LteRadioBearerTagUdpClient (uint16_t rnti, uint8_t lcid);

  virtual ~LteRadioBearerTagUdpClient ();

  /**
   * \brief set the remote address and port
   * \param ip remote IP address
   * \param port remote port
   */
  void SetRemote (Ipv4Address ip, uint16_t port);

protected:
  virtual void DoDispose (void);

private:

  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void ScheduleTransmit (Time dt);
  void Send (void);

  uint32_t m_count;
  Time m_interval;
  uint32_t m_size;

  uint32_t m_sent;
  Ptr<Socket> m_socket;
  Ipv4Address m_peerAddress;
  uint16_t m_peerPort;
  EventId m_sendEvent;

  uint16_t m_rnti;
  uint8_t m_lcid;

};



TypeId
LteRadioBearerTagUdpClient::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LteRadioBearerTagUdpClient")
    .SetParent<Application> ()
    .AddConstructor<LteRadioBearerTagUdpClient> ()
    .AddAttribute ("MaxPackets",
                   "The maximum number of packets the application will send",
                   UintegerValue (100),
                   MakeUintegerAccessor (&LteRadioBearerTagUdpClient::m_count),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Interval",
                   "The time to wait between packets", TimeValue (Seconds (1.0)),
                   MakeTimeAccessor (&LteRadioBearerTagUdpClient::m_interval),
                   MakeTimeChecker ())
    .AddAttribute (
      "RemoteAddress",
      "The destination Ipv4Address of the outbound packets",
      Ipv4AddressValue (),
      MakeIpv4AddressAccessor (&LteRadioBearerTagUdpClient::m_peerAddress),
      MakeIpv4AddressChecker ())
    .AddAttribute ("RemotePort", "The destination port of the outbound packets",
                   UintegerValue (100),
                   MakeUintegerAccessor (&LteRadioBearerTagUdpClient::m_peerPort),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("PacketSize",
                   "Size of packets generated. The minimum packet size is 12 bytes which is the size of the header carrying the sequence number and the time stamp.",
                   UintegerValue (1024),
                   MakeUintegerAccessor (&LteRadioBearerTagUdpClient::m_size),
                   MakeUintegerChecker<uint32_t> (12,1500))
  ;
  return tid;
}

LteRadioBearerTagUdpClient::LteRadioBearerTagUdpClient ()
  : m_rnti (0),
    m_lcid (0)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_sent = 0;
  m_socket = 0;
  m_sendEvent = EventId ();
}

LteRadioBearerTagUdpClient::LteRadioBearerTagUdpClient (uint16_t rnti, uint8_t lcid)
  : m_rnti (rnti),
    m_lcid (lcid)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_sent = 0;
  m_socket = 0;
  m_sendEvent = EventId ();
}

LteRadioBearerTagUdpClient::~LteRadioBearerTagUdpClient ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

void
LteRadioBearerTagUdpClient::SetRemote (Ipv4Address ip, uint16_t port)
{
  m_peerAddress = ip;
  m_peerPort = port;
}

void
LteRadioBearerTagUdpClient::DoDispose (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  Application::DoDispose ();
}

void
LteRadioBearerTagUdpClient::StartApplication (void)
{
  NS_LOG_FUNCTION_NOARGS ();

  if (m_socket == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid);
      m_socket->Bind ();
      m_socket->Connect (InetSocketAddress (m_peerAddress, m_peerPort));
    }

  m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
  m_sendEvent = Simulator::Schedule (Seconds (0.0), &LteRadioBearerTagUdpClient::Send, this);
}

void
LteRadioBearerTagUdpClient::StopApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();
  Simulator::Cancel (m_sendEvent);
}

void
LteRadioBearerTagUdpClient::Send (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_ASSERT (m_sendEvent.IsExpired ());
  SeqTsHeader seqTs;
  seqTs.SetSeq (m_sent);
  Ptr<Packet> p = Create<Packet> (m_size-(8+4)); // 8+4 : the size of the seqTs header
  p->AddHeader (seqTs);

  LteRadioBearerTag tag (m_rnti, m_lcid);
  p->AddPacketTag (tag);

  if ((m_socket->Send (p)) >= 0)
    {
      ++m_sent;
      NS_LOG_INFO ("TraceDelay TX " << m_size << " bytes to "
                                    << m_peerAddress << " Uid: " << p->GetUid ()
                                    << " Time: " << (Simulator::Now ()).GetSeconds ());

    }
  else
    {
      NS_LOG_INFO ("Error while sending " << m_size << " bytes to "
                                          << m_peerAddress);
    }

  if (m_sent < m_count)
    {
      m_sendEvent = Simulator::Schedule (m_interval, &LteRadioBearerTagUdpClient::Send, this);
    }
}



struct UeUlTestData
{
  UeUlTestData (uint32_t n, uint32_t s, uint16_t r, uint8_t l);

  uint32_t numPkts;
  uint32_t pktSize;
  uint16_t rnti;
  uint8_t lcid;
 
  Ptr<PacketSink> serverApp;
  Ptr<Application> clientApp;
};

  UeUlTestData::UeUlTestData (uint32_t n, uint32_t s, uint16_t r, uint8_t l)
  : numPkts (n),
    pktSize (s),
    rnti (r),
    lcid (l)
{
}

struct EnbUlTestData
{
  std::vector<UeUlTestData> ues;
};


class EpcS1uUlTestCase : public TestCase
{
public:
  EpcS1uUlTestCase (std::string name, std::vector<EnbUlTestData> v);
  virtual ~EpcS1uUlTestCase ();

private:
  virtual void DoRun (void);
  std::vector<EnbUlTestData> m_enbUlTestData;
};


EpcS1uUlTestCase::EpcS1uUlTestCase (std::string name, std::vector<EnbUlTestData> v)
  : TestCase (name),
    m_enbUlTestData (v)
{
}

EpcS1uUlTestCase::~EpcS1uUlTestCase ()
{
}

void 
EpcS1uUlTestCase::DoRun ()
{
  Ptr<EpcHelper> epcHelper = CreateObject<EpcHelper> ();
  Ptr<Node> pgw = epcHelper->GetPgwNode ();
  
  // Create a single RemoteHost
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  InternetStackHelper internet;
  internet.Install (remoteHostContainer);

  // Create the internet
  PointToPointHelper p2ph;
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);  
  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetNodesIpIfaceContainer = ipv4h.Assign (internetDevices);
  
  // setup default gateway for the remote hosts
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());

  // hardcoded UE addresses for now
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.255.255.0"), 1);
  


  uint16_t udpSinkPort = 1234;
    
  NodeContainer enbs;

  for (std::vector<EnbUlTestData>::iterator enbit = m_enbUlTestData.begin ();
       enbit < m_enbUlTestData.end ();
       ++enbit)
    {
      Ptr<Node> enb = CreateObject<Node> ();
      enbs.Add (enb);

      // we test EPC without LTE, hence we use:
      // 1) a CSMA network to simulate the cell
      // 2) a raw socket opened on the CSMA device to simulate the LTE socket

      NodeContainer ues;
      ues.Create (enbit->ues.size ());

      NodeContainer cell;
      cell.Add (ues);
      cell.Add (enb);

      CsmaHelper csmaCell;      
      NetDeviceContainer cellDevices = csmaCell.Install (cell);

      // the eNB's CSMA NetDevice acting as an LTE NetDevice. 
      Ptr<NetDevice> lteEnbNetDevice = cellDevices.Get (cellDevices.GetN () - 1);

      // Note that the EpcEnbApplication won't care of the actual NetDevice type
      epcHelper->AddEnb (enb, lteEnbNetDevice);      
      
      // we install the IP stack on UEs only
      InternetStackHelper internet;
      internet.Install (ues);

      // assign IP address to UEs, and install applications
      for (uint32_t u = 0; u < ues.GetN (); ++u)
        {
          Ptr<NetDevice> ueLteDevice = cellDevices.Get (u);
          Ipv4InterfaceContainer ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueLteDevice));

          Ptr<Node> ue = ues.Get (u);

          // disable IP Forwarding on the UE. This is because we use
          // CSMA broadcast MAC addresses for this test. The problem
          // won't happen with a LteUeNetDevice. 
          Ptr<Ipv4> ueIpv4 = ue->GetObject<Ipv4> ();
          ueIpv4->SetAttribute ("IpForward", BooleanValue (false));

          // tell the UE to route all packets to the GW
          Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueIpv4);
          Ipv4Address gwAddr = epcHelper->GetUeDefaultGatewayAddress ();
          NS_LOG_INFO ("GW address: " << gwAddr);
          ueStaticRouting->SetDefaultRoute (gwAddr, 1);

          // since the UEs in this test use CSMA with IP enabled, and
          // the eNB uses CSMA but without IP, we fool the UE's ARP
          // cache into thinking that the IP address of the GW can be
          // reached by sending a CSMA packet to the broadcast
          // address, so the eNB will get it.       
          int32_t ueLteIpv4IfIndex = ueIpv4->GetInterfaceForDevice (ueLteDevice);
          Ptr<Ipv4L3Protocol> ueIpv4L3Protocol = ue->GetObject<Ipv4L3Protocol> ();
          Ptr<Ipv4Interface> ueLteIpv4Iface = ueIpv4L3Protocol->GetInterface (ueLteIpv4IfIndex);
          Ptr<ArpCache> ueArpCache = ueLteIpv4Iface->GetArpCache (); 
          ueArpCache->SetAliveTimeout (Seconds (1000));          
          ArpCache::Entry* arpCacheEntry = ueArpCache->Add (gwAddr);
          arpCacheEntry->MarkWaitReply(0);
          arpCacheEntry->MarkAlive (Mac48Address::GetBroadcast ()); 
  
          
          PacketSinkHelper packetSinkHelper ("ns3::UdpSocketFactory", 
                                             InetSocketAddress (Ipv4Address::GetAny (), udpSinkPort));          
          ApplicationContainer sinkApp = packetSinkHelper.Install (remoteHost);
          sinkApp.Start (Seconds (1.0));
          sinkApp.Stop (Seconds (10.0));
          enbit->ues[u].serverApp = sinkApp.Get (0)->GetObject<PacketSink> ();
          
          Time interPacketInterval = Seconds (0.01);
          Ptr<LteRadioBearerTagUdpClient> client = CreateObject<LteRadioBearerTagUdpClient> (enbit->ues[u].rnti, enbit->ues[u].lcid);
          client->SetAttribute ("RemoteAddress", Ipv4AddressValue (internetNodesIpIfaceContainer.GetAddress (1)));
          client->SetAttribute ("RemotePort", UintegerValue (udpSinkPort));          
          client->SetAttribute ("MaxPackets", UintegerValue (enbit->ues[u].numPkts));
          client->SetAttribute ("Interval", TimeValue (interPacketInterval));
          client->SetAttribute ("PacketSize", UintegerValue (enbit->ues[u].pktSize));
          ue->AddApplication (client);
          ApplicationContainer clientApp;
          clientApp.Add (client);
          clientApp.Start (Seconds (2.0));
          clientApp.Stop (Seconds (10.0));   
          enbit->ues[u].clientApp = client;

          uint16_t rnti = u+1;
          uint16_t lcid = 1;
          epcHelper->ActivateEpsBearer (ueLteDevice, lteEnbNetDevice, EpcTft::Default (), rnti, lcid);
          
          // need this since all sinks are installed in the same node
          ++udpSinkPort; 
        } 
            
    } 
  
  Simulator::Run ();

  for (std::vector<EnbUlTestData>::iterator enbit = m_enbUlTestData.begin ();
       enbit < m_enbUlTestData.end ();
       ++enbit)
    {
      for (std::vector<UeUlTestData>::iterator ueit = enbit->ues.begin ();
           ueit < enbit->ues.end ();
           ++ueit)
        {
          NS_TEST_ASSERT_MSG_EQ (ueit->serverApp->GetTotalRx (), (ueit->numPkts) * (ueit->pktSize), "wrong total received bytes");
        }      
    }
  
  Simulator::Destroy ();
}





/**
 * Test that the S1-U interface implementation works correctly
 */
class EpcS1uUlTestSuite : public TestSuite
{
public:
  EpcS1uUlTestSuite ();
  
} g_epcS1uUlTestSuiteInstance;

EpcS1uUlTestSuite::EpcS1uUlTestSuite ()
  : TestSuite ("epc-s1u-uplink", SYSTEM)
{  
  std::vector<EnbUlTestData> v1;  
  EnbUlTestData e1;
  UeUlTestData f1 (1, 100, 1, 1);
  e1.ues.push_back (f1);
  v1.push_back (e1);
  AddTestCase (new EpcS1uUlTestCase ("1 eNB, 1UE", v1));


  std::vector<EnbUlTestData> v2;  
  EnbUlTestData e2;
  UeUlTestData f2_1 (1, 100, 1, 1);
  e2.ues.push_back (f2_1);
  UeUlTestData f2_2 (2, 200, 2, 1);
  e2.ues.push_back (f2_2);
  v2.push_back (e2);
  AddTestCase (new EpcS1uUlTestCase ("1 eNB, 2UEs", v2));


  std::vector<EnbUlTestData> v3;  
  v3.push_back (e1);
  v3.push_back (e2);
  AddTestCase (new EpcS1uUlTestCase ("2 eNBs", v3));


  EnbUlTestData e3;
  UeUlTestData f3_1 (3, 50, 1, 1);
  e3.ues.push_back (f3_1);
  UeUlTestData f3_2 (5, 1472, 2, 1);
  e3.ues.push_back (f3_2);
  UeUlTestData f3_3 (1, 1, 3, 1);
  e3.ues.push_back (f3_2);
  std::vector<EnbUlTestData> v4;  
  v4.push_back (e3);
  v4.push_back (e1);
  v4.push_back (e2);
  AddTestCase (new EpcS1uUlTestCase ("3 eNBs", v4));

}



}  // namespace ns3

