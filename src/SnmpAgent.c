/*
 * Implements core functions of an uSNMP agent to
 * 1. initialise , traverse and access a MIB tree
 * 2. receive, parse and transmit SNMPv1 packet
 * 3. Parse a varbind list
 * 4. send a SNMPv1 trap
 *
 * This file is part of uSNMP ("micro-SNMP").
 * uSNMP is released under a BSD-style license. The full text follows.
 *
 * Copyright (c) 2022 Francis Tay. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, is hereby granted without fee provided that the following
 * conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 3. Neither the name of Francis Tay nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY FRANCIS TAY AND CONTRIBUTERS 'AS
 * IS' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOTLIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.	IN NO EVENT SHALL FRANCIS TAY OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARAY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * This file extends the Embedded SNMP Server, presented in chapter 8 of the
 * book "TCP/IP Application Layer Protocols for Embedded Systems" by M. Tim Jones
 * (Charles River Media, published July 2002. ISBN 1-58450-247-9), which is
 * covered under a BSD-style license as shown here:
 *
 * Copyright (c) 2002 Charles River Media. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, is hereby granted without fee provided that the following
 * conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 3. Neither the name of Charles River Media nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY CHARLES RIVER MEDIA AND CONTRIBUTERS 'AS
 * IS' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOTLIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.	IN NO EVENT SHALL CHARLES RIVER MEDIA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARAY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ARDUINO
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#ifdef _WIN32
#include <winsock.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netdb.h>
#include <sys/un.h>
#endif
#endif

#include "SnmpAgent.h"

#ifdef ARDUINO
extern IPAddress hostIpAddr, hostGateway, hostNetmask, dnsServer;
IPAddress remoteIpAddr;
#ifdef ARDUINO_ETHERNET
extern unsigned char hostMacAddr[];
EthernetUDP Udp;
#else  // ARDUINO_WIFI
extern char staSSID[];
extern char staPSK[];
WiFiUDP Udp;
#endif
#else
#include "mibutil.h"
char hostIpAddr[16], remoteIpAddr[16];
#endif
Boolean debug = FALSE;

uint16_t remotePort;
char *enterpriseOID;
char *roCommunity, *rwCommunity, remoteCommunity[COMM_STR_SIZE];
Boolean (*checkCommunity)(char *commstr, int reqType) = NULL;
LIST *mibTree;

struct messageStruct request, response;
unsigned char requestBuffer[REQUEST_BUFFER_SIZE], responseBuffer[RESPONSE_BUFFER_SIZE];
unsigned char errorStatus = 0 , errorIndex = 0;

#define COPY_SEGMENT(x) \
	{ \
	request->index += seglen; \
	memcopy( response->buffer+response->index, request->buffer+x.start, seglen ); \
	response->index += seglen; \
	}

int snmpGet(MIB *thismib, struct messageStruct *response, int *len )
{
	int error_code;

	if (thismib->get != NULL && (error_code=thismib->get(thismib)) != NO_ERR) return error_code;
	if ((response->index+3+thismib->dataLen) >= RESPONSE_BUFFER_SIZE)  /* 3 = 1 Tag + 2 Length conservatively*/
		return BUFFER_FULL;
	response->buffer[response->index] = thismib->dataType;
	response->buffer[response->index+1] = 0;  /* Set length field to zero first */
	switch(thismib->dataType) {
		case OCTET_STRING :
		case OBJECT_IDENTIFIER :
			{
				*len = thismib->dataLen;
		 		memcopy((unsigned char *) response->buffer+response->index+2, thismib->u.octetstring, *len);
			}
			break;
		case INTEGER :
		case TIMETICKS :
		case COUNTER :
		case GAUGE :
			{
				*(response->buffer+response->index+1) = INT_SIZE;
				h2nl_byte((uint32_t)thismib->u.intval, response->buffer+response->index+2);
				compactInt(response->buffer+response->index);
				*len = (int) *(response->buffer+response->index+1);
			}
			break;
		default :
			return INVALID_DATA_TYPE;
	}
	return NO_ERR;
}

