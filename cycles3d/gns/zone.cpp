/***************************************************************************
                           zone.cpp  -  description
                             -------------------

    Manages known nodes in the world topology.

    begin                : Thu Sep  18 22:24:34 EDT 2002
    copyright            : (C) 2002 by Chris Haag
    email                : dragonslayer_developer@hotmail.com
 ***************************************************************************/

#ifdef WIN32
#include <winsock.h>	/* For htonl() */
#elif defined(__GNUC__)
#include <arpa/inet.h>
#endif
#include "node.h"
#include "zone.h"

CNode g_Root;	/* The root node */

int Zone_Empty()
{
	int iRet = Zone_Delete(NULL);
	return iRet;
}

int Zone_Add(const char* szDomain, GNS_IP ip)
{
	return g_Root.Add(szDomain, strlen(szDomain), ip);
}

int Zone_Delete(const char* szDomain)
{
	if (!g_Root.m_pChild)
		return -1;

	CNode* pNode;
	if (!szDomain || !strlen(szDomain))
		pNode = &g_Root;
	else
		pNode = g_Root.m_pChild->Find(szDomain);
	if (!pNode)
		return -1;

	/* Delete all the children. This is not called
	in the destructor; just because we delete a node
	doesn't mean we want to delete the children. */
	pNode->DeleteChildren();

	/* Delete the node. This also automatically updates info
	in the parent. */
	if (pNode != &g_Root)
		delete pNode;

	return 0;
}

int Zone_ReadFile(const char* szFilename)
{
	FILE* hFile = fopen(szFilename, "rt");
	char* pData;
	int nFileSize;
	int iRet;

	if (!hFile)
		return -1;

	/* Grab the file size */
	fseek(hFile, 0, SEEK_END);
	nFileSize = ftell(hFile);
	fseek(hFile, 0, SEEK_SET);

	/* Read the file */
	pData = new char[nFileSize];
	if (!pData)
	{
		fclose(hFile);
		return -1;
	}
  memset(pData, 0, nFileSize);
	fread(pData, nFileSize, sizeof(char), hFile);
	fclose(hFile);

	/* Parse the file */
	iRet = Zone_Parse(pData);

	delete pData;
	return iRet;
}

int Zone_Parse(char* pData)
{
	enum { EDomain, ETTL, EClass, EType, EData } eField;
	char* psz = strtok(pData, "\r\n");
	char sz[1024]; /* Maximum length of a line in the data */
	char bMultiline = 0;
	int iTTL;
	char *szDomain = NULL, *szClass = NULL, *szType = NULL, *szData = NULL;

	while (psz)
	{		
		int len;
		eField = EDomain;
		strncpy(sz, psz, 1024);

		/* Truncate the string where the comment is, if any */
		if (strchr(sz, ';'))
			*strchr(sz, ';') = 0;

		/* Keep the length for use later */
		len = strlen(sz);

		/* We'll have to do our own tokenizing by traversing
		spaces */
		psz = sz;
		while ((psz - sz) < len)
		{
			/* Change the whitespace or tab into a NULL */
			if (strchr(psz, ' '))
				*strchr(psz, ' ') = 0;

			if (strchr(psz, '\t'))
				*strchr(psz, '\t') = 0;

			if (strchr(psz, '('))
				bMultiline = 1;

			if (strchr(psz, ')'))
				bMultiline = 0;

			if (*psz)
			{
				// Store the field
				switch (eField)
				{
				case EDomain: szDomain = psz; eField = ETTL; break;
				case ETTL:
					iTTL = atoi(psz);
					/* if iTTL is 0, assume that psz is not a number, and hence
					must be a class. TODO: Break this assumption with an itoa test
					and string comparison. */
					if (iTTL != 0)
					{
						eField = EClass; break;
					}
				case EClass: szClass = psz; eField = EType; break;
				case EType: szType = psz; eField = EData; break;
				case EData:	szData = psz; break;
				}
			}
			psz += strlen(psz) + 1;
		}
		
		/* If this is not multi-line, lets parse it */
		if (!bMultiline) 
		{
			/* This is the only combination we support */
			if (szClass && szType && !strcmp(szClass, "IN") && !strcmp(szType, "A"))
			{
				Zone_Add(szDomain, GNS_IP(szData));
				Zone_SetProperty(szDomain, "GNS_TTL", &iTTL, 4);
			}

			/* These no longer apply */
			szDomain = NULL; szClass = NULL; szType = NULL; szData = NULL;
		}

		psz = strtok(NULL, "\r\n");
	}
	return 0;
}

int Zone_Lookup(const char* szDomain, GNS_IP* pIP)
{
	if (!pIP) return -1;
	if (!g_Root.m_pChild) return -1;

	CNode* pNode = g_Root.m_pChild->Find(szDomain, strlen(szDomain));
	if (!pNode)
		return -1;
	
	*pIP = pNode->m_ip;
	return 0;
}

