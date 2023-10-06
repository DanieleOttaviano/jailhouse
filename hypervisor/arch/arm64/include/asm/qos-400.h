/*
 * ARM QoS Support for Jailhouse. Definitions for the QoS-400 NIC.
 *
 * Copyright (c) Boston University, 2020
 *
 * Authors:
 *  Renato Mancuso <rmancuso@bu.edu>
 *  Rohan Tabish <rtabish@illinois.edu>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.
 * See the COPYING file in the top-level directory.
 */
#ifndef _JAILHOUSE_ARM64_QOS_400_H
#define _JAILHOUSE_ARM64_QOS_400_H

/* Board-independent QoS support */
#define FLAGS_HAS_RWQOS		(1 << 0)
#define FLAGS_HAS_REGUL		(1 << 1)
#define FLAGS_HAS_DYNQOS	(1 << 2)

/* Offsets of control registers from beginning of device-specific
 * config space */

/* The typical QoS interface has the following layout:
 *
 * BASE: 0x??80
 * read_qos    = BASE
 * write_qos   = + 0x04
 * fn_mod      = + 0x08
----- REGULATION ------
 * qos_cntl    = + 0x0C
 * max_ot      = + 0x10
 * max_comb_ot = + 0x14
 * aw_p        = + 0x18
 * aw_b        = + 0x1C
 * aw_r        = + 0x20
 * ar_p        = + 0x24
 * ar_b        = + 0x28
 * ar_r        = + 0x2C
----- DYNAMIC QOS -----
 * tgt_latency = + 0x30
 * ki          = + 0x34
 * qos_range   = + 0x38
 */

#define READ_QOS           0x00
#define WRITE_QOS          0x04
#define FN_MOD             0x08
#define QOS_CNTL           0x0C
#define MAX_OT             0x10
#define MAX_COMB_OT        0x14
#define AW_P               0x18
#define AW_B               0x1C
#define AW_R               0x20
#define AR_P               0x24
#define AR_B               0x28
#define AR_R               0x2C
#define TGT_LATENCY        0x30
#define KI                 0x34
#define QOS_RANGE          0x38

/* QOS_CNTL REgister  */
#define EN_AWAR_OT_SHIFT    (7)
#define EN_AR_OT_SHIFT      (6)
#define EN_AW_OT_SHIFT      (5)
#define EN_AR_LATENCY_SHIFT (4)
#define EN_AW_LATENCY_SHIFT (3)
#define EN_AWAR_RATE_SHIFT  (2)
#define EN_AR_RATE_SHIFT    (1)
#define EN_AW_RATE_SHIFT    (0)
#define EN_NO_ENABLE        (31)

/* Number of settable QoS parameters */
#define QOS_PARAMS          22

/* Bit fields and masks in control registers  */
#define READ_QOS_SHIFT      (0)
#define READ_QOS_MASK       (0x0f)
#define WRITE_QOS_SHIFT     (0)
#define WRITE_QOS_MASK      (0x0f)

#define AW_MAX_OTF_SHIFT    (0)
#define AW_MAX_OTI_SHIFT    (8)
#define AR_MAX_OTF_SHIFT    (16)
#define AR_MAX_OTI_SHIFT    (24)
#define AW_MAX_OTF_MASK     (0xff)
#define AW_MAX_OTI_MASK     (0x3f)
#define AR_MAX_OTF_MASK     (0xff)
#define AR_MAX_OTI_MASK     (0x3f)

#define AWAR_MAX_OTF_SHIFT  (0)
#define AWAR_MAX_OTI_SHIFT  (8)
#define AWAR_MAX_OTF_MASK   (0xff)
#define AWAR_MAX_OTI_MASK   (0x7f)

#define AW_P_SHIFT          (24)
#define AW_B_SHIFT          (0)
#define AW_R_SHIFT          (20)
#define AW_P_MASK           (0xff)
#define AW_B_MASK           (0xffff)
#define AW_R_MASK           (0xfff)

#define AR_P_SHIFT          (24)
#define AR_B_SHIFT          (0)
#define AR_R_SHIFT          (20)
#define AR_P_MASK           (0xff)
#define AR_B_MASK           (0xffff)
#define AR_R_MASK           (0xfff)

#define AR_TGT_LAT_SHIFT    (16)
#define AW_TGT_LAT_SHIFT    (0)
#define AR_TGT_LAT_MASK     (0xfff)
#define AW_TGT_LAT_MASK     (0xfff)

#define AR_KI_SHIFT         (8)
#define AW_KI_SHIFT         (0)
#define AR_KI_MASK          (0x7)
#define AW_KI_MASK          (0x7)

#define AR_MAX_QOS_SHIFT    (24)
#define AR_MIN_QOS_SHIFT    (16)
#define AW_MAX_QOS_SHIFT    (8)
#define AW_MIN_QOS_SHIFT    (0)
#define AR_MAX_QOS_MASK     (0xf)
#define AR_MIN_QOS_MASK     (0xf)
#define AW_MAX_QOS_MASK     (0xf)
#define AW_MIN_QOS_MASK     (0xf)

#endif
