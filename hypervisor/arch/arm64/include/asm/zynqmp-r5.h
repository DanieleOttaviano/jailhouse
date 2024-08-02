/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Zynqmp RPUs and TCM management
 *
 * Copyright (c) Universita' di Napoli Federico II, 2024
 *
 * Authors:
 *   Daniele Ottaviano <daniele.ottaviano@unina.it>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#ifndef _JAILHOUSE_ZYNQMP_R5_H
#define _JAILHOUSE_ZYNQMP_R5_H

#include <asm/zynqmp-pm.h>

#define ZYNQMP_R5_TCMA_ID	0
#define ZYNQMP_R5_TCMB_ID	1

#if defined(CONFIG_OMNIVISOR) && defined(CONFIG_MACH_ZYNQMP_ZCU102)

int zynqmp_r5_start(enum pm_node_id node_id, u32 bootaddr);
int zynqmp_r5_stop(enum pm_node_id node_id);
int zynqmp_r5_tcm_request(int tcm_id);
int zynqmp_r5_tcm_release(int tcm_id);

#endif /* CONFIG_OMNIVSIOR && CONFIG_MACH_ZYNQMP_ZCU102 */

#endif /* _JAILHOUSE_ZYNQMP_R5_H */
