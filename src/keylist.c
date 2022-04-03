/*
 * Implements a key-value-pair list.
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
#include <string.h>
#include <ctype.h>
#include "keylist.h"

char *strtrim(char *s);

#define BUF_SIZE 256

char *strtrim(char *s)
{
	int i=0, j, k=0;
	char c;

	j = strlen(s);
	if (j==0)
		return s;
	else
		j--;
	while ((c=s[i])==' ' || c=='\n' || c=='\r')
		i++;
	while (j>i && ((c=s[j])==' ' || c=='\n' || c=='\r'))
		j--;
	while (i<=j) {
		s[k]=s[i];
		k++; i++;
	}
	s[k]='\0';
	return s;
}

LIST *keylistnew()
{
	LIST *keylist;
	if ((keylist=listnew(sizeof(KEY), 0)))
		return keylist;
	else
		return (LIST *) NULL;
}

void keylistclear(LIST *keylist)
{
	listclear(keylist);
}

void keylistfree(LIST *keylist)
{
	keylistclear(keylist);
	listfree(keylist);
}

int keyscan(char *buf, char *key, char *val)
{
	int i, n, p = 0;

	strtrim(buf);
	/* First non-alphanum character indicates comment line */
	if (!isalnum((int)buf[0])) return FAIL;
	n = strlen(buf);
	for (i = 0; buf[p]!='=' && p<n; i++, p++) key[i] = buf[p];
	if (p == n)
		return FAIL;
	else
		{ key[i] = '\0'; strtrim(key); }
	for (i = 0, p++; p<n; i++, p++) val[i] = buf[p];
	val[i]='\0'; strtrim(val);
	return SUCCESS;
}

void keylistprint(LIST *keylist, FILE *f)
{
	KEY *thiskey;

	thiskey=(KEY *)listgohead(keylist);
	while (thiskey) {
		fprintf(f, "%s=%s\n", thiskey->key, thiskey->val);
		thiskey=(KEY *)listgonext(keylist);
	}
}

int keylistwrite(LIST *keylist, char *fn)
{
	FILE *f;

	if ((f = fopen(fn, "w"))) {
		keylistprint(keylist, f);
		fclose(f);
		return SUCCESS;  
	}
	else
		return FAIL;
}

int keylistread(LIST *keylist, char *fn)
{
	char buf[BUF_SIZE];
	FILE *f;
	KEY key, *thiskey;

	if ((f = fopen(fn, "r"))) {
		while (fgets(buf, BUF_SIZE, f)) {
			if (keyscan(buf, key.key, key.val)==SUCCESS) {
				/* Replace key's value if it exist, add key-value pair otherwise */
				if ((thiskey=keylistgokey(keylist, key.key)) != NULL )
					strcpy(thiskey->val, key.val);
				else
					keylistadd(keylist, key.key, key.val);
 		 	}
		}
		fclose(f);
		return SUCCESS;  
	}
	else
		return FAIL;
}

int keylistgetval(char *fn, char *k, char *v)
{
	char key[KEY_SIZE];
	LIST *keylist;
	KEY *thiskey;
	int retcode;

	keylist = keylistnew();
	strcpy(key, k); strtrim(key);
	if ( keylistread(keylist, fn)==SUCCESS && (thiskey=keylistgokey(keylist, key)) ) {
		strcpy(v, thiskey->val);
		retcode = SUCCESS;
	}
	else retcode = FAIL;
	keylistfree(keylist);
	return retcode;
}

int keylistsetval(char *fn, char *k, char *v)
{
	char key[KEY_SIZE], val[VAL_SIZE];
	LIST *keylist;
	KEY *thiskey;
	int retcode;

	keylist = keylistnew();
	strcpy(key, k); strtrim(key);
	strcpy(val, v); strtrim(val);
	if ( keylistread(keylist, fn)==SUCCESS &&
		(thiskey=keylistgokey(keylist, key)) ) {
		strcpy(thiskey->val, val);
		keylistwrite(keylist, fn);
		retcode = SUCCESS;
	}
	else retcode = FAIL;
	keylistfree(keylist);
	return retcode;
}

KEY *keylistadd(LIST *keylist, char *k, char *v)
{
	char key[KEY_SIZE], val[VAL_SIZE];
	KEY *thiskey;

	strcpy(key, k); strtrim(key);
	strcpy(val, v); strtrim(val);
	if ( keylistgokey(keylist, key)==NULL &&
		(thiskey=(KEY *)listaddnode(keylist, AFTER))) {
		strcpy(thiskey->key, key);
		strcpy(thiskey->val, val);
		return thiskey;
	}
	else
		return NULL;
}

KEY *keylistset(LIST *keylist, char *k, char *v)
{
	char key[KEY_SIZE], val[VAL_SIZE];
	KEY *thiskey;

	strcpy(key, k); strtrim(key);
	strcpy(val, v); strtrim(val);
	if ((thiskey=keylistgokey(keylist, key))) {
		strcpy(thiskey->val, val);
		return thiskey;
	}
	else
		return keylistadd(keylist, key, val);
}

Boolean keylistdel(LIST *keylist)
{
	return listdelnode(keylist);
}

KEY *keylistgokey(LIST *keylist, char *k)
{
	char key[KEY_SIZE];
	KEY *thiskey;
	int i;

	strcpy(key, k); strtrim(key);
	if ( (thiskey=(KEY *)listgetthis(keylist))==NULL ||
		strcmp(key, thiskey->key)!=0 )
		thiskey=(KEY *)listgohead(keylist);
	while ( thiskey && (i=strcmp(thiskey->key, key))!=0 )
		thiskey=(KEY *)listgonext(keylist);
	if (thiskey==NULL || i)
		return (KEY *)NULL;
	else
		return (KEY *)thiskey;
}

KEY *keylistgovalue(LIST *keylist, char *v)
{
	char val[VAL_SIZE];
	KEY *thiskey;
	int i;

	strcpy(val, v); strtrim(val);
	if ( (thiskey=(KEY *)listgetthis(keylist))==NULL ||
		strcmp(val, thiskey->val)!=0 )
		thiskey=(KEY *)listgohead(keylist);
	while ( thiskey && (i=strcmp(thiskey->val, val))!=0 )
		thiskey=(KEY *)listgonext(keylist);
	if (thiskey==NULL || i)
		return (KEY *)NULL;
	else
		return (KEY *)thiskey;
}

int keylistsize(LIST *keylist)
{
	return listsize(keylist);
}

KEY *keylistgetthis(LIST *keylist)
{
	return (KEY *)listgetthis(keylist);
}

KEY *keylistgetnext(LIST *keylist)
{
	return (KEY *)listgetnext(keylist);
}

KEY *keylistgetprev(LIST *keylist)
{
	return (KEY *)listgetprev(keylist);
}

KEY *keylistgohead(LIST *keylist)
{
	return (KEY *)listgohead(keylist);
}

KEY *keylistgotail(LIST *keylist)
{
	return (KEY *)listgotail(keylist);
}

KEY *keylistgonext(LIST *keylist)
{
	return (KEY *)listgonext(keylist);
}
