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

#include <string.h>

#include <cy_pdl.h>
#include <cy_syspm.h>

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
#endif /* CONFIG_PSOC_EDGE_M55_SRF_SUPPORT */

void z_sys_poweroff(void)
{
#if defined(CONFIG_PSOC_EDGE_M55_SRF_SUPPORT)
	/*
	 * sys_poweroff() masks interrupts before calling us, but the SRF
	 * round-trip to the secure image completes on an IPC interrupt and
	 * blocks on an RTOS semaphore. Re-enable interrupts so that ISR can
	 * run: on this path the chip is about to hibernate, so there is
	 * nothing left to protect from preemption.
	 */
	irq_unlock(0);

	(void)ifx_cm55_enter_hibernate_srf();
#else
	Cy_SysPm_SystemEnterHibernate();
#endif

	CODE_UNREACHABLE;
}
