/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/toolchain.h>
#include <zephyr/logging/log.h>

#include <errno.h>
#include <string.h>

#include <cy_pdl.h>
#include <cy_syspm.h>

LOG_MODULE_REGISTER(soc_poweroff, CONFIG_SOC_LOG_LEVEL);

#if defined(CONFIG_PSOC_EDGE_M55_SRF_SUPPORT)
#include <cy_syspm_srf.h>
#include <mtb_srf_pool.h>

/*
 * Mirror of the PDL's private status output struct (cy_syspm_v4.c). The
 * secure SysPm operations return a single cy_en_syspm_status_t value.
 */
struct ifx_syspm_srf_status_out {
	cy_en_syspm_status_t ret_val;
};

/*
 * Mirror of the PDL's private input struct for CY_PDL_SYSPM_OP_HIBWAKEPREP
 * (cy_syspm_v4.c). The secure handler expects a single wakeup-source word.
 */
struct ifx_syspm_srf_hibwakeprep_in {
	uint32_t wakeup_source;
};

/*
 * Enter hibernate from the CM55 non-secure core.
 *
 * The PDL compiles the SRF relay out of Cy_SysPm_SystemEnterHibernate()
 * for CM55 (guarded by !(CY_CPU_CORTEX_M55)), so the stock call writes
 * SRSS_PWR_HIBERNATE directly and bus-faults from the non-secure core.
 * The secure image already registers the hibernate operation
 * (CY_PDL_SYSPM_OP_SYSTEMENTERHIBERNATE), so invoke it over the SRF
 * directly - the same relay the PDL uses for the other cores.
 */
