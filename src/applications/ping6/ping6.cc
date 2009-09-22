/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007-2009 Strasbourg University
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
 * Author: Sebastien Vincent <vincent@clarinet.u-strasbg.fr>
 */

#include "ns3/log.h"
#include "ns3/nstime.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/socket.h"
#include "ns3/uinteger.h"
#include "ns3/ipv6.h"
#include "ns3/ipv6-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/icmpv6-header.h"
#include "ns3/ipv6-raw-socket-factory.h"
#include "ns3/ipv6-header.h"

#include "ping6.h"

namespace ns3 
{

NS_LOG_COMPONENT_DEFINE ("Ping6Application");

NS_OBJECT_ENSURE_REGISTERED (Ping6);

TypeId Ping6::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::Ping6")
    .SetParent<Application>()
    .AddConstructor<Ping6>()
    .AddAttribute ("MaxPackets", 
        "The maximum number of packets the application will send",
        UintegerValue (100),
        MakeUintegerAccessor (&Ping6::m_count),
        MakeUintegerChecker<uint32_t>())
    .AddAttribute ("Interval", 
        "The time to wait between packets",
        TimeValue (Seconds (1.0)),
        MakeTimeAccessor (&Ping6::m_interval),
        MakeTimeChecker ())
    .AddAttribute ("RemoteIpv6", 
        "The Ipv6Address of the outbound packets",
        Ipv6AddressValue (),
        MakeIpv6AddressAccessor (&Ping6::m_peerAddress),
        MakeIpv6AddressChecker ())
    .AddAttribute ("LocalIpv6", 
        "Local Ipv6Address of the sender",
        Ipv6AddressValue (),
        MakeIpv6AddressAccessor (&Ping6::m_localAddress),
        MakeIpv6AddressChecker ())
    .AddAttribute ("PacketSize", 
        "Size of packets generated",
        UintegerValue (100),
        MakeUintegerAccessor (&Ping6::m_size),
        MakeUintegerChecker<uint32_t>())
    ;
  return tid;
}

Ping6::Ping6 ()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_sent = 0;
  m_socket = 0;
  m_seq = 0;
  m_sendEvent = EventId ();
}

Ping6::~Ping6 ()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_socket = 0;
}

void Ping6::DoDispose ()
{
  NS_LOG_FUNCTION_NOARGS ();
  Application::DoDispose ();
}

void Ping6::StartApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();

  if (!m_socket)
  {
    TypeId tid = TypeId::LookupByName ("ns3::Ipv6RawSocketFactory");
    m_socket = Socket::CreateSocket (GetNode (), tid);

    NS_ASSERT (m_socket);

    m_socket->Bind (Inet6SocketAddress (m_localAddress, 0));
    m_socket->Connect (Inet6SocketAddress (m_peerAddress, 0));
    m_socket->SetAttribute ("Protocol", UintegerValue (58)); /* ICMPv6 */
    m_socket->SetRecvCallback (MakeCallback (&Ping6::HandleRead, this));
  }

  ScheduleTransmit (Seconds (0.));
}

void Ping6::SetLocal (Ipv6Address ipv6) 
{
  NS_LOG_FUNCTION (this << ipv6);
  m_localAddress = ipv6;
}

void Ping6::SetRemote (Ipv6Address ipv6)
{
  NS_LOG_FUNCTION (this << ipv6);
  m_peerAddress = ipv6;
}

void Ping6::StopApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();

  if (m_socket)
  {
    m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> >());
  }

  Simulator::Cancel (m_sendEvent);
}

void Ping6::SetIfIndex (uint32_t ifIndex)
{
  m_ifIndex = ifIndex;
}

void Ping6::ScheduleTransmit (Time dt)
{
  NS_LOG_FUNCTION (this << dt);
  m_sendEvent = Simulator::Schedule (dt, &Ping6::Send, this);
}

void Ping6::Send ()
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_ASSERT (m_sendEvent.IsExpired ());
  Ptr<Packet> p = 0;
  uint8_t data[4];
  Ipv6Address src;
  Ptr<Ipv6> ipv6 = GetNode ()->GetObject<Ipv6> ();

  if (m_ifIndex > 0)
  {
    /* hack to have ifIndex in Ipv6RawSocketImpl
     * maybe add a SetIfIndex in Ipv6RawSocketImpl directly 
     */
    src = GetNode ()->GetObject<Ipv6> ()->GetAddress (m_ifIndex, 0).GetAddress ();
  }
  else
  {
    src = m_localAddress;
  }

  data[0] = 0xDE;
  data[1] = 0xAD;
  data[2] = 0xBE;
  data[3] = 0xEF;

  p = Create<Packet>(data, sizeof (data));
  Icmpv6Echo req (1);

  req.SetId (0xBEEF);
  req.SetSeq (m_seq);
  m_seq++;
  
  /* we do not calculate pseudo header checksum here, because we are not sure about 
   * source IPv6 address. Checksum is calculated in Ipv6RawSocketImpl.
   */

  p->AddHeader (req);
  m_socket->Bind (Inet6SocketAddress (src, 0));
  m_socket->Send (p, 0);
  ++m_sent;

  NS_LOG_INFO ("Sent " << p->GetSize () << " bytes to " << m_peerAddress);

  if (m_sent < m_count)
  {
    ScheduleTransmit (m_interval);
  }
}

void Ping6::HandleRead (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  Ptr<Packet> packet=0;
  Address from;
  while (packet = socket->RecvFrom (from))
  {
    if (Inet6SocketAddress::IsMatchingType (from))
    {
      Ipv6Header hdr;
      Icmpv6Echo reply (0);
      Inet6SocketAddress address = Inet6SocketAddress::ConvertFrom (from);

      packet->RemoveHeader (hdr);

      switch (*packet->PeekData ())
      {
        case Icmpv6Header::ICMPV6_ECHO_REPLY:
          packet->RemoveHeader (reply);

          NS_LOG_INFO ("Received Echo Reply size  = " << std::dec << packet->GetSize () << " bytes from " << address.GetIpv6 () << " id =  " << (uint16_t)reply.GetId () << " seq = " << (uint16_t)reply.GetSeq ());
          break;
        default:
          /* other type, discard */
          break;
      }
    }
  }
}

} /* namespace ns3 */

