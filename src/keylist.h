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

/* A keylist data file contains key-value pairs in the format <key>=<value>. */

#ifndef _KEYLIST_H
#define _KEYLIST_H

#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif 

#define KEY_SIZE 32
#define VAL_SIZE 256

typedef struct {
	char key[KEY_SIZE];
	char val[VAL_SIZE];
} KEY;

char *strtrim(char *s);

LIST *keylistnew(void);
void keylistclear(LIST *l);
void keylistfree(LIST *l);
void keylistprint(LIST *l, FILE *f);
int keylistwrite(LIST *l, char *fn);
int keylistread(LIST *l, char *fn);

int keylistgetval(char *fn, char *key, char *val);
int keylistsetval(char *fn, char *key, char *val);
KEY *keylistadd(LIST *l, char *key, char *val);
KEY *keylistset(LIST *l, char *key, char *val);
Boolean keylistdel(LIST *l);

KEY *keylistgokey(LIST *l, char *key);
KEY *keylistgovalue(LIST *l, char *val);

int keylistsize(LIST *l);
KEY *keylistgetthis(LIST *l);
KEY *keylistgetnext(LIST *l);
KEY *keylistgetprev(LIST *l);
KEY *keylistgohead(LIST *l);
KEY *keylistgotail(LIST *l);
KEY *keylistgonext(LIST *l);

#ifdef __cplusplus
}
#endif

#endif
