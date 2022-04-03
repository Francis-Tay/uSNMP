/*
 * Data strructure and functions to manage a MIB leaf.
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

#ifndef _MIB_H
#define _MIB_H

#include <stdint.h>
#include "snmpdefs.h"
#include "oid.h"

#ifdef __cplusplus
extern "C" {
#endif 

typedef struct mib {
	OID oid;
	unsigned char dataType;
	int dataLen;
	union {
		unsigned char *octetstring;
		uint32_t intval;
	} u;
	char access;
	int (*get)(struct mib *);
	int (*set)(struct mib *, void *, int);
} MIB;

/* Octet String and OID are copied as-is, assumed as octet and BER-encoded
   respectively. Set size=0 for numeric values. */
void mibsetvalue(MIB *thismib, void *u, int size);

/* (*get)() is expected to compute or fetch, then fill in the new data in *mib.
   (*set)() should actuate *data, then change the data in *mib.
   Both return a SNMP Operations function return codes defined in snmpdefs.h
*/
void mibsetcallback(MIB *thismib, int (*get)(MIB *mib), int (*set)(MIB *mib, void *data, int len));

#ifdef __cplusplus
}
#endif

#endif
