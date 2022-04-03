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

#ifndef _MIBLIST_H
#define _MIBLIST_H

#include "list.h"
#include "mib.h"

#ifdef __cplusplus
extern "C" {
#endif 

LIST *miblistnew(int size);
void miblistclear(LIST *l);
void miblistfree(LIST *l);

/* *data is a user-supplied space to hold the data of the MIB node. size
   refers to the length of this supplied space; and may be set to 0 for
   interger/gauge/counter/timertick types as it will default to 4. */
MIB *miblistadd(LIST *l, char *oidstr, unsigned char dataType, char access,
	void *data, int size);

MIB *miblistput(LIST *l, MIB *mib);
Boolean miblistdel(LIST *l);

MIB *miblistset(LIST *l, OID *oid, void *u, int size);
MIB *miblistgooid(LIST *l, OID *oid);
MIB *miblistgetthis(LIST *l);
MIB *miblistgetnext(LIST *l);
MIB *miblistgetprev(LIST *l);
MIB *miblistgohead(LIST *l);
MIB *miblistgotail(LIST *l);
MIB *miblistgonext(LIST *l);

#ifdef __cplusplus
}
#endif

#endif
