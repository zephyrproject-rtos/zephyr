/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC__H_
#define _SOC__H_

#include <zephyr/sys/util.h>

#ifndef _ASMLANGUAGE

#include <fsl_common.h>

#include <zephyr/devicetree.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Bring-up steps that can give up, reported as the fatal-error reason. The
 * console does not exist yet when they run, so the identifier is the whole
 * diagnostic.
 */
enum soc_early_init_step {
	SOC_STEP_ELE_SEND,
	SOC_STEP_ELE_RECEIVE,
	SOC_STEP_ELE_REFUSED,
	SOC_STEP_LLC_TAG_INIT,
	SOC_STEP_LLC_DATA_INIT,
};

FUNC_NORETURN void soc_early_init_failed(enum soc_early_init_step step);

/*
 * Bound on the pre-console polling loops. A bounded loop that reports through
 * the fatal path names the step that gave up, where a hang would not. A
 * liveness guard, not a timing specification.
 */
#define SOC_POLL_LIMIT 1000000U

#define SOC_POLL_UNTIL(cond, step)                                                                 \
	do {                                                                                       \
		uint32_t _spins = SOC_POLL_LIMIT;                                                  \
                                                                                                   \
		while (!(cond)) {                                                                  \
			if (--_spins == 0U) {                                                      \
				soc_early_init_failed(step);                                       \
			}                                                                          \
		}                                                                                  \
	} while (false)

/* Hand the Resource Domain Controller and the DMA masters to the CPU's domain. */
void soc_trdc_setup(void);

/*
 * Way-valid mask for LLC whole-cache maintenance. The boot ROM partitions most
 * ways as scratchpad SRAM (CCUSPCR0[NUMSCPADWAYS]); only the remaining ways are
 * cache. A whole-cache flush/invalidate must target exactly those cache ways --
 * the CCUCMWVR 0xFF reset value would operate on scratchpad SRAM instead. The
 * mask is derived from the live scratchpad partition, so it tracks whatever the
 * ROM configured. Returns a WAYVALID bitmask (bit i set => way i is cache).
 */
uint32_t soc_llc_way_valid_mask(void);


#ifdef __cplusplus
}
#endif

#endif /* !_ASMLANGUAGE */

#endif /* _SOC__H_ */
