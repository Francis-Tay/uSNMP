/*
 * Utility functions to file and display MIB data, mainly for Windows
 * and *nix.
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
#include <errno.h>
#include <string.h>
#include "mibutil.h"

#define BUF_SIZE 256

/* Prints the MIB data as a keylist string in s. */
void mibprint(MIB *thismib, char *s)
{
	char oidstr[MIB_DATA_SIZE], oidstr2[MIB_DATA_SIZE], str[BUF_SIZE];

	oid2str(&thismib->oid, oidstr);
	switch(thismib->dataType) {
 		case NULL_ITEM :
			sprintf(s, "%s=N,%c,-", oidstr, thismib->access);
			break;
 		case OBJECT_IDENTIFIER :
			ber2str(thismib->u.octetstring, thismib->dataLen, oidstr2);
			sprintf(s, "%s=O,%c,%s", oidstr, thismib->access, oidstr2);
			break;
		case OCTET_STRING :
			oct2str(thismib->u.octetstring, thismib->dataLen, str);
			sprintf(s, "%s=S,%c,%s", oidstr, thismib->access, str);
			if (octIsprint(thismib->u.octetstring, thismib->dataLen)) {
				memcopy((unsigned char *)str, thismib->u.octetstring, thismib->dataLen);
				str[thismib->dataLen] = '\0';
				sprintf(s+strlen(s), " [%s]", str);
			}
			break;
		case INTEGER :
			sprintf(s, "%s=I,%c,%d", oidstr, thismib->access, (int) thismib->u.intval);
			break;
		case TIMETICKS :
			sprintf(s, "%s=T,%c,%u", oidstr, thismib->access, (unsigned int) thismib->u.intval);
			break;
		case COUNTER :
			sprintf(s, "%s=C,%c,%u", oidstr, thismib->access, (unsigned int) thismib->u.intval);
			break;
		case GAUGE :
			sprintf(s, "%s=G,%c,%u", oidstr, thismib->access, (unsigned int) thismib->u.intval);
			break;
	}
}

/* Scans a keylist string of MIB data into the MIB structure.
   Returns Success(0) or Fail(-1) */
int mibscan(MIB *thismib, char *s)
{
	int i;
	char dataType, access, oidstr[MIB_DATA_SIZE], str[BUF_SIZE];
	OID oid;

	sscanf(s, "%[^=]=%c,%c,%s", oidstr, &dataType, &access, str);
	if (str2oid(oidstr, &oid) == 0) return FAIL;
	thismib->oid.len = oid.len;
	for (i = 0; i<oid.len; i++)
		thismib->oid.array[i] = oid.array[i];
	thismib->access = access;
	switch(dataType) {
 		case 'O' :
			thismib->dataType = OBJECT_IDENTIFIER;
			thismib->dataLen = str2ber(str, thismib->u.octetstring);
			break;
		case 'S' :
			thismib->dataType = OCTET_STRING;
			thismib->dataLen = str2oct(str, thismib->u.octetstring);
			break;
		case 'I' :
			thismib->dataType = INTEGER;
			thismib->u.intval = atoi(str);
			thismib->dataLen = INT_SIZE;
			break;
		case 'T' :
			thismib->dataType = TIMETICKS;
			thismib->u.intval = atoi(str);
			thismib->dataLen = INT_SIZE;
			break;
		case 'C' :
			thismib->dataType = COUNTER;
			thismib->u.intval = atoi(str);
			thismib->dataLen = INT_SIZE;
			break;
		case 'G' :
			thismib->dataType = GAUGE;
			thismib->u.intval = atoi(str);
			thismib->dataLen = INT_SIZE;
			break;
	}
	return SUCCESS;
}

/*
 * Reads from a file and populates a MIB list.
 */
int miblistread(LIST *miblist, char *fn)
{
	char buf[BUF_SIZE];
	unsigned char octetdata[BUF_SIZE];
	FILE *f;
	MIB mib, *thismib;

	if ((f = fopen(fn, "r"))) {
		while (fgets(buf, BUF_SIZE, f)) {
			mib.u.octetstring = octetdata;     // reset u
			if (mibscan(&mib, buf) == SUCCESS) {
				if ((thismib=miblistgooid(miblist, &mib.oid))==NULL) {
					thismib = malloc(sizeof(MIB));
					*thismib = mib;
					if (thismib->dataType==OBJECT_IDENTIFIER || thismib->dataType==OCTET_STRING)
						if (mib.dataLen < MIB_DATA_SIZE)
							thismib->u.octetstring = malloc(MIB_DATA_SIZE);
						else
							thismib->u.octetstring = malloc(mib.dataLen);
					thismib->get = NULL;
					thismib->set = NULL;
					miblistput(miblist, thismib);
				}
				if (thismib->dataType==OBJECT_IDENTIFIER || thismib->dataType==OCTET_STRING)
					memcopy(thismib->u.octetstring, mib.u.octetstring, mib.dataLen);
				else
					thismib->u.intval = mib.u.intval;
			}
		}
		fclose(f);
		return SUCCESS;  
	}
	else
		return FAIL;
}

void miblistprint(LIST *miblist, FILE *f)
{
	MIB *thismib;
	char s[BUF_SIZE];

	thismib=(MIB *)listgohead(miblist);
	while (thismib) {
		mibprint(thismib, s);
		fprintf(f, "%s\n", s);
		thismib=(MIB *)listgonext(miblist);
	}
}

int miblistwrite(LIST *miblist, char *fn)
{
	FILE *f;

	if ((f = fopen(fn, "w"))) {
		miblistprint(miblist, f);
		fclose(f);
		return SUCCESS;  
	}
	else
		return FAIL;
}

void vblistPrint(struct messageStruct *vblist, FILE *f)
{
	MIB vb;
	unsigned char octetdata[BUF_SIZE];
	char s[BUF_SIZE];

	vb.u.octetstring = octetdata;
	if (vblistGet(vblist, &vb, 0) > 0) {
		mibprint(&vb, s);
		fprintf(f, "%s\n", s);
		vb.u.octetstring = octetdata;     // reset u
		while (vblistGet(vblist, &vb, 1) > 0) {
			mibprint(&vb, s);
			fprintf(f, "%s\n", s);
			vb.u.octetstring = octetdata;   // reset u
		}
	}
}

/*
 * Display packet, mainly used for debugging.
 */
void showMessage(struct messageStruct *pkt)
{
	int i;
	for (i = 0 ; i < pkt->len ; i++) {
		if ((i % 16) == 0) printf("\n  %04x : ", i);
		printf("%02x ", (unsigned char)pkt->buffer[i]);
	}
	printf("\n");
}
