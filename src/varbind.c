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

#include <string.h>
#include "varbind.h"

/* Computes the length field of a TLV and returns the size of this Length field. */
int parseLength(unsigned char *msg, int *len)
{
	int i, tlen=1;

	if (msg[0] & '\x80') {
		i = (msg[0] & '\x7F') - 1;
		*len = msg[tlen++];
		while (i--) {
			*len <<= 8;
			*len |= msg[tlen++];
		}
	} else {
		*len = msg[0];
	}
	return tlen;  /* Size of the Length field in the TLV */
}

/* Given a length, builds the length field of a TLV and returns the size of this field. */
int buildLength(unsigned char *buffer, int len)
{
	int tlen;
	
	if (len < 0x80) {
		tlen = 1;
		*buffer = (unsigned char)len;
	}
	else if (len <= 0xFF) { /* 0x80 <= length <= 0xFF */
		tlen = 2;
		*buffer++ = (unsigned char)('\x81');
		*buffer = (unsigned char)(len & '\xFF');
	}
	else if (len <= 0xFFFF) { /* 0xFF < length <= 0xFFFF */
		tlen = 3;
		*buffer++ = (unsigned char)('\x82');
		*buffer++ = (unsigned char)((len >> 8) & '\xFF');
		*buffer = (unsigned char)(len & '\xFF');
	}
	return tlen;
}

/* Given a request TLV and a new size, this inserts the size argument into the
   L element of the response TLV and returns the size of this length field. */
int insertRespLen(struct messageStruct *request, int reqStart, struct messageStruct *response, int respStart, int size)
{
	int clen, tlen;
	unsigned char l[3];  /* Cater for 2 bytes, equivalent to length of 0xFFFF */

	if (request->buffer[reqStart+1] & '\x80')
		clen = (request->buffer[reqStart+1] & '\x7F') + 1;
	else
		clen = 1;
	tlen = buildLength(l, size);

	if (clen != tlen)
		memcopy(response->buffer+respStart+1+tlen, response->buffer+respStart+1+clen, size);
	memcopy(response->buffer+respStart+1, l, tlen);
	return tlen;
}

/* Compacts BER-encoded integer and returns the compacted size. */ 
int compactInt(unsigned char *tlv)
{
	int len;
	unsigned char *v;
	len = *(tlv + 1);
	v = tlv + 2;  /* Assume Length field of 1 byte */

	switch(tlv[0]) {
		case INTEGER :
			/* Compact encoding by reducing leading 1's and 0's */
			if (*v == '\xFF')
				while (len > 1 && *v == '\xFF' && ((*(v+1) & '\x80') == '\x80')) {
					v++;
					len--;
				}
			else
				if (*v == '\x00')
					while (len > 1 && *v == '\x00' && ((*(v+1) & '\x80') == '\x00')) {
						v++;
						len--;
					}
			tlv[1] = (char) len;
			memcopy(tlv+2, v, len);
			break;
		case TIMETICKS :
		case COUNTER :
		case GAUGE :
			/* Add a leading 0 if first bit is a 1 */
			if ((*v & '\x80') == '\x80') {
				memcopy(v+1, v, len);
				*v = '\x00';
				len++;
			}
			else {
				/* Compact encoding by reducing leading 0's */
				if (*v == '\x00')
					while (len > 1 && *v == '\x00' && ((*(v+1) & '\x80') == '\x00')) {
						v++;
						len--;
					}
				memcopy(tlv+2, v, len);
			}
			tlv[1] = (char) len;
			break;
		default :
			return INVALID_DATA_TYPE;
	}
	return len;
}

/* Extracts integer value. */ 
uint32_t getValue(unsigned char *vptr, int vlen, unsigned char datatype)
{
	int index = 0;
	uint32_t value;

	if ( (datatype == INTEGER) && (vptr[0] & '\x80') == '\x80' ) value = -1; else value = 0;
	while (index < vlen) {
		value <<= 8;
		value |= vptr[index++];
	}
	return value;
}

/* Extracts a TLV from msg starting at index. Return Success(0) or error code (<0). */ 
int parseTLV(unsigned char *msg, int index, tlvStructType *tlv)
{
	int tlen = 0;

	tlv->start = index;
	tlen = parseLength(msg+index+1, &(tlv->len));
	tlv->vstart = index + tlen + 1;
	switch (msg[index]) {
		case SEQUENCE:
		case GET_REQUEST:
		case GET_NEXT_REQUEST:
		case SET_REQUEST:
		case GET_RESPONSE:
		case TRAP_PACKET:
			tlv->nstart = tlv->vstart;
			break;
		case NULL_ITEM:
			if (tlv->len != 0) return ILLEGAL_LENGTH;
			tlv->nstart = tlv->vstart;
			break;
		case IP_ADDRESS:
			if (tlv->len != 4) return ILLEGAL_LENGTH;
			tlv->nstart = tlv->vstart + tlv->len;
			break;
		case INTEGER:
		case COUNTER:
		case GAUGE:
		case TIMETICKS:
			if (tlv->len > INT_SIZE) return ILLEGAL_LENGTH;
			tlv->nstart = tlv->vstart + tlv->len;
			break;
		case OBJECT_IDENTIFIER:
		case OCTET_STRING:
		case OPAQUE_TYPE:
			tlv->nstart = tlv->vstart + tlv->len;
			break;
		default:
			return INVALID_DATA_TYPE;
	}
	return SUCCESS;
}