int snmpSet(MIB *thismib, unsigned char dataType, void *val, int vlen)
{
	uint32_t intval;
	int error_code;

	if (thismib->access != RD_WR)
		return RD_ONLY_ACCESS;
	else if ( thismib->dataType != dataType )
		return INVALID_DATA_TYPE;
	switch(dataType) {
		case OCTET_STRING :
		case OBJECT_IDENTIFIER :
			if (thismib->set != NULL) {
				if ((error_code=thismib->set(thismib, val, vlen)) != NO_ERR)
					return error_code;
			}
			else {
				thismib->dataLen = vlen;
	 			memcopy(thismib->u.octetstring, (unsigned char *) val, vlen);
			}
			break;
		case INTEGER :
		case TIMETICKS :
		case COUNTER :
		case GAUGE :
			intval = getValue((unsigned char *)val, vlen, thismib->dataType);
			if (thismib->set != NULL) {
				if ((error_code=thismib->set(thismib, &intval, INT_SIZE)) != NO_ERR)
					return error_code;
			}
			else {
				thismib->dataLen = INT_SIZE;
				thismib->u.intval = intval;
			}
			break;
		default:
			return INVALID_DATA_TYPE;
	}
	return NO_ERR;
}

int parseVarBind ( int reqType, struct messageStruct *request, struct messageStruct *response )
{
	int seglen = 0, size, len;
	tlvStructType name, value;
	MIB *thismib;
	OID oid;

	if (parseTLV(request->buffer, request->index, &name) != SUCCESS ||
		request->buffer[name.start] != OBJECT_IDENTIFIER ) {
		errorStatus = BAD_VALUE;
		return ILLEGAL_DATA;
	}

	/* For normal GET_REQUEST/SET_REQUEST and TRAP_PACKET, copy the NAME (OID) tlv
	 * over and continue. But for GET_NEXT_REQUEST, identify the next OID, then
	 * copy it in as if it is the requested object.
	 */
	ber2oid(request->buffer+name.vstart, name.len, &oid);
	thismib = miblistgooid(mibTree, &oid);
	if (reqType == GET_REQUEST || reqType == TRAP_PACKET || reqType == SET_REQUEST) {
		seglen = name.nstart - name.start;
		COPY_SEGMENT(name);
	} else
		if (reqType == GET_NEXT_REQUEST) {
			if (thismib!=NULL)
				thismib = miblistgonext(mibTree);
			else
				thismib = miblistgetthis(mibTree);
			if (thismib==NULL) {  /* end of MIB tree */
				errorStatus = NO_SUCH_NAME;
				return OID_NOT_FOUND;
			} else {
				/* Skip the name TLV and replace with the next OID in the MIB tree */
				request->index += name.nstart - name.start;
				response->buffer[response->index] = OBJECT_IDENTIFIER;
				response->buffer[response->index+1] = (unsigned char) oid2ber(&thismib->oid,
				response->buffer+response->index+2);  /* OID length field assumed 1 byte long */
				seglen = response->buffer[response->index+1]+2;
				response->index += seglen ;
			}
		}
		else return INVALID_PDU_TYPE;
	size = seglen;

	/* Parse the value TLV, and process accordingly */
	parseTLV(request->buffer, request->index, &value);
	if (reqType == TRAP_PACKET && request->buffer[value.start] != NULL_ITEM) {
		seglen = value.nstart - value.start;  /* Retained */
		COPY_SEGMENT(value);
	}
	else
		if (thismib) {
			if (reqType == SET_REQUEST) {
				seglen = value.nstart - value.start; /* Retained */
				COPY_SEGMENT(value);
				switch (snmpSet( thismib, request->buffer[value.start], request->buffer+value.vstart, value.len )) {
					case SUCCESS: break;
					case RD_ONLY_ACCESS:
						errorStatus = READ_ONLY; return RD_ONLY_ACCESS;
					case INVALID_DATA_TYPE:
						errorStatus = BAD_VALUE; return INVALID_DATA_TYPE;
					default:
						errorStatus = GEN_ERROR; return FAIL;
				}
			}
			else
				if (request->buffer[value.start] != NULL_ITEM)
					 { errorStatus = BAD_VALUE; return INVALID_DATA_TYPE; }
				else
					if (reqType == GET_REQUEST || reqType == TRAP_PACKET || reqType == GET_NEXT_REQUEST) {
						switch (snmpGet(thismib, response, &len)) {
							case SUCCESS: break;
							case BUFFER_FULL:
								errorStatus = TOO_BIG; return BUFFER_FULL;
							case INVALID_DATA_TYPE:
								errorStatus = GEN_ERROR; return INVALID_DATA_TYPE;
							 default:
								errorStatus = GEN_ERROR; return FAIL;
						}
						seglen = 1 + insertRespLen(request, value.start, response, response->index, len) + len;
						response->index += seglen;
						/* Skip the NULL TLV in the request stream */
						request->index += (value.nstart - value.start);
					}
		}
		else {
			errorStatus = NO_SUCH_NAME;
			return OID_NOT_FOUND;
		}

	size += seglen;
	return size;
}

