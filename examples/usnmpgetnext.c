/*
 * A demo program to send a SNMPv1 GET-NEXT request, receive and display the
 * response.
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
#include "SnmpMgr.h"

void printHelp( char *prog )
{
	printf("Usage:\n");
	printf("%s [OPTIONS] TARGET OID...\n", prog);
	printf("Options: -i RequestID  default is 1\n");
	printf("         -c Community  default is 'public'\n");
	printf("         -p Port       default target port is 161\n");
	printf("         -t Seconds    default time-out is 2 seconds\n");
	printf("         -d            enables debug mode\n");
	printf("Prefix OID with B:Mgmt-Mib2(1.3.6.1.2.1), E:Experimental(1.3.6.1.3), P:Private(1.3.6.1.4.1)\n");
	printf("E.g. %s 192.168.1.252 -c public B.1.1.0 B.1.2.0 B.1.3.0\n", prog);
}

int main(int argc, char **argv)
{
	int c,	port = SNMP_PORT, timeout = 2; 
	char *target, *community="public", *oid;

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
		vblistAdd(&vblist, oid, NULL_ITEM, NULL, 0);
		optind++;
	}

	if (debug) {
		printf("Request varbind:\n");
		vblistPrint(&vblist, stdout);
	}
	reqBuild( &request, GET_NEXT_REQUEST, reqId, &vblist );
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
