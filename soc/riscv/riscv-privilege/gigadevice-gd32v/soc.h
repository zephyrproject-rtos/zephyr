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

#include <soc_common.h>
#include <devicetree.h>

/* Timer configuration */
#define RISCV_MTIME_BASE             (0xD1000000UL + 0)
#define RISCV_MTIMECMP_BASE          (0xD1000000UL + 0x8)

/* lib-c hooks required RAM defined variables */
#define RISCV_RAM_BASE               DT_SRAM_BASE_ADDRESS
#define RISCV_RAM_SIZE               KB(DT_SRAM_SIZE)

#define GD32_PORTA 'A'
#define GD32_PORTB 'B'
#define GD32_PORTC 'C'
#define GD32_PORTD 'D'
#define GD32_PORTE 'E'
#define GD32_PORTF 'F'
#define GD32_PORTG 'G'
#define GD32_PORTH 'H'
#define GD32_PORTI 'I'
#define GD32_PORTJ 'J'
#define GD32_PORTK 'K'

#ifndef _ASMLANGUAGE
#include <toolchain.h>
#include <gd32vf103.h>
#include <core_feature_timer.h>

/* common clock control device name for all GD32 chips */
#define GD32_CLOCK_CONTROL_NODE DT_NODELABEL(rcu)

struct gd32_pclken {
	uint32_t bus;
	uint32_t enr;
};

#endif  /* !_ASMLANGUAGE */

#endif  /* __RISCV_GIGADEVICE_GD32VF103_SOC_H_ */
