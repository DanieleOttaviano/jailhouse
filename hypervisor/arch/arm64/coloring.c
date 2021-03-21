/*
 * Jailhouse Cache Coloring Support
 *
 * Copyright (C) Università di Modena e Reggio Emilia, 2018
 * Copyright (C) Boston University, 2020
 * Copyright (C) Technical University of Munich, 2020
 *
 * Authors:
 *  Luca Miccio <lucmiccio@gmail.com>
 *  Renato Mancuso (BU) <rmancuso@bu.edu>
 *  Andrea Bastoni <andrea.bastoni@tum.de> at https://rtsl.cps.mw.tum.de
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See the
 * COPYING file in the top-level directory.
 */
#include <jailhouse/control.h>
#include <jailhouse/paging.h>
#include <jailhouse/printk.h>
#include <jailhouse/unit.h>
#include <jailhouse/cell.h>
#include <jailhouse/coloring.h>
#include <asm/coloring.h>

/**
 *  Only parameter needed to determine the coloring.
 */
u64 coloring_way_size = 0;

/** Temporary load-mapping parameter */
u64 coloring_root_map_offset = 0;

static int dispatch_op(
	struct color_op *op,
	unsigned long bphys,
	unsigned long bvirt,
	unsigned long bsize)
{
	if (op->op & (COL_OP_CREATE | COL_OP_LOAD)) {
		if (op->op & COL_OP_LOAD) {
			/* Fix addr to match the driver's IPA ioremap */
			bvirt += coloring_root_map_offset;
		}
		return paging_create(op->pg_structs, bphys, bsize, bvirt,
				     op->access_flags, op->paging_flags);
	}

	if (op->op & (COL_OP_DESTROY | COL_OP_START)) {
		if (op->op & COL_OP_START) {
			/* Match the address specified during load */
			bvirt += coloring_root_map_offset;
		}
		return paging_destroy(op->pg_structs, bvirt, bsize,
				      op->paging_flags);
	}

	if (op->op & COL_OP_FLUSH) {
		arm_dcache_flush_memory_region(bphys, bsize, op->flush_type);
		return 0;
	}

	return -EINVAL;
}

int color_do_op(struct color_op *op)
{
	unsigned long bvirt, bphys, bsize;
	/* bit: start, low, contiguous bit range width */
	unsigned int bs, bl, bw;
	unsigned int n;
	u64 colors;
	int err;

	col_print("[%c] OP 0x%x: P: 0x%08lx V: 0x%08lx "
			"(S: 0x%lx C: 0x%08llx A: 0x%lx P: 0x%lx F: 0x%d)\n",
			(op->pg_structs == &root_cell.arch.mm) ? 'r' : 'c',
			op->op, op->phys, op->virt, op->size, op->color_mask,
			op->access_flags, op->paging_flags, op->flush_type);

	n = 0;
	bvirt = op->virt;
	bphys = bsize = 0;
	while (bvirt < op->virt + op->size) {
		bs = bl = bw = 0;
		colors = op->color_mask;

		while (colors != 0) {
			/* update colors with next color-range */
			get_bit_range(&colors, &bl, &bw);
			bs += bl;
			bsize = bw * PAGE_SIZE;
			bphys = op->phys + (bs * PAGE_SIZE) +
					(n * coloring_way_size);

			err = dispatch_op(op, bphys, bvirt, bsize);
			if (err)
				return err;

			/* update next round */
			bvirt += bsize;
		}
		n++;
	}

	col_print("end P: 0x%08lx V: 0x%08lx (bsize = 0x%08lx)\n",
			bphys, bvirt - bsize, bsize);

	return err;
}
