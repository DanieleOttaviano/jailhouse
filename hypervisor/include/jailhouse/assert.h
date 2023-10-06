/*
 * Runtime assert.
 *
 * Copyright (C) Technical University of Munich, 2020
 *
 * Authors:
 *  Andrea Bastoni <andrea.bastoni@tum.de> at https://rtsl.cps.mw.tum.de
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */
#ifndef _ASSERT_H
#define _ASSERT_H

#ifndef CONFIG_DEBUG
/* runtime assert does nothing in non-debug configurations */
#define assert(e) do { } while(0)

#else
extern void __assert_fail(
		const char *file,
		unsigned int line,
		const char *func,
		const char *expr) __attribute__((noreturn));

#define assert(e) \
	do { \
		if (e) { \
			/* empty */ \
		} else { \
			__assert_fail(__FILE__, __LINE__, __FUNCTION__, #e); \
		} \
	} while (0)

#endif /* CONFIG_DEBUG */

#endif
