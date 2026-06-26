/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#if defined(CONFIG_APP_ROLE_COMPANION)
/* CM33 non-secure (TF-M) companion: PM-driven DeepSleep demo that blinks led0
 * and enters the DeepSleep mode requested by the CM55 over the SRF mailbox.
 * Brought over from samples/basic/blinky so this sample builds the CM33 image
 * as well as the CM55 controller.
 */
#include <zephyr/drivers/gpio.h>
#include <string.h>
#include <stdio.h>

#include <cy_syspm.h>
#include <soc.h>
#include <cy_pdl.h>
#include <cy_syslib.h>
#include <cy_syspm_srf.h>
#include <mtb_srf_pool.h>
#include <system_edge.h>

#include "ds_mbox.h"

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS 1000

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

/* Map a mailbox command to the PDL DeepSleep-mode enum. */
static cy_en_syspm_deep_sleep_mode_t ds_cmd_to_cy_mode(uint32_t mode)
{
	switch (mode) {
	case DS_MBOX_CMD_DS:
		return CY_SYSPM_MODE_DEEPSLEEP;
	case DS_MBOX_CMD_DS_OFF:
		return CY_SYSPM_MODE_DEEPSLEEP_OFF;
	case DS_MBOX_CMD_DS_RAM:
	default:
		return CY_SYSPM_MODE_DEEPSLEEP_RAM;
	}
}

/* Mirror of the PDL's private SRF structs (cy_syspm_v4.c).  The dedicated
 * DS-OFF token operation takes the target DeepSleep mode plus a prepare/restore
 * flag and returns a single cy_en_syspm_status_t.
 */
struct ds_srf_dsofftoken_in {
	uint32_t deep_sleep_mode;
	bool restore;
};

struct ds_srf_status_out {
	cy_en_syspm_status_t ret_val;
};

/*
 * Arm (prepare) or undo (restore) the DS-OFF retention token and the
 * SOCMEM/APPCPU power dependencies over the dedicated secure operation.  These
 * writes target secured registers that fault from the non-secure core, so relay
 * them to the secure image - the same prepare/restore the PDL performs inside
 * its CpuEnterDeepSleep relay.  Must run from a context where the SRF/IPC
 * round-trip can complete (interrupts enabled), which is why it is issued here
 * from the blink thread rather than from pm_state_set().
 */
static cy_en_syspm_status_t ds_arm_dsoff_token_srf(uint32_t deep_sleep_mode, bool restore)
{
	mtb_srf_invec_ns_t *in_vec = NULL;
	mtb_srf_outvec_ns_t *out_vec = NULL;
	mtb_srf_output_ns_t *output = NULL;
	struct ds_srf_dsofftoken_in input_args = {
		.deep_sleep_mode = deep_sleep_mode,
		.restore = restore,
	};
	struct ds_srf_status_out output_args = {
		.ret_val = CY_SYSPM_FAIL,
	};
	cy_rslt_t result;

	result = mtb_srf_pool_allocate(&cy_pdl_srf_default_pool, &in_vec, &out_vec,
				       CY_PDL_SYSPM_SRF_POOL_TIMEOUT);
	if (result != CY_RSLT_SUCCESS) {
		return CY_SYSPM_FAIL;
	}

	cy_pdl_invoke_srf_args invoke_args = {
		.inVec = in_vec,
		.outVec = out_vec,
		.output_ptr = &output,
		.op_id = (uint8_t)CY_PDL_SYSPM_OP_SETDSOFFTOKEN,
		.submodule_id = CY_PDL_SECURE_SUBMODULE_SYSPM,
		.base = NULL,
		.sub_block = 0UL,
		.input_base = (uint8_t *)&input_args,
		.input_len = sizeof(input_args),
		.output_base = (uint8_t *)&output_args,
		.output_len = sizeof(output_args),
		.invec_bases = NULL,
		.invec_sizes = 0UL,
		.outvec_bases = NULL,
		.outvec_sizes = 0UL,
	};

	result = _Cy_PDL_Invoke_SRF(&invoke_args);
	if ((result == CY_RSLT_SUCCESS) && (output != NULL)) {
		memcpy(&output_args, &output->output_values[0], sizeof(output_args));
	}

	(void)mtb_srf_pool_free(&cy_pdl_srf_default_pool, in_vec, out_vec);

	return (result == CY_RSLT_SUCCESS) ? output_args.ret_val : CY_SYSPM_FAIL;
}

/* Program every DeepSleep-mode selector this core owns for the requested
 * low-power mode and, for DS-RAM / DS-OFF, arm the retention token and tear
 * down the SOCMEM/APPCPU dependencies over the SRF.  Everything here relays to
 * the secure image, so it must run from thread context (interrupts enabled);
 * pm_state_set() runs interrupt-masked and only issues the local WFI.
 */
static void ds_apply_mode(uint32_t mode)
{
	cy_en_syspm_deep_sleep_mode_t cy_mode = ds_cmd_to_cy_mode(mode);

	Cy_SysPm_DeepSleepSetup(cy_mode);
	Cy_SysPm_SetSysDeepSleepMode(cy_mode);
	Cy_SysPm_SetAppDeepSleepMode(cy_mode);

	/* Regular DeepSleep needs neither the token nor the PDCM teardown. */
	if ((cy_mode == CY_SYSPM_MODE_DEEPSLEEP_RAM) || (cy_mode == CY_SYSPM_MODE_DEEPSLEEP_OFF)) {
		(void)ds_arm_dsoff_token_srf((uint32_t)cy_mode, false);
	}
}

static int cm33_blinky_main(void)
{
	int ret;

	if (!gpio_is_ready_dt(&led)) {
		return 0;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return 0;
	}

	/* Bring up the CM55 -> CM33-NS DeepSleep command mailbox; the CM55
	 * controller requests the low-power mode and owns the SW0 wake source.
	 */
	ds_mbox_receiver_init();

	while (1) {
		uint32_t mode = 0U;

		ret = gpio_pin_toggle_dt(&led);
		if (ret < 0) {
			return 0;
		}

		/* Zephyr PM (CONFIG_PM=y) drives the SoC into DeepSleep from the
		 * idle thread via this SoC's pm_state_set().  ds_mbox_wait() blocks
		 * for one blink period; if the CM55 commanded a mode, apply it and
		 * enter DeepSleep.
		 */
		if (ds_mbox_wait(&mode, K_MSEC(SLEEP_TIME_MS)) == 0) {
			printf("DeepSleep request (mode=%u)\n", (unsigned int)mode);
			ds_apply_mode(mode);
			(void)Cy_SysPm_CpuEnterDeepSleep(CY_SYSPM_WAIT_FOR_INTERRUPT);
			SCB_SCR &= (uint32_t)~SCB_SCR_SLEEPDEEP_Msk;
		}

		k_msleep(SLEEP_TIME_MS);
	}

	return 0;
}

/* Application entry for the CM33-NS companion.  Invoked from the shared worker
 * thread in main.c.
 */
void app_main(void)
{
	(void)cm33_blinky_main();
}

#endif /* CONFIG_APP_ROLE_COMPANION */
