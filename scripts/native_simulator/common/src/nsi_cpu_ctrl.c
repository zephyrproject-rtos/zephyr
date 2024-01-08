/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include "nsi_config.h"
#include "nsi_cpun_if.h"
#include "nsi_tracing.h"

static bool cpu_auto_start[NSI_N_CPUS] = {true}; /* Only the first core starts on its own */
static bool cpu_booted[NSI_N_CPUS];

#define CPU_N_RANGE_CHECK(cpu_n) \
	if (cpu_n >= NSI_N_CPUS) { \
		nsi_print_error_and_exit("%s called with cpu_n(%i) >= NSI_N_CPUS (%i)\n", \
					 cpu_n, NSI_N_CPUS); \
	}

void nsi_cpu_set_auto_start(int cpu_n, bool auto_start)
{
	CPU_N_RANGE_CHECK(cpu_n);

	cpu_auto_start[cpu_n] = auto_start;
}

bool nsi_cpu_get_auto_start(int cpu_n)
{
	return cpu_auto_start[cpu_n];
}

void nsi_cpu_auto_boot(void)
{
	for (int i = 0; i < NSI_N_CPUS; i++) {
		if (cpu_auto_start[i] == true) {
			cpu_booted[i] = true;
			nsif_cpun_boot(i);
		}
	}
}

void nsi_cpu_boot(int cpu_n)
{
	CPU_N_RANGE_CHECK(cpu_n);
	if (cpu_booted[cpu_n]) {
		nsi_print_warning("%s called with cpu_n(%i) which was already booted\n",
				  cpu_n);
	}
	cpu_booted[cpu_n] = true;
	nsif_cpun_boot(cpu_n);
}
