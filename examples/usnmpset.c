/*
 * A demo program to send a SNMPv1 SET request, receive and display the response.
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
#include <winsock2.h>
#include <ws2tcpip.h>
#include "wingetopt.h"
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif
#include "SnmpMgr.h"

void printHelp( char *prog )
{
	printf("Usage:\n");
	printf("%s [OPTIONS] TARGET [OID TYPE VALUE]...\n", prog);
	printf("Options: -i RequestID  default is 1\n");
	printf("         -c Community  default is 'private'\n");
	printf("         -p Port       default target port is 161\n");
	printf("         -t Seconds    default time-out is 2 seconds\n");
	printf("         -d enables debug mode\n");
	printf("Prefix OID with B:Mgmt-Mib2(1.3.6.1.2.1), E:Experimental(1.3.6.1.3), P:Private(1.3.6.1.4.1)\n");
	printf("Type is O:OID, S:DisplayString, X:OctetString, A:IpAddress, I:Integer, T:Timeticks, C:Counter, G:Gauge\n");
	printf("E.g. %s 192.168.1.252 -c public B.1.6.0 S 18thFloor\n", prog);
}

int main(int argc, char **argv)
{
	int c,	port = SNMP_PORT, timeout = 2;
	char *target, *community="private", *oid;
	void *val;

	optind = 1;
	while ((c = getopt (argc, argv, "i:c:p:t:dh")) != -1)
		switch (c) {
			case 'i':
				reqId = atoi(optarg);
				break;
			case 'c':
				community = optarg;
				break;
			case 'p':
				port = atoi(optarg);
				break;
			case 't':
				timeout = atoi(optarg);
				break;
			case 'd':
				debug = TRUE;
				break;
			default:
				printHelp( argv[0] );
				return -1;
		}
	if ( optind >= argc) {
		printHelp( argv[0] );
		return -1;
	}

	target = argv[optind++];
	initSnmpMgr( 0 );  /* use an ephemeral port */
	while ( optind < argc ) {
		oid = argv[optind];
		if (optind+2 >= argc) {	printHelp(argv[0]); return -1; }
		switch( *argv[++optind] ) {
			case 'O':
			case 'o':
				vblistAdd(&vblist, oid, OBJECT_IDENTIFIER, argv[++optind], 0);
				break;
			case 'S':
			case 's':
				val = (void *) argv[++optind];
				vblistAdd(&vblist, oid, OCTET_STRING, val, strlen(val));
				break;
			case 'X':
			case 'x':
				c = str2oct(argv[++optind], requestBuffer);
				vblistAdd(&vblist, oid, OCTET_STRING, requestBuffer, c);
				break;
			case 'A':
			case 'a':
        inet_pton(AF_INET, argv[++optind], (struct in_addr *)requestBuffer);
				vblistAdd(&vblist, oid, IP_ADDRESS, requestBuffer, 4);
				break;
			case 'I':
			case 'i':
				c = atoi(argv[++optind]);
				vblistAdd(&vblist, oid, INTEGER, &c, 4);
				break;
			case 'T':
			case 't':
				c = atoi(argv[++optind]);
				vblistAdd(&vblist, oid, TIMETICKS, &c, 4);
				break;
			case 'C':
			case 'c':
				c = atoi(argv[++optind]);
				vblistAdd(&vblist, oid, COUNTER, &c, 4);
				break;
			case 'G':
			case 'g':
				c = atoi(argv[++optind]);
				vblistAdd(&vblist, oid, GAUGE, &c, 4);
				break;
			default:
				printf("Wrong data type.");
				return -1;
		}
		optind++;
	}

	if (debug) {
		printf("Request varbind:\n");
		vblistPrint(&vblist, stdout);
	}
	reqBuild( &request, SET_REQUEST, reqId, &vblist );
	if (reqSend( &request, &response, target, port, community, timeout )==SUCCESS &&
		parseResponse(&response, remoteCommunity, &reqId, &errorStatus, &errorIndex, &vblist)==SUCCESS) {
		if (errorStatus != 0)
			printf("ErrorStatus:%u, ErrorIndex:%u\n", errorStatus, errorIndex);
		else
			vblistPrint(&vblist, stdout);
	}
	else
		printf("Fail!\n");
	exitSnmpMgr();
	return 0;
}
