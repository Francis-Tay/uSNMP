/*
 * Implements core functions of uSnmpAgent to
 * 1. initialise, traverse and access a MIB tree
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

#ifndef SNMPAGENT_H
#define SNMPAGENT_H

#ifdef ARDUINO
 #if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega2560__)
  #define ARDUINO_ETHERNET    // ...assume Ethernet boards
  #include <SPI.h>
  #include <Ethernet.h>
  #include <EthernetUdp.h>
 #else
  #define ARDUINO_WIFI
  #ifdef ESP8266
   #include <ESP8266WiFi.h>
   #include <WiFiUdp.h>
  #endif
  // ... add other WIFI boards here
 #endif
#else
#include "mibutil.h"
#include <time.h>
#endif

#include "miblist.h"
#include "varbind.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Global variables */
#ifdef ARDUINO
extern IPAddress remoteIpAddr;
#else
extern char hostIpAddr[], remoteIpAddr[];
#endif
extern uint16_t remotePort;
extern char *enterpriseOID, *roCommunity, *rwCommunity, remoteCommunity[];
extern Boolean (*checkCommnuity)(char *commstr, int reqType);

extern LIST *mibTree; 		// Holds the MIB tree for this agent
extern struct messageStruct request, response;
extern unsigned char requestBuffer[], responseBuffer[];
extern unsigned char errorStatus, errorIndex;
extern Boolean debug;

/* Prototypes */

/* Initialise SNMP agent to listen at port. Returns Success(0) or Fail(-1). The
   pointers to entoid and the community strings are copied to the global
   variables enterpriseOID, roCommunity, rwCommunity and trapCommunity
   respectively (not the contents they point to). */
int initSnmpAgent( int port, char *entoid, char *rocommstr, char *rwcommstr );

/* Sets the function used to validate the requester's community string. The
   agent validates it against rwCommunity by default.
   remoteCommunity holds the requester's community string, and is useful for
   implementing a multiplex agent. */
void setCheckCommunity ( Boolean (*func)(char *commstr, int reqtype) );

/* Process request and construct the response. Returns response length or Fail(-1). */
int processSNMP( void );

void exitSnmpAgent( void );

uint32_t sysUpTime( void );

/* Parses a varbind string into the global response buffer and returns its length. */
int vblistParse(int reqType, struct messageStruct *vblist);

/* Builds a trap and returns its length. */
#ifdef ARDUINO
int trapBuild(struct messageStruct *trap, char *entoid, IPAddress &agentaddr, int gen, int spec, struct messageStruct *vblist);
#else
int trapBuild(struct messageStruct *trap, char *entoid, char *agentaddr, int gen, int spec, struct messageStruct *vblist);
#endif

void trapSend(struct messageStruct *trap, char *dst, uint16_t port_no, char *commstr);

#ifdef __cplusplus
}
#endif

#endif
