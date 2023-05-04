/***************************************************************************
                            udppredictionlist.h  -  description
                             -------------------

  Header file for dedicated server functionality.

    begin                : Sun Oct  27 1:40:49 EDT 2002
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

/*

  UDP Prediction is the act of sending a UDP packet with
  data content "X" at approximately the same time a TCP packet
  with the same data content "X" is sent, and the receiver
  processes whichever of the two comes first, and ignores
  the packet that comes in second. This has worked effectively
  in two network games I wrote.

*/

#ifndef __UDPPREDICTIONLIST_H__
#define __UDPPREDICTIONLIST_H__

class CUDPPredictionList
{
private:
	unsigned long m_adwIPs[32];
	unsigned long m_adwIDsIN[32];
	unsigned long m_adwIDsOUT[32];
	unsigned long m_nIPs;

	unsigned long IndexFromIP(unsigned long IP);

	unsigned long m_dwMyIP;

public:
	CUDPPredictionList();
	~CUDPPredictionList();

	void Reset();
	void Reset(unsigned long dwMyIP);
	void Synchronize(unsigned long* pdwIPs, unsigned long nIPs);
	void Add(unsigned long IP);
	void Remove(unsigned long IP);

	/* Returns zero if the related packet should be ignored,
	and non-zero if the packet should be processed.
	
	This function applies to both TCP and UDP packets.
	*/
	unsigned char Validate(unsigned long dwTimeStamp, unsigned long dwIP);

	void PingAll();

	unsigned long GetCount() { return m_nIPs; }
	unsigned long* GetIPList() { return m_adwIPs; }
	unsigned long* GetIDListIN() { return m_adwIDsIN; }
	unsigned long* GetIDListOUT() { return m_adwIDsOUT; }

	unsigned long GetTimestamp(unsigned long dwIP, unsigned char bIncrementAfter = 0 /* false */);
};

#endif
