/*
 * Copyright (c) 2021 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Infineon PSOC 6 SOC.
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <cy_sysint.h>

#ifdef CONFIG_SOC_FAMILY_PSOC6_M0
static unsigned int _m0_irq;

void Cy_SysInt_SetInterruptSource(IRQn_Type IRQn, cy_en_intr_t devIntrSrc)
{
		/* Disconnection feature doesn't work for CPUSS_V2 */
		CY_ASSERT_L1(CY_CPUSS_DISCONNECTED_IRQN != devIntrSrc);
		CPUSS_CM0_SYSTEM_INT_CTL[devIntrSrc] =
			_VAL2FLD(CPUSS_V2_CM0_SYSTEM_INT_CTL_CPU_INT_IDX, IRQn)
						| CPUSS_V2_CM0_SYSTEM_INT_CTL_CPU_INT_VALID_Msk;
}

void Cy_SysInt_DisconnectInterruptSource(IRQn_Type IRQn, cy_en_intr_t devIntrSrc)
{
	CPUSS_CM0_SYSTEM_INT_CTL[devIntrSrc] &=
		(uint32_t) ~CPUSS_V2_CM0_SYSTEM_INT_CTL_CPU_INT_VALID_Msk;
}

IRQn_Type Cy_SysInt_GetNvicConnection(cy_en_intr_t devIntrSrc)
{
	uint32_t tempReg = CY_CPUSS_NOT_CONNECTED_IRQN;

	if (CY_SYSINT_ENABLE == _FLD2VAL(CPUSS_V2_CM0_SYSTEM_INT_CTL_CPU_INT_VALID,
						CPUSS_CM0_SYSTEM_INT_CTL[devIntrSrc])) {
		tempReg = _FLD2VAL(CPUSS_V2_CM0_SYSTEM_INT_CTL_CPU_INT_IDX,
					CPUSS_CM0_SYSTEM_INT_CTL[devIntrSrc]);
	}

	return ((IRQn_Type)tempReg);
}

cy_en_intr_t Cy_SysInt_GetInterruptActive(IRQn_Type IRQn)
{
	uint32_t tempReg = CY_CPUSS_NOT_CONNECTED_IRQN;
	uint32_t locIdx = (uint32_t)IRQn & CY_SYSINT_INT_STATUS_MSK;

	if (CY_SYSINT_ENABLE == _FLD2VAL(CPUSS_V2_CM0_INT0_STATUS_SYSTEM_INT_VALID,
						CPUSS_CM0_INT_STATUS[locIdx])) {
		tempReg = _FLD2VAL(CPUSS_V2_CM0_INT0_STATUS_SYSTEM_INT_IDX,
					CPUSS_CM0_INT_STATUS[locIdx]);
	}

	return ((cy_en_intr_t)tempReg);
}
#endif

#if defined(CONFIG_DYNAMIC_INTERRUPTS) && defined(CONFIG_GEN_ISR_TABLES)
void psoc6_irq_connect_dynamic(unsigned int irq, unsigned int priority,
			void (*routine)(const void *parameter),
			const void *parameter, uint32_t flags)
{
	/* NOTE:
	 * PendSV IRQ (which is used in Cortex-M variants to implement thread
	 * context-switching) is assigned the lowest IRQ priority level.
	 * If priority is same as PendSV, we will catch assertion in
	 * z_arm_irq_priority_set function. To avoid this, change priority
	 * to IRQ_PRIO_LOWEST, if it > IRQ_PRIO_LOWEST. Macro IRQ_PRIO_LOWEST
	 * takes in to account PendSV specific.
	 */
	unsigned int prio = (priority > IRQ_PRIO_LOWEST) ? IRQ_PRIO_LOWEST : priority;

#if (CONFIG_SOC_FAMILY_PSOC6_M0)
	if (_m0_irq > 7) {
		return;
	}
	/* Configure a dynamic interrupt */
	(void) irq_connect_dynamic(_m0_irq, prio, routine, parameter, 0);
	Cy_SysInt_SetInterruptSource((IRQn_Type) _m0_irq, irq);
	_m0_irq++;
#else
	/* Configure a dynamic interrupt */
	(void) irq_connect_dynamic(irq, prio, routine, parameter, 0);
#endif /* (CONFIG_SOC_FAMILY_PSOC6_M0) */
}
#endif /* defined(CONFIG_DYNAMIC_INTERRUPTS) && defined(CONFIG_GEN_ISR_TABLES) */

cy_en_sysint_status_t Cy_SysInt_Init(const cy_stc_sysint_t *config, cy_israddress userIsr)
{
	CY_ASSERT_L3(CY_SYSINT_IS_PRIORITY_VALID(config->intrPriority));
	cy_en_sysint_status_t status = CY_SYSINT_SUCCESS;

	/* The interrupt vector will be relocated only if the vector table was
	 * moved to SRAM (CONFIG_DYNAMIC_INTERRUPTS and CONFIG_GEN_ISR_TABLES
	 * must be enabled). Otherwise it is ignored.
	 */

#if (CY_CPU_CORTEX_M0P) && !defined(CONFIG_SOC_FAMILY_PSOC6_M0)
	#error Cy_SysInt_Init does not support CM0p core.
#endif /* defined(CY_CPU_CORTEX_M0P) && !defined(CONFIG_SOC_FAMILY_PSOC6_M0) */

#if defined(CONFIG_DYNAMIC_INTERRUPTS) && defined(CONFIG_GEN_ISR_TABLES)
	if (config != NULL) {
#if (CONFIG_SOC_FAMILY_PSOC6_M0)
		psoc6_irq_connect_dynamic(config->cm0pSrc, config->intrPriority,
									(void *) userIsr, NULL, 0);
#else
		psoc6_irq_connect_dynamic(config->intrSrc, config->intrPriority,
									(void *) userIsr, NULL, 0);
#endif /* (CONFIG_SOC_FAMILY_PSOC6_M0) */
	} else {
		status = CY_SYSINT_BAD_PARAM;
	}
#endif /* defined(CONFIG_DYNAMIC_INTERRUPTS) && defined(CONFIG_GEN_ISR_TABLES) */

	return status;
}

static int init_cycfg_platform_wraper(void)
{

	/* Initializes the system */
	SystemInit();

#if defined(CONFIG_SOC_FAMILY_PSOC6_M0)
	Cy_SysEnableCM4(DT_REG_ADDR(DT_NODELABEL(flash_m0)) +
		DT_REG_SIZE(DT_NODELABEL(flash_m0)));
#endif
	return 0;
}

SYS_INIT(init_cycfg_platform_wraper, PRE_KERNEL_1, 0);
