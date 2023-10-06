/*
 * Jailhouse Cache Coloring Support
 *
 * Copyright (C) Technical University of Munich, 2020
 *
 * Authors:
 *  Andrea Bastoni <andrea.bastoni@tum.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See the
 * COPYING file in the top-level directory.
 */
/** MSB/LSB function names differs between Jailhouse and Linux */
#define _lsb(x)	ffsl(x)
#define _msb(x)	msbl(x)

/**
 *  Get range of contiguous bits in a bitmask.
 *
 *  The function returns:
 *  - bitmask without the extracted bit range.
 *  - low: original bit position of range start.
 *  - size: size of the range
 *
 *  The function assumes bitmask is not 0.
 */
static inline void get_bit_range(
	u64 *bitmask,
	unsigned int *low,
	unsigned int *size)
{
	unsigned int _range;

	*low = _lsb(*bitmask);
	_range = *bitmask >> *low;
	*bitmask = _range & (_range + 1UL);

	_range = _range ^ *bitmask;
	*size = _msb(_range) + 1;
}
