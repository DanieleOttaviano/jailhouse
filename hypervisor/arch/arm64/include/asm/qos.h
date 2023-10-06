/*
 * ARM QoS Support for Jailhouse
 *
 * Copyright (c) Boston University, 2020
 *
 * Authors:
 *  Renato Mancuso <rmancuso@bu.edu>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.
 * See the COPYING file in the top-level directory.
 */
#ifndef _JAILHOUSE_ASM_QOS_H
#define _JAILHOUSE_ASM_QOS_H

#include <jailhouse/types.h>
#include <jailhouse/string.h>
#include <jailhouse/qos-common.h>

/* QOS parameter declarations */
struct qos_param {
	char name [QOS_PARAM_NAMELEN];
	__u16 reg;
	__u8 enable;
	__u8 shift;
	__u32 mask;
};

/* Main entry point for QoS management call */
extern int qos_call(unsigned long count, unsigned long settings_ptr);

#endif /* _JAILHOUSE_ASM_QOS_H  */
