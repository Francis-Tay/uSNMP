/*
 * Data structures and functions to construct and traverse a sequence of
 * varaible bindings.
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

#ifndef _VARBIND_H
#define _VARBIND_H

#include "endian.h"
#include "mib.h"

#ifdef __cplusplus
extern "C" {
#endif 

struct messageStruct {
	unsigned char *buffer;
	int size;
	int len;
	int index;
};

typedef struct {
	int start;		/* Absolute Index of the TLV */
	int len;			/* The L value of the TLV */
	int vstart; 	/* Absolute Index of this TLV's Value */
	int nstart; 	/* Absolute Index of the next TLV */
} tlvStructType;

/* Computes the length field of a TLV and returns the size of this Length field. */
int parseLength(unsigned char *msg, int *len);

/* Given a length, builds the length field of a TLV and returns the size of this field. */
int buildLength(unsigned char *buffer, int len);

/* Given a request TLV and a new size, this inserts the size argument into the
   L element of the response TLV and returns the size of this length field. */
int insertRespLen(struct messageStruct *request, int reqStart, struct messageStruct *response, int respStart, int size);

/* Compacts BER-encoded integer and returns the compacted size. */ 
int compactInt(unsigned char *tlv);

/* Extracts integer value. */ 
uint32_t getValue(unsigned char *vptr, int vlen, unsigned char datatype);

/* Extracts a TLV from msg starting at index. Return Success(0) or error code (<0). */ 
int parseTLV(unsigned char *msg, int index, tlvStructType *tlv);

/* Resets a varbind list to empty. */
void vblistReset(struct messageStruct *vblist);

/* Adds a varbind into a varbind list. Returns the total length of the resultant list. */
int vblistAdd(struct messageStruct *vblist, char *oidstr, unsigned char dataType, void *val, int vlen );

/* Traverses a varbind list where opt=0 for first varbind, non-zero for next.
   Returns the nth order of the extracted varbind. */
int vblistGet(struct messageStruct *vblist, MIB *vb, unsigned char opt);

#ifdef __cplusplus
}
#endif

#endif
