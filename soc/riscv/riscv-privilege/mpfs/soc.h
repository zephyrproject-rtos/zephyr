/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2020-2021 Microchip Technology Inc
 */
#ifndef __RISCV64_MPFS_SOC_H_
#define __RISCV64_MPFS_SOC_H_

#include <soc_common.h>
#include <zephyr/devicetree.h>


#define RISCV_MSIP_BASE					0x02000000

/* lib-c hooks required RAM defined variables */
#define RISCV_RAM_BASE					CONFIG_SRAM_BASE_ADDRESS
#define RISCV_RAM_SIZE					KB(CONFIG_SRAM_SIZE)

#endif /* __RISCV64_MPFS_SOC_H_ */