int parseSequence ( int reqType, struct messageStruct *request, struct messageStruct *response )
{
	int seglen, size, respLoc;
	tlvStructType seq;

	if (request->index >= request->len) return ILLEGAL_LENGTH;
	if (parseTLV(request->buffer, request->index, &seq) !=SUCCESS ||
		request->buffer[seq.start] != SEQUENCE ) return ILLEGAL_DATA;
	seglen = seq.vstart - seq.start;
	respLoc = response->index;
	COPY_SEGMENT(seq);
	size = parseVarBind( reqType, request, response );
	if (size >= 0) {
		if (reqType == SET_REQUEST)
			return (size + seglen);
		else
			return (size + insertRespLen(request, seq.start, response, respLoc, size) + 1);
	}
	else return size;
}

int parseSequenceOf ( int reqType, struct messageStruct *request, struct messageStruct *response )
{
	int ret, seglen, size = 0, respLoc, index = 0;
	tlvStructType seqof;

	if (request->index >= request->len) return ILLEGAL_LENGTH;
	if (parseTLV(request->buffer, request->index, &seqof) != SUCCESS ||
		request->buffer[seqof.start] != SEQUENCE_OF) return ILLEGAL_DATA;
	seglen = seqof.vstart - seqof.start;
	respLoc = response->index;
	COPY_SEGMENT(seqof);
	errorStatus = NO_ERR; errorIndex = 0;
	while (request->index < request->len) {
		index++;
		if ( (ret=parseSequence( reqType, request, response )) < 0 ) {  /* Indicates error */
			if (errorStatus==NO_ERR) errorStatus=GEN_ERROR;
			errorIndex = index;
			return ret;
		}
		else size += ret;
	}
	if (reqType == SET_REQUEST)
		return (size + seglen);
	else
		return (size + insertRespLen(request, seqof.start, response, respLoc, size) + 1);
}

#undef COPY_SEGMENT
#define COPY_SEGMENT(x) \
	{ \
		request.index += seglen; \
				memcopy( response.buffer+response.index, \
						 request.buffer+x.start, seglen ); \
					response.index += seglen; \
	}

Boolean valid_community(char *commstr, int reqType)
{
	if (checkCommunity != NULL)
		return checkCommunity(commstr, reqType);
	else
		if ( strcmp(commstr, rwCommunity)==0 ||
		     (reqType!=SET_REQUEST && strcmp(commstr, roCommunity)==0) )
			return TRUE;
		else
			return FALSE;
}

