/*
 * Copyright (c) 2024 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_TC_IR_PRIV_H_
#define ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_TC_IR_PRIV_H_

#if IS_ENABLED(CONFIG_SOC_SERIES_TC3X)
typedef struct SRCR_Bits {
	uint32_t SRPN: 8;
	uint32_t reserved_8: 2;
	uint32_t SRE: 1;
	uint32_t TOS: 3;
	uint32_t reserved_14: 2;
	uint32_t ECC: 5;
	uint32_t reserved_21: 3;
	uint32_t SRR: 1;
	uint32_t CLRR: 1;
	uint32_t SETR: 1;
	uint32_t IOV: 1;
	uint32_t IOVCLR: 1;
	uint32_t SWS: 1;
	uint32_t SWSCLR: 1;
	uint32_t reserved_31: 1;
} SRCR_Bits;
#define GET_TOS(coreId) (coreId == 0 ? coreId : coreId + 1)
#elif IS_ENABLED(CONFIG_SOC_SERIES_TC4X)
typedef struct SRCR_Bits {
	uint32_t SRPN: 8;
	uint32_t VM: 3;
	uint32_t CS: 1;
	uint32_t TOS: 4;
	uint32_t reserved_16: 7;
	uint32_t SRE: 1;
	uint32_t SRR: 1;
	uint32_t CLRR: 1;
	uint32_t SETR: 1;
	uint32_t IOV: 1;
	uint32_t IOVCLR: 1;
	uint32_t reserved_29: 3;
} SRCR_Bits;
#define GET_TOS(coreId) coreId
#endif

typedef union {
	uint32_t U;
	int32_t I;
	SRCR_Bits B;
} SRCR;

#define GET_SRC(irq) (SRC_BASE + (irq)*4)

#endif

