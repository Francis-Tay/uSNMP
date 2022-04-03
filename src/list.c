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

#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>
#include "list.h"

LIST *listnew(int datasize, int max)
{
	LIST *l;

	if ((l=(LIST *)malloc(sizeof(LIST)))) {
		l->datasize = datasize;
		l->head = l->curr = l->prev = NULL;
		l->eol = TRUE;
		l->limit = max;  /* 0 for unlimited */
		l->size = 0;
		return l;
	}
	else
		return NULL;
}

void listclear(LIST *l)
{
	listgohead(l);
	while (listdelnode(l)) ;
}

void listfree(LIST *l)
{
	listclear(l);
	free(l);
}

int listlimit(LIST *l)
{
	return l->limit;
}

int listsize(LIST *l)
{
	return l->size;
}

Boolean listeol(LIST *l)
{
	return l->eol;
}

void *listaddnode(LIST *l, int mode)
{
	void *data;

	if ((l->limit == 0 || l->size < l->limit) &&
		(data=(void *)malloc(l->datasize)))
		return listputnode(l, data, mode);
	else
		return NULL;
}

void *listputnode(LIST *l, void *data, int mode)
{
	NODE *n;

	if ((l->limit == 0 || l->size < l->limit) &&
		(n=(NODE *)malloc(sizeof(NODE)))) {
		n->data = data;
		if (l->head == NULL) {	/* empty list */
			l->head = l->curr = n;
			n->next = NULL;
		}
		else
			if (l->curr == NULL) {  /* eol */
				n->next = NULL;
				l->prev->next = n;
			}
			else
				if (mode == BEFORE) {
					n->next = l->curr;
					if (l->prev == NULL)
						l->head = n;  /* make n the head node */
					else
						l->prev->next = n;
				}
				else {
		 			n->next = l->curr->next;
					l->curr->next = n;
					l->prev = l->curr;
				}
		l->curr = n;
		l->size++;
		l->eol = FALSE;
		return data;
	}
	else
		return NULL;
}

void *listsetnode(LIST *l, void *data)
{
	if (l->curr == NULL)
		return NULL;		 
	else {
		l->curr->data = data;
		return data;
	}
}

Boolean listdelnode(LIST *l)
{
	if (l->curr == NULL)
		return FALSE;
	else {
		free(l->curr->data);
		listcutnode(l);
		return TRUE;
	}
}

Boolean listcutnode(LIST *l)
{
	NODE *n;

	n = l->curr;
	if (n == NULL)
		return FALSE; 		
	else {
		if (l->prev == NULL) {  /* first node */
			l->head = l->curr = n->next;
			free(n);
			if (l->head == NULL) l->eol = TRUE;  /* list is now empty */
		}
		else
			if (n->next == NULL) {  /* last node */
				l->prev->next = NULL;
				l->curr = l->prev;
				free(n);
				listgotail(l);  /* to set l->prev */
			}
			else {
				l->curr = l->prev->next = n->next;
				free(n);
			}
		l->size--;
		return TRUE;
	}
}

void *listgetthis(LIST *l)
{
	if (l->curr == NULL)
		return NULL;
	else
		return l->curr->data;
}

void *listgetnext(LIST *l)
{
	if (l->curr == NULL)
		return NULL;
	else
		return l->curr->next->data;
}

void *listgetprev(LIST *l)
{
	if (l->prev == NULL)
		return NULL;
	else
		return l->prev->data;
}

void *listgohead(LIST *l)
{
	if (l->head == NULL)
		return NULL;
	else {
		l->eol = FALSE;
		l->prev = NULL;
		l->curr = l->head;
		return l->curr->data;
	}
}

void *listgotail(LIST *l)
{
	if (l->head == NULL)
		return NULL;
	if (l->curr == NULL)
		listgohead(l);
	while (listgetnext(l) != NULL)
		listgonext(l);
	return l->curr->data;
}

void *listgonext(LIST *l)
{
	if (l->curr==NULL)
		return NULL;
	else {
		l->prev = l->curr;
		if (l->curr->next == NULL) {  /* last node */
			l->eol = TRUE;
			l->curr = NULL;
			return NULL;
		}
		else {
			l->curr = l->curr->next;
			return l->curr->data;
		}
	}
}