static int parseRequest ( char *commstr )
{
	int ret, seglen, size = 0, reqType, reqLoc, reqLen, errStatusLoc, errIndexLoc;
	tlvStructType tlv;

	if (request.index >= request.len) return ILLEGAL_LENGTH;
	if ( (ret=parseTLV(request.buffer, request.index, &tlv)) != SUCCESS) return ret;
	reqType = request.buffer[tlv.start];

	if ( !VALID_REQUEST(reqType) ) return INVALID_PDU_TYPE;
	if ( !valid_community( commstr, reqType) ) return COMM_STR_MISMATCH;

	seglen = tlv.vstart - tlv.start;
	reqLoc = tlv.start; reqLen = seglen + tlv.len;  /* Holds the Request-PDU */
	COPY_SEGMENT(tlv);

	response.buffer[reqLoc] = GET_RESPONSE;

	/* Parse Request ID */
	if (parseTLV(request.buffer, request.index, &tlv) != SUCCESS ||
		request.buffer[tlv.start]!=INTEGER) return REQ_ID_ERR;
	seglen = tlv.nstart - tlv.start;
	size += seglen;
	COPY_SEGMENT(tlv);

	/* Parse Error Status */
	if (parseTLV(request.buffer, request.index, &tlv) != SUCCESS ||
		request.buffer[tlv.start]!=INTEGER || tlv.len!=1 ||
		request.buffer[tlv.vstart]!='\0') return ILLEGAL_ERR_STATUS;
	errStatusLoc = tlv.vstart;
	seglen = tlv.nstart - tlv.start;
	size += seglen;
	COPY_SEGMENT(tlv);

	/* Parse Error Index */
	if (parseTLV(request.buffer, request.index, &tlv) != SUCCESS ||
		request.buffer[tlv.start]!=INTEGER || tlv.len!=1 ||
		request.buffer[tlv.vstart]!='\0') return ILLEGAL_ERR_INDEX;
	errIndexLoc = tlv.vstart;
	seglen = tlv.nstart - tlv.start;
	size += seglen;
	COPY_SEGMENT(tlv);

	ret = parseSequenceOf(reqType, &request, &response);
	if (ret < 0) {
		if ( errorIndex!=0 ) {
			/* In the event of a parsing error, the Request-PDU is restored
				 and added with the error status and index */
			memcopy(response.buffer+reqLoc, request.buffer+reqLoc, reqLen);
			response.buffer[reqLoc] = GET_RESPONSE;
	 		response.buffer[errStatusLoc] = errorStatus;
			response.buffer[errIndexLoc] = errorIndex;
			return reqLen;
		}
		else return ret;
	}
	else {
		size += ret;
		return (size + insertRespLen(&request, reqLoc, &response, reqLoc, size) + 1);
	}
}

static int parseCommunity ( )
{
	int seglen, size;
	tlvStructType community;

	if (request.index >= request.len) return ILLEGAL_LENGTH;
	if (parseTLV(request.buffer, request.index, &community) != SUCCESS ||
		community.len >= COMM_STR_SIZE) return COMM_STR_ERR;
	memcopy( (unsigned char *)remoteCommunity, request.buffer+community.vstart,
		community.len );
	remoteCommunity[community.len] = '\0';
	if (request.buffer[community.start] == OCTET_STRING) {
		seglen = community.nstart - community.start;
		COPY_SEGMENT(community);
		if ( (size=parseRequest(remoteCommunity)) >= 0 ) return size + seglen;
		else return size;
	}
	return INVALID_DATA_TYPE;
}

static int parseVersion ( )
{
	int seglen, size;
	tlvStructType tlv;

	if (request.index >= request.len) return ILLEGAL_LENGTH;
	if (parseTLV(request.buffer, request.index, &tlv) != SUCCESS ||
		request.buffer[tlv.start] != INTEGER ||
		request.buffer[tlv.vstart] != SNMP_V1) return ILLEGAL_DATA;
	seglen = tlv.nstart - tlv.start;
	COPY_SEGMENT(tlv);
	size = parseCommunity();
	if (size >= 0) return (size + seglen); else return size;
}

