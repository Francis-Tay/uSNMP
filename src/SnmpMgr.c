/*
 * Implements core functions of of an uSNMP manager to
 * 1. build a SNMPv1 command
 * 2. send, receive and parse SNMPv1 packet
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#endif

#include "SnmpMgr.h"

int snmpfd;
struct sockaddr_in cliaddr;
char hostIpAddr[16], remoteIpAddr[16], remoteCommunity[COMM_STR_SIZE];
struct messageStruct request, response, vblist;
unsigned char errorStatus = 0, errorIndex = 0;
unsigned int reqId = 1;
unsigned char requestBuffer[REQUEST_BUFFER_SIZE], responseBuffer[RESPONSE_BUFFER_SIZE],
	vbBuffer[VB_BUFFER_SIZE];
Boolean debug = FALSE;

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

/* Initialise SNMP manager. Set port to 0 for ephemeral.
   Returns socket fd or -1 if fail. */
int initSnmpMgr( int port )
{
	struct sockaddr_in servaddr;
#ifdef _WIN32
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0) {
		fprintf( stderr, "WSAStartup() failed" );
		return -1;
	}
#endif

	endianness = endian();
	request.buffer = requestBuffer; request.size = REQUEST_BUFFER_SIZE;
	response.buffer = responseBuffer; response.size = RESPONSE_BUFFER_SIZE;
	vblist.buffer = vbBuffer; vblist.size = VB_BUFFER_SIZE;
	vblistReset(&vblist);
  gethostaddr( NULL, &servaddr ); inet_ntop(AF_INET, &(servaddr.sin_addr), hostIpAddr, 16);
	if (debug)
#ifdef _WIN32
		printf ("Local Windows system host address is %s\n", hostIpAddr);
#else
		printf ("Local system host address is %s\n", hostIpAddr);
#endif
	snmpfd = socket(PF_INET, SOCK_DGRAM, 0);
	cliaddr.sin_family = AF_INET;
	cliaddr.sin_addr.s_addr = INADDR_ANY;
	cliaddr.sin_port = htons(port);
	if (bind(snmpfd, (struct sockaddr *)&cliaddr, sizeof(cliaddr)) == 0)
		return snmpfd;
	else
		return -1;
}

void exitSnmpMgr( void )
{
#ifdef _WIN32
	closesocket(snmpfd);
	WSACleanup();
#else
	close(snmpfd);
#endif
}

/* Builds a request PDU and returns its constructed length. */
int reqBuild(struct messageStruct *req, unsigned char reqType, unsigned int reqId,
	struct messageStruct *vblist)
{
	uint32_t *p;

	/* PDU header */
	req->buffer[0] = reqType;
	req->buffer[1] = '\0';	/* Set length field to zero first */

	/* Request ID */
	req->buffer[2] = INTEGER;
	req->buffer[3] = INT_SIZE;
	p = (uint32_t *)(req->buffer+4);
	*p = h2nl((uint32_t)reqId);
	req->index = (compactInt(req->buffer+2) + 4);

	/* Error status */
	req->buffer[req->index++] = INTEGER;
	req->buffer[req->index++] = 1;
	req->buffer[req->index++] = 0;
	
	/* Error index */
	req->buffer[req->index++] = INTEGER;
	req->buffer[req->index++] = 1;
	req->buffer[req->index++] = 0;

	/* Varbind list */
	if (vblist == NULL) {  /* No varbind list */
		req->buffer[req->index++] = SEQUENCE_OF;
		req->buffer[req->index++] = '\0';
		req->len = req->index - 2;  /* Subtract length of PDU header */
	}
	else {
		memcopy(req->buffer+req->index, vblist->buffer, vblist->len);
		req->len = req->index + vblist->len - 2;  /* Subtract length of PDU header */
		}
	req->len = ( 1 + insertRespLen(req, 0, req, 0, req->len) + req->len );
	return req->len;
}

