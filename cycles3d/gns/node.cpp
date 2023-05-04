/***************************************************************************
                           node.cpp  -  description
                             -------------------

    Implementation of a single node in the world topology.

    begin                : Thu Sep  18 22:24:34 EDT 2002
    copyright            : (C) 2002 by Chris Haag
    email                : dragonslayer_developer@hotmail.com
 ***************************************************************************/

#include <string.h>
#ifdef __GNUC__
#define stricmp strcasecmp
#define strnicmp strncasecmp
#endif
#include <memory.h>
#include "gns.h"
#include "node.h"

CNodeProperty::CNodeProperty(const char* szName)
{
	strncpy(m_szName, szName, MAX_PROP_NAME_LENGTH);
	m_nBytes = 0;
	m_pProp = NULL;
	m_pNext = NULL;
}

CNodeProperty::CNodeProperty(const char* szName, const void* pData, unsigned int nBytes)
{
	strncpy(m_szName, szName, MAX_PROP_NAME_LENGTH);
	m_nBytes = nBytes;
	m_pProp = new char[nBytes];
	memcpy(m_pProp, pData, nBytes);
	m_pNext = NULL;
}

CNodeProperty::~CNodeProperty()

{
	if (m_pProp)
		delete (char*)m_pProp;
}

CNode::CNode()
{
	Init(NULL, 0, GNS_IP("0.0.0.0"));
}

CNode::~CNode()
{
	/* Delete the data */
	if (m_pLabel)
		delete m_pLabel;

	/* Delete properties */
	CNodeProperty* p = m_pPropList, *pNext;
	while (p)
	{
		pNext = p->m_pNext;
		delete p;
		p = pNext;
	}

	/* Update other children */
	if (m_pPrev)
		m_pPrev->m_pNext = m_pNext;
	else if (m_pParent) /* Update the parent */
		m_pParent->m_pChild = m_pNext;
}

CNode::CNode(const char* szLabel, GNS_IP ip)
{
	Init(szLabel, strlen(szLabel), ip);
}

CNode::CNode(const char* szLabel, int nLabelSize, GNS_IP ip)
{
	Init(szLabel, nLabelSize, ip);
}

int CNode::Init(const char* szLabel, int nLabelSize, GNS_IP ip)
{
	/* Reset all variables except the label */
	m_pNext = NULL;
	m_pPrev = NULL;
	m_pChild = NULL;
	m_pParent = NULL;
	m_pPropList = NULL;
	m_ip = ip;

	/* Set up the label */
	if (!szLabel || !nLabelSize)
		m_pLabel = NULL;
	else
	{
		m_pLabel = new char[nLabelSize + 1];
		strncpy(m_pLabel, szLabel, nLabelSize);
		m_pLabel[nLabelSize] = '\0';
	}

	/* Reset our current time */
	TimeReset();
	return 0;
}

int CNode::TimeReset()
{
	GNS_TIME iCurTime = GNS_GetCurrentTime();
	return SetProperty("GNS_LastRefreshTime", &iCurTime, sizeof(GNS_TIME));
}

CNode* CNode::GetParent()
{
	CNode* pNode = this;
	while (pNode && !pNode->m_pParent)
		pNode = pNode->m_pPrev;
	if (!pNode)
		return NULL;
	return pNode->m_pParent;
}

int CNode::DeleteChildren()
{
	CNode* pNode = m_pChild, *pNext;
	while (pNode)
	{
		pNext = pNode->m_pNext;
		delete pNode;
		pNode = pNext;
	}
	m_pChild = NULL;
	return 0;
}

int CNode::Add(const char* szDomain, int nLen, GNS_IP ip)
{
	if (!szDomain) return -1;
	if (!nLen) return -1;
	if (nLen == -1) return 0; /* Means a recursion is done */

	/* Grab the last label in the domain, assuming that the string
	is nLen characters long. */
	const char *p, *pszLabel = szDomain;
	do {
		p = pszLabel;
	} while ((pszLabel = strchr(pszLabel, '.') + 1) != (const char*)1 && (int)(pszLabel - szDomain) < nLen);

	/* Now make nLen the length of the label */
	if (!strchr(p, '.'))
		nLen = strlen(p);
	else
		nLen = (int)(strchr(p, '.') - p);

	/* Find this label in our linked list of labels */
	CNode* pNode = m_pChild, *pPrev = NULL;
	while (pNode)
	{
		if (pNode->m_pLabel && !strnicmp(p, pNode->m_pLabel, nLen))
		{
			/* Yes we found it */
			if (p == szDomain)
			{
				/* This is the node! It's already there. Lets copy over the IP */
				pNode->m_ip = ip;
				/* Reset its current time */
				pNode->TimeReset();
				return 0;
			}
			else
				return pNode->Add(szDomain, (int)(p - szDomain - 1), ip);
		}
		pPrev = pNode;
		pNode = pNode->m_pNext;
	}

	/* No, we didn't find it. Add it to this list */
	pNode = new CNode(p, nLen, ip);
	if (!pNode)
		return -1; /* Bad news */

	if (!pPrev)
	{		
		pNode->m_pParent = this;		
		m_pChild = pNode;
	}
	else
	{
		pNode->m_pPrev = pPrev;
		pPrev->m_pNext = pNode;
	}
	
	/* Copy over the TTL */
	CNode* pParent;
	const void* pTTL;
	unsigned int nTTLBytes;
	if (pNode->m_pParent)
		pParent = pNode->m_pParent;
	else
	{
		pParent = pNode;
		while (pParent->m_pPrev)
			pParent = pParent->m_pPrev;
		pParent = pParent->m_pParent;
	}
	if (!pParent->GetProperty("GNS_ChildTTL", pTTL, &nTTLBytes))
		pNode->SetProperty("GNS_TTL", pTTL, nTTLBytes);

	/* Now recurse */
	return pNode->Add(szDomain, (int)(p - szDomain - 1), ip);
}

