/*
 * Defines space allocations, according to target processor.
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

#ifndef _USNMP_H
#define _USNMP_H

#define COMM_STR_SIZE 16
/* Allocated size in each MIB leaf to hold an octet string or OID */
#if defined(__AVR_ATmega328P__)
#define MIB_DATA_SIZE 32
#else
#define MIB_DATA_SIZE 128
#endif

/* OID array size; not too big that its TLV is > 128 bytes long.
   array[0] is a character to denote OID prefixes
     B denotes Mgmt-Mib2 - 1.3.6.1.2.1
     E denotes Experimental - 1.3.6.1.3
     P denotes Private-Enterprises - 1.3.6.1.4.1
     U denotes unknown, and a ill-formed OID
   Each subsequent array element corresponds to a dot-separated
   number in the OID. In systems of 16-bit integer, this number
   should not exceed 65535.
*/
#if defined(__AVR_ATmega328P__)
#define OID_SIZE 8
#else
#define OID_SIZE 16
#endif

/* Buffers to hold request and response packets, and a varbind pair. */
#if defined(__AVR_ATmega328P__)
#define REQUEST_BUFFER_SIZE 	96
#define RESPONSE_BUFFER_SIZE	128
#define VB_BUFFER_SIZE 32
#else
#define REQUEST_BUFFER_SIZE 	960
#define RESPONSE_BUFFER_SIZE	1280
#define VB_BUFFER_SIZE 256
#endif

#endif
