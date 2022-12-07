/* Copyright (c) 2021 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_SOC_INTEL_ADSP_IPC_REGS_H_
#define ZEPHYR_SOC_INTEL_ADSP_IPC_REGS_H_

#include <intel_adsp_ipc_devtree.h>
#include <stdint.h>

/**
 * @brief IPC registers layout for Intel ADSP cAVS CNL SoC.
 */
struct intel_adsp_ipc {
	uint32_t tdr;
	uint32_t tda;
	uint32_t tdd;
	uint32_t unused0;
	uint32_t idr;
	uint32_t ida;
	uint32_t idd;
	uint32_t unused1;
	uint32_t cst;
	uint32_t csr;
	uint32_t ctl;
};

#define INTEL_ADSP_IPC_BUSY BIT(31)
#define INTEL_ADSP_IPC_DONE BIT(31)

#define INTEL_ADSP_IPC_CTL_TBIE BIT(0)
#define INTEL_ADSP_IPC_CTL_IDIE BIT(1)

#endif /* ZEPHYR_SOC_INTEL_ADSP_IPC_REGS_H_ */
