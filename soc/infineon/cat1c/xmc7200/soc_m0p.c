/*
 * Copyright (c) 2021 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Infineon XMC7200 SOC.
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>

#include <cy_sysint.h>
#include <cy_wdt.h>

cy_en_sysint_status_t Cy_SysInt_Init(const cy_stc_sysint_t *config, cy_israddress userIsr)
{
	CY_ASSERT_L3(CY_SYSINT_IS_PRIORITY_VALID(config->intrPriority));
	cy_en_sysint_status_t status = CY_SYSINT_SUCCESS;

	/* The interrupt vector will be relocated only if the vector table was
	 * moved to SRAM (CONFIG_DYNAMIC_INTERRUPTS and CONFIG_GEN_ISR_TABLES
	 * must be enabled). Otherwise it is ignored.
	 */

#if defined(CONFIG_DYNAMIC_INTERRUPTS) && defined(CONFIG_GEN_ISR_TABLES)
	if (config != NULL) {
		uint32_t priority;

		/* NOTE:
		 * PendSV IRQ (which is used in Cortex-M variants to implement thread
		 * context-switching) is assigned the lowest IRQ priority level.
		 * If priority is same as PendSV, we will catch assertion in
		 * z_arm_irq_priority_set function. To avoid this, change priority
		 * to IRQ_PRIO_LOWEST, if it > IRQ_PRIO_LOWEST. Macro IRQ_PRIO_LOWEST
		 * takes in to account PendSV specific.
		 */
		priority = (config->intrPriority > IRQ_PRIO_LOWEST) ? IRQ_PRIO_LOWEST
								    : config->intrPriority;

		/* Configure a dynamic interrupt */
		(void)irq_connect_dynamic(config->intrSrc, priority, (void *)userIsr, NULL, 0);
	} else {
		status = CY_SYSINT_BAD_PARAM;
	}
#endif /* defined(CONFIG_DYNAMIC_INTERRUPTS) && defined(CONFIG_GEN_ISR_TABLES) */

	return status;
}

/* This function was taken from PDL */
void Cy_SysInt_EnableSystemInt(cy_en_intr_t sysIntSrc)
{
	if (CY_CPU_CORTEX_M0P) {
		CPUSS_CM0_SYSTEM_INT_CTL[sysIntSrc] |= CPUSS_CM0_SYSTEM_INT_CTL_CPU_INT_VALID_Msk;
	}
#if defined(CY_IP_M7CPUSS)
	else if (CY_IS_CM7_CORE_0) {
		CPUSS_CM7_0_SYSTEM_INT_CTL[sysIntSrc] |=
			CPUSS_CM7_0_SYSTEM_INT_CTL_CPU_INT_VALID_Msk;
	} else {
		CPUSS_CM7_1_SYSTEM_INT_CTL[sysIntSrc] |=
			CPUSS_CM7_1_SYSTEM_INT_CTL_CPU_INT_VALID_Msk;
	}
#elif (CY_IP_M4CPUSS)
	else {
		CPUSS_CM4_SYSTEM_INT_CTL[sysIntSrc] |=
			CPUSS_V2_CM4_SYSTEM_INT_CTL_CPU_INT_VALID_Msk;
	}
#else
#error This version of the sysint driver only supports CM4 and CM7 devices.
#endif
}

/* This function was taken from PDL */
cy_en_intr_t Cy_SysInt_GetInterruptActive(IRQn_Type IRQn)
{
	uint32_t tempReg = CY_CPUSS_NOT_CONNECTED_IRQN;
	uint32_t locIdx = (uint32_t)IRQn & CY_SYSINT_INT_STATUS_MSK;

#if defined(CY_IP_M4CPUSS)
#if (CY_CPU_CORTEX_M0P)
	if (CY_SYSINT_ENABLE ==
	    _FLD2VAL(CPUSS_V2_CM0_INT0_STATUS_SYSTEM_INT_VALID, CPUSS_CM0_INT_STATUS[locIdx])) {
		tempReg = _FLD2VAL(CPUSS_V2_CM0_INT0_STATUS_SYSTEM_INT_IDX,
				   CPUSS_CM0_INT_STATUS[locIdx]);
	}
#else
	if (!CY_CPUSS_V1) {
		if (CY_SYSINT_ENABLE == _FLD2VAL(CPUSS_V2_CM4_INT0_STATUS_SYSTEM_INT_VALID,
						 CPUSS_CM4_INT_STATUS[locIdx])) {
			tempReg = _FLD2VAL(CPUSS_V2_CM4_INT0_STATUS_SYSTEM_INT_IDX,
					   CPUSS_CM4_INT_STATUS[locIdx]);
		}
	}
#endif /* (CY_CPU_CORTEX_M0P) */
#endif

#if defined(CY_IP_M7CPUSS)
#if (CY_CPU_CORTEX_M0P)
	if (CY_SYSINT_ENABLE ==
	    _FLD2VAL(CPUSS_CM0_INT0_STATUS_SYSTEM_INT_VALID, CPUSS_CM0_INT_STATUS_BASE[locIdx])) {
		tempReg = _FLD2VAL(CPUSS_CM0_INT0_STATUS_SYSTEM_INT_IDX,
				   CPUSS_CM0_INT_STATUS_BASE[locIdx]);
	}
#else
	if ((CY_IS_CM7_CORE_0) &&
	    (CY_SYSINT_ENABLE ==
	     _FLD2VAL(CPUSS_CM7_0_INT_STATUS_SYSTEM_INT_VALID, CPUSS_CM7_0_INT_STATUS[locIdx]))) {
		tempReg = _FLD2VAL(CPUSS_CM7_0_INT_STATUS_SYSTEM_INT_IDX,
				   CPUSS_CM7_0_INT_STATUS[locIdx]);
	} else if (CY_SYSINT_ENABLE == _FLD2VAL(CPUSS_CM7_1_INT_STATUS_SYSTEM_INT_VALID,
						CPUSS_CM7_1_INT_STATUS[locIdx])) {
		tempReg = _FLD2VAL(CPUSS_CM7_1_INT_STATUS_SYSTEM_INT_IDX,
				   CPUSS_CM7_1_INT_STATUS[locIdx]);
	} else {
		/* No active core */
	}
#endif
#endif
	return ((cy_en_intr_t)tempReg);
}

