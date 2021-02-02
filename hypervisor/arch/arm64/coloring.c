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

	/* Similar to LOAD/START: create a linear VA mapping for the HV, which
	 * is colored in PA. The mapping is temporary and is removed after
	 * the root_cell has been copied into the colored PA.
	 */
	if (op->op & COL_OP_ROOT_MAP) {
		bvirt += coloring_root_map_offset;
		return paging_create(op->pg_structs, bphys, bsize, bvirt,
				     op->access_flags, op->paging_flags);
	}

	if (op->op & COL_OP_ROOT_UNMAP) {
		bvirt += coloring_root_map_offset;
		return paging_destroy(op->pg_structs, bvirt, bsize,
				      op->paging_flags);
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

static void do_copy_root(const struct jailhouse_memory *mr)
{
	unsigned long phys_addr;
	unsigned long virt_addr;
	unsigned long tot_size;
	unsigned long size;
	unsigned long dst;
	unsigned long src;

	tot_size = mr->size;

	/* Find the first page that does not belong to the non-colored
	 * mapping */
	phys_addr = mr->phys_start + tot_size;

	while (tot_size > 0) {
		size = MIN(tot_size, NUM_TEMPORARY_PAGES * PAGE_SIZE);
		phys_addr -= size;

		/* If we have reached the beginning (end of copy),
		 * make sure we do not exceed the boundary of the
		 * region */
		if (phys_addr < mr->phys_start) {
			size -= (mr->phys_start - phys_addr);
			phys_addr = mr->phys_start;
		}

		/* cannot fail, mapping area is preallocated */
		paging_create(&this_cpu_data()->pg_structs, phys_addr,
				size, TEMPORARY_MAPPING_BASE,
				PAGE_DEFAULT_FLAGS,
				PAGING_NON_COHERENT | PAGING_NO_HUGE);

		/* Destination: end of colored mapping created via ROOT_MAP */
		/* NOTE: the colored mapping created via ROOT_MAP has
		 * an offset that depends on the virtual_address of
		 * the target mapping. Make sure that the copy is
		 * correct even if phys_addr and virt_addr differ */
		virt_addr = mr->virt_start + (phys_addr - mr->phys_start);
		dst = coloring_root_map_offset + virt_addr + size;
		/* Source: end of non-colored mapping defined above */
		src = TEMPORARY_MAPPING_BASE + size;
		tot_size -= size;

		/* Actual data copy operation */
		while (size > 0) {
			size -= PAGE_SIZE;
			dst -= PAGE_SIZE;
			src -= PAGE_SIZE;
			memcpy((void*)dst, (void*)src, PAGE_SIZE);
		}
	}
}

static void do_uncopy_root(const struct jailhouse_memory *mr)
{
	unsigned long phys_addr;
	unsigned long tot_size;
	unsigned long size;
	unsigned long dst;
	unsigned long src;

	tot_size = mr->size;

	/* Find the first page that does not belong to the non-colored
	 * mapping */
	phys_addr = mr->phys_start;

	while (tot_size > 0) {
		size = MIN(tot_size, NUM_TEMPORARY_PAGES * PAGE_SIZE);

		/* cannot fail, mapping area is preallocated */
		paging_create(&this_cpu_data()->pg_structs, phys_addr,
				size, TEMPORARY_MAPPING_BASE,
				PAGE_DEFAULT_FLAGS,
				PAGING_NON_COHERENT | PAGING_NO_HUGE);

		/* Destination: non-colored mapping */
		dst = TEMPORARY_MAPPING_BASE;
		/* Source: colored mapping created via ROOT_MAP */
		src = coloring_root_map_offset + phys_addr;

		tot_size -= size;
		phys_addr += size;

		while (size > 0) {
			memcpy((void*)dst, (void*)src, PAGE_SIZE);
			dst += PAGE_SIZE;
			src += PAGE_SIZE;
			size -= PAGE_SIZE;
		}
	}
}

int color_copy_root(struct cell *root, bool init)
{
	const struct jailhouse_memory *mr;
	struct color_op op;
	unsigned int n;
	int err;

	if (coloring_way_size == 0) {
		return 0;
	}

	for_each_mem_region(mr, root->config, n) {
		if ((mr->flags & JAILHOUSE_MEM_COLORED) == 0) {
			/* Only copy color regions */
			continue;
		}

		if ((mr->flags & JAILHOUSE_MEM_COLORED_NO_COPY) != 0) {
			/* Skip copy for colored regions where no copy
			 * is explicitely requested. This is useful if
			 * the IPA range that corresponds to the
			 * region is marked as reserved in the
			 * root-cell's DTB and hence contains no
			 * useful data. */
			continue;
		}

		op.pg_structs = &this_cpu_data()->pg_structs;
		op.phys = mr->phys_start;
		op.virt = mr->virt_start;
		op.size = mr->size;
		op.access_flags = PAGE_DEFAULT_FLAGS;
		op.color_mask = mr->colors;
		op.flush_type = 0;

		/* temporary color map */
		op.op = COL_OP_ROOT_MAP;
		op.paging_flags = PAGING_NON_COHERENT | PAGING_NO_HUGE;
		err = color_do_op(&op);
		if (err) {
			return err;
		}

		if (init) {
			/* copy root memory into colored ranges */
			do_copy_root(mr);
		} else {
			/* copy root into non-colored regions */
			do_uncopy_root(mr);
		}

		/* remove mapping */
		op.op = COL_OP_ROOT_UNMAP;
		op.paging_flags = PAGING_NON_COHERENT;
		err = color_do_op(&op);
		if (err) {
			return err;
		}
	}

	return err;
}