/* Sends a SNMP request and wait for a response. Returns Success(0) or Fail(-1). */
int reqSend(struct messageStruct *req, struct messageStruct *resp,
	char *dst, uint16_t port_no, char *comm_str, int time_out)
{
	struct sockaddr_in to;
	char dstAddr[16];
#ifdef _WIN32
	int fromlen;
#else
	socklen_t fromlen;
	fd_set readfds;
	struct timeval tv;
#endif
	struct sockaddr from;

	int len = strlen(comm_str);
	memcopy(req->buffer+7+len, req->buffer, req->len);

	/* Packet header */
	req->buffer[0] = SEQUENCE;
	req->buffer[1] = '\0';  /* Set length field to zero first */

	/* Version 1 */
	req->buffer[2] = INTEGER;
	req->buffer[3] = 1;
	req->buffer[4] = '\0';

	/* Community string */
	req->buffer[5] = OCTET_STRING;
	req->buffer[6] = len;
	memcopy(req->buffer+7, (unsigned char *)comm_str, len);

	/* Varbind list */
	req->index = 7 + len;
	req->len = req->index + req->len - 2;  /* Subtract SEQUENCE TL field */
	req->len = ( 1 + insertRespLen(req, 0, req, 0, req->len) + req->len );

	fromlen = sizeof(from);
	to.sin_family = AF_INET;
	if (gethostaddr(dst, &to)==SUCCESS) {
#ifdef _WIN32
		time_out *= 1000;
		setsockopt(snmpfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&time_out, sizeof(int));
#else
		FD_ZERO( &readfds );
		FD_SET ( snmpfd, &readfds );
		tv.tv_sec = time_out; tv.tv_usec = 0;
#endif
		to.sin_port = htons(port_no);
		if (debug) {
			getsockname(snmpfd, &from, &fromlen);
      inet_ntop(AF_INET, &(to.sin_addr), dstAddr, 16);
			printf("Send request to %s from port %u:", dstAddr, ntohs(((struct sockaddr_in *)&from)->sin_port));
			showMessage(req);
		}
		sendto(snmpfd, req->buffer, req->len, 0, (struct sockaddr *)&to, sizeof(to));
		resp->len = 0;
#ifdef _WIN32
#else
		if (select(snmpfd+1, &readfds, NULL, NULL, &tv) < 0) {
			if (debug) printf("On select() error.");
  		return FAIL;
		}
		if (FD_ISSET(snmpfd, &readfds))
#endif
		resp->len = recvfrom(snmpfd, resp->buffer, RESPONSE_BUFFER_SIZE, 0, &from, &fromlen);
		if (resp->len > 0) {
			if (debug) {
				printf("Response:");
				showMessage(resp);
			}
			return SUCCESS;
		}
		else if (debug) printf("No response.");
	}
	return FAIL;
}

/* Parses a SNMP response. Returns Success(0) or Fail(-1). */
int parseResponse(struct messageStruct *resp, char *comm_str, unsigned int *reqId,
	unsigned char *errorStatus, unsigned char *errorIndex, struct messageStruct *vblist)
{
	tlvStructType tlv;

	if (parseTLV(resp->buffer, 0, &tlv) != SUCCESS ||
		resp->buffer[tlv.start] != SEQUENCE_OF)
		return FAIL;
	/* SNMP version */
	if (parseTLV(resp->buffer, tlv.nstart, &tlv) !=SUCCESS ||
		resp->buffer[tlv.start] != INTEGER || 
		tlv.len != 1 || 
		resp->buffer[tlv.vstart] != 0) 
		return FAIL;
	/* Community string */
	if (parseTLV(resp->buffer, tlv.nstart, &tlv) !=SUCCESS ||
		resp->buffer[tlv.start] != OCTET_STRING) 
		return FAIL;
	else {
		memcopy((unsigned char *)comm_str, resp->buffer+tlv.vstart, tlv.len);
		comm_str[tlv.len] = 0;
	}
	if (parseTLV(resp->buffer, tlv.nstart, &tlv) != SUCCESS ||
		resp->buffer[tlv.start] != GET_RESPONSE)
		return FAIL;
	/* Request ID */
	if (parseTLV(resp->buffer, tlv.nstart, &tlv) !=SUCCESS ||
		resp->buffer[tlv.start] != INTEGER)
		return FAIL;
	else
		*reqId = getValue(resp->buffer+tlv.vstart, tlv.len, INTEGER);
	/* Error status */
	if (parseTLV(resp->buffer, tlv.nstart, &tlv) !=SUCCESS ||
		resp->buffer[tlv.start] != INTEGER)
		return FAIL;
	else
		*errorStatus = getValue(resp->buffer+tlv.vstart, tlv.len, INTEGER);
	/* Error index */
	if (parseTLV(resp->buffer, tlv.nstart, &tlv) !=SUCCESS ||
		resp->buffer[tlv.start] != INTEGER)
		return FAIL;
	else
		*errorIndex = getValue(resp->buffer+tlv.vstart, tlv.len, INTEGER);
	/* Varbind list */ 
	if (parseTLV(resp->buffer, tlv.nstart, &tlv) !=SUCCESS ||
		resp->buffer[tlv.start] != SEQUENCE_OF)
		return FAIL;
	else {
		vblist->index = 0;
		vblist->len = tlv.vstart - tlv.start + tlv.len;
		memcopy(vblist->buffer, resp->buffer+tlv.start, vblist->len);
	}
	return SUCCESS;
}

