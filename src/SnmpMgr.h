/*
 * Implements core functions of an uSNMP manager to
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

#ifndef SNMPMGR_H
#define SNMPMGR_H

#include "mibutil.h"

#ifdef __cplusplus
extern "C" {
#endif 

extern char hostIpAddr[], remoteIpAddr[], remoteCommunity[];
extern struct messageStruct request, response, vblist;
extern unsigned char requestBuffer[], responseBuffer[], vbBuffer[];
extern unsigned char errorStatus, errorIndex;
extern unsigned int reqId;
extern Boolean debug;

/* Initialise SNMP manager to listen at port. Returns socket fd or -1 if fail. */
int initSnmpMgr( int port );

void exitSnmpMgr( void );

/* Builds a request PDU and returns its constructed length. */
int reqBuild(struct messageStruct *req, unsigned char reqType, unsigned int reqId,
	struct messageStruct *vblist);

/* Sends a SNMP request and wait for a response. Returns Success(0) or Fail(-1). */
int reqSend(struct messageStruct *req, struct messageStruct *resp,
	char *dst, uint16_t port_no, char *comm_str, int time_out);

/* Parses a SNMP response. Returns Success(0) or Fail(-1). */
int parseResponse(struct messageStruct *resp, char *comm_str, unsigned int *reqId,
	unsigned char *errorStatus, unsigned char *errorIndex, struct messageStruct *vblist);

/* Parses a SNMP trap. Returns Success(0) or Fail(-1). */
int parseTrap(struct messageStruct *resp, char *comm_str, OID *entoid, char *agentaddr,
	unsigned int *gen, unsigned int *spec, unsigned int *timestamp, struct messageStruct *vblist);
	
#ifdef __cplusplus
}
#endif

#endif