CNode* CNode::Find(const char* szDomain)
{
	return Find(szDomain, strlen(szDomain));
}

CNode* CNode::Find(const char* szDomain, int nLen)
{
	if (!szDomain) return NULL;
	if (!nLen) return NULL;
	if (nLen == -1) return NULL; /* Means a recursion is done */

	/* We're not supposed to this if we are not the first node
	in a list of children for a parent */
	if (m_pPrev)
	{
		/* TODO: Find Linux equivalent before implementing */
		/* ASSERT(FALSE);*/
		return NULL;
	}

	/* Grab the last label in the domain, assuming that the string
	is nLen characters long. */
	const char *p, *pszLabel = szDomain;
	do {
		p = pszLabel;
	} while ((pszLabel = strchr(pszLabel, '.') + 1) != (const char*)1 && (int)(pszLabel - szDomain) < nLen);

	/* Now make nLen the length of the label */
	if (!strchr(p, '.'))
		nLen = strlen(p);
	else
		nLen = (int)(strchr(p, '.') - p);

	/* Find this label in our linked list of labels */
	CNode* pNode = this;
	while (pNode)
	{
		if (pNode->m_pLabel && !strnicmp(p, pNode->m_pLabel, nLen))
		{
			/* Yes we found it */
			if (p == szDomain)
			{
				/* This is the node! */
				return pNode;
			}
			else if (!pNode->m_pChild)
				return NULL; /* Nope, it doesn't exist */
			else
				return pNode->m_pChild->Find(szDomain, (int)(p - szDomain - 1));
		}
		pNode = pNode->m_pNext;
	}

	/* Failed to find it */
	return NULL;
}

int CNode::SetProperty(const char* szPropName, const void* pData, unsigned int nBytes)
{
	/* See if it already exists */
	CNodeProperty* p = m_pPropList;
	while (p)
	{
		if (!stricmp(szPropName, p->m_szName))
		{
			/* Yeah, set it */

			/* This code makes it so we don't have to allocate every single time */
			if (p->m_nBytes < nBytes)
			{
				delete (char*)p->m_pProp;
				p->m_pProp = new char[nBytes];
			}
			else
				p->m_nBytes = nBytes;

			memcpy(p->m_pProp, pData, nBytes);
			return 0;
		}
		p = p->m_pNext;
	}

	p = new CNodeProperty(szPropName, pData, nBytes);
	if (!p)
		return -1;
	p->m_pNext = m_pPropList;
	m_pPropList = p;
	return 0;
}

int CNode::GetProperty(const char* szPropName, const void*& pData, unsigned int* pnBytes /* = NULL*/)
{
	CNodeProperty* p = m_pPropList;
	while (p)
	{
		if (!stricmp(p->m_szName, szPropName))
			break;
		p = p->m_pNext;
	}
	if (!p)
		return -1;

	pData = p->m_pProp;
	if (pnBytes)
		*pnBytes = p->m_nBytes;
	return 0;
}

int CNode::Maintenance(unsigned int flags)
{
	if (flags & GNS_MAINT_RETIRE_OLD_NODES)
	{
		/* Delete all children with expired TTL's */
		CNode* pChild = m_pChild, *pNext;
		GNS_TIME tCurTime = GNS_GetCurrentTime();
		while (pChild)
		{
			const void* nttlSeconds;
			const void* tLastRefresh;

			pNext = pChild->m_pNext;

			/* Recurse */
			pChild->Maintenance(flags);

			/* Remove expired nodes */
			if (!pChild->GetProperty("GNS_TTL", nttlSeconds) &&
				!pChild->GetProperty("GNS_LastRefreshTime", tLastRefresh))
			{
				if (*((GNS_TIME*)nttlSeconds) && tCurTime > *((GNS_TIME*)tLastRefresh) + *((GNS_TIME*)(nttlSeconds)))
				{
					GNS_LogProc("Game '%s' expired!", m_pLabel);
					pChild->DeleteChildren();
					delete pChild;
				}
			}
			pChild = pNext;
		}
	}
	return 0;
}

int CNode::GetChildCount()
{
	CNode* pNode = m_pChild;
	int nChildren = 0;
	while (pNode)
	{
		nChildren++;
		pNode = pNode->m_pNext;
	}
	return nChildren;
}
