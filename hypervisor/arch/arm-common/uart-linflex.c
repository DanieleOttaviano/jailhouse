/*
 * Jailhouse UART drivers for LINFlexD UART tx console.
 *
 * Includes from the fsl_linflexuart.c in Linux.
 * Copyright 2012-2016 Freescale Semiconductor, Inc.
 * Copyright 2017-2020 NXP
 * Copyright (C) Boston University, 2020
 * Copyright (C) Technical University of Munich, 2020
 *
 * Authors:
 *  Renato Mancuso (BU) <rmancuso@bu.edu>
 *  Andrea Bastoni <andrea.bastoni@tum.de> at https://rtsl.cps.mw.tum.de
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <jailhouse/mmio.h>
#include <jailhouse/uart.h>

/* All registers are 32-bit width */

#define LINCR1	0x0000	/* LIN control register				*/
#define LINIER	0x0004	/* LIN interrupt enable register		*/
#define LINSR	0x0008	/* LIN status register				*/
#define LINESR	0x000C	/* LIN error status register			*/
#define UARTCR	0x0010	/* UART mode control register			*/
#define UARTSR	0x0014	/* UART mode status register			*/
#define LINTCSR	0x0018	/* LIN timeout control status register		*/
#define LINOCR	0x001C	/* LIN output compare register			*/
#define LINTOCR	0x0020	/* LIN timeout control register			*/
#define LINFBRR	0x0024	/* LIN fractional baud rate register		*/
#define LINIBRR	0x0028	/* LIN integer baud rate register		*/
#define LINCFR	0x002C	/* LIN checksum field register			*/
#define LINCR2	0x0030	/* LIN control register 2			*/
#define BIDR	0x0034	/* Buffer identifier register			*/
#define BDRL	0x0038	/* Buffer data register least significant	*/
#define BDRM	0x003C	/* Buffer data register most significant	*/
#define IFER	0x0040	/* Identifier filter enable register		*/
#define IFMI	0x0044	/* Identifier filter match index		*/
#define IFMR	0x0048	/* Identifier filter mode register		*/
#define GCR	0x004C	/* Global control register			*/
#define UARTPTO	0x0050	/* UART preset timeout register			*/
#define UARTCTO	0x0054	/* UART current timeout register		*/
/* The offsets for DMARXE/DMATXE in master mode only			*/
#define DMATXE	0x0058	/* DMA Tx enable register			*/
#define DMARXE	0x005C	/* DMA Rx enable register			*/

#define LINFLEXD_LINCR1_INIT		(1 << 0)
#define LINFLEXD_LINSR_LINS_INITMODE	(1 << 12)
#define LINFLEXD_LINSR_LINS_MASK	(0xF << 12)

#define LINFLEXD_LINIER_DRIE		(1 << 2)
#define LINFLEXD_LINIER_DTIE		(1 << 1)

#define LINFLEXD_UARTCR_RXEN		(1 << 5)
#define LINFLEXD_UARTCR_TXEN		(1 << 4)
#define LINFLEXD_UARTCR_PC0		(1 << 3)
#define LINFLEXD_UARTCR_WL0		(1 << 1)
#define LINFLEXD_UARTCR_UART		(1 << 0)

#define LINFLEXD_UARTCR_RFBM		(1 << 9)
#define LINFLEXD_UARTCR_TFBM		(1 << 8)
#define LINFLEXD_UARTCR_WL1		(1 << 7)
#define LINFLEXD_UARTCR_PC1		(1 << 6)

#define LINFLEXD_UARTSR_DRFRFE		(1 << 2)
#define LINFLEXD_UARTSR_DTFTFF		(1 << 1)

static void uart_init(struct uart_chip *chip)
{
	return;
}

static bool uart_is_busy(struct uart_chip *chip)
{
	/* NOTE: it seems that the LINFlexD UART should be checked for
	 * busy transfers in Buffered Mode _after_ writing something.
	 * Checking before writing a char doesn't cause the HW to do
	 * the transmission. We poll after each write_char and fake
	 * Jailhouse here.
	 */
	return false;
}

static void uart_write_char(struct uart_chip *chip, char c)
{
	mmio_write32(chip->virt_base + BDRL, c);

	/* Poll */
	while ((mmio_read32(chip->virt_base + UARTSR) & LINFLEXD_UARTSR_DTFTFF)
			!= LINFLEXD_UARTSR_DTFTFF)
		;

	/* Reset */
	mmio_write32(chip->virt_base + UARTSR,
		     (mmio_read32(chip->virt_base + UARTSR) |
		      LINFLEXD_UARTSR_DTFTFF));
}

static unsigned long saved_cr;
static void uart_hyp_enter(struct uart_chip *chip)
{
	unsigned long cr;
	unsigned long lincr;

	/* Put the device in init mode */
	lincr = mmio_read32(chip->virt_base + LINCR1);
	lincr |= (LINFLEXD_LINCR1_INIT);
	mmio_write32(chip->virt_base + LINCR1, lincr);

	while ((mmio_read32(chip->virt_base + LINSR) & LINFLEXD_LINSR_LINS_MASK)
			!= LINFLEXD_LINSR_LINS_INITMODE);

	/* Switch to Buffer Mode: avoid conflicing with Linux
	 * settings. We don't have full DMA visibility on what Linux is
	 * doing and staying in FIFO mode doesn't work for us i.e.,
	 * LINFLEXD_UARTCR_RFBM, LINFLEXD_UARTCR_TFBM should not be set.
	 */
	cr = mmio_read32(chip->virt_base + UARTCR);
	saved_cr = cr;

	/* UART, 8bit, no parity, TXenabled only, buffer mode */
	cr = (LINFLEXD_UARTCR_UART | LINFLEXD_UARTCR_WL0);
	cr |= (LINFLEXD_UARTCR_PC1 | LINFLEXD_UARTCR_PC0);
	cr |= LINFLEXD_UARTCR_TXEN;
	mmio_write32(chip->virt_base + UARTCR, cr);

	/* End Init Mode */
	lincr &= ~(LINFLEXD_LINCR1_INIT);
	mmio_write32(chip->virt_base + LINCR1, lincr);
}

static void uart_hyp_leave(struct uart_chip *chip)
{
	unsigned long lincr;

	lincr = mmio_read32(chip->virt_base + LINCR1);
	lincr |= (LINFLEXD_LINCR1_INIT);
	mmio_write32(chip->virt_base + LINCR1, lincr);

	while ((mmio_read32(chip->virt_base + LINSR) & LINFLEXD_LINSR_LINS_MASK)
			!= LINFLEXD_LINSR_LINS_INITMODE);

	/* Switch back to previous mode, requires re-init */
	mmio_write32(chip->virt_base + UARTCR, saved_cr);

	lincr &= ~(LINFLEXD_LINCR1_INIT);
	mmio_write32(chip->virt_base + LINCR1, lincr);
}

struct uart_chip uart_linflex_ops = {
	.init = uart_init,
	.is_busy = uart_is_busy,
	.write_char = uart_write_char,
	.hyp_mode_enter = uart_hyp_enter,
	.hyp_mode_leave = uart_hyp_leave,
};

