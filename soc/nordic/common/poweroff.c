/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/poweroff.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>
#include <zephyr/drivers/retained_mem/nrf_retained_mem.h>

#if defined(CONFIG_IRONSIDE_TE_CLIENT)
#include <ironside_client/services/platform.h>
#elif defined(CONFIG_SOC_SERIES_NRF51) || defined(CONFIG_SOC_SERIES_NRF52)
#include <hal/nrf_power.h>
#elif defined(CONFIG_SOC_SERIES_NRF54H) || defined(CONFIG_SOC_SERIES_NRF92)
#include <soc_power.h>
#else
#include <hal/nrf_regulators.h>
#endif
#if defined(CONFIG_SOC_SERIES_NRF54L)
#include <helpers/nrfx_reset_reason.h>
#include <hal/nrf_memconf.h>
#endif

#if defined(CONFIG_HAS_NORDIC_RAM_CTRL)
#include <helpers/nrfx_ram_ctrl.h>
#endif

#if defined(CONFIG_IRONSIDE_TE_CLIENT) && defined(CONFIG_RETAINED_MEM_NRF_RAM_CTRL)
/* Refused at build time because, unlike the TF-M service that a board can
 * simply leave disabled, this branch has no second implementation to fall
 * back to.
 */
#error "CONFIG_RETAINED_MEM_NRF_RAM_CTRL is not implemented for IronSide TE targets: \
RAM retention is disabled for all sections when entering System OFF."
#endif

#if defined(CONFIG_SOC_SERIES_NRF54L)
#define VPR_POWER_IDX 1
#define VPR_RET_BIT   MEMCONF_POWER_RET_MEM0_Pos
#endif

void z_sys_poweroff(void)
{
#if defined(CONFIG_IRONSIDE_TE_CLIENT)
	/* MEMCONF and REGULATORS are secure-only here: a non-secure store to
	 * either of them takes a SecureFault that IronSide TE turns into a reset.
	 * IronSide TE enters System OFF with RAM retention as it finds it, and
	 * retention resets enabled, so disable it first as on the other targets.
	 * A refused write leaves retention on and costs System OFF current only.
	 */
	const uint32_t vpr_ret_addr = (uint32_t)&NRF_MEMCONF_S->POWER[VPR_POWER_IDX].RET;
	const struct ironside_client_platform_mem_write_pair ret_off[] = {
		{.addr = (uint32_t)&NRF_MEMCONF_S->POWER[0].RET, .value = 0U},
#if NRF_MEMCONF_HAS_RET2
		{.addr = (uint32_t)&NRF_MEMCONF_S->POWER[0].RET2, .value = 0U},
#endif
	};
	uint32_t vpr_ret;

	(void)ironside_platform_mem_write(ret_off, ARRAY_SIZE(ret_off), NULL, 0U);

	/* Set VPR to remain in its reset state when waking from OFF */
	if (ironside_platform_reg_read(vpr_ret_addr, &vpr_ret) == 0) {
		(void)ironside_platform_reg_write(vpr_ret_addr, vpr_ret & ~BIT(VPR_RET_BIT), NULL);
	}

	(void)ironside_platform_system_off();
#else

#if defined(CONFIG_HAS_NORDIC_RAM_CTRL)
	/* nRF71 disables RAM retention during secure early initialization. */
#if !defined(CONFIG_SOC_SERIES_NRF71)
	/* Disable retention for all memory blocks */
	nrfx_ram_ctrl_retention_enable_all_set(false);
#endif /* !defined(CONFIG_SOC_SERIES_NRF71) */

#endif /* defined(CONFIG_HAS_NORDIC_RAM_CTRL) */

#if defined(CONFIG_RETAINED_MEM_NRF_RAM_CTRL)
	/* Retention is best-effort because z_sys_poweroff() cannot return an
	 * error. Continue powering off if applying the devicetree configuration
	 * fails, in which case the configured retained data may not survive.
	 */
	(void)z_nrf_retained_mem_retention_apply();
#endif

#if defined(CONFIG_SOC_SERIES_NRF54L)
	/* Set VPR to remain in its reset state when waking from OFF */
	nrf_memconf_ramblock_ret_enable_set(NRF_MEMCONF, VPR_POWER_IDX, VPR_RET_BIT, false);

	nrfx_reset_reason_clear(UINT32_MAX);
#endif
#if defined(CONFIG_SOC_SERIES_NRF51) || defined(CONFIG_SOC_SERIES_NRF52)
	nrf_power_system_off(NRF_POWER);
#elif defined(CONFIG_SOC_SERIES_NRF54H) || defined(CONFIG_SOC_SERIES_NRF92)
	nrf_poweroff();
#elif defined(CONFIG_SOC_SERIES_NRF71)
	nrf_regulators_system_off(NRF_REGULATORS_NS);
#else
	nrf_regulators_system_off(NRF_REGULATORS);
#endif

#endif /* CONFIG_IRONSIDE_TE_CLIENT */

	CODE_UNREACHABLE;
}
