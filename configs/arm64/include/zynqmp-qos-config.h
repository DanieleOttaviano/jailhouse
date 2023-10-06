/*
 * QoS configuration for ZCU102.
 *
 * Copyright (c) Boston University, 2020
 * Copyright (C) Technical University of Munich, 2020
 *
 * Authors:
 *  Rohan Tabish <rtabish@illinois.edu>
 *  Andrea Bastoni <andrea.bastoni@tum.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

/* List QoS register offsets here, so that we don't have to search
 * them in the manuals each time...
 */
/* LPD_OFFSET: (0xFE100000) - (0xFD700000) = 0xA00000 */
#define LPD_OFFSET 		0xA00000

/* Peripherials in LPD with QoS Support */
#define M_RPU0_BASE		LPD_OFFSET + (0x42100)
#define M_RPU1_BASE		LPD_OFFSET + (0x43100)
#define M_ADMA_BASE 		LPD_OFFSET + (0x44100)
#define M_AFIFM6_BASE		LPD_OFFSET + (0x45100)
#define M_DAP_BASE		LPD_OFFSET + (0x47100)
#define M_USB0_BASE		LPD_OFFSET + (0x48100)
#define M_USB1_BASE		LPD_OFFSET + (0x49100)
#define M_INTIOU_BASE		LPD_OFFSET + (0x4A100)
#define M_INTCSUPMU_BASE	LPD_OFFSET + (0x4B100)
#define M_INTLPDINBOUND_BASE	LPD_OFFSET + (0x4C100)
#define M_INTLPDOCM_BASE	LPD_OFFSET + (0x4D100)
#define M_IB5_BASE 		LPD_OFFSET + (0xC3100)
#define M_IB6_BASE		LPD_OFFSET + (0xC4100)
#define M_IB8_BASE 		LPD_OFFSET + (0xC5100)
#define M_IB0_BASE		LPD_OFFSET + (0xC6100)
#define M_IB11_BASE 		LPD_OFFSET + (0xC7100)
#define M_IB12_BASE 		LPD_OFFSET + (0xC8100)

/* Peripherials in FPD with QoS Support */
#define M_INTFPDCCI_BASE 	(0x42100)
#define M_INTFPDSMMUTBU3_BASE	(0x43100)
#define M_INTFPDSMMUTBU4_BASE	(0x44100)
#define M_AFIFM0_BASE		(0x45100)
#define M_AFIFM1_BASE		(0x46100)
#define M_AFIFM2_BASE		(0x47100)
#define M_INITFPDSMMUTBU5_BASE	(0x48100)
#define M_DP_BASE		(0x49100)
#define M_AFIFM3_BASE		(0x4A100)
#define M_AFIFM4_BASE		(0x4B100)
#define M_AFIFM5_BASE		(0x4C100)
#define M_GPU_BASE 		(0x4D100)
#define M_PCIE_BASE		(0x4E100)
#define M_GDMA_BASE		(0x4F100)
#define M_SATA_BASE 		(0x50100)
#define M_CORESIGHT_BASE	(0x52100)
#define ISS_IB2_BASE		(0xC2100)
#define ISS_IB6_BASE		(0xC3100)
