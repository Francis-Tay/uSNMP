/*
 * A demo program to receive a SNMPv1 trap, and display its content.
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
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#ifdef _WIN32
#include <winsock.h>
#include "wingetopt.h"
#else
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netdb.h>
#include <sys/un.h>
#endif
#include "SnmpMgr.h"

void printHelp( char *prog )
{
	printf("Usage:\n");
	printf("%s [OPTIONS]\n", prog);
	printf("Options: -p Port  default listening port is 162\n");
	printf("         -d       enables debug mode\n");
}

int main(int argc, char **argv)
{
	int c, port = TRAP_DST_PORT, snmpfd;
	unsigned int gen, spec, remotePort, timestamp;
	OID entoid;
	char agentaddr[32], remoteIpAddr[32], oid[64];
#ifdef _WIN32
	int fromlen;
#else
	socklen_t fromlen;
#endif
	struct sockaddr from;

	optind = 1;
	while ((c = getopt (argc, argv, "p:dh")) != -1)
		switch (c) {
			case 'p':
				port = atoi(optarg);
				break;
			case 'd':
				debug = TRUE;
				break;
			case 'h':
				printHelp( argv[0] );
			default:
				return -1;
		}

	snmpfd = initSnmpMgr( port );
	fromlen = sizeof(from);
	while ( (response.len = recvfrom(snmpfd, response.buffer, RESPONSE_BUFFER_SIZE,
		0, &from, &fromlen)) ) {
		if (debug) {
			strcpy( remoteIpAddr, inet_ntoa(((struct sockaddr_in *)&from)->sin_addr));
			remotePort = ntohs(((struct sockaddr_in *)&from)->sin_port);
			if (debug) printf("Receive trap from %s, port %u\n",
				remoteIpAddr, remotePort);
			printf("Trap:");
			showMessage(&response);
		}
		if ( parseTrap(&response, remoteCommunity, &entoid, agentaddr, &gen, &spec,
			&timestamp, &vblist) == SUCCESS) {
			printf("Community: %s\n", remoteCommunity);
			oid2str(&entoid, oid); printf("Enterprise OID: %s\n", oid);
			printf("Agent address: %s\n", agentaddr);
			printf("Generic trap code: %u\n", gen);
			printf("Specific trap code: %u\n", spec);
			printf("Timestamp: %u\n", timestamp);
			printf("Trap varbind:\n");
			vblistPrint(&vblist, stdout);
		}
		else
			printf("Trap parse fail!\n");
	}
	return 0;
}
