/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <fsl_clock.h>
#include <fsl_cmc.h>
#include <fsl_lpuart.h>
#include "power_mgmt.h"

#define CONSOLE_NODE DT_CHOSEN(zephyr_console)
#define CONSOLE_BASE ((LPUART_Type *)DT_REG_ADDR(CONSOLE_NODE))

BUILD_ASSERT(DT_NODE_HAS_COMPAT(CONSOLE_NODE, nxp_lpuart),
	     "MCXW7x S2RAM requires an LPUART console");

#if defined(CONFIG_PINCTRL_NON_STATIC)
PINCTRL_DT_DEV_CONFIG_DECLARE(CONSOLE_NODE);
#else
PINCTRL_DT_DEFINE(CONSOLE_NODE);
#endif

static const struct device *const console = DEVICE_DT_GET(CONSOLE_NODE);
static struct uart_config console_config;
static uint32_t sram_disable;
static uint32_t debug_control;
static uint32_t core_frequency;

void test_pm_s2ram_prepare(void)
{
	CMC_Type *cmc = (CMC_Type *)DT_REG_ADDR(DT_NODELABEL(cmc));

	zassert_true(device_is_ready(console), "Console is not ready");
	zassert_ok(uart_config_get(console, &console_config), "Cannot read console configuration");
	sram_disable = cmc->SRAMDIS[0] & CMC_SRAMDIS_DIS_MASK;
#if !defined(CONFIG_SOC_SERIES_MCXW7XX_SHUTDOWN_UNUSED_RAM)
	zassert_equal(sram_disable, 0U, "Unexpected SRAM disable with static shutdown off");
#endif
	debug_control = cmc->DBGCTL & CMC_DBGCTL_SOD_MASK;
	core_frequency = CLOCK_GetCoreSysClkFreq();
	zassert_true(WAIT_FOR((LPUART_GetStatusFlags(CONSOLE_BASE) &
			      kLPUART_TransmissionCompleteFlag) != 0U,
			     100U * USEC_PER_MSEC, k_busy_wait(10)),
		     "Console TX did not drain before S2RAM");
}

void test_pm_s2ram_resume(void)
{
	CMC_Type *cmc = (CMC_Type *)DT_REG_ADDR(DT_NODELABEL(cmc));
	uint32_t resumed_sram_disable = cmc->SRAMDIS[0] & CMC_SRAMDIS_DIS_MASK;
	uint32_t resumed_debug_control = cmc->DBGCTL & CMC_DBGCTL_SOD_MASK;
	uint32_t resumed_core_frequency = CLOCK_GetCoreSysClkFreq();

	zassert_ok(pinctrl_apply_state(PINCTRL_DT_DEV_CONFIG_GET(CONSOLE_NODE),
				      PINCTRL_STATE_DEFAULT),
		   "Console pinctrl recovery failed");
	zassert_ok(uart_configure(console, &console_config),
		   "Console configuration recovery failed");
	zassert_equal(resumed_sram_disable, sram_disable, "SRAM disable policy was not restored");
	zassert_equal(resumed_debug_control, debug_control, "Debug entry setting was not restored");
	zassert_equal(resumed_core_frequency, core_frequency, "Core clock was not restored");
}
