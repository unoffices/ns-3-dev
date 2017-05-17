/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 Danilo Abrignani
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
 * Author: Danilo Abrignani <danilo.abrignani@unibo.it> (Carrier Aggregation - GSoC 2015)
 */


#include "cc-helper.h"
#include <ns3/component-carrier.h>
#include <ns3/string.h>
#include <ns3/log.h>
#include <ns3/abort.h>
#include <ns3/pointer.h>
#include <iostream>
#include <ns3/uinteger.h>

#define MIN_CC 1
#define MAX_CC 2
namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("CcHelper");

NS_OBJECT_ENSURE_REGISTERED (CcHelper);

CcHelper::CcHelper (void)
{
  NS_LOG_FUNCTION (this);
  m_ccFactory.SetTypeId (ComponentCarrier::GetTypeId ());
}

void
CcHelper::DoInitialize (void)
{
  NS_LOG_FUNCTION (this);
}

TypeId CcHelper::GetTypeId (void)
{
  static TypeId
    tid =
    TypeId ("ns3::CcHelper")
    .SetParent<Object> ()
    .AddConstructor<CcHelper> ()
    .AddAttribute ("NumberOfComponentCarriers",
                   "Set the number of Component Carriers to setup per eNodeB"
                   "Currently the maximum Number of Component Carriers allowed is 2",
                   UintegerValue (1),
                   MakeUintegerAccessor (&CcHelper::m_numberOfComponentCarriers),
                   MakeUintegerChecker<uint16_t> (MIN_CC, MAX_CC))
    .AddAttribute ("UlEarfcn",
                   "Set Ul Channel [EARFCN] for the first carrier component",
                   UintegerValue (0),
                   MakeUintegerAccessor (&CcHelper::m_ulEarfcn),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("DlEarfcn",
                   "Set Dl Channel [EARFCN] for the first carrier component",
                   UintegerValue (0),
                   MakeUintegerAccessor (&CcHelper::m_dlEarfcn),
                   MakeUintegerChecker<uint32_t> (0))
    .AddAttribute ("DlBandwidth",
                   "Set Dl Bandwidth for the first carrier component",
                   UintegerValue (25),
                   MakeUintegerAccessor (&CcHelper::m_dlBandwidth),
                   MakeUintegerChecker<uint16_t> (0,100))
    .AddAttribute ("UlBandwidth",
                   "Set Dl Bandwidth for the first carrier component",
                   UintegerValue (25),
                   MakeUintegerAccessor (&CcHelper::m_ulBandwidth),
                   MakeUintegerChecker<uint16_t> (0,100))
  ;

  return tid;
}

CcHelper::~CcHelper (void)
{
  NS_LOG_FUNCTION (this);
}

void
CcHelper::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  Object::DoDispose ();
}

void 
CcHelper::SetCcAttribute (std::string n, const AttributeValue &v)
{
  NS_LOG_FUNCTION (this << n);
  m_ccFactory.Set (n, v);
}

void
CcHelper:: SetNumberOfComponentCarriers (uint16_t nCc)
{
  m_numberOfComponentCarriers = nCc;
}

void
CcHelper::SetUlEarfcn (uint32_t ulEarfcn)
{
  m_ulEarfcn = ulEarfcn;
}

void
CcHelper::SetDlEarfcn (uint32_t dlEarfcn)
{
  m_dlEarfcn = dlEarfcn;
}

void
CcHelper::SetDlBandwidth (uint16_t dlBandwidth)
{
  m_dlBandwidth = dlBandwidth;
}

void
CcHelper::SetUlBandwidth (uint16_t ulBandwidth)
{
  m_ulBandwidth = ulBandwidth;
}

uint16_t
CcHelper::GetNumberOfComponentCarriers ()
{
  return m_numberOfComponentCarriers;
}

uint32_t CcHelper::GetUlEarfcn ()
{
  return m_ulEarfcn;
}

uint32_t CcHelper::GetDlEarfcn ()
{
  return m_dlEarfcn;
}

uint16_t CcHelper::GetDlBandwidth ()
{
  return m_dlBandwidth;
}

uint16_t CcHelper::GetUlBandwidth ()
{
  return m_ulBandwidth;
}

ComponentCarrier
CcHelper::DoCreateSingleCc (uint16_t ulBandwidth, uint16_t dlBandwidth, uint32_t ulEarfcn, uint32_t dlEarfcn, bool isPrimary)
{
  return CreateSingleCc (ulBandwidth, dlBandwidth, ulEarfcn, dlEarfcn, isPrimary);
}

std::map< uint8_t, ComponentCarrier >
CcHelper::EquallySpacedCcs ()
{
  std::map< uint8_t, ComponentCarrier > ccmap;

  for (uint8_t i = 0; i < m_numberOfComponentCarriers; i++)
    {
      bool pc =false;

      // One RB is 200 kHz wide, while increment by 1 in corresponds to 100 kHz shift
      // Therefore, we need to multiply by 2 here.
      uint32_t ul = m_ulEarfcn + i * 2 * m_ulBandwidth;
      uint32_t dl = m_dlEarfcn + i * 2 * m_dlBandwidth;
      if (i == 0)
        {
          pc = true;
        }
      ComponentCarrier cc = CreateSingleCc (m_ulBandwidth, m_dlBandwidth, ul, dl, pc);
      ccmap.insert (std::pair<uint8_t, ComponentCarrier >(i, cc));

      NS_LOG_INFO(" ulBandwidth:"<<m_ulBandwidth<<" , dlBandwidth: "<<m_dlBandwidth<<" , ul:"<<ul<<" , dl:"<<dl);
    }

  return ccmap;
}
ComponentCarrier
CcHelper::CreateSingleCc (uint16_t ulBandwidth, uint16_t dlBandwidth, uint32_t ulEarfcn, uint32_t dlEarfcn, bool isPrimary)
{
  ComponentCarrier cc;
  if ( m_ulEarfcn != 0)
    {
      cc.SetUlEarfcn (ulEarfcn);
    }
  else 
    {
      uint16_t ul = cc.GetUlEarfcn () + ulEarfcn;
      cc.SetUlEarfcn (ul);
    }
  if ( m_dlEarfcn != 0)
    {
      cc.SetDlEarfcn (dlEarfcn);
    }
  else 
    {
      uint16_t dl = cc.GetDlEarfcn () + dlEarfcn;
      cc.SetDlEarfcn (dl);
    }
  cc.SetDlBandwidth (dlBandwidth);
  cc.SetUlBandwidth (ulBandwidth);

  cc.SetAsPrimary (isPrimary);

  return cc;


}

} // namespace ns3
