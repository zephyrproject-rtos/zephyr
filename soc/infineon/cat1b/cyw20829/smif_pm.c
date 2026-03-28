/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 *  SMIF (Serial Memory Interface) Power Management implementation for CYW20829
 *
 * This file provides power management functionality for the SMIF controller,
 * including deep sleep callbacks to properly handle external memory during
 * power state transitions. It manages SMIF enable/disable, GPIO configuration,
 * and SMIF crypto input register configuration preservation across sleep cycles.
 */
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include "cyhal.h"
#include "qspi_memslot.h"
#include "smif_pm.h"


#define SMIF_HW                      (SMIF0)
/** Maximum retries for checking if memory is ready (high enough for sector erase) */
#define MEMORY_BUSY_CHECK_RETRIES    (750ul)
/** Callback order priority for external memory power management */
#define EXT_MEMORY_PM_CALLBACK_ORDER (254u)

/* SMIF internal context data */
static cy_stc_smif_context_t smif_context;

/** GPIO port selection register 0 - saved before deep sleep */
static uint32_t smif_port_sel0;
/** GPIO port selection register 1 - saved before deep sleep */
static uint32_t smif_port_sel1;
/** GPIO configuration register - saved before deep sleep */
static uint32_t smif_cfg;
/** GPIO output register - saved before deep sleep */
static uint32_t smif_out;

/** SMIF crypto input register 0 - saved before deep sleep */
static uint32_t smif_ip_crypto_input0;
/** SMIF crypto input register 1 - saved before deep sleep */
static uint32_t smif_ip_crypto_input1;
/** SMIF crypto input register 2 - saved before deep sleep */
static uint32_t smif_ip_crypto_input2;
/** SMIF crypto input register 3 - saved before deep sleep */
static uint32_t smif_ip_crypto_input3;

/** Deep sleep callback params structure */
static cy_stc_syspm_callback_params_t smif_deep_sleep_param = {NULL, NULL};

/**
 * @brief Disables the SMIF controller and saves GPIO/crypto state.
 *
 * This function performs the following operations:
 * - Saves SMIF crypto input registers if crypto is enabled
 * - Disables the SMIF controller
 * - Saves GPIO port configuration (HSIOM, CFG, OUT registers)
 * - Reconfigures GPIO pins to minimal power state
 *
 * @note This code assumes all SMIF pins are on the same GPIO port to minimize
 *       deep sleep entry latency.
 *
 * @return None
 */
static void smif_disable(void)
{
	/*
	 * to minimize DeepSleep latency this code assumes that all of the SMIF pins are on the same
	 * port
	 */
	int port_number = CYHAL_GET_PORT(QSPI_SLAVE_SELECT);

	/*
	 * Store crypto input registers if encryption is enabled
	 */
	if ((((SMIF_Type *)SMIF0_BASE)->DEVICE[0].CTL) & (1 << SMIF_DEVICE_CTL_CRYPTO_EN_Pos)) {
		smif_ip_crypto_input0 = SMIF0->SMIF_CRYPTO_BLOCK[0].CRYPTO_INPUT0;
		smif_ip_crypto_input1 = SMIF0->SMIF_CRYPTO_BLOCK[0].CRYPTO_INPUT1;
		smif_ip_crypto_input2 = SMIF0->SMIF_CRYPTO_BLOCK[0].CRYPTO_INPUT2;
		smif_ip_crypto_input3 = SMIF0->SMIF_CRYPTO_BLOCK[0].CRYPTO_INPUT3;
	}
	SMIF0->CTL = SMIF0->CTL & ~SMIF_CTL_ENABLED_Msk;
	smif_port_sel0 = ((HSIOM_PRT_Type *)&HSIOM->PRT[port_number])->PORT_SEL0;
	smif_port_sel1 = ((HSIOM_PRT_Type *)&HSIOM->PRT[port_number])->PORT_SEL1;
	smif_cfg = ((GPIO_PRT_Type *)&GPIO->PRT[port_number])->CFG;
	smif_out = ((GPIO_PRT_Type *)&GPIO->PRT[port_number])->OUT;
	((HSIOM_PRT_Type *)&HSIOM->PRT[port_number])->PORT_SEL0 = 0x00;
	((HSIOM_PRT_Type *)&HSIOM->PRT[port_number])->PORT_SEL1 = 0x00;
	((GPIO_PRT_Type *)&GPIO->PRT[port_number])->CFG = 0x600006;
	((GPIO_PRT_Type *)&GPIO->PRT[port_number])->OUT = 0x1;
}

/**
 * @brief  Enables the SMIF controller and restores GPIO/crypto state.
 *
 * This function performs the following operations:
 * - Enables the SMIF controller
 * - Restores SMIF crypto registers if crypto is enabled
 * - Restores GPIO port configuration (HSIOM, CFG, OUT registers)
 *
 * @note Must be called after smif_disable() to properly restore SMIF state
 *       following a deep sleep cycle.
 *
 * @return None
 */