static cy_en_syspm_status_t ifx_cm55_enter_hibernate_srf(void)
{
	mtb_srf_invec_ns_t *in_vec = NULL;
	mtb_srf_outvec_ns_t *out_vec = NULL;
	mtb_srf_output_ns_t *output = NULL;
	struct ifx_syspm_srf_status_out output_args = {
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
		.op_id = (uint8_t)CY_PDL_SYSPM_OP_SYSTEMENTERHIBERNATE,
		.submodule_id = CY_PDL_SECURE_SUBMODULE_SYSPM,
		.base = NULL,
		.sub_block = 0UL,
		.input_base = NULL,
		.input_len = 0UL,
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

/*
 * Release the Hibernate I/O freeze and arm a Hibernate wakeup source from a
 * non-secure core.  Both Cy_SysPm_IoUnfreeze() and
 * Cy_SysPm_SetHibernateWakeupSource() write PPC-secured SRSS registers that
 * bus-fault from a non-secure core and have no stock SRF relay, so invoke the
 * combined secure operation (CY_PDL_SYSPM_OP_HIBWAKEPREP) over the SRF - the
 * same relay used above for the Hibernate-entry operation.  Runs in thread
 * context; the SRF round-trip completes on an IPC interrupt.
 */
static int ifx_hib_wake_prepare_srf(uint32_t wakeup_source)
{
	mtb_srf_invec_ns_t *in_vec = NULL;
	mtb_srf_outvec_ns_t *out_vec = NULL;
	mtb_srf_output_ns_t *output = NULL;
	struct ifx_syspm_srf_hibwakeprep_in input_args = {
		.wakeup_source = wakeup_source,
	};
	cy_rslt_t result;

	result = mtb_srf_pool_allocate(&cy_pdl_srf_default_pool, &in_vec, &out_vec,
				       CY_PDL_SYSPM_SRF_POOL_TIMEOUT);
	if (result != CY_RSLT_SUCCESS) {
		return -EIO;
	}

	cy_pdl_invoke_srf_args invoke_args = {
		.inVec = in_vec,
		.outVec = out_vec,
		.output_ptr = &output,
		.op_id = (uint8_t)CY_PDL_SYSPM_OP_HIBWAKEPREP,
		.submodule_id = CY_PDL_SECURE_SUBMODULE_SYSPM,
		.base = NULL,
		.sub_block = 0UL,
		.input_base = (uint8_t *)&input_args,
		.input_len = sizeof(input_args),
		.output_base = NULL,
		.output_len = 0UL,
		.invec_bases = NULL,
		.invec_sizes = 0UL,
		.outvec_bases = NULL,
		.outvec_sizes = 0UL,
	};

	result = _Cy_PDL_Invoke_SRF(&invoke_args);

	(void)mtb_srf_pool_free(&cy_pdl_srf_default_pool, in_vec, out_vec);

	return (result == CY_RSLT_SUCCESS) ? 0 : -EIO;
}

#if DT_HAS_COMPAT_STATUS_OKAY(infineon_hibernate_wakeup)

#define IFX_HIB_WAKEUP_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(infineon_hibernate_wakeup)

/*
 * Build the Hibernate wakeup-source mask from the infineon,hibernate-wakeup
 * devicetree node.
 */
static uint32_t ifx_hib_wakeup_source_mask(void)
{
	uint32_t src = 0U;

#if DT_NODE_HAS_PROP(IFX_HIB_WAKEUP_NODE, wakeup_pin0_trigger)
	src |= (DT_ENUM_IDX(IFX_HIB_WAKEUP_NODE, wakeup_pin0_trigger) == 0)
		       ? CY_SYSPM_HIBERNATE_PIN0_LOW
		       : CY_SYSPM_HIBERNATE_PIN0_HIGH;
#endif
#if DT_NODE_HAS_PROP(IFX_HIB_WAKEUP_NODE, wakeup_pin1_trigger)
	src |= (DT_ENUM_IDX(IFX_HIB_WAKEUP_NODE, wakeup_pin1_trigger) == 0)
		       ? CY_SYSPM_HIBERNATE_PIN1_LOW
		       : CY_SYSPM_HIBERNATE_PIN1_HIGH;
#endif
#if DT_NODE_HAS_PROP(IFX_HIB_WAKEUP_NODE, wakeup_lpcomp0_trigger)
	src |= (DT_ENUM_IDX(IFX_HIB_WAKEUP_NODE, wakeup_lpcomp0_trigger) == 0)
		       ? CY_SYSPM_HIBERNATE_LPCOMP0_LOW
		       : CY_SYSPM_HIBERNATE_LPCOMP0_HIGH;
#endif
#if DT_NODE_HAS_PROP(IFX_HIB_WAKEUP_NODE, wakeup_lpcomp1_trigger)
	src |= (DT_ENUM_IDX(IFX_HIB_WAKEUP_NODE, wakeup_lpcomp1_trigger) == 0)
		       ? CY_SYSPM_HIBERNATE_LPCOMP1_LOW
		       : CY_SYSPM_HIBERNATE_LPCOMP1_HIGH;
#endif
#if DT_PROP(IFX_HIB_WAKEUP_NODE, wakeup_wdt)
	src |= CY_SYSPM_HIBERNATE_WDT;
#endif
#if DT_PROP(IFX_HIB_WAKEUP_NODE, wakeup_rtc_alarm)
	src |= CY_SYSPM_HIBERNATE_RTC_ALARM;
#endif

	return src;
}

/*
 * Release any Hibernate I/O freeze from a previous wake and arm the Hibernate
 * wakeup source(s) described by the infineon,hibernate-wakeup devicetree node.
 * On the non-secure CM55 both the unfreeze and the wakeup-source write target
 * PPC-secured SRSS registers, so the combined HIBWAKEPREP secure operation
 * performs them over the SRF.  Runs in thread context (POST_KERNEL or later,
 * once the kernel is alive) so the blocking SRF round-trip can complete.
 */
static int ifx_hib_wakeup_arm(void)
{
	uint32_t src = ifx_hib_wakeup_source_mask();
	int ret;

	if (src == 0U) {
		return 0;
	}

	ret = ifx_hib_wake_prepare_srf(src);
	if (ret != 0) {
		LOG_WRN("Hibernate wakeup source arm failed (%d)", ret);
	}

	return 0;
}

SYS_INIT(ifx_hib_wakeup_arm, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

#endif /* DT_HAS_COMPAT_STATUS_OKAY(infineon_hibernate_wakeup) */
#endif /* CONFIG_PSOC_EDGE_M55_SRF_SUPPORT */

void z_sys_poweroff(void)
{
	cy_en_syspm_status_t status;

#if defined(CONFIG_PSOC_EDGE_M55_SRF_SUPPORT)
	/*
	 * sys_poweroff() masks interrupts before calling us, but the SRF
	 * round-trip to the secure image completes on an IPC interrupt and
	 * blocks on an RTOS semaphore. Re-enable interrupts so that ISR can
	 * run: on this path the chip is about to hibernate, so there is
	 * nothing left to protect from preemption.
	 */
	irq_unlock(0);

	status = ifx_cm55_enter_hibernate_srf();
#else
	status = Cy_SysPm_SystemEnterHibernate();
#endif

	/*
	 * A successful hibernate request never returns - the system powers down.
	 * Reaching here means the request failed (the secure relay rejected it,
	 * or the direct PDL call could not enter hibernate), which is
	 * unrecoverable. z_sys_poweroff() is FUNC_NORETURN, so log the failure
	 * and spin with interrupts masked instead of returning into an undefined
	 * state.
	 */
	LOG_ERR("Hibernate entry failed (status %d); halting", (int)status);

	(void)irq_lock();
	for (;;) {
		__WFI();
	}
}
