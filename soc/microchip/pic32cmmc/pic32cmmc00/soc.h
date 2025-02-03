/*
 * Copyright (c) 2025 GP Orcullo
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC_MICROCHIP_PIC32CMMC00_SOC_H_
#define _SOC_MICROCHIP_PIC32CMMC00_SOC_H_

#include <zephyr/arch/arm/cortex_m/nvic.h>

typedef union {
	uint32_t reg;
} _reg32_t;

typedef volatile struct {
	uint32_t res0[32];
	_reg32_t PCHCTRL[34];
} __packed gclk_t;

typedef volatile struct {
	uint32_t res0[5];
	_reg32_t APBAMASK;
	_reg32_t APBBMASK;
} __packed mclk_t;

typedef union {
	struct {
		uint8_t: 1;
		uint8_t ENABLE: 1;
		uint8_t: 6;
	} bit;
	uint8_t reg;
} eic_ctrla_t;

typedef volatile struct {
	eic_ctrla_t CTRLA;
	uint8_t res0[3];
	const _reg32_t SYNCBUSY;
	uint32_t res1;
	_reg32_t INTENCLR;
	_reg32_t INTENSET;
	_reg32_t INTFLAG;
	uint32_t res2;
	_reg32_t CONFIG[2];
} __packed eic_t;

typedef enum IRQn {
	PendSV_IRQn = -2,
	SysTick_IRQn = -1,
} IRQn_Type;

#define __MPU_PRESENT    CONFIG_CPU_HAS_ARM_MPU
#define __NVIC_PRIO_BITS NUM_IRQ_PRIO_BITS
#define __VTOR_PRESENT   CONFIG_CPU_CORTEX_M_HAS_VTOR

#include <core_cm0plus.h>

#define EIC              ((eic_t *)DT_REG_ADDR(DT_INST(0, atmel_sam0_eic)))
#define EIC_CTRLA        ((uintptr_t)EIC + 0x00)
#define EIC_SYNCBUSY     ((uintptr_t)EIC + 0x04)
#define REG_EIC_CTRLA    EIC_CTRLA
#define REG_EIC_SYNCBUSY EIC_SYNCBUSY

#define EIC_EXTINT_NUM 16
#define EIC_GCLK_ID    2

#define EIC_CONFIG_SENSE0_RISE 1
#define EIC_CONFIG_SENSE0_FALL 2
#define EIC_CONFIG_SENSE0_BOTH 3
#define EIC_CONFIG_SENSE0_HIGH 4
#define EIC_CONFIG_SENSE0_LOW  5
#define EIC_CONFIG_FILTEN0     8

#define GCLK          ((gclk_t *)DT_REG_ADDR(DT_INST(0, atmel_sam0_gclk)))
#define GCLK_GENCTRL0 ((uintptr_t)GCLK + 0x20)
#define GCLK_SYNCBUSY ((uintptr_t)GCLK + 0x04)

#define GCLK_GENCTRL_DIV_MASK       GENMASK(31, 16)
#define GCLK_GENCTRL_GENEN          BIT(8)
#define GCLK_GENCTRL_SRC_MASK       GENMASK(4, 0)
#define GCLK_GENCTRL_SRC_OSC48M_VAL 6
#define GCLK_PCHCTRL_CHEN           BIT(6)
#define GCLK_PCHCTRL_GEN_GCLK0      0
#define GCLK_SYNCBUSY_GENCTRL0_BIT  2

#define MCLK        ((mclk_t *)DT_REG_ADDR(DT_INST(0, atmel_sam0_mclk)))
#define MCLK_CPUDIV ((uintptr_t)MCLK + 0x04)

#define MCLK_APBAMASK_EIC           BIT(10)
#define MCLK_CPUDIV_CPUDIV_DIV1_VAL 1

#define NVMCTRL       DT_REG_ADDR(DT_INST(0, atmel_sam0_nvmctrl))
#define NVMCTRL_CTRLB (NVMCTRL + 0x04)

#define NVMCTRL_CTRLB_RWS_MASK GENMASK(4, 1)

#define NVM_SW_CAL_ADDR        0x00806020
#define NVM_SW_CAL_CAL48M_MASK GENMASK64(40, 19)

#define OSCCTRL                DT_REG_ADDR(DT_PATH(soc, oscctrl_40001000))
#define OSCCTRL_CAL48M         (OSCCTRL + 0x38)
#define OSCCTRL_OSC48MDIV      (OSCCTRL + 0x15)
#define OSCCTRL_OSC48MSYNCBUSY (OSCCTRL + 0x18)
#define OSCCTRL_STATUS         (OSCCTRL + 0x0C)

#define OSCCTRL_CAL48M_MASK                  GENMASK(21, 0)
#define OSCCTRL_OSC48MSYNCBUSY_OSC48MDIV_BIT 2
#define OSCCTRL_STATUS_OSC48MRDY_BIT         4

#define PORT_GROUPS 2

#define SOC_ATMEL_SAM0_GCLK0_FREQ_HZ CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC

#endif /* _SOC_MICROCHIP_PIC32CMMC00_SOC_H_ */