/* Parses a SNMP trap. Returns Success(0) or Fail(-1). */
int parseTrap(struct messageStruct *resp, char *comm_str, OID *entoid,
	char *agentaddr, unsigned int *gen, unsigned int *spec, unsigned int *timestamp,
	struct messageStruct *vblist)
{
	tlvStructType tlv;

	if (parseTLV(resp->buffer, 0, &tlv) != SUCCESS ||
		resp->buffer[tlv.start] != SEQUENCE_OF)
		return FAIL;
	/* SNMP version */
	if (parseTLV(resp->buffer, tlv.nstart, &tlv) !=SUCCESS ||
		resp->buffer[tlv.start] != INTEGER || 
		tlv.len != 1 || 
		resp->buffer[tlv.vstart] != 0) 
		return FAIL;
	/* Community string */
	if (parseTLV(resp->buffer, tlv.nstart, &tlv) !=SUCCESS ||
		resp->buffer[tlv.start] != OCTET_STRING) 
		return FAIL;
	else {
		memcopy((unsigned char *)comm_str, resp->buffer+tlv.vstart, tlv.len);
		comm_str[tlv.len] = 0;
	}
	if (parseTLV(resp->buffer, tlv.nstart, &tlv) != SUCCESS ||
		resp->buffer[tlv.start] != TRAP_PACKET)
		return FAIL;
	/* Enterprise OID */
	if (parseTLV(resp->buffer, tlv.nstart, &tlv) !=SUCCESS ||
		resp->buffer[tlv.start] != OBJECT_IDENTIFIER)
		return FAIL;
	else
		ber2oid(resp->buffer+tlv.vstart, tlv.len, entoid);
	/* Agent IP Address */
	if (parseTLV(resp->buffer, tlv.nstart, &tlv) !=SUCCESS ||
		resp->buffer[tlv.start] != IP_ADDRESS || tlv.len != 4)
		return FAIL;
	else {
		sprintf(agentaddr, "%d.%d.%d.%d", resp->buffer[tlv.vstart],
			resp->buffer[tlv.vstart+1], resp->buffer[tlv.vstart+2],
			resp->buffer[tlv.vstart+3]);
	}
	/* Generic trap number */
	if (parseTLV(resp->buffer, tlv.nstart, &tlv) !=SUCCESS ||
		resp->buffer[tlv.start] != INTEGER)
		return FAIL;
	else
		*gen = getValue(resp->buffer+tlv.vstart, tlv.len, INTEGER);
	/* Specific trap number list */ 
	if (parseTLV(resp->buffer, tlv.nstart, &tlv) !=SUCCESS ||
		resp->buffer[tlv.start] != INTEGER)
		return FAIL;
	else
		*spec = getValue(resp->buffer+tlv.vstart, tlv.len, INTEGER);
	/* Time stamp */
	if (parseTLV(resp->buffer, tlv.nstart, &tlv) !=SUCCESS ||
		resp->buffer[tlv.start] != TIMETICKS)
		return FAIL;
	else
		*timestamp = getValue(resp->buffer+tlv.vstart, tlv.len, TIMETICKS);
	/* Varbind list */
	if (parseTLV(resp->buffer, tlv.nstart, &tlv) !=SUCCESS ||
		resp->buffer[tlv.start] != SEQUENCE_OF)
		return FAIL;
	else {
		vblist->index = 0;
		vblist->len = tlv.vstart - tlv.start + tlv.len;
		memcopy(vblist->buffer, resp->buffer+tlv.start, vblist->len);
	}
	return SUCCESS;
}