int parseSNMPMessage ( )
{
	int  seglen, size, respLoc;
	tlvStructType tlv;

	if (request.index >= request.len) return ILLEGAL_LENGTH;
	if ((size=parseTLV(request.buffer, request.index, &tlv)) != SUCCESS) return size;
	if (request.buffer[tlv.start] != SEQUENCE_OF) return ILLEGAL_DATA;
	seglen = tlv.vstart - tlv.start;
	respLoc = tlv.start;
	COPY_SEGMENT(tlv);
	size = parseVersion();
	if (size >= 0) return (size + insertRespLen(&request, tlv.start, &response, respLoc, size) + 1);
		else return size;
}

void setCheckCommunity ( Boolean (*func)(char *commstr, int reqtype) )
{
	checkCommunity = func;
}

#ifdef ARDUINO

uint32_t sysUpTime( void )  /* in hundredths of a second */
{
	uint32_t i;
	i = (uint32_t) millis();
	return ( i>>3 - i>>6 - i>>7 );  /* i/10, roughly 1/8 - 1/64 - 1/128 */
}

#ifdef ARDUINO_ETHERNET
#include "Dns.h"
int gethostaddr( char *hostname, IPAddress& ipaddr )
{
	DNSClient dns_client;

	dns_client.begin(dnsServer);
	return dns_client.getHostByName(hostname, ipaddr);
}
#else
int gethostaddr( char *hostname, IPAddress& ipaddr )
{
	return WiFi.hostByName(hostname, ipaddr);
}
#endif

int initSnmpAgent( int port, char *entoid, char *rocommstr, char *rwcommstr )
{
	endianness = endian();
	request.buffer = requestBuffer; request.size = REQUEST_BUFFER_SIZE;
	response.buffer = responseBuffer; response.size = RESPONSE_BUFFER_SIZE;
	enterpriseOID = entoid;
	roCommunity = rocommstr; rwCommunity = rwcommstr;
	mibTree = miblistnew(0);
#ifdef ARDUINO_ETHERNET
	Ethernet.begin(hostMacAddr, hostIpAddr, dnsServer, hostGateway, hostNetmask);
#else
	WiFi.mode(WIFI_STA);
	WiFi.config(hostIpAddr, dnsServer, hostGateway, hostNetmask);
	WiFi.begin(staSSID, staPSK);
	Serial.begin(9600);
	Serial.print("Connecting to AP...");
	while(WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}
	Serial.print("OK. Local IP address is "); Serial.println(WiFi.localIP());
#endif
	Udp.begin(port);
	return SUCCESS;
}

int processSNMP( void )
{
	if (Udp.parsePacket() < REQUEST_BUFFER_SIZE) {
		remoteIpAddr = Udp.remoteIP(); remotePort = Udp.remotePort();
		request.len = Udp.read(request.buffer, REQUEST_BUFFER_SIZE);
		if (request.len > 0) {
			request.index = 0;
			response.index = 0;
			response.len = parseSNMPMessage();
			if (response.len > 0) {
				response.index = response.len;
				Udp.beginPacket(remoteIpAddr, remotePort);
				Udp.write(response.buffer, response.index);
				Udp.endPacket();
			}
			return response.len;
		}
	}
	return FAIL;
}

#else

int gethostaddr( char *hostname, struct sockaddr_in *sin )
{
	struct hostent *he;
	char machine_name[ 32 ];

	if (hostname == NULL || hostname[0] == '\x00')
	{
		if (gethostname (machine_name, sizeof (machine_name)))
			return FAIL;
	}
	else
		strcpy(machine_name, hostname);
	he = gethostbyname (machine_name);
	if (he == NULL) return FAIL;
	memset (sin, 0, sizeof (struct sockaddr_in));
	sin->sin_family = AF_INET;
	memmove (&sin->sin_addr, he->h_addr_list[ 0 ], he->h_length);
	return SUCCESS;
}

uint32_t sysUpTime( void )	/* in hundredths of a second */
{
	static time_t startTime = 0;
	if (startTime == 0) {
		startTime = time(NULL);
		return 0;
	}
	else
		return (uint32_t)((time(NULL)-startTime)*100);
}

