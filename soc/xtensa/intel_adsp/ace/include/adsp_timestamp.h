/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_INTEL_ADSP_ACE_TIMESTAMP_H_
#define ZEPHYR_SOC_INTEL_ADSP_ACE_TIMESTAMP_H_

#include <stdint.h>

/* Captured timestamp data - contains a copy of all DfTTS snapshot registers. */
struct intel_adsp_timestamp {
	uint32_t iscs;		/* copy of DfISCS register */
	uint64_t lscs;		/* copy of DfLSCS register */
	uint64_t dwccs;		/* copy of DfDWCCS register */
	uint64_t artcs;		/* copy of DfARTCS register */
	uint32_t lwccs;		/* copy of DfLWCCS register */
};

/*
 * @brief Perform timestamping process using DfTTS logic.
 *
 * @param tsctrl Value to be applied to DfTSCTRL register to control timestamping logic
 * @param timestamp Captured timestamp data
 */
int intel_adsp_get_timestamp(uint32_t tsctrl, struct intel_adsp_timestamp *timestamp);

#endif /* ZEPHYR_SOC_INTEL_ADSP_ACE_TIMESTAMP_H_ */