int Zone_Exists(const char* szDomain)
{
	if (!g_Root.m_pChild) return 0;

	CNode* pNode = g_Root.m_pChild->Find(szDomain, strlen(szDomain));
	if (!pNode)
		return 0;
	return 1;
}

int Zone_GenerateGameListing(const char* szDomain, void** pData, unsigned int* pDataLen)
{
	if (!g_Root.m_pChild) return -1;
	CNode* pNode = g_Root.m_pChild->Find(szDomain, strlen(szDomain));
	const void* pExtraInfo;
	unsigned int nExtraInfoBytes;
	if (!pNode)
		return -1;
	unsigned int& lDataLen = *pDataLen;
	void*& p = *pData;
	char* pCur;

	/* Find out how big the data we're sending is going to be */
	CNode* pChild = pNode->m_pChild;
	p = NULL;
	lDataLen = 0;

	/* Do for every child node */
	/* TODO: RECURSE */
	while (pChild)
	{
		lDataLen += sizeof(unsigned int); /* Length of label */
		lDataLen += strlen(pChild->m_pLabel); /* Label */
		lDataLen += sizeof(GNS_IPv4); /* IP (v4) */

		/* Do extra info too */
		if (!pChild->GetProperty("GNS_ExtraInfo", pExtraInfo, &nExtraInfoBytes))
			lDataLen += sizeof(unsigned int) + nExtraInfoBytes;
		else
			lDataLen += sizeof(unsigned int);

		pChild = pChild->m_pNext;
	}

	/* If we have nothing, send nothing */
	if (!lDataLen)
		return 0;

	/* Now allocate data */
	p = pCur = new char[lDataLen];

	/* Now write the data */
	pChild = pNode->m_pChild;
	while (pChild)
	{
		*((unsigned int*)pCur) = strlen(pChild->m_pLabel); pCur += sizeof(int);
		memcpy(pCur, pChild->m_pLabel, strlen(pChild->m_pLabel)); pCur += strlen(pChild->m_pLabel);
		*((GNS_IPv4*)pCur) = pChild->m_ip.ip[3];
		pCur += sizeof(GNS_IPv4);
		
		/* Do extra info too */
		if (!pChild->GetProperty("GNS_ExtraInfo", pExtraInfo, &nExtraInfoBytes))
		{
			*((unsigned int*)pCur) = nExtraInfoBytes;
			pCur += sizeof(int);
			memcpy(pCur, pExtraInfo, nExtraInfoBytes);
			pCur += nExtraInfoBytes;
		}
		else
		{
			*((unsigned int*)pCur) = 0;
			pCur += sizeof(int);
		}

		/* Write data for this entry */
		pChild = pChild->m_pNext;
	}

	/* Done! */
	return 0;
}

int Zone_ParseGameListing(const char* szDomain, const void* pData, int nBytes, const void* /*pUserData*/)
{
	const char* p = (const char*)pData;

	if (!nBytes)
		GNS_LogProc("Server response: No games found for %s", szDomain);

	while (nBytes)
	{
		if (nBytes < 0) /* Underflow */
			return -1;

		unsigned int nLabelBytes;
		const char* pLabel; /* Also the game instance name */
		GNS_IPv4 ip;
		char* szCompleteDomain;
		unsigned int nExtraBytes;

		nLabelBytes = *((unsigned int*)p); p += sizeof(unsigned int);
		pLabel = p; p += nLabelBytes;
		/* Assume the server gives uses network byte order */
		ip = htonl(*((GNS_IPv4*)p)); p += sizeof(GNS_IPv4);
		nExtraBytes = *((unsigned int*)p); p += sizeof(unsigned int);

		/* Create the new node. TODO: We can certainly find a cleaner way than this. */
		szCompleteDomain = new char[strlen(szDomain) + nLabelBytes + 2];
		if (!szCompleteDomain)
		{
			GNS_LogProc("Error adding domain %s", szCompleteDomain);
			delete szCompleteDomain;
			return -1;
		}
		memcpy(szCompleteDomain, pLabel, nLabelBytes);
		szCompleteDomain[nLabelBytes] = '.';
		memcpy(szCompleteDomain + nLabelBytes + 1, szDomain, strlen(szDomain)+1);
		if (Zone_Add(szCompleteDomain, ip))
		{
			GNS_LogProc("Error adding domain %s", szCompleteDomain);
			delete szCompleteDomain;
			return -1;
		}
		if (nExtraBytes)
		{
			Zone_SetProperty(szCompleteDomain, "GNS_ExtraInfo", p, nExtraBytes);
		}
		GNS_LogProc("Added domain %s", szCompleteDomain);

		/* Cleanup */
		delete szCompleteDomain;
		
		p += nExtraBytes;
		nBytes -= sizeof(int) + nLabelBytes + sizeof(GNS_IPv4) + sizeof(int) + nExtraBytes;
	}
	return 0;
}

