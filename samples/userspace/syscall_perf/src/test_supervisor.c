/*
 * Copyright (c) 2020 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <stdio.h>

/*
 * 0xB00 is CSR mcycle
 * 0xB02 is CSR minstret
 */
void supervisor_thread_function(void *p1, void *p2, void *p3)
{
	register unsigned long cycle_before, cycle_count;
	register unsigned long inst_before, inst_count;
	k_tid_t thread;

	printf("Supervisor thread started\n");

	while (1) {
		k_sleep(K_MSEC(2000));

		inst_before = csr_read(0xB02);
		cycle_before = csr_read(0xB00);
		thread = k_current_get();
		cycle_count = csr_read(0xB00);
		inst_count = csr_read(0xB02);

		if (cycle_count > cycle_before) {
			cycle_count -= cycle_before;
		} else {
			cycle_count += 0xFFFFFFFF - cycle_before;
		}

		if (inst_count > inst_before) {
			inst_count -= inst_before;
		} else {
			inst_count += 0xFFFFFFFF - inst_before;
		}

		/* Remove CSR accesses to be more accurate */
		inst_count -= 3;

		printf("Supervisor thread(%p):\t%8lu cycles\t%8lu instructions\n",
			thread, cycle_count, inst_count);
	}
}
