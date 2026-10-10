/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/cpu_freq/cpu_freq.h>
#include <zephyr/cpu_freq/pstate.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>

#include "clock_control/clock_stm32_ll_common.h"

#define CPU_BOOT_HZ   DT_PROP(DT_NODELABEL(cpusw), clock_frequency)
#define CPU_PARENT_HZ ((uint64_t)CPU_BOOT_HZ * DT_PROP(DT_NODELABEL(ic1), ic_div))

/* Voltage remains at the boot setting; only the IC1 divider changes. */
struct stm32n6_pstate {
	uint32_t frequency;
};

#define DEFINE_STM32N6_PSTATE(node_id)                                                             \
	BUILD_ASSERT(DT_NODE_HAS_COMPAT(node_id, st_stm32n6_pstate),                               \
		     "STM32N6 requires st,stm32n6-pstate nodes");                                  \
	BUILD_ASSERT(DT_PROP(node_id, clock_frequency) > 0U &&                                     \
			     DT_PROP(node_id, clock_frequency) <= CPU_BOOT_HZ,                     \
		     "P-state frequency must not exceed the boot CPU frequency");                  \
	BUILD_ASSERT(CPU_PARENT_HZ % DT_PROP(node_id, clock_frequency) == 0U &&                    \
			     CPU_PARENT_HZ / DT_PROP(node_id, clock_frequency) >= 1U &&            \
			     CPU_PARENT_HZ / DT_PROP(node_id, clock_frequency) <= 256U,            \
		     "P-state requires an exact IC1 divider from 1 through 256");                  \
	static const struct stm32n6_pstate CONCAT(stm32n6_pstate_, node_id) = {                    \
		.frequency = DT_PROP(node_id, clock_frequency),                                    \
	};                                                                                         \
	PSTATE_DT_DEFINE(node_id, &CONCAT(stm32n6_pstate_, node_id))

DT_FOREACH_CHILD_STATUS_OKAY(DT_PATH(performance_states), DEFINE_STM32N6_PSTATE)

int cpu_freq_pstate_set(const struct pstate *state)
{
	const struct stm32n6_pstate *cfg;

	if (state == NULL || state->config == NULL) {
		return -EINVAL;
	}

	cfg = state->config;
	return stm32_clock_control_set_cpu_rate(cfg->frequency);
}