static int snmpfd;
int initSnmpAgent( int port, char *entoid, char *rocommstr, char *rwcommstr )
{
	struct sockaddr_in servaddr;
#ifdef _WIN32
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0) {
		fprintf( stderr, "WSAStartup() failed" );
		return FAIL;
	}
#endif

	sysUpTime();
	endianness = endian();
	request.buffer = requestBuffer; request.size = REQUEST_BUFFER_SIZE;
	response.buffer = responseBuffer; response.size = RESPONSE_BUFFER_SIZE;
	enterpriseOID = entoid;
	roCommunity = rocommstr; rwCommunity = rwcommstr;
	mibTree = miblistnew(0);
	gethostaddr( NULL, &servaddr ); strcpy( hostIpAddr, inet_ntoa(servaddr.sin_addr) );
	if (debug)
#ifdef _WIN32
		printf ("Local Windows system host address is %s\n", hostIpAddr);
#else
		printf ("Local system host address is %s\n", hostIpAddr);
#endif
	snmpfd = socket(PF_INET, SOCK_DGRAM, 0);
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = INADDR_ANY;
	servaddr.sin_port = htons(port);
	if (bind(snmpfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0 ) {
		if (debug)
#ifdef _WIN32
			printf("Unable to bind to UDP port %d during initialisation (%d).\n", port, WSAGetLastError());
#else
			printf("Unable to bind to UDP port %d during initialisation (%d).\n", port, errno);
#endif
		return FAIL;
	}
	else
		return SUCCESS;
}

int processSNMP( void )
{
#ifdef _WIN32
	int fromlen;
#else
	socklen_t fromlen;
#endif
	struct sockaddr from;
		
	fromlen = sizeof(from);
	request.len = recvfrom(snmpfd, request.buffer, REQUEST_BUFFER_SIZE, 0, &from, &fromlen);
	if (request.len > 0) {
		strcpy( remoteIpAddr, inet_ntoa(((struct sockaddr_in *)&from)->sin_addr));
		remotePort = ntohs(((struct sockaddr_in *)&from)->sin_port);
		if (debug) {
			printf("\nReceive %d bytes from %s:%u", request.len, remoteIpAddr, remotePort);
			showMessage(&request);
		}
		request.index = 0;
		response.index = 0;
		response.len = parseSNMPMessage();
		if (response.len > 0) {
			response.index = response.len;
			sendto(snmpfd, response.buffer, response.index, 0,
						 (struct sockaddr *)&from, fromlen);
			if (debug) {
				if (errorStatus==0) printf("Response:");
				else printf("Response with Error Status %u, Index %u:", errorStatus, errorIndex);
				showMessage(&response);
			}
		}
		else
			if (debug) printf("Parse fail! Error %d at byte position %d. Error Status %u, Index %u\n",
				response.len, request.index, errorStatus, errorIndex);
		return response.len;
	}
	else return FAIL;
}

void exitSnmpAgent( void )
{
#ifdef _WIN32
	closesocket(snmpfd);
	WSACleanup();
#else
	close(snmpfd);
#endif
	if ( mibTree != NULL ) miblistfree(mibTree);
}

#endif

/*
 * SNMP Trap Parsing and Operations
 */

/*
 * Parses a varbind string into the global response buffer and returns its length.
 */
int vblistParse(int reqType, struct messageStruct *vblist)
{
	vblist->index = 0;
	response.index = 0;

	response.len = parseSequenceOf ( reqType, vblist, &response );
	if (response.len >= 0 ) {
		memcopy(vblist->buffer, response.buffer, response.len);
		vblist->len = response.len;
	}
 	return response.len;
}

/*
 * Builds a trap and returns its length.
 */
