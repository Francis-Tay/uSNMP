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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "oid.h"

#ifndef ARDUINO

/* Converts OID arrary to string, returns length of string. */
int oid2str(OID *oid, char *str)
{
	int i;
	char buf[8];

	if (oid->len<=0)
		return 0;
	else {
		str[0]=oid->array[0]; str[1]='.'; str[2]='\0';
		for (i=1; i<oid->len; i++) {
			sprintf(buf, "%d.", oid->array[i]);
			strcat(str, buf);
		}
		i = strlen(str)-1;
		str[i]='\0';  /* Remove the trailing dot */
		return i;     /* Return length of the string */
	}
}

/* Converts BER to string, returns length of string. */
int ber2str(unsigned char *ber, int len, char *str)
{
	OID oid;

	if ( ber2oid(ber, len, &oid) >= 0 )
		return oid2str(&oid, str);
	else
		return 0;
}

#endif

/* Converts string to OID arrary, returns length of array. */
int str2oid(char *str, OID *oid)
{
	char *p, buf[8];
	int l = 0;

	while ( str[l]==' ' ) l++;  /* Remove leading spaces */
	if ( (str[l]=='B' || str[l]=='E' || str[l]=='P') && str[l+1]=='.' )
		{ oid->array[0]=str[l];	oid->len=1; p=str+l+2; }
	else { oid->array[0]='U'; oid->len=0; return 0; }
	while (*p) {
		for (l=0; p[l]!='.' && p[l]!='\0'; l++);
		memcopy((unsigned char *)buf, (unsigned char *)p, l);
		buf[l]='\0';
		oid->array[oid->len++] = atoi(buf);
		if (p[l] != '\0')
			p=p+l+1;
		else
			break;
	}
	return oid->len;
}

/* Converts OID arrary to BER, returns length of encoded BER string. */
int oid2ber(OID *oid, unsigned char *str)
{
	int i, j;
	unsigned int k;

	switch (oid->array[0]) {
		case 'B':
			str[3] = '\x02';
			str[4] = '\x01';  /* MIB-2 is "1.3.6.1.2.1" */
			j = 5;
			break;
		case 'E':
			str[3] = '\x03';  /* EXPERIMENTAL is "1.3.6.1.3" */
			j = 4;
			break;
		case 'P':
			str[3] = '\x04';
			str[4] = '\x01';  /* ENTERPRISES is "1.3.6.1.4.1" */
			j = 5;
			break;
		default:
			return 0;
	}
	str[0] = '\x2B';  /* Common prefix of "1.3.6.1" */
	str[1] = '\x06';
	str[2] = '\x01';

	for (i=1; i<oid->len; i++) {
		k = oid->array[i];
		if (k <= 0x7F) {
			str[j++] = (unsigned char) k;
		}
		else
			if (k <= 0x3FFF) {
				str[j+1] = (unsigned char) (k & '\x7F');
				k >>= 7;
				str[j] = (unsigned char) ((k & '\x7F') | '\x80');
				j = j + 2;
			}
			else
#ifdef ARDUINO
				if (k <= 0xFFFF) {
#else
				if (k <= 0x1FFFFF) {
#endif
					str[j+2] = (unsigned char) (k & '\x7F');
					k >>= 7;
					str[j+1] = (unsigned char) ((k & '\x7F') | '\x80');
					k >>= 7;
					str[j] = (unsigned char) ((k & '\x7F') | '\x80');
					j = j + 3;
				}
#ifndef ARDUINO
				else
					if (k <= 0xFFFFFFF) {
						str[j+3] = (unsigned char) (k & '\x7F');
						k >>= 7;
						str[j+2] = (unsigned char) ((k & '\x7F') | '\x80');
						k >>= 7;
						str[j+1] = (unsigned char) ((k & '\x7F') | '\x80');
						k >>= 7;
						str[j] = (unsigned char) ((k & '\x7F') | '\x80');
						j = j + 4;
					}
#endif
	}
	return j;  /* Return length of the BER-encoded string */
}

/* Converts BER to OID arrary, returns length of array. */
int ber2oid(unsigned char *str, int len, OID *oid)
{
	int i=5, j=1;
	unsigned int k;

	if ( str[0] == '\x2B' && str[1] == '\x06' && str[2] == '\x01') {  /* Common prefix of "1.3.6.1" */
		if ( str[3] == '\x02' && str[4] == '\x01' )
			oid->array[0] = 'B'; 
		else
			if ( str[3] == '\x03' )
				{ oid->array[0] = 'E'; i=4; }
			else
				if ( str[3] == '\x04' && str[4] == '\x01' )
					oid->array[0] = 'P';
				else oid->array[0] = 'U';
	}
	else oid->array[0] = 'U';

	if ( oid->array[0] == 'U' )
		oid->len = 0;
	else {
		while (i < len) {
			k = 0;
			while (str[i] & '\x80')
				k = (k | (str[i++] & '\x7F')) << 7;
			oid->array[j++] = k | str[i++];
		}
		oid->len = j;
	}
	return oid->len;
}

/* Converts string to BER, returns length of BER-encoded string. */
int str2ber(char *str, unsigned char *ber)
{
	OID oid;

	if ( str2oid(str, &oid) >= 0 )
		return oid2ber(&oid, ber);
	else
		return 0;
}

/* Compares two OID arrays, return 0 if equal, >0 if oid1>oid2, <0 if oid1<oid2 */
int oidcmp(OID *oid1, OID *oid2)
{
	int k=0, ret=0;
	unsigned char inverse;
	OID *i, *j;

	if (oid1->len <= oid2->len) {
		inverse = 0;
		i = oid1; j = oid2;
	}
	else {
		inverse = 1;
		i = oid2; j = oid1;
	}
 	while (k<i->len)
 		if (i->array[k] > j->array[k]) {
 			ret = 1;  /* array i is greater */
 			break;
 		}
 		else
 			if (i->array[k] < j->array[k]) {
 				ret = -1;  /* array i is smaller */
 				break;
 			}
 			else
 				k++;
	if (ret == 0 && i->len<j->len) ret = -1;  /* array i is shorter and smaller */
	if (inverse==0) return ret; else return -ret;
}
