/*
 * Copyright (c) 2023 Antmicro <www.antmicro.com>
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_RENESAS_RZT2M_SOC_H_
#define ZEPHYR_SOC_RENESAS_RZT2M_SOC_H_

typedef enum IRQn {
	SoftwareGeneratedInt0 = 0,
	SoftwareGeneratedInt1,
	SoftwareGeneratedInt2,
	SoftwareGeneratedInt3,
	SoftwareGeneratedInt4,
	SoftwareGeneratedInt5,
	SoftwareGeneratedInt6,
	SoftwareGeneratedInt7,
	SoftwareGeneratedInt8,
	SoftwareGeneratedInt9,
	SoftwareGeneratedInt10,
	SoftwareGeneratedInt11,
	SoftwareGeneratedInt12,
	SoftwareGeneratedInt13,
	SoftwareGeneratedInt14,
	SoftwareGeneratedInt15,
	DebugCommunicationsChannelInt = 22,
	PerformanceMonitorCounterOverflowInt = 23,
	CrossTriggerInterfaceInt = 24,
	VritualCPUInterfaceMaintenanceInt = 25,
	HypervisorTimerInt = 26,
	VirtualTimerInt = 27,
	NonSecurePhysicalTimerInt = 30,
	SHARED_PERIPHERAL_INTERRUPTS_MAX_ENTRIES = CONFIG_NUM_IRQS
} IRQn_Type;

/* Do not let CMSIS to handle GIC and Timer */
#define __GIC_PRESENT 0
#define __TIM_PRESENT 0
#define __FPU_PRESENT 1

/* Porting FSP IRQ configuration by an empty function */
/* Let Zephyr handle IRQ configuration */
void bsp_irq_core_cfg(void);

#endif /* ZEPHYR_SOC_RENESAS_RZT2M_SOC_H_ */
