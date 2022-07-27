/*
 * Copyright (c) 2021 Andes Technology Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Macros for the Andes AE350 platform
 */

#ifndef __RISCV_ANDES_AE350_SOC_H_
#define __RISCV_ANDES_AE350_SOC_H_

#include <soc_common.h>

/* Machine timer memory-mapped registers */
#define RISCV_MTIME_BASE             0xE6000000
#define RISCV_MTIMECMP_BASE          0xE6000008

/* Include CSRs available for Andes V5 SoCs */
#include "soc_v5.h"

/* Include platform peripherals */
#include "smu.h"

#endif /* __RISCV_ANDES_AE350_SOC_H_ */
