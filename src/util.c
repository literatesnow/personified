/*
  Copyright (C) 2004 Chris Cuthbertson

  This file is part of Personified.

  Personified is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Personified is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Personified.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "util.h"

//There's a good chance this is similar to code in the Quake source (possibly ZQuake, FuhQuake or vanilla)
float readfloat(char *s)
{
	union	{
		char b[4];
		float	f;
		int	l;
	} dat;

	dat.b[0] = *s++;
	dat.b[1] = *s++;
	dat.b[2] = *s++;
	dat.b[3] = *s++;
	
	return dat.f;	
}

//There's a good chance this is similar to code in the Quake source (possibly ZQuake, FuhQuake or vanilla)
int readint(char *s)
{
  int i;

  i = (unsigned char)*s++;
  i += 256 * (unsigned char)*s++;
  i += 65536 * (unsigned char)*s++;
  i += 16777216 * (unsigned char)*s++;

  return i;
}

int memcatz(char *dst, int dstlen, const char *src, int srclen, int siz)
{
  if (dstlen + srclen > siz)
    return 0;

  dst += dstlen;

  memcpy(dst, src, srclen);
  dst[srclen] = 0;

  return 1;
}

void redtext(unsigned char *str)
{
  while (*str) {
    if ((*str > 32) && (*str < 128))
      *str += 128;
    str++;
  }
}

void tokenize(char *line, char *key, int keysize, char *value, int valuesize, char chr)
{
  char *s;

  s = strchr(line, chr);
  if (s) {       
    strlcpy(value, &line[s-line+1], valuesize);
    *s = '\0';
  }
  else {
    *value = '\0';
  }
  strlcpy(key, line, keysize);
}

float distance(vec3_t pos1, vec3_t pos2)
{
  float xd, yd, zd;

  xd = pos2[0] - pos1[0];
  yd = pos2[1] - pos1[1];
  zd = pos2[2] - pos1[2];

  return (float)sqrt(xd * xd + yd * yd + zd * zd) / 100;
}

int bound(int min, int max, int num)
{
  if (num < min)
    return min;
  else if (num > max)
    return max;
  else return num;
}

int isip(char *s)
{
  while (*s) {
    if (!isdigit(*s) && *s != '.')
      return 0;
    *s++;
  }
  return 1;
}

//There's a good chance this is similar to code in the Quake source (possibly ZQuake, FuhQuake or vanilla)
int hex2dec(char a, char b)
{
  char c;
  int i;

  c = tolower(a);
  if (c >= '0' && c <= '9')
	  i = (c - '0') << 4;
  else if (c >= 'a' && c <= 'f')
		i = (c - 'a' + 10) << 4;
  else return -1;
  	c = tolower(b);
  if (c >= '0' && c <= '9')
	  i += (c - '0');
  else if (c >= 'a' && c <= 'f')
    i += (c - 'a' + 10);
  else return -1;

  return i;
}

/*	$OpenBSD: strlcat.c and strlcpy.c,v 1.11 2003/06/17 21:56:24 millert Exp $
 *
 * Copyright (c) 1998 Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Appends src to string dst of size siz (unlike strncat, siz is the
 * full size of dst, not space left).  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz <= strlen(dst)).
 * Returns strlen(src) + MIN(siz, strlen(initial dst)).
 * If retval >= siz, truncation occurred.
 */
size_t strlcat(char *dst, const char *src, size_t siz)
{
	register char *d = dst;
	register const char *s = src;
	register size_t n = siz;
	size_t dlen;

	/* Find the end of dst and adjust bytes left but don't go past end */
	while (n-- != 0 && *d != '\0')
		d++;
	dlen = d - dst;
	n = siz - dlen;

	if (n == 0)
		return(dlen + strlen(s));
	while (*s != '\0') {
		if (n != 1) {
			*d++ = *s;
			n--;
		}
		s++;
	}
	*d = '\0';

	return(dlen + (s - src));	/* count does not include NUL */
}

/*
 * Copy src to string dst of size siz.  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz == 0).
 * Returns strlen(src); if retval >= siz, truncation occurred.
 */
size_t strlcpy(char *dst, const char *src, size_t siz)
{
	register char *d = dst;
	register const char *s = src;
	register size_t n = siz;

	/* Copy as many bytes as will fit */
	if (n != 0 && --n != 0) {
		do {
			if ((*d++ = *s++) == 0)
				break;
		} while (--n != 0);
	}

	/* Not enough room in dst, add NUL and traverse rest of src */
	if (n == 0) {
		if (siz != 0)
			*d = '\0';		/* NUL-terminate dst */
		while (*s++)
			;
	}

	return(s - src - 1);	/* count does not include NUL */
}

