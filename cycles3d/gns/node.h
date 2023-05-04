/***************************************************************************
                           node.h  -  description
                             -------------------

    Implementation of a single node in the world topology.

    begin                : Thu Sep  18 22:24:34 EDT 2002
    copyright            : (C) 2002 by Chris Haag
    email                : dragonslayer_developer@hotmail.com
 ***************************************************************************/

#ifndef __NODE_H__
#define __NODE_H__

#include "gns.h"

#define MAX_PROP_NAME_LENGTH	256

class CNodeProperty
{
public:
	char m_szName[MAX_PROP_NAME_LENGTH];
	unsigned int m_nBytes;
	void* m_pProp;

	CNodeProperty* m_pNext;
public:	
	CNodeProperty(const char* szName);
	CNodeProperty(const char* szName, const void* pData, unsigned int nBytes);
	virtual ~CNodeProperty();
};

class CNode
{
public:
	CNode* m_pNext;		/* All nodes in this list have the same parent */
	CNode* m_pPrev;		/* Previous node in the list */
	CNode* m_pChild;	/* Linked list of children */
	CNode* m_pParent;	/* Redundant, but used for ease of traversal */

	CNodeProperty* m_pPropList;	/* List of generic properties */

public:
	/* Standard data for all implementations */
	GNS_IP m_ip;		/* IP address */
	char* m_pLabel;		/* Label (dynamically allocated, zero-terminated) */

	/* Functions */
public:	
	CNode();
	CNode(const char* szLabel, GNS_IP ip);
	CNode(const char* szLabel, int nLabelSize, GNS_IP ip);
	virtual ~CNode();

	int Init(const char* szLabel, int nLabelSize, GNS_IP ip);
	int Add(const char* szDomain, int nLen, GNS_IP ip);
	CNode* Find(const char* szDomain);
	CNode* Find(const char* szDomain, int nLen);
	int DeleteChildren();
	int SetProperty(const char* szPropName, const void* pData, unsigned int nBytes);
	int GetProperty(const char* szPropName, const void*& pData, unsigned int* pnBytes = NULL);
	int Maintenance(unsigned int flags);
	int TimeReset();
	int GetChildCount();
	CNode* GetParent();
};

#endif
