/*
 * Copyright (c) 2021 Tokita, Hiroshi <tokita.hiroshi@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC configuration macros for the GigaDevice GD32V processor
 */

#ifndef __RISCV_GIGADEVICE_GD32V_SOC_H_
#define __RISCV_GIGADEVICE_GD32V_SOC_H_

#include <toolchain.h>
#include <soc_common.h>
#include <devicetree.h>
#include <arch/riscv/csr.h>
#ifndef _LINKER
#include <riscv_encoding.h>
#endif

/* Timer configuration */
#define RISCV_MTIME_BASE             (0xD1000000UL + 0)
#define RISCV_MTIMECMP_BASE          (0xD1000000UL + 0x8)

/* lib-c hooks required RAM defined variables */
#define RISCV_RAM_BASE               DT_SRAM_BASE_ADDRESS
#define RISCV_RAM_SIZE               KB(DT_SRAM_SIZE)

#ifndef _ASMLANGUAGE
#include <gd32vf103.h>

/**
 * @brief structure to convey pinctrl information for stm32 soc
 * value
 */
struct soc_gpio_pinctrl {
	uint32_t pinmux;
	uint32_t pincfg;
};

#endif  /* !_ASMLANGUAGE */

#endif  /* __RISCV_GIGADEVICE_GD32VF103_SOC_H_ */