static void smif_enable(void)
{
	int port_number = CYHAL_GET_PORT(QSPI_SLAVE_SELECT);

	SMIF0->CTL = SMIF0->CTL | SMIF_CTL_ENABLED_Msk;

	/*
	 * Restore from global variable after wake up from deep sleep
	 */
	if ((((SMIF_Type *)SMIF0_BASE)->DEVICE[0].CTL) & (1 << SMIF_DEVICE_CTL_CRYPTO_EN_Pos)) {
		SMIF0->SMIF_CRYPTO_BLOCK[0].CRYPTO_INPUT0 = smif_ip_crypto_input0;
		SMIF0->SMIF_CRYPTO_BLOCK[0].CRYPTO_INPUT1 = smif_ip_crypto_input1;
		SMIF0->SMIF_CRYPTO_BLOCK[0].CRYPTO_INPUT2 = smif_ip_crypto_input2;
		SMIF0->SMIF_CRYPTO_BLOCK[0].CRYPTO_INPUT3 = smif_ip_crypto_input3;
	}

	((HSIOM_PRT_Type *)&HSIOM->PRT[port_number])->PORT_SEL0 = smif_port_sel0;
	((HSIOM_PRT_Type *)&HSIOM->PRT[port_number])->PORT_SEL1 = smif_port_sel1;
	((GPIO_PRT_Type *)&GPIO->PRT[port_number])->CFG = smif_cfg;
	((GPIO_PRT_Type *)&GPIO->PRT[port_number])->OUT = smif_out;
}

/**
 * Polls the memory device to check whether it is ready to accept new commands or
 * not until either it is ready or the retries have exceeded the limit.
 *
 * This function repeatedly checks the busy status of the external memory device
 * with a delay between attempts. The maximum number of retries is defined
 * by MEMORY_BUSY_CHECK_RETRIES.
 *
 * @param mem_config Pointer to the memory device configuration structure
 *
 * @return Status of the operation:
 *         0: Memory is ready to accept new commands
 *         -ETIMEDOUT: Memory is still busy after max retries
 */
static int is_memory_ready(cy_stc_smif_mem_config_t const *mem_config)
{
	bool ret;

	ret = WAIT_FOR(!Cy_SMIF_Memslot_IsBusy(SMIF_HW, (cy_stc_smif_mem_config_t *)mem_config,
						&smif_context),
		       MEMORY_BUSY_CHECK_RETRIES * 15, Cy_SysLib_DelayUs(15));

	return ret ? 0 : -ETIMEDOUT;
}

/**
 * External Memory power-down/power-up PM callback.
 *
 * This callback is invoked during device power mode transitions to properly
 * manage the external memory state:
 * - Before deep sleep: puts external memory into low power mode and disables SMIF
 * - After waking: enables SMIF and wakes up external memory from low power mode
 *
 * The callback ensures that external memory is in the correct power state and
 * ready for operation after system wake-up.
 *
 * @param callback_params Pointer to power management callback parameters (unused)
 * @param mode Power management callback mode indicating the transition phase:
 *             - CY_SYSPM_BEFORE_TRANSITION: Before entering deep sleep
 *             - CY_SYSPM_AFTER_DS_WFI_TRANSITION: After waking from deep sleep
 *
 * @return Status of the operation:
 *         - CY_SYSPM_SUCCESS: Callback executed successfully
 *         - CY_SYSPM_FAIL: Callback failed to execute properly
 */
cy_en_syspm_status_t smif_pm_callback(cy_stc_syspm_callback_params_t *callback_params,
				      cy_en_syspm_callback_mode_t mode)
{
	cy_en_smif_status_t smif_functions_status;
	cy_en_syspm_status_t ret_val = CY_SYSPM_FAIL;

	if (mode == CY_SYSPM_BEFORE_TRANSITION) {
		smif_functions_status =
			Cy_SMIF_MemCmdPowerDown(SMIF_HW, &sfdp_slave_slot_0, &smif_context);
		smif_disable();
		if (CY_SMIF_SUCCESS == smif_functions_status) {
			ret_val = CY_SYSPM_SUCCESS;
		}
	} else if (mode == CY_SYSPM_AFTER_DS_WFI_TRANSITION) {
		smif_enable();
		smif_functions_status =
			Cy_SMIF_MemCmdReleasePowerDown(SMIF_HW, &sfdp_slave_slot_0, &smif_context);
		if (CY_SMIF_SUCCESS == smif_functions_status) {
			if (!is_memory_ready(&sfdp_slave_slot_0)) {
				ret_val = CY_SYSPM_SUCCESS;
			}
		}
	} else {
		ret_val = CY_SYSPM_SUCCESS;
	}
	return ret_val;
}

/** Deep sleep callback structure */
static cy_stc_syspm_callback_t smif_deep_sleep = {
	&smif_pm_callback,           CY_SYSPM_DEEPSLEEP, 0, &smif_deep_sleep_param, NULL, NULL,
	EXT_MEMORY_PM_CALLBACK_ORDER};

/**
 *
 * Initialize External Memory power management.
 *
 * This function registers the deep sleep callback for SMIF power management,
 * enabling automatic handling of external memory power states during system
 * sleep/wake cycles. Should be called during system initialization.
 *
 * @return none
 *
 *******************************************************************************/
void smif_pm_init(void)
{
	Cy_SysPm_RegisterCallback(&smif_deep_sleep);
}
