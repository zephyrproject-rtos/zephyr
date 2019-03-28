/*
 * Copyright (c) 2018 - 2019 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __RISCV32_LITEX_VEXRISCV_SOC_H_
#define __RISCV32_LITEX_VEXRISCV_SOC_H_

#include "../riscv-privilege/common/soc_common.h"
#include <generated_dts_board.h>

/* lib-c hooks required RAM defined variables */
#define RISCV_RAM_BASE              DT_MMIO_SRAM_0_BASE_ADDRESS
#define RISCV_RAM_SIZE              DT_MMIO_SRAM_0_SIZE

#endif /* __RISCV32_LITEX_VEXRISCV_SOC_H_ */
