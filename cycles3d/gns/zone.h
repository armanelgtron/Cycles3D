/***************************************************************************
                           zone.h  -  description
                             -------------------

    Manages known nodes in the world topology.

    begin                : Thu Sep  18 22:24:34 EDT 2002
    copyright            : (C) 2002 by Chris Haag
    email                : dragonslayer_developer@hotmail.com
 ***************************************************************************/

#ifndef __ZONE_H__
#define __ZONE_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Clears the entire tree */
int Zone_Empty();

/* Adds a domain */
int Zone_Add(const char* szDomain, GNS_IP ip);

/* Adds a domain */
int Zone_AddChild(const char* szDomain, const char* pLabel, int nLabelBytes, GNS_IP ip);

/* Removes a domain and all its children */
int Zone_Delete(const char* szDomain);

/* Appends nodes to the current topology based on the entries
of a zone file */
int Zone_ReadFile(const char* szFilename);

/* Appends nodes to the current zone tree based on the entries
of ASCII formatted zone data */
int Zone_Parse(char* pData);

/* Do an IP lookup */
int Zone_Lookup(const char* szDomain, GNS_IP* pIP);

/* Ensure a zone exists */
int Zone_Exists(const char* szDomain);

/* Traverses through all immediate children and generates raw
data to send to a GNS client who wants a game listing */
int Zone_GenerateGameListing(const char* szDomain, void** pData, unsigned int* pDataLen);

/* Parse the listing generated by Zone_GenerateGameListing */
int Zone_ParseGameListing(const char* szDomain, const void* pData, int nBytes, const void* pUserData);

/* Make it so this code is sent every time you do an action on the server so it
can identify you. This is completely custom data. */
int Zone_SetAuthentication(const char* szDomain, const void* pAuthCode, unsigned int nBytes);

/* Get the authentication code (used in the GNS soure code) */
int Zone_GetAuthentication(const char* szDomain, const void** ppAuthCode, unsigned int* pnBytes);

/* Get the first child node of a given domain. This is the equivalent to getting
the name of the first game hosted under a domain */
GNS_HANDLE Zone_GetFirstChildNode(const char* szDomain);

/* Get the next child node. This is the equivalent to getting
the name of the next game hosted under a domain */
GNS_HANDLE Zone_GetNextChildNode(GNS_HANDLE hCurChild);

/* Returns the number of child nodes */
int Zone_GetChildCount(const char* szDomain);

/* Get information about a node. This can be perceived as the hosted game name and IP */
int Zone_GetNodeInfo(GNS_HANDLE hNode, const char** ppLabel, const GNS_IP** pIP);

/* Set the interval in which a client hosting a game under you must reaffirm its existence.
To do this, it merely has to send any GNS packet (except one that says it is not hosting).
If it does not do that in that interval, it will be removed from the list. If this
applies to all games, and your GNS server is yourgame.gns, then set szDomain to
"yourgame.gns" and this value will propogate to all hosted games. Otherwise, you
can set it for individual games with, for example, "game40.yourgame.gns" */
int Zone_SetChildTTL(const char* szDomain, int nSeconds);

int Zone_GetChildTTL(const char* szDomain, int* pnSeconds);

/* Performs maintenance to drop retired zone nodes, among other things we
have yet to implement in the future */
int Zone_Maintenance(const char* szDomain, unsigned int flags);

/* Get a property for a node */
int Zone_GetProperty(const char* szDomain, const char* szPropName, const void** ppData, unsigned int* pnBytes);

/* Set a property for a node */
int Zone_SetProperty(const char* szDomain, const char* szPropName, const void* pData, unsigned int nBytes);

/* Returns the full domain of a node */
char* Zone_GetDomainFromNode(GNS_HANDLE hNode, char* szDomain);

#ifdef __cplusplus
}
#endif

#endif