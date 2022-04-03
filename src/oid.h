/*
 * Defines the OID data structure and functions to converts OID notations.
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

#ifndef _OID_H
#define _OID_H

#include "usnmp.h"
#include "misc.h"

#ifdef __cplusplus
extern "C" {
#endif 

/* OID array size; not too big that its TLV is > 128 bytes long.
   array[0] is a character to denote OID prefixes
     B denotes Mgmt-Mib2 - 1.3.6.1.2.1
     E denotes Experimental - 1.3.6.1.3
     P denotes Private-Enterprises - 1.3.6.1.4.1
     U denotes unknown, and a ill-formed OID
   Each subsequent array element corresponds to a dot-separated
   number in the OID. In systems of 16-bit integer, this number
   should not exceed 65535. This should not usually be a concern
   until the Private Enterprise Number exceeds that.
*/
typedef struct {
	unsigned char len;
	unsigned int array[OID_SIZE];
} OID;

/* Converts string to OID arrary, returns length of array. */
int str2oid(char *str, OID *oid);

/* Converts OID arrary to BER, returns length of encoded BER string. */
int oid2ber(OID *oid, unsigned char *str);

/* Converts BER to OID arrary, returns length of array. */
int ber2oid(unsigned char *str, int len, OID *oid);

/* Converts string to BER, returns length of BER-encoded string. */
int str2ber(char *str, unsigned char *ber);

/* Compares two OID arrays, return 0 if equal, >0 if oid1>oid2, <0 if oid1<oid2 */
int oidcmp(OID *oid1, OID *oid2);

#ifndef ARDUINO
/* Converts OID arrary to string, returns length of string. */
int oid2str(OID *oid, char *str);

/* Converts BER to string, returns length of string. */
int ber2str(unsigned char *ber, int len, char *str);
#endif

#ifdef __cplusplus
}
#endif

#endif
