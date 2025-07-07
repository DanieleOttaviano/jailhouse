/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Omnivisor demo RISC-V
 *
 * Copyright (c) Daniele Ottaviano, 2024
 *
 * Authors:
 *   Daniele Ottaviano <danieleottaviano97@gmail.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */
/*
 * Simple bandwidth benchmark on a RISC‑V bare‑metal core
 *   – touches one cache‑line per call to bench_run()
 *   – prints MB/s to shared memory for a Linux core to read
 */

#include <stdint.h>

/* ---------- Hardware/Memory layout ------------------------------------- */
#define SYSTEM_COUNTER_ADDR   0xFF250000      /* 100 MHz free‑running counter */
#define MEM_ARRAY_BASE        0x70FF0000
#define SHARED_MEM_ADDR       0x46D00000

/* ---------- Parameters you can tweak --------------------------------------------*/
#define MEM_ARRAY_SIZE        (32 * 1024) 			/* 32 KiB test buffer 		   */
#define SYSTEM_COUNTER_FREQ   100000000ULL    		/* 100 MHz                     */
#define MEASURE_PERIOD_MS     1000            		/* print result every 1000 ms  */
#define BYTES_PER_RUN         4 			  		/* One word (4 bytes) */

/* ---------- Shared‑memory bookkeeping ---------------------------------- */
#define SHARED_MEM_SIZE       1024
#define FLAG_INDEX            (SHARED_MEM_SIZE - 1)

/* ---------- Low‑level world touch --------------------------------- */
static inline void modify_world(void *addr)
{
    uint32_t *p = (uint32_t *)addr;
    __asm__ volatile ("" : : : "memory");
    p[0] = (uint32_t)(uintptr_t)addr;   /* read‑modify‑write one word     */
    __asm__ volatile ("" : : : "memory");
}


/* Bench: touch each cache line once in mem_array */
void bench_run(uint32_t *mem_array) {
    for (uint32_t i = 0; i < (MEM_ARRAY_SIZE / sizeof(uint32_t)); i++) {
        modify_world(&mem_array[i]);
    }
}

/* ---------------------------------------------------------------------- */
void main(void)
{
    volatile uint32_t *syscnt   = (uint32_t *)SYSTEM_COUNTER_ADDR;
    volatile uint32_t *shm      = (uint32_t *)SHARED_MEM_ADDR;
    uint32_t          *mem_array= (uint32_t *)MEM_ARRAY_BASE;

    /* clear shared memory */
    for (uint32_t i = 0; i < SHARED_MEM_SIZE; i++)
        shm[i] = 0;

    /* optional: init buffer so it resides in DRAM, not BSS‑zero page */
    for (uint32_t i = 0; i < MEM_ARRAY_SIZE / sizeof(uint32_t); i++)
        mem_array[i] = i;

    /* measurement window in counter ticks */
    const uint32_t delay_ticks = (MEASURE_PERIOD_MS *
                                 (SYSTEM_COUNTER_FREQ / 1000));

    uint32_t start_ticks = *syscnt;
    uint32_t end_ticks   = start_ticks + delay_ticks;
    uint32_t runs        = 0;

    while (1) {
        /* ------------------------------------------------- main hot loop */
        bench_run(mem_array);
        runs++;

        uint32_t now = *syscnt;

        /* end of 1000 ms window?  (handle wrap‑around with unsigned diff) */
        if ((uint32_t)(now - end_ticks) < 0x80000000U) {
            // uint32_t bytes_accessed= (uint32_t)runs * BYTES_PER_RUN;
			uint32_t bytes_accessed = (uint64_t)runs * MEM_ARRAY_SIZE;

            uint32_t elapsed_ticks = now - start_ticks;
            uint32_t elapsed_ns    = ((uint32_t)elapsed_ticks * 1000000000ULL)
                                     / SYSTEM_COUNTER_FREQ;

			/* compute bandwidth in KB/s */
			uint32_t bw_KBps = ((uint32_t)(bytes_accessed >> 10) * 1000) /
			                   (uint32_t)(elapsed_ns / 1000000ULL);

            /* write results for Linux side */
            shm[0] = (uint32_t)bw_KBps;        /* KB/s          */
            shm[1] = (uint32_t)bytes_accessed; /* bytes touched */
            shm[2] = (uint32_t)elapsed_ns;     /* ns elapsed    */
            shm[3] = runs;                     /* #iterations   */

            shm[FLAG_INDEX] = 1;               /* signal ready  */

            /* wait until Linux clears the flag */
            while (shm[FLAG_INDEX] != 0);

            /* reset for next window */
            start_ticks = *syscnt;
            end_ticks   = start_ticks + delay_ticks;
            runs        = 0;
        }
    }
}