#ifdef ARDUINO
int trapBuild(struct messageStruct *trap, char *entoid, IPAddress &agentaddr, int gen, int spec, struct messageStruct *vblist)
#else
int trapBuild(struct messageStruct *trap, char *entoid, char *agentaddr, int gen, int spec, struct messageStruct *vblist)
#endif
{
	uint32_t *p;

	/* PDU header */
	trap->buffer[0] = TRAP_PACKET;
	trap->buffer[1] = '\0';  /* Set length field to zero first */

	/* Enterprise OID */
	trap->buffer[2] = OBJECT_IDENTIFIER;
	trap->buffer[3] = str2ber(entoid, trap->buffer+4);  /* OID length field assumed to be of 1 byte */
	trap->index = 4 + trap->buffer[3];

	/* Agent IP address */
	trap->buffer[trap->index++] = IP_ADDRESS;
	trap->buffer[trap->index++] = 4;  /* IPv4 address length */
#ifdef ARDUINO
	trap->buffer[trap->index++] = agentaddr[0];
	trap->buffer[trap->index++] = agentaddr[1];
	trap->buffer[trap->index++] = agentaddr[2];
	trap->buffer[trap->index++] = agentaddr[3];
#else
	if (agentaddr == NULL)
		inet_aton(hostIpAddr, (struct in_addr *) (trap->buffer+trap->index));
	else
		inet_aton(agentaddr, (struct in_addr *) (trap->buffer+trap->index));
	trap->index += 4;
#endif

	/* Generic Trap ID */
	trap->buffer[trap->index++] = INTEGER;
	trap->buffer[trap->index++] = '\x01';  /* Genric Trap Code assumed to be of 1 byte */
	trap->buffer[trap->index++] = gen;

	/* Specific Trap ID */
	trap->buffer[trap->index++] = INTEGER;
	trap->buffer[trap->index++] = '\x01';  /* Specific Trap Code assumed to be of 1 byte */
	trap->buffer[trap->index++] = spec;

	/* Time stamp */
	trap->buffer[trap->index] = TIMETICKS;  /* Time stamp */
	trap->buffer[trap->index+1] = INT_SIZE;
	h2nl_byte(sysUpTime(), trap->buffer+trap->index+2);

	trap->index += (compactInt(trap->buffer+trap->index) + 2);

	/* Varbind list */
	if (vblist == NULL) {  /* No varbind list */
		trap->buffer[trap->index++] = SEQUENCE_OF;
		trap->buffer[trap->index++] = '\0';
		trap->len = trap->index - 2;  /* Subtract length of PDU header */
	}
	else {
		memcopy(trap->buffer+trap->index, vblist->buffer, vblist->len);
		trap->len = trap->index + vblist->len - 2; /* Subtract length of PDU header */
		}
	trap->len = ( 1 + insertRespLen(trap, 0, trap, 0, trap->len) + trap->len );
	return trap->len;
}

/*
 * Send a trap
 */
void trapSend(struct messageStruct *trap, char *dst, uint16_t port_no, char *comm_str)
{
#ifdef ARDUINO
	IPAddress dstAddr;
#else
	int snmpfd;
	struct sockaddr_in servaddr, to;
	char dstAddr[16];
#endif

	int len = strlen(comm_str);
	memcopy(trap->buffer+7+len, trap->buffer, trap->len);

	/* Packet header */
	trap->buffer[0] = SEQUENCE;
	trap->buffer[1] = '\0';  /* Set length field to zero first */

	/* Version 1 */
	trap->buffer[2] = INTEGER;
	trap->buffer[3] = 1;
	trap->buffer[4] = '\0';

	/* Community string */
	trap->buffer[5] = OCTET_STRING;
	trap->buffer[6] = len;
	memcopy(trap->buffer+7, (unsigned char *)comm_str, len);

	/* Varbind list */
	trap->index = 7 + len;
	trap->len = trap->index + trap->len - 2;  /* Subtract SEQUENCE TL field */
	trap->len = ( 1 + insertRespLen(trap, 0, trap, 0, trap->len) + trap->len );

#ifdef ARDUINO
		gethostaddr(dst, dstAddr);
		Udp.beginPacket(dstAddr, port_no);																								 
		Udp.write(trap->buffer, trap->len);
		Udp.endPacket();
}
#else
	snmpfd = socket(PF_INET, SOCK_DGRAM, 0);
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = INADDR_ANY;
	to.sin_family = AF_INET;
	if (gethostaddr(dst, &to) == SUCCESS) {
		to.sin_port = htons(port_no);
		for (servaddr.sin_port = htons(port_no);
			bind(snmpfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0;
			servaddr.sin_port = htons(++port_no));
		if (debug) {
			strcpy(dstAddr, inet_ntoa(to.sin_addr));
			printf("Send trap to %s from port %d:", dstAddr, port_no);
			showMessage(trap);
		}
		sendto(snmpfd, trap->buffer, trap->len, 0, (struct sockaddr *)&to, sizeof(to));
	}
#ifdef _WIN32
		closesocket(snmpfd);
#else
		close(snmpfd);
#endif
}
#endif

