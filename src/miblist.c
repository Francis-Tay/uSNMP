/*
 * Implements a MIB tree as a singly linked list, stored in lexicographic order.
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

/*
 * Implements a MIB tree using a singly linked list with dynamically
 * allocate dmemory
 */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include "miblist.h"

LIST *miblistnew(int size)
{
	LIST *miblist;
	if ((miblist=listnew(sizeof(MIB), size)))
		return miblist;
	else
		return (LIST *) NULL;
}

void miblistclear(LIST *miblist)
{
	listgohead(miblist);
	while (miblistdel(miblist))
		;
}

void miblistfree(LIST *miblist)
{
	miblistclear(miblist);
	listfree(miblist);
}

/* *data is a user-supplied space to hold the data. size
   refers to the length of this supplied space; and may be set to 0 for
   interger/gauge/counter/timertick types as it will default to 4. */
MIB *miblistadd(LIST *miblist, char *oidstr, unsigned char dataType, char access,
	void *data, int size)
{
	int i;
	OID oid;
	MIB *thismib;

	str2oid(oidstr, &oid);
	if ((thismib=miblistgooid(miblist, &oid)))
		return NULL;
	if (listeol(miblist))
		thismib = (MIB *) listaddnode(miblist, AFTER);
	else
		thismib = (MIB *) listaddnode(miblist, BEFORE);  
	thismib->access = access;
	thismib->dataType = dataType;
	thismib->oid.len = oid.len;
	for (i = 0; i<oid.len; i++)
		thismib->oid.array[i] = oid.array[i];
	thismib->get = NULL;
	thismib->set = NULL;
	if (dataType == OCTET_STRING || dataType == OBJECT_IDENTIFIER) {
		thismib->u.octetstring = (unsigned char *) data;
		thismib->dataLen = size;
	}
	else {
		thismib->u.intval = 0;
		thismib->dataLen = INT_SIZE;
	}
	return thismib;
}

MIB *miblistput(LIST *miblist, MIB *mib)
{
	miblistgooid(miblist, &mib->oid); 																																													
	if (listeol(miblist))
		return (MIB *) listputnode(miblist, mib, AFTER);
	else
		return (MIB *) listputnode(miblist, mib, BEFORE);
}

MIB *miblistset(LIST *miblist, OID *oid, void *u, int size)
{
	MIB *thismib;

	if ((thismib=miblistgooid(miblist, oid))) {
		mibsetvalue(thismib, u, size);
		return thismib;
	}
	else
		return NULL;
}

Boolean miblistdel(LIST *miblist)
{
	MIB *thismib;

	if ( (thismib=(MIB *)listgetthis(miblist))==NULL )
		return FALSE;
	else {
		if (thismib->dataType == OCTET_STRING || thismib->dataType == OBJECT_IDENTIFIER)
			free(thismib->u.octetstring);
		return listdelnode(miblist);
	}
}

MIB *miblistgooid(LIST *miblist, OID *oid)
{
	MIB *thismib;
	int i;

	if ( (thismib=(MIB *)listgetthis(miblist))==NULL ||
		oidcmp(oid, &thismib->oid)<0 )
		thismib=(MIB *)listgohead(miblist);
	while ( thismib && (i=oidcmp(oid, &thismib->oid))>0 )
		thismib=(MIB *)listgonext(miblist);
	if (thismib==NULL || i)
		return (MIB *)NULL;
	else
		return (MIB *)thismib;
}

int miblistsize(LIST *miblist)
{
	return listsize(miblist);
}

MIB *miblistgetthis(LIST *miblist)
{
	return (MIB *)listgetthis(miblist);
}

MIB *miblistgetnext(LIST *miblist)
{
	return (MIB *)listgetnext(miblist);
}

MIB *miblistgetprev(LIST *miblist)
{
	return (MIB *)listgetprev(miblist);
}

MIB *miblistgohead(LIST *miblist)
{
	return (MIB *)listgohead(miblist);
}

MIB *miblistgotail(LIST *miblist)
{
	return (MIB *)listgotail(miblist);
}

MIB *miblistgonext(LIST *miblist)
{
	return (MIB *)listgonext(miblist);
}
