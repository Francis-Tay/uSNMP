/*
 * Implements a singly linked list.
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
list.c implements a singly-linked list object.

LIST *listnew(int datasize, int max);
	Instantiate a linked list of data whose bucket size is given in datasize.
	max is the maximum number of nodes the list may have, 0 if unlimited.
	This is initially an empty list. Analogous to fopen() which return
	FILE *, newlist() return LIST * as a "magic cookie" to be used in
	other functions.

void listclear(LIST *l);
	Delete all nodes in the list. The list remains active until freed.

void listfree(LIST *l);
	Delete all nodes in the list and then free it.

int listmax(LIST *l);
	Returns the maximum number of nodes the list may have.

int listsize(LIST *l);
	Returns the number of nodes in the list.

Boolean listeol(LIST *l);
	The eol flag is set when the list is empty or when a listgonext() is called
	when the current node is the last in the list.

void *listaddnode(LIST *l, int mode);
	Depnding on mode, adds an node before or after the current node. The
	function allocates a data bucket of size datasize and returns the pointer
	to this bucket. It is the responsibility of the user to typecast it.

void *listputnode(LIST *l, void *data, int mode);
	Depnding on mode, adds an node before or after the current node. The
	function then set the node to point to the data bucket of *data and
	returns the same. It is the responsibility of the user to typecast it.

void *listsetnode(LIST *l, void *data);
	Sets the current node to point to *data bucket.

Boolean listdelnode(LIST *l);
	Deletes the current node, freeing the node and the data bucket.

Boolean listcutnode(LIST *l);
	Deletes the current node, freeing the node.

void *listgetthis(LIST *l);
void *listgetnext(LIST *l);
void *listgetprev(LIST *l);
	Returns the pointer to the data bucket of the current, next or previous
	node repsectively. The current node is unchanged.

void *listgohead(LIST *l);
void *listgotail(LIST *l);
	Makes respectively the head or tail node current and returns its pointer.

void *listgonext(LIST *l);
	Makes the next node current and returns its pointer. If the current node
	is the tail node, eol is set, current pointer is set to NULL.
	There isn't a listgoprev(). While traversing a singly linked list forward, the
	pointer to the previous node may be tracked, but not for a backward traversal.
*/

#ifndef _LIST_H
#define _LIST_H

#include "retval.h"
#ifndef Boolean
#define Boolean signed char
#endif

#ifdef __cplusplus
extern "C" {
#endif 

typedef struct node {
	void *data;
	struct node *next;
} NODE;

typedef struct {
	int datasize;
	NODE *head;
	NODE *curr;
	NODE *prev;
	Boolean eol;
	int limit;
	int size;
} LIST;

LIST *listnew(int datasize, int max);
void listclear(LIST *l);
void listfree(LIST *l);
int listlimit(LIST *l);
int listsize(LIST *l);
Boolean listeol(LIST *l);

#define BEFORE 0
#define AFTER  1
void *listaddnode(LIST *l, int mode);
void *listputnode(LIST *l, void *data, int mode);
void *listsetnode(LIST *l, void *data);
Boolean listdelnode(LIST *l);
Boolean listcutnode(LIST *l);

void *listgetthis(LIST *l);
void *listgetnext(LIST *l);
void *listgetprev(LIST *l);
void *listgohead(LIST *l);
void *listgotail(LIST *l);
void *listgonext(LIST *l);

#ifdef __cplusplus
}
#endif

#endif