int Zone_GetChildCount(const char* szDomain)
{
	if (!g_Root.m_pChild || !szDomain || !strlen(szDomain))
		return 0;

	CNode* pNode = g_Root.m_pChild->Find(szDomain);
	if (!pNode)
		return 0;

	return pNode->GetChildCount();
}

GNS_HANDLE Zone_GetFirstChildNode(const char* szDomain)
{
	if (!g_Root.m_pChild || !szDomain || !strlen(szDomain))
		return 0;

	CNode* pNode = g_Root.m_pChild->Find(szDomain);
	if (!pNode)
		return 0;

	return (GNS_HANDLE)pNode->m_pChild;
}

GNS_HANDLE Zone_GetNextChildNode(GNS_HANDLE hCurChild)
{
	if (!hCurChild)
		return 0;

	CNode* pNode = (CNode*)hCurChild;
	return (GNS_HANDLE)pNode->m_pNext;
}

int Zone_GetNodeInfo(GNS_HANDLE hNode, const char** ppLabel, const GNS_IP** ppIP)
{
	if (!hNode || !ppLabel || !ppIP)
		return -1;

	CNode* pNode = (CNode*)hNode;
	*ppLabel = pNode->m_pLabel;
	*ppIP = &pNode->m_ip;
	return 0;	
}

int Zone_SetAuthentication(const char* szDomain, const void* pAuthCode, unsigned int nBytes)
{
	if (!g_Root.m_pChild)
		return -1;

	CNode* pNode = g_Root.m_pChild->Find(szDomain);
	if (!pNode)
		return -1;

	return pNode->SetProperty("GNS_Auth0", pAuthCode, nBytes);
}

int Zone_GetAuthentication(const char* szDomain, const void** ppAuthCode, unsigned int* pnBytes)
{
	if (!g_Root.m_pChild || !ppAuthCode || !pnBytes)
		return -1;

	CNode* pNode = g_Root.m_pChild->Find(szDomain);
	if (!pNode)
		return -1;

	return pNode->GetProperty("GNS_Auth0", *ppAuthCode, pnBytes);
}

int Zone_SetChildTTL(const char* szDomain, int nSeconds)
{
	if (!g_Root.m_pChild)
		return -1;

	CNode* pNode = g_Root.m_pChild->Find(szDomain);
	if (!pNode)
		return -1;

	return pNode->SetProperty("GNS_ChildTTL", &nSeconds, sizeof(unsigned int));
}

int Zone_GetChildTTL(const char* szDomain, int* pnSeconds)
{
	if (!g_Root.m_pChild || !szDomain || !pnSeconds)
		return -1;

	CNode* pNode = g_Root.m_pChild->Find(szDomain);
	if (!pNode)
		return -1;

	const void* pData;
	unsigned int nBytes;
	if (pNode->GetProperty("GNS_ChildTTL", pData, &nBytes))
		return -1;

	*pnSeconds = *((int*)pData);
	return 0;
}

int Zone_Maintenance(const char* szDomain, unsigned int flags)
{
	CNode* pNode;
	if (!g_Root.m_pChild)
		return -1;

	/* Choose the parent node for maintenance */
	if (szDomain && strlen(szDomain))
		pNode = g_Root.m_pChild->Find(szDomain);
	else
		pNode = &g_Root;

	/* Perform recursive maintenance */
	return pNode->Maintenance(flags);
}

int Zone_SetProperty(const char* szDomain, const char* szPropName, const void* pData, unsigned int nBytes)
{
	if (!g_Root.m_pChild) return 0;

	CNode* pNode = g_Root.m_pChild->Find(szDomain, strlen(szDomain));
	if (!pNode)
		return -1;
	
	return pNode->SetProperty(szPropName, pData, nBytes);
}

int Zone_GetProperty(const char* szDomain, const char* szPropName, const void** ppData, unsigned int* pnBytes)
{
	if (!szDomain || !szPropName || !ppData)
		return -1;

	CNode* pNode = g_Root.m_pChild->Find(szDomain, strlen(szDomain));
	if (!pNode)
		return -1;

	return pNode->GetProperty(szPropName, *ppData, pnBytes);
}

char* Zone_GetDomainFromNode(GNS_HANDLE hNode, char* szDomain)
{
	CNode* pNode = (CNode*)hNode;
	if (!pNode) return NULL;

	strcpy(szDomain, pNode->m_pLabel);
	while (pNode)
	{
		pNode = pNode->GetParent();
		if (pNode && pNode->m_pLabel)
		{
			strcat(szDomain, ".");
			strcat(szDomain, pNode->m_pLabel);
		}
	}
	return szDomain;
}
