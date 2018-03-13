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

#include <stdio.h>
#include <string.h>
#include <math.h>
#ifdef WIN32
#include <windows.h>
#endif

typedef float vec_t;
typedef vec_t	vec3_t[3];

float readfloat(char *s);
int readint(char *s);
int memcatz(char *dst, int dstlen, const char *src, int srclen, int siz);
void tokenize(char *line, char *key, int keysize, char *value, int valuesize, char chr);
void redtext(unsigned char *str);
float distance(vec3_t pos1, vec3_t pos2);
int bound(int min, int max, int num);
int isip(char *s);
int hex2dec(char a, char b);

size_t strlcat(char *dst, const char *src, size_t siz);
size_t strlcpy(char *dst, const char *src, size_t siz);

