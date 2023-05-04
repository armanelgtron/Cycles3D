/***************************************************************************
                            udppredictionlist.cpp  -  description
                             -------------------

  Contains functionality for the UDP prediction list (See the header
  for details)

    begin                : Sun Oct  27 1:54:52 EDT 2002
    copyright            : (C) 2002 by Chris Haag
    email                : dragonslayer_developer@hotmail.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/*								FLOW OF CONTROL


  * Whenever you are not connected, the list must be empty
  * The server is NOT to be included in the list


	The list is reset:
		Upon construction
		Upon ::Disconnect


	One entry is added to the list:
		If you are the server and a client just joined you

	One entry is removed from the list:
		If you are the server and a client disconnected

	List is synchronized
		If you got a player list packet


  TODO: Error checking
*/

#include "udppredictionlist.h"

#include "../cycles3d.h"
#include "netbase.h"

#include <memory.h>

CUDPPredictionList::CUDPPredictionList()
{
	Reset(0);
}

CUDPPredictionList::~CUDPPredictionList()
{
}

unsigned long CUDPPredictionList::IndexFromIP(unsigned long IP)
{
	for (unsigned long i=0; i < m_nIPs; i++)
	{
		if (m_adwIPs[i] == IP)
			return i;
	}
	return 0xFFFFFFFF;
}

void CUDPPredictionList::Reset(unsigned long dwMyIP)
{
	memset(m_adwIPs, 0, sizeof(m_adwIPs));

	for (int i=0; i < 32; i++)
	{

		m_adwIDsIN[i] = 1;
		m_adwIDsOUT[i] = 1;
	}
	m_nIPs = 0;
	m_dwMyIP = dwMyIP;
	if (m_dwMyIP) Add(m_dwMyIP);
}

void CUDPPredictionList::Reset()
{
	Reset(m_dwMyIP);
}

void CUDPPredictionList::Synchronize(unsigned long* pdwIPs, unsigned long nIPs)
{
	unsigned long adwNewIDsIN[32];
	unsigned long adwNewIDsOUT[32];

	for (unsigned long i=0; i < nIPs; i++)
	{
		unsigned long dwIndex = IndexFromIP(pdwIPs[i]);
		if (dwIndex != 0xFFFFFFFF)
		{
			adwNewIDsIN[i] = m_adwIDsIN[dwIndex];
			adwNewIDsOUT[i] = m_adwIDsOUT[dwIndex];
		}
		else {
			adwNewIDsIN[i] = 1;
			adwNewIDsOUT[i] = 1;
		}
	}

	memcpy(m_adwIPs, pdwIPs, sizeof(unsigned long) * nIPs);
	memcpy(m_adwIDsIN, adwNewIDsIN, sizeof(unsigned long) * nIPs);
	memcpy(m_adwIDsOUT, adwNewIDsOUT, sizeof(unsigned long) * nIPs);
	m_nIPs = nIPs;
}

void CUDPPredictionList::Add(unsigned long IP)
{
	m_adwIPs[m_nIPs] = IP;
	m_adwIDsIN[m_nIPs] = 1;
	m_adwIDsOUT[m_nIPs++] = 1;
}

void CUDPPredictionList::Remove(unsigned long IP)
{
	unsigned long dwIndex = IndexFromIP(IP);
	if (dwIndex != 0xFFFFFFFF)
	{
		m_adwIPs[dwIndex] = m_adwIPs[m_nIPs-1];
		m_adwIDsIN[dwIndex] = m_adwIDsIN[m_nIPs-1];
		m_adwIDsOUT[dwIndex] = m_adwIDsOUT[m_nIPs-1];
		m_nIPs--;
	}
}

void CUDPPredictionList::PingAll()
{
	extern CPingClient g_PingClient;

	for (unsigned long i=0; i < m_nIPs; i++)
	{
		g_PingClient.Ping(m_adwIPs[i], UNIVERSAL_PING_DATA);
	}
}

unsigned char CUDPPredictionList::Validate(unsigned long dwTimeStamp, unsigned long dwIP)
{
	unsigned long dwIndex;
	if (dwTimeStamp == 0) return 1; // Zero is a sentinel value
	if ((dwIndex = IndexFromIP(dwIP)) == 0xFFFFFFFF) return 1;

	/* If the timestamp is less than the current one in the array,
	it's invalid */
	if (dwTimeStamp < m_adwIDsIN[dwIndex]) return 0;

	/* If the timestamp is greater or equal (it should never be
	greater but that doesn't mean it never will be), move to
	the next one. */
	m_adwIDsIN[dwIndex] = dwTimeStamp + 1;

	/* Zero is a reserved sentinel value */
	if (m_adwIDsIN[dwIndex] == 0) m_adwIDsIN[dwIndex] = 1;

	return 1;
}

unsigned long CUDPPredictionList::GetTimestamp(unsigned long dwIP, unsigned char bIncrementAfter /* = 0 (false)*/)
{
	unsigned long dwIndex, dwRetval;
	if ((dwIndex = IndexFromIP(dwIP)) == 0xFFFFFFFF) return 0xFFFFFFFF;

	dwRetval = m_adwIDsOUT[dwIndex];

	if (bIncrementAfter) 
	{
		if (++m_adwIDsOUT[dwIndex] == 0) m_adwIDsOUT[dwIndex] = 1;
	}
		
	return dwRetval;
}
