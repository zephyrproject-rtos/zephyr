/*
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC configuration macros for the QEMU Virt Machine
 */

#ifndef __RISCV_QEMU_VIRT_SOC_H_
#define __RISCV_QEMU_VIRT_SOC_H_

#include <soc_common.h>
#include <devicetree.h>

/* Timer configuration */
#define RISCV_MTIME_BASE             0x0200BFF8
#define RISCV_MTIMECMP_BASE          0x02004000

/* IPI configuration */
#define RISCV_MSIP_BASE              0x02000000

/* lib-c hooks required RAM defined variables */
#define RISCV_RAM_BASE               DT_SRAM_BASE_ADDRESS
#define RISCV_RAM_SIZE               KB(DT_SRAM_SIZE)

#endif /* __RISCV_QEMU_VIRT_SOC_H_ */
