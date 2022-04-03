/*
 * SNMP definitions
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

#ifndef SNMPDEFS_H
#define SNMPDEFS_H

/* Local ports to listen on */
#define SNMP_PORT         161 		 
#define TRAP_DST_PORT     162
#define SNMP_V1           0
#define GET_REQUEST       0xa0
#define GET_NEXT_REQUEST  0xa1
#define GET_RESPONSE      0xa2
#define SET_REQUEST       0xa3
#define TRAP_PACKET       0xa4

#define VALID_REQUEST(x)  ((x == GET_REQUEST) || \
                          (x == GET_NEXT_REQUEST) || \
                          (x == SET_REQUEST))

#define INTEGER           0x02
#define OCTET_STRING      0x04
#define NULL_ITEM         0x05
#define OBJECT_IDENTIFIER 0x06
#define SEQUENCE          0x30
#define SEQUENCE_OF       SEQUENCE
#define IP_ADDRESS        0x40
#define COUNTER           0x41
#define GAUGE             0x42
#define TIMETICKS         0x43
#define OPAQUE_TYPE       0x44

#define RD_ONLY           'R'
#define RD_WR             'W'

#define INT_SIZE        4           /* size of Integer, Gauge and Counter type */
#define MAX_INTEGER     2147483647  /* 2^31-1 */
#define MIN_INTEGER    -2147483648
#define MAX_COUNTER     4294967295  /* 2^32-1 */
#define MAX_GAUGE       4294967295
#define MAX_TIMETICKS   4294967295

/* SNMP Message Error Codes */
#define NO_ERR               0
#define TOO_BIG              1
#define NO_SUCH_NAME         2
#define BAD_VALUE            3
#define READ_ONLY            4
#define GEN_ERROR            5

/* SNMP Trap Generic Codes */
#define COLD_START           0
#define WARM_START           1
#define LINK_DOWN            2
#define LINK_UP              3
#define AUTHENTICATE_FAIL    4
#define EGPNEIGHBOR_LOSS     5
#define ENTERPRISE_SPECIFIC  6

/* SNMP Operations function return codes */
#define SUCCESS              0
#define FAIL                -1
#define OID_NOT_FOUND       -2
#define ILLEGAL_DATA        -3
#define ILLEGAL_LENGTH      -4
#define BUFFER_FULL         -5
#define INVALID_DATA_TYPE   -6
#define INVALID_PDU_TYPE    -7
#define REQ_ID_ERR          -8
#define ILLEGAL_ERR_STATUS  -9
#define ILLEGAL_ERR_INDEX   -10
#define RD_ONLY_ACCESS      -11
#define COMM_STR_MISMATCH   -12
#define COMM_STR_ERR        -13

#endif
