/*
 * A SNMP agent that reads MIB definition and value from a file.
 * Illustrate how to use the uSNMP library to build a SNMPv1 agent, MIB tree,
 * read and write to it using dynamic functions.
 * This can also be used as a SNMPv1 gateway by having a poller program formats
 * and writes its received data to this file.
 *
 * This file is part of uSnmp ("micro-SNMP").
 * uSnmpAgent is released under a BSD-style license. The full text follows.
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
#include <string.h>
#ifdef _WIN32
#include "wingetopt.h"
#else
#include <unistd.h>
#endif
#include "SnmpAgent.h"
#include "keylist.h"
#include "timer.h"

char *cfg_file = "usnmpd.cfg", *dat_file = "usnmpd.dat";
void initMibTree( void );
void timerHandler( void );
Boolean noAuth = FALSE;
Boolean checkCommStr(char *cstr, int reqType);
void trapSend2(struct messageStruct *trap, char *fn);

void printHelp( char *prog )
{
	printf("Usage:\n");
	printf("%s [OPTIONS] ENTERPRISE-OID\n", prog);
	printf("Options: -p Port  default listening port is 161\n");
	printf("         -c File  default configuration file is usnmpd.cfg\n");
	printf("         -f File  default MIB definition and data file is usnmpd.dat\n");
	printf("         -a- do not authenticate community string\n");
	printf("         -d turn on debug mode\n");
	printf("Prefix OID with B:Mgmt-Mib2(1.3.6.1.2.1), E:Experimental(1.3.6.1.3), P:Private(1.3.6.1.4.1)\n");
	printf("E.g. %s P.38644.30 -f usnmpd.dat\n", prog);
}

int main(int argc, char *argv[])
{
	int c, port = SNMP_PORT;

	if ( argc < 2) {
		printHelp( argv[0] );
		return FAIL;
	}

	optind = 1;
	while ((c = getopt (argc, argv, "p:c:f:ad")) != -1)
		switch (c) {
			case 'p':
				port = atoi(optarg);
				break;
			case 'c':
				cfg_file = optarg;
				break;
			case 'f':
				dat_file = optarg;
				break;
			case 'a':
				if ( argv[optind][2]=='-' ) noAuth = TRUE;
				break;
			case 'd':
				debug = TRUE;
				break;
			case 'h':
				printHelp( argv[0] );
			default:
				return FAIL;
		}

	if ( initSnmpAgent(port, argv[1], "public", "private") == FAIL ) {
		printf("Fail to initialise agent.\n");
		return FAIL;
	}
	else {
		initMibTree();
		timer_start(1000, timerHandler);  /* timer function to update MIB values */
		trapBuild(&request, enterpriseOID, NULL, COLD_START, 0, NULL);
		if (debug) printf("Coldstart. ");
		trapSend2(&request, cfg_file);
		setCheckCommunity(checkCommStr);
		printf("Entering loop...\n");
		for ( ; ; )
			if ( processSNMP() == COMM_STR_MISMATCH ) {
				trapBuild(&request, enterpriseOID, NULL, AUTHENTICATE_FAIL, 0, NULL);
				if (debug) printf("Authentication failure. ");
				trapSend2(&request, cfg_file);
			}
		exitSnmpAgent();
		return SUCCESS;
	}
}

/* Community string checker function */
Boolean checkCommStr(char *cstr, int reqType)
{
	char roCommunity[COMM_STR_SIZE], rwCommunity[COMM_STR_SIZE];
	LIST *keylist; KEY *key;

	if (noAuth) return TRUE;
	keylist = keylistnew();
	keylistread(keylist, cfg_file);
	keylistgohead(keylist);
	if ( (key=keylistgokey(keylist, remoteIpAddr)) ||
		(key=keylistgokey(keylist, "0.0.0.0")) )
		sscanf(key->val, "%[^,],%[^,],%*s", roCommunity, rwCommunity);
	keylistfree(keylist);
	if ( key && ( strcmp(cstr, rwCommunity)==0 ||
		   (reqType!=SET_REQUEST && strcmp(cstr, roCommunity)==0) ) )
		return TRUE;
	else
		return FALSE;
}

/* Send trap to all managers listed in the agent config file */
void trapSend2(struct messageStruct *trap, char *fn )
{
	char trapCommunity[COMM_STR_SIZE];
	LIST *keylist; KEY *key;

	keylist = keylistnew();
	keylistread(keylist, fn);
	keylistgohead(keylist);
	while ((key=keylistgetthis(keylist))!=NULL) {
		sscanf(key->val, "%*[^,],%*[^,],%s", trapCommunity);
		if (strcmp(key->key, "0.0.0.0")!=0)
			trapSend(trap, key->key, TRAP_DST_PORT, trapCommunity);
		keylistgonext(keylist);
	}
	keylistfree(keylist);
}

/* MIB data callback functions */
int get_uptime(MIB *thismib)
{
	thismib->u.intval = sysUpTime();
	thismib->dataLen = INT_SIZE;
	return SUCCESS;
}

int set(MIB *thismib, void *ptr, int len)
{
	mibsetvalue(thismib, ptr, len);
	miblistwrite(mibTree, dat_file);
	return SUCCESS;
}

/* Timer function to update MIB values from file periodically */
void timerHandler( void )
{
	miblistread(mibTree, dat_file);
}

/* MIB initialization */
void initMibTree( void )
{
	MIB *thismib;
	OID sysUptime = { 4, { 'B', 1, 3, 0 } };
	
	if (miblistread(mibTree, dat_file)==SUCCESS) {
		miblistprint(mibTree, stdout);
		thismib = miblistgohead(mibTree);
		while (thismib) {
			if (thismib->access=='W')
				mibsetcallback(thismib, NULL, set);
			thismib = miblistgonext(mibTree);
		}
		if ((thismib=miblistgooid(mibTree, &sysUptime)))
			mibsetcallback(thismib, get_uptime, NULL);
	}
}
