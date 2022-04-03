/*
 * Functions to translate octet string and display string.
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
#include <ctype.h>
#include "octet.h"

/* Convert a dash-delimited hexadecimal display string to octet string,
   returns length of the octet string. */
int str2oct(char *str, unsigned char *oct)
{
	int i = 0, j = 0;
	unsigned char c;

	oct[0] = '\0';
	for ( ; ; ) {
		c = str[i++];
		if (c >= '0' && c <= '9')
			c -= 48;  /* ASCII '0' is 48 */
		else if (c >= 'a' && c <= 'f')
				c -= 87;  /* ASCII 'a' is 97 */
			else if (c >= 'A' && c <= 'F')
					c -= 55;  /* ASCII 'A' is 65 */
				else
					return 0;
		oct[j] = c << 4;
		c = str[i++];
		if (c >= '0' && c <= '9')
			c -= 48;
		else if (c >= 'a' && c <= 'f')
				c -= 87;
			else if (c >= 'A' && c <= 'F')
					c -= 55;
				else
					return 0;
		oct[j++] |= c;
		if ( str[i++] != '-' ) {
			oct[j] = '\0';
			return j;
		}
	}
}

/* Convert octet string to a dash-delimited hexadecimal display string,
   returns length of the hexa string. */
int oct2str(unsigned char *oct, int len, char *str)
{
	int i, j;
	for (i=0, j=0 ; i<len; i++, j+=3)
		sprintf(str+j, "%02x-", oct[i]);
	if (j>0) str[--j]='\0';
	return j;
}

/* Test if the octet string is printable; return 1 if yes, 0 if not. */
int octIsprint(unsigned char *oct, int len)
{
	int i;
	for (i=0; i<len; i++)
		if (!isprint(oct[i])) return 0;
	return 1;
}