#ifdef _WIN32
/* $Id: inet_aton.c,v 2.0 2003/01/30 12:50:45 eric Exp $
 *
 *	This inet_aton() function was taken from the GNU C library and
 *	incorporated into Postgres for those systems which do not have this
 *	routine in their standard C libraries.
 *
 *	The function was been extracted whole from the file inet_aton.c in
 *	Release 5.3.12 of the Linux C library, which is derived from the
 *	GNU C library, by Bryan Henderson in October 1996.	The copyright
 *	notice from that file is below.
 */

/*
 * Copyright (c) 1983, 1990, 1993
 *		The Regents of the University of California.	All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *		notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *		notice, this list of conditions and the following disclaimer in the
 *		documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *		must display the following acknowledgement:
 *		This product includes software developed by the University of
 *		California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *		may be used to endorse or promote products derived from this software
 *		without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.	IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.  */

/*
 * Check whether "cp" is a valid ascii representation
 * of an Internet address and convert to a binary address.
 * Returns 1 if the address is valid, 0 if not.
 * This replaces inet_addr, the return value from which
 * cannot distinguish between failure and a local broadcast address.
 */  
int inet_aton(const char *cp, struct in_addr * addr)
 {
	unsigned int	val;
	int		base, n;
	char		c;
	unsigned int		parts[4];
	unsigned int		 *pp = parts;

	for (;;)
	{
		/*
		 * Collect number up to ``.''. Values are specified as for C:
		 * 0x=hex, 0=octal, other=decimal.
		 */
		val = 0;
		base = 10;
		if (*cp == '0')
		{
			if (*++cp == 'x' || *cp == 'X')
				base = 16, cp++;
			else
				base = 8;
		}
		while ((c = *cp) != '\0')
		{
			if (isdigit((unsigned char) c))
			{
				val = (val * base) + (c - '0');
				cp++;
				continue;
			}
			if (base == 16 && isxdigit((unsigned char) c))
			{
				val = (val << 4) +
					(c + 10 - (islower((unsigned char) c) ? 'a' : 'A'));
				cp++;
				continue;
			}
			break;
		}
		if (*cp == '.')
		{
			/*
			 * Internet format: a.b.c.d a.b.c	(with c treated as
			 * 16-bits) a.b		(with b treated as 24 bits)
			 */
			if (pp >= parts + 3 || val > 0xff)
				return 0;
			*pp++ = val, cp++;
		}
		else
			break;
	}

	/*
	 * Check for trailing junk.
	 */
	while (*cp)
		if (!isspace((unsigned char) *cp++))
			return 0;

	/*
	 * Concoct the address according to the number of parts specified.
	 */
	n = pp - parts + 1;
	switch (n)
	{

		case 1:			/* a -- 32 bits */
			break;

		case 2:			/* a.b -- 8.24 bits */
			if (val > 0xffffff)
				return 0;
			val |= parts[0] << 24;
			break;

		case 3:			/* a.b.c -- 8.8.16 bits */
			if (val > 0xffff)
				return 0;
			val |= (parts[0] << 24) | (parts[1] << 16);
			break;

		case 4:			/* a.b.c.d -- 8.8.8.8 bits */
			if (val > 0xff)
				return 0;
			val |= (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8);
			break;
	}
	if (addr)
		addr->s_addr = htonl(val);
	return 1;
} 
#endif
