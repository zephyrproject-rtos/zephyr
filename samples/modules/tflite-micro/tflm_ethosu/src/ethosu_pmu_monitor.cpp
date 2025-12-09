/*
 * SPDX-FileCopyrightText: <text>Copyright 2025 Arm Limited and/or its
 * affiliates <open-source-office@arm.com></text>
 * SPDX-License-Identifier: Apache-2.0
 */

/* PMU integration for Ethos-U in Zephyr TFLM sample */

#include <ethosu_driver.h>
#include <pmu_ethosu.h>

#include <cinttypes>
#include <cstdio>

/* Selected PMU events (configured via CMake definitions ETHOSU_PMU_EVENT_N) */
static const enum ethosu_pmu_event_type pmu_events[ETHOSU_PMU_NCOUNTERS] = {
	ETHOSU_PMU_EVENT_0, ETHOSU_PMU_EVENT_1, ETHOSU_PMU_EVENT_2, ETHOSU_PMU_EVENT_3,
#if (ETHOSU_PMU_NCOUNTERS > 4)
	ETHOSU_PMU_EVENT_4, ETHOSU_PMU_EVENT_5, ETHOSU_PMU_EVENT_6, ETHOSU_PMU_EVENT_7,
#endif
};

static inline uint32_t pmu_counter_mask(uint32_t idx)
{
	switch (idx) {
	case 0:
		return ETHOSU_PMU_CNT1_Msk;
	case 1:
		return ETHOSU_PMU_CNT2_Msk;
	case 2:
		return ETHOSU_PMU_CNT3_Msk;
	case 3:
		return ETHOSU_PMU_CNT4_Msk;
#if (ETHOSU_PMU_NCOUNTERS > 4)
	case 4:
		return ETHOSU_PMU_CNT5_Msk;
	case 5:
		return ETHOSU_PMU_CNT6_Msk;
	case 6:
		return ETHOSU_PMU_CNT7_Msk;
	case 7:
		return ETHOSU_PMU_CNT8_Msk;
#endif
	default:
		return 0;
	}
}

extern "C" void ethosu_inference_begin(struct ethosu_driver *drv, void *)
{
	ETHOSU_PMU_Enable(drv);

	for (uint32_t i = 0; i < ETHOSU_PMU_NCOUNTERS; ++i) {
		ETHOSU_PMU_Set_EVTYPER(drv, i, pmu_events[i]);
		ETHOSU_PMU_CNTR_Enable(drv, pmu_counter_mask(i));
	}

	ETHOSU_PMU_PMCCNTR_CFG_Set_Stop_Event(drv, ETHOSU_PMU_NPU_IDLE);
	ETHOSU_PMU_PMCCNTR_CFG_Set_Start_Event(drv, ETHOSU_PMU_NPU_ACTIVE);
	ETHOSU_PMU_CNTR_Enable(drv, ETHOSU_PMU_CCNT_Msk);
	ETHOSU_PMU_CYCCNT_Reset(drv);
	ETHOSU_PMU_EVCNTR_ALL_Reset(drv);
}

extern "C" void ethosu_inference_end(struct ethosu_driver *drv, void *)
{
	const uint64_t cycles = ETHOSU_PMU_Get_CCNTR(drv);

	uint32_t pmu_values[ETHOSU_PMU_NCOUNTERS] = {};
	for (uint32_t i = 0; i < ETHOSU_PMU_NCOUNTERS; ++i) {
		pmu_values[i] = ETHOSU_PMU_Get_EVCNTR(drv, i);
	}
	ETHOSU_PMU_Disable(drv);

	printf("\nEthos-U PMU report:\n");
	printf("ethosu_pmu_cycle_cntr : %" PRIu64 "\n", cycles);
	for (uint32_t i = 0; i < ETHOSU_PMU_NCOUNTERS; ++i) {
		printf("ethosu_pmu_cntr%" PRIu32 " : %" PRIu32 "\n", i, pmu_values[i]);
	}
}