/* Resets a varbind list to empty. */
void vblistReset(struct messageStruct *vblist)
{
	vblist->buffer[0] = SEQUENCE_OF;
	vblist->buffer[1] = '\0';
	vblist->len = 2;
	vblist->index = 2;
}

/* Adds a varbind into a varbind list. Returns the total length of the resultant list. */
int vblistAdd(struct messageStruct *vblist, char *oidstr, unsigned char dataType, void *val, int vlen )
{
	static struct messageStruct vb;
	static unsigned char vbBuffer[VB_BUFFER_SIZE];
	vb.buffer = vbBuffer; vb.size = VB_BUFFER_SIZE;

	unsigned char *tlv;
	int length; uint32_t *p;

	/* SEQUENCE header */
	vb.buffer[0] = SEQUENCE;
	vb.buffer[1] = '\0';  /* Set length field to zero first */

	/* Build OID TLV */
	vb.buffer[2] = OBJECT_IDENTIFIER;
	vb.buffer[3] = str2ber(oidstr, vb.buffer+4);  /* OID length field assumed to be of 1 byte */
	vb.len = 2 + vb.buffer[3];  /* Length of OID TLV */

	vb.index = vb.len + 2;  /* Including the SEQUENCE header */
	tlv = vb.buffer+vb.index;
	/* Build Value TLV */
	*tlv = dataType;
	*(tlv+1) = 0;  /* Set length field to zero first */
	switch(dataType) {
		case NULL_ITEM :
			length = 0;
			break;
		case OBJECT_IDENTIFIER :
			length = str2ber((char *) val, tlv+2);  /* OID length field assumed to be of 1 byte */
			break;
		case OCTET_STRING :
			length = vlen;
			memcopy(tlv+2, (unsigned char *)val, vlen);
			break;
		case INTEGER :
		case TIMETICKS :
		case COUNTER :
		case GAUGE :
			*(tlv+1) = INT_SIZE;
			h2nl_byte(*(uint32_t *)val, tlv+2);
			compactInt(tlv);
			length = (int) *(tlv+1);
			break;
	}
	length = 1 + insertRespLen(&vb, vb.index, &vb, vb.index, length) + length;  /* Length of Value TLV */
	vb.len += length;  /* Length of OID + Value TLV's */
	vb.len = ( 1 + insertRespLen(&vb, 0, &vb, 0, vb.len) + vb.len );  /* Length of Varbind sequence */

	parseLength(vblist->buffer + 1, &length);  /* Length of Varbind list sequence */
	if ( (vblist->len + vb.len) > vblist->size )
		return FAIL;
	else {
		vblist->index = vblist->len;
		memcopy(vblist->buffer+vblist->index, vb.buffer, vb.len);
		vblist->len = ( 1 + insertRespLen(vblist, 0, vblist, 0, length + vb.len) + length + vb.len );
		return vblist->len;
	}
}

/* Traverses a varbind list where opt=0 for first varbind, non-zero for next.
   Returns the nth order of the extracted varbind. */
int vblistGet(struct messageStruct *vblist, MIB *vb, unsigned char opt)
{
	static unsigned char i = 0;
	static tlvStructType tlv;

	if ( opt == 0 ) {
		if (parseTLV(vblist->buffer, 0, &tlv) != SUCCESS ||
			vblist->buffer[tlv.start] != SEQUENCE_OF)
			return FAIL;
		else i = 0;
	}
	if (tlv.nstart < vblist->len) {
		if (parseTLV(vblist->buffer, tlv.nstart, &tlv) !=SUCCESS ||
			vblist->buffer[tlv.start] != SEQUENCE )
			return FAIL;
		if (parseTLV(vblist->buffer, tlv.nstart, &tlv) != SUCCESS ||
			vblist->buffer[tlv.start] != OBJECT_IDENTIFIER )
			return FAIL;
		else
			ber2oid(vblist->buffer+tlv.vstart, tlv.len, &(vb->oid));
		if (parseTLV(vblist->buffer, tlv.nstart, &tlv) !=SUCCESS )
			return FAIL;
		else {
			vb->dataType = vblist->buffer[tlv.start];
		 	switch(vb->dataType) {
				case NULL_ITEM:
					break;
				case OCTET_STRING :
				case OBJECT_IDENTIFIER :
					memcopy(vb->u.octetstring, vblist->buffer+tlv.vstart, tlv.len);
					vb->dataLen = tlv.len;
					break;
				case INTEGER :
				case TIMETICKS :
				case COUNTER :
				case GAUGE :
	 				vb->u.intval = getValue(vblist->buffer+tlv.vstart, tlv.len, INTEGER);
					vb->dataLen = INT_SIZE;
					break;
				default :
					return FAIL;
			}
			vb->access='-'; vb->get = NULL; vb->set = NULL;
			return ++i;
		}
	} else return 0;
}
