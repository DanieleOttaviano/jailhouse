/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (C) 2018 OTH Regensburg
 *
 * Authors:
 *  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *  Rohan Tabish <rtabish@illinois.edu> (smc_arg4)
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

static inline long smc(unsigned long id)
{
	register unsigned long __id asm("r0") = id;

	asm volatile ("smc #0\n\t"
		: "+r" (__id)
		: : "memory", "x1", "x2", "x3");

	return __id;
}

static inline long smc_arg1(unsigned long id, unsigned long par1)
{
	register unsigned long __id asm("r0") = id;
	register unsigned long __par1 asm("r1") = par1;

	asm volatile ("smc #0\n\t"
		: "+r" (__id), "+r" (__par1)
		: : "memory", "x2", "x3");

	return __id;
}

/* NOTE: use the same convention as Linux to mark r/w constraints */
static inline long smc_arg2(unsigned long id, unsigned long par1,
			    unsigned long par2)
{
	register unsigned long __id asm("r0") = id;
	register unsigned long __par1 asm("r1") = par1;
	register unsigned long __par2 asm("r2") = par2;
	register unsigned long __tmp3 asm("r3");

	asm volatile ("smc #0\n\t"
		: "+r"(__id), "+r"(__par1), "+r"(__par2), "=&r"(__tmp3)
		: : "memory");

	return __id;
}

static inline int smc_arg4(unsigned long id, unsigned long par1,
			   unsigned long par2, unsigned long par3,
			   unsigned long par4)
{
	register unsigned long __id asm("r0") = id;
	register unsigned long __par1 asm("r1") = par1;
	register unsigned long __par2 asm("r2") = par2;
	register unsigned long __par3 asm("r3") = par3;
	register unsigned long __par4 asm("r4") = par4;

	asm volatile ("smc #0\n\t"
		: "+r"(__id), "+r"(__par1), "+r"(__par2), "+r"(__par3)
		: "r"(__par4)
		: "memory");

	return __id;
}

static inline long smc_arg5(unsigned long id, unsigned long par1,
			    unsigned long par2, unsigned long par3,
			    unsigned long par4, unsigned long par5)
{
	register unsigned long __id asm("r0") = id;
	register unsigned long __par1 asm("r1") = par1;
	register unsigned long __par2 asm("r2") = par2;
	register unsigned long __par3 asm("r3") = par3;
	register unsigned long __par4 asm("r4") = par4;
	register unsigned long __par5 asm("r5") = par5;

	asm volatile ("smc #0\n\t"
		: "+r" (__id), "+r" (__par1), "+r" (__par2), "+r" (__par3),
		  "+r"(__par4), "+r"(__par5)
		: : "memory");

	return __id;
}

static inline long smc_arg7(unsigned long id, unsigned long par1,
			    unsigned long par2, unsigned long par3,
			    unsigned long par4, unsigned long par5,
			    unsigned long par6, unsigned long par7)
{
	register unsigned long __id asm("r0") = id;
	register unsigned long __par1 asm("r1") = par1;
	register unsigned long __par2 asm("r2") = par2;
	register unsigned long __par3 asm("r3") = par3;
	register unsigned long __par4 asm("r4") = par4;
	register unsigned long __par5 asm("r5") = par5;
	register unsigned long __par6 asm("r6") = par6;
	register unsigned long __par7 asm("r7") = par7;

	asm volatile ("smc #0\n\t"
		: "+r" (__id), "+r" (__par1), "+r" (__par2), "+r" (__par3),
		  "+r"(__par4), "+r"(__par5), "+r"(__par6), "+r"(__par7)
		: : "memory");

	return __id;
}
