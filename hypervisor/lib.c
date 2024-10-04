/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (c) Siemens AG, 2013
 *
 * Authors:
 *  Jan Kiszka <jan.kiszka@siemens.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <jailhouse/string.h>

void *memset(void *s, int c, size_t n)
{
	u8 *p = s;

	while (n-- > 0)
		*p++ = c;
	return s;
}

int strcmp(const char *s1, const char *s2)
{
	while (*s1 == *s2) {
		if (*s1 == '\0')
			return 0;
		s1++;
		s2++;
	}
	return *(unsigned char *)s1 - *(unsigned char *)s2;
}

int strncmp(const char *s1, const char *s2, size_t n)
{
	while (*s1 == *s2 && n--) {
		if (*s1 == '\0')
			return 0;
		s1++;
		s2++;
	}
	return *(unsigned char *)s1 - *(unsigned char *)s2;
}

void *memcpy(void *dest, const void *src, size_t n)
{
	const u8 *s = src;
	u8 *d = dest;

	while (n-- > 0)
		*d++ = *s++;
	return dest;
}

void *memmove(void *d, const void *s, size_t n)
{

	u8* from = (u8*)s;
	u8* to = (u8*) d;

	if (from == to || n == 0)
		return d;
	if (to > from && to-from < (int)n) {
		/* to overlaps with from */
		/*  <from......>         */
		/*         <to........>  */
		/* copy in reverse, to avoid overwriting from */
		int i;
		for(i=n-1; i>=0; i--)
			to[i] = from[i];
		return d;
	}
	if (from > to && from-to < (int)n) {
		/* to overlaps with from */
		/*        <from......>   */
		/*  <to........>         */
		/* copy forwards, to avoid overwriting from */
		size_t i;
		for(i=0; i<n; i++)
			to[i] = from[i];
		return d;
	}
	memcpy(d, s, n);
	return d;



}
