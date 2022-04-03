/*
 * A demo program to send a SNMPv1 trap.
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
#endif
#include "SnmpAgent.h"

void printHelp( char *prog )
{
	printf("Usage:\n");
	printf("%s [OPTIONS] TARGET ENTERPRISE-OID GENERIC-CODE SPECIFIC-CODE [OID TYPE VALUE]...\n", prog);
	printf("Options: -a Agent Address  default is local host address\n");
	printf("         -c Community      default is 'public'\n");
	printf("         -p Port           default destinaion port is 162\n");
	printf("         -d enables debug mode\n");
	printf("Prefix OID with B:Mgmt-Mib2(1.3.6.1.2.1), E:Experimental(1.3.6.1.3), P:Private(1.3.6.1.4.1)\n");
	printf("Type is O:OID, S:DisplayString, X:OctetString, I:Integer, T:Timeticks, C:Counter, G:Gauge\n");
	printf("E.g. %s 192.168.1.252 P.38644.22.11 3 0 -c public -a 192.168.1.170 B.2.2.1.1.18 I 18\n", prog);
}

int main(int argc, char **argv)
{
	struct messageStruct trap, vblist;
	unsigned char trapBuffer[REQUEST_BUFFER_SIZE], vbBuffer[VB_BUFFER_SIZE];
	int gen, spec, c, port = TRAP_DST_PORT;
	char *target, *enterpriseOID, *oid, *community = "public", *agentaddr = NULL;
	void *val;

	endianness = endian();
	sysUpTime();
	trap.buffer = trapBuffer; trap.size = REQUEST_BUFFER_SIZE;
	vblist.buffer = vbBuffer; vblist.size = VB_BUFFER_SIZE;
	vblistReset(&vblist);

	optind = 1;
	while ((c = getopt (argc, argv, "a:c:p:dh")) != -1)
		switch (c) {
			case 'a':
				agentaddr = optarg;
				break;
			case 'c':
				community = optarg;
				break;
			case 'p':
				port = atoi(optarg);
				break;
			case 'd':
				debug = TRUE;
				break;
			default:
				printHelp( argv[0] );
				return -1;
		}
	if ( optind+4 >= argc) {
		printHelp( argv[0] );
		return -1;
	}

	target = argv[optind++];
	enterpriseOID = argv[optind++];
	gen = atoi(argv[optind++]);
	spec = atoi(argv[optind++]);
#ifdef _WIN32
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0) {
		fprintf( stderr, "WSAStartup() failed" );
		return -1;
	}
#endif

	while ( optind < argc ) {
		oid = argv[optind];
		if (optind+2 >= argc) {	printHelp(argv[0]); return -1; }
		switch( *argv[++optind] ) {
			case 'O':
			case 'o':
				val = (void *) argv[++optind];
				vblistAdd(&vblist, oid, OBJECT_IDENTIFIER, val, 0);
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

	if (agentaddr == NULL) {
		if (target[0]=='1' && target[1]=='2' && target[2]=='7')
			trapBuild(&trap, enterpriseOID, target, gen, spec, &vblist);
		else
			trapBuild(&trap, enterpriseOID, hostIpAddr, gen, spec, &vblist);
	}
	else
			trapBuild(&trap, enterpriseOID, agentaddr, gen, spec, &vblist);
	if (debug) {
		printf("Trap varbind:\n");
		vblistPrint(&vblist, stdout);
	}
	trapSend( &trap, target, port, community );
#ifdef _WIN32
	WSACleanup();
#endif
	return 0;
}
