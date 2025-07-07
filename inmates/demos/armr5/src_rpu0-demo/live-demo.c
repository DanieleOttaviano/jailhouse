/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Omnivisor demo RPU: live demo execution time.
 * Complete demo is available at: 
 *  https://github.com/DanieleOttaviano/test_omnivisor_host/tree/main/demo 
 *  https://github.com/DanieleOttaviano/test_omnivisor_guest/tree/main/demo
 * 
 * Copyright (c) Daniele Ottaviano, 2024
 *
 * Authors:
 *   Daniele Ottaviano <danieleottaviano97@gmail.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */
#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"
#include "xtime_l.h"
#include "sleep.h"
#include "xil_cache.h"

#define NPAGES 1024
#define DIM (12 * NPAGES)   // 12*1024 = 12288
#define REP 10
#define FREQUENCY COUNTS_PER_USECOND
#define PERIOD 200000 // 200 ms

#define SHM_BASE       ((volatile u32 *)0x46d00000)
#define COUNT          128
#define WRITE_PTR_IDX  COUNT
#define READ_PTR_IDX   (COUNT + 1)

int main() {
    u32 *mem_array = (u32 *)0x3EE00000;
    volatile u32 *shared_memory = SHM_BASE;

    XTime start, end, diff;
    u32 readsum = 0;
    u32 time_us = 0;
    int i, j;

    init_platform();
    Xil_DCacheDisable();
    Xil_ICacheDisable();

    xil_printf("[RPU-0] Start!\n\r");

    // Initialize memory array
    for (i = 0; i < DIM; i++) {
        mem_array[i] = i;
    }

    while(1) {
        // Perform memory read workload
        XTime_GetTime(&start);
        for (j = 0; j < REP; j++) {
            for (i = 0; i < DIM; i++) {
                readsum += mem_array[i]; // Simulate READ
            }
        }
        XTime_GetTime(&end);

        diff = end - start;
        time_us = diff / FREQUENCY;

        // --- Ring buffer write ---
        u32 write_ptr = shared_memory[WRITE_PTR_IDX];
        u32 read_ptr  = shared_memory[READ_PTR_IDX];

        // Check if buffer is full (optional, can block or drop)
        u32 next_ptr = (write_ptr + 1) % COUNT;
        if (next_ptr == read_ptr) {
            xil_printf("[RPU-0] WARNING: Buffer full! Dropping value.\n\r");
        } else {
            shared_memory[write_ptr] = time_us;
            shared_memory[WRITE_PTR_IDX] = next_ptr;
        }

        xil_printf("[RPU-0] time(us): %u\n\r", time_us);

        // Wait until the full PERIOD has passed
        while (time_us < PERIOD) {
            XTime_GetTime(&end);
            diff = end - start;
            time_us = diff / FREQUENCY;
        }
    }

    xil_printf("[RPU-0] Done.\n\r");
    cleanup_platform();
    return 0;
}
