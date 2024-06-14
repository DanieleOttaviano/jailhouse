/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Memory Bomb and Util-stress bare-metal Jailhouse inmate util-macros
 *
 * Copyright (C) Boston University, MA, USA, 2020
 * Copyright (C) Technical University of Munich, 2020 - 2021
 * Copyright (C) Minerva Systems, 2022
 *
 * Authors:
 *  Renato Mancuso <rmancuso@bu.edu>
 *  Andrea Bastoni <andrea.bastoni@tum.de>
 *  Mirko Cangiano <mirko.cangiano@minervasys.tech>
 *  Fabio Spanò <fabio.spano@minervasys.tech>
 *  Filippo Fontana <filippo.fontana@minervasys.tech>
 */
#include "config.h"

#define xstr(s) str(s)
#define str(s) #s

/* NOTE: This controls the settings in the template memory bombs inmates */
#define NUM_CPU			4
#define BOMB_CPU		1 << (BOMB_ID + 1)


/*NOTE: Choose on which architecture you are running membomb to use the correct memory addresses*/
//#define CONFIG_ZCU102
//#define CONFIG_MACH_NXP_S32
// #define CONFIG_MACH_NXP_S32g2
#define CONFIG_MACH_NXP_IMX8MQ
//#define CONFIG_ZCU104

/**
 * NOTE: Hacky but effective way of configure different parameters
 * for different DRAM layouts on ZCU104, ZCU102 and S32V.
 *
 * The PHYS values controls the placement in physical memory, the VIRT
 * address doesn't change (MEM_VIRT_START). The regions identify the buffers
 * where different memory bombs / util-stress utilities for the different
 * CPUs will operate.
 *
 * Changing the values here affects: the config templates (phys values,
 * and phys 2 virt configuration), the inmates mem-bomb (mapping).
 */
#ifdef CONFIG_MACH_NXP_S32
/* S32: start at high 256 of DDR0 and let the other go in the DDR1 */
//#define MAIN_PHYS_BASE		0xb0000000
#define COMM_PHYS_BASE		0xa0000000
/* S32: same DDR0 MC, different banks, reduce linux to only 256 MB */
//#define MAIN_PHYS_BASE		0x90000000
#define MAIN_PHYS_BASE		0xc0000000
//#define COMM_PHYS_BASE		0xc0000000 // move comm in the high DDR
#endif
#ifdef CONFIG_MACH_NXP_S32g2
#define MAIN_PHYS_BASE		0x884000000
#define COMM_PHYS_BASE		0x8A0000000
#endif
#ifdef CONFIG_MACH_NXP_IMX8MQ
#define MAIN_PHYS_BASE		0xC0010000
#define COMM_PHYS_BASE		0xF0000000
#endif
#ifdef CONFIG_ZCU104
#define COMM_PHYS_BASE		0x50020000
#define MAIN_PHYS_BASE		0x57c00000
#endif
#ifdef CONFIG_ZCU102
/* ZCU102 */
#define MAIN_PHYS_BASE		0x800200000	// high dram
//#define MAIN_PHYS_BASE		0x020000000	// low dram
#define COMM_PHYS_BASE		0x87c000000
#endif
/* Main program */
#define MAIN_SIZE		0x200000
/* Memory Area for the experiments */
#define MEM_SIZE_MB		64
#define MEM_SIZE		(MEM_SIZE_MB * 1024 * 1024UL)

/* To make them hit different banks (typically ~256 MiB) add a mult factor */
//#define MAIN_PHYS_START		MAIN_PHYS_BASE + BOMB_ID * 4  * (MEM_SIZE + MAIN_SIZE)
#define MAIN_PHYS_START		MAIN_PHYS_BASE + BOMB_ID * (MEM_SIZE + MAIN_SIZE)
/* For colored combinations, same start for all */
//#define MAIN_PHYS_START		MAIN_PHYS_BASE + (MEM_SIZE + MAIN_SIZE)

#define MEM_PHYS_START		MAIN_PHYS_START + MAIN_SIZE

#define COMM_PHYS_ADDR		(COMM_PHYS_BASE + BOMB_ID * 0x1000)

/* Virtual addresses */
#define MEM_VIRT_START		MAIN_SIZE
#define COMM_VIRT_ADDR		MAIN_SIZE + MEM_SIZE

/* COMM SINGLE SIZE should match a PAGE_SIZE */
#define COMM_SINGLE_SIZE	0x1000
#define COMM_TOTAL_SIZE		COMM_SINGLE_SIZE * NUM_CPU

#define LINE_SIZE		(64) //Cache line size is 64 bytes
#define LOG2_LINE_SIZE		(6)
#define BIT(x)			(1UL << x)

/* Virtual address of command and control interface */
#define CMD_ENABLE		BIT(0)
#define CMD_DO_READS		BIT(1)
#define CMD_DO_WRITES		BIT(2)
#define CMD_VERBOSE		BIT(3)
#define CMD_MEMGUARD		BIT(4)
#define CMD_UTILSTRESS		BIT(5)
#define CMD_UTILSTRESS_TEST	BIT(6)
