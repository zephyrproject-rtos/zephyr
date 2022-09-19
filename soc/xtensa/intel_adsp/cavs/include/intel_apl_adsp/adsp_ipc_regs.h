/* Copyright (c) 2021 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_SOC_INTEL_ADSP_IPC_REGS_H_
#define ZEPHYR_SOC_INTEL_ADSP_IPC_REGS_H_

#include <intel_adsp_ipc_devtree.h>
#include <stdint.h>

/**
 * @brief IPC registers layout for Intel ADSP cAVS APL SoCs.
 */
struct intel_adsp_ipc {
	uint32_t tdr;
	uint32_t tdd;
	uint32_t idr;
	uint32_t idd;
	uint32_t ctl;
	uint32_t ida; /* Fakes for source compatibility; these ...  */
	uint32_t tda; /* ... two registers don't exist in hardware. */
};

#define INTEL_ADSP_IPC_BUSY BIT(31)
#define INTEL_ADSP_IPC_DONE BIT(30)

#define INTEL_ADSP_IPC_CTL_TBIE BIT(0)
#define INTEL_ADSP_IPC_CTL_IDIE BIT(1)

#endif /* ZEPHYR_SOC_INTEL_ADSP_IPC_REGS_H_ */