IRQn_Type Cy_SysInt_GetNvicConnection(cy_en_intr_t devIntrSrc)
{
	uint32_t tempReg = CY_CPUSS_NOT_CONNECTED_IRQN;

#if defined(CY_IP_M4CPUSS)
#if (CY_CPU_CORTEX_M0P)
	if (CY_SYSINT_ENABLE == _FLD2VAL(CPUSS_V2_CM0_SYSTEM_INT_CTL_CPU_INT_VALID,
					 CPUSS_CM0_SYSTEM_INT_CTL[devIntrSrc])) {
		tempReg = _FLD2VAL(CPUSS_V2_CM0_SYSTEM_INT_CTL_CPU_INT_IDX,
				   CPUSS_CM0_SYSTEM_INT_CTL[devIntrSrc]);
	}
#else
	if ((!CY_CPUSS_V1) &&
	    (CY_SYSINT_ENABLE == _FLD2VAL(CPUSS_V2_CM4_SYSTEM_INT_CTL_CPU_INT_VALID,
					  CPUSS_CM4_SYSTEM_INT_CTL[devIntrSrc]))) {
		tempReg = _FLD2VAL(CPUSS_V2_CM4_SYSTEM_INT_CTL_CPU_INT_IDX,
				   CPUSS_CM4_SYSTEM_INT_CTL[devIntrSrc]);
	}
#endif /* (CY_CPU_CORTEX_M0P) */
#endif

#if defined(CY_IP_M7CPUSS)
#if (CY_CPU_CORTEX_M0P)
	if ((CY_SYSINT_ENABLE == _FLD2VAL(CPUSS_CM0_SYSTEM_INT_CTL_CPU_INT_VALID,
					  CPUSS_CM0_SYSTEM_INT_CTL[devIntrSrc]))) {
		tempReg = _FLD2VAL(CPUSS_CM0_SYSTEM_INT_CTL_CM0_CPU_INT_IDX,
				   CPUSS_CM0_SYSTEM_INT_CTL[devIntrSrc]);
	}
#else
	if (CY_IS_CM7_CORE_0 &&
	    (CY_SYSINT_ENABLE == _FLD2VAL(CPUSS_CM7_0_SYSTEM_INT_CTL_CPU_INT_VALID,
					  CPUSS_CM7_0_SYSTEM_INT_CTL[devIntrSrc]))) {
		tempReg = _FLD2VAL(CPUSS_CM7_0_SYSTEM_INT_CTL_CPU_INT_IDX,
				   CPUSS_CM7_0_SYSTEM_INT_CTL[devIntrSrc]);
	} else if ((CY_SYSINT_ENABLE == _FLD2VAL(CPUSS_CM7_1_SYSTEM_INT_CTL_CPU_INT_VALID,
						 CPUSS_CM7_1_SYSTEM_INT_CTL[devIntrSrc]))) {
		tempReg = _FLD2VAL(CPUSS_CM7_1_SYSTEM_INT_CTL_CPU_INT_IDX,
				   CPUSS_CM7_1_SYSTEM_INT_CTL[devIntrSrc]);
	} else {
		/* No active core */
	}
#endif
#endif
	return ((IRQn_Type)tempReg);
}

void EnableEcc(void)
{
	/* Enable ECC checking in SRAM controllers again
	 * (had been switched off by assembly startup code)
	 */
	CPUSS->RAM0_CTL0 &= ~(0x80000); /* set bit 19 to 0 */
#if (CPUSS_RAMC1_PRESENT == 1u)
	CPUSS->RAM1_CTL0 &= ~(0x80000); /* set bit 19 to 0 */
#endif
#if (CPUSS_RAMC2_PRESENT == 1u)
	CPUSS->RAM2_CTL0 &= ~(0x80000); /* set bit 19 to 0 */
#endif
}

static int init_cycfg_platform_wraper(void)
{

	EnableEcc();
	SystemIrqInit();
	Cy_WDT_Unlock();
	Cy_WDT_Disable();
	Cy_SystemInit();
	SystemCoreClockUpdate();

	return 0;
}

SYS_INIT(init_cycfg_platform_wraper, PRE_KERNEL_1, 0);
