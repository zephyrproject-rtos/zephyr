/* Copyright (c) 2024 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Poor man driver for 917 clocks. 917 includes High Performace (HP) clock
 * (@46000000), Ultra Lower Power (ULP) clock (@24041400) and ULP VBAT (@24048000)
 *
 */
#include <zephyr/dt-bindings/clock/silabs/siwx91x-clock.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/logging/log.h>

#include "rsi_power_save.h"
#include "rsi_rom_ulpss_clk.h"
#include "rsi_rom_clks.h"
#include "clock_update.h"
#include "sl_si91x_clock_manager.h"

#define DT_DRV_COMPAT silabs_siwx91x_clock

LOG_MODULE_REGISTER(siwx91x_clock, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

struct siwx91x_clock_data {
	uint32_t enable;
};

static int siwx91x_clock_on(const struct device *dev, clock_control_subsys_t sys)
{
	struct siwx91x_clock_data *data = dev->data;
	uintptr_t clockid = (uintptr_t)sys;

	switch (clockid) {
	case SIWX91X_CLK_ULP_UART:
		RSI_PS_UlpssPeriPowerUp(ULPSS_PWRGATE_ULP_UART);
		RSI_ULPSS_UlpUartClkConfig(ULPCLK, ENABLE_STATIC_CLK,
					   false, ULP_UART_ULP_MHZ_RC_CLK, 1);
		break;
	case SIWX91X_CLK_ULP_I2C:
		RSI_PS_UlpssPeriPowerUp(ULPSS_PWRGATE_ULP_I2C);
		RSI_ULPSS_PeripheralEnable(ULPCLK, ULP_I2C_CLK, ENABLE_STATIC_CLK);
		break;
	case SIWX91X_CLK_ULP_DMA:
		RSI_PS_UlpssPeriPowerUp(ULPSS_PWRGATE_ULP_UDMA);
		RSI_ULPSS_PeripheralEnable(ULPCLK, ULP_UDMA_CLK, ENABLE_STATIC_CLK);
		break;
	case SIWX91X_CLK_UART1:
		RSI_PS_M4ssPeriPowerUp(M4SS_PWRGATE_ULP_EFUSE_PERI);
		/* RSI_CLK_UsartClkConfig() calls RSI_CLK_PeripheralClkEnable(); */
		RSI_CLK_UsartClkConfig(M4CLK, ENABLE_STATIC_CLK, 0, USART1, 0, 1);
		break;
	case SIWX91X_CLK_UART2:
		RSI_PS_M4ssPeriPowerUp(M4SS_PWRGATE_ULP_EFUSE_PERI);
		/* RSI_CLK_UsartClkConfig() calls RSI_CLK_PeripheralClkEnable(); */
		RSI_CLK_UsartClkConfig(M4CLK, ENABLE_STATIC_CLK, 0, USART2, 0, 1);
		break;
	case SIWX91X_CLK_I2C0:
		RSI_PS_M4ssPeriPowerUp(M4SS_PWRGATE_ULP_EFUSE_PERI);
		RSI_CLK_I2CClkConfig(M4CLK, true, 0);
		break;
	case SIWX91X_CLK_I2C1:
		RSI_PS_M4ssPeriPowerUp(M4SS_PWRGATE_ULP_EFUSE_PERI);
		RSI_CLK_I2CClkConfig(M4CLK, true, 1);
		break;
	case SIWX91X_CLK_DMA0:
		RSI_PS_M4ssPeriPowerUp(M4SS_PWRGATE_ULP_EFUSE_PERI);
		RSI_CLK_PeripheralClkEnable(M4CLK, UDMA_CLK, ENABLE_STATIC_CLK);
		break;
	default:
		return -EINVAL;
	}
	data->enable |= BIT(clockid);

	return 0;
}

static int siwx91x_clock_off(const struct device *dev, clock_control_subsys_t sys)
{
	struct siwx91x_clock_data *data = dev->data;
	uintptr_t clockid = (uintptr_t)sys;

	switch (clockid) {
	case SIWX91X_CLK_ULP_I2C:
		RSI_ULPSS_PeripheralDisable(ULPCLK, ULP_I2C_CLK);
		break;
	case SIWX91X_CLK_ULP_DMA:
		RSI_ULPSS_PeripheralDisable(ULPCLK, ULP_UDMA_CLK);
		break;
	case SIWX91X_CLK_UART1:
		RSI_CLK_PeripheralClkDisable(M4CLK, USART1_CLK);
		break;
	case SIWX91X_CLK_UART2:
		RSI_CLK_PeripheralClkDisable(M4CLK, USART2_CLK);
		break;
	case SIWX91X_CLK_DMA0:
		RSI_CLK_PeripheralClkDisable(M4CLK, UDMA_CLK);
		break;
	case SIWX91X_CLK_ULP_UART:
	case SIWX91X_CLK_I2C0:
	case SIWX91X_CLK_I2C1:
		/* Not supported */
		return 0;
	default:
		return -EINVAL;
	}

	data->enable &= ~BIT(clockid);
	return 0;
}

static int siwx91x_clock_get_rate(const struct device *dev, clock_control_subsys_t sys,
				  uint32_t *rate)
{
	uintptr_t clockid = (uintptr_t)sys;

	switch (clockid) {
	case SIWX91X_CLK_ULP_UART:
		*rate = RSI_CLK_GetBaseClock(ULPSS_UART);
		return 0;
	case SIWX91X_CLK_UART1:
		*rate = RSI_CLK_GetBaseClock(M4_USART0);
		return 0;
	case SIWX91X_CLK_UART2:
		*rate = RSI_CLK_GetBaseClock(M4_UART1);
		return 0;
	default:
		/* For now, no other driver need clock rate */
		return -EINVAL;
	}
}

static enum clock_control_status siwx91x_clock_get_status(const struct device *dev,
							  clock_control_subsys_t sys)
{
	struct siwx91x_clock_data *data = dev->data;
	uintptr_t clockid = (uintptr_t)sys;

	if (data->enable & BIT(clockid)) {
		return CLOCK_CONTROL_STATUS_ON;
	} else {
		return CLOCK_CONTROL_STATUS_OFF;
	}
}

static int siwx91x_clock_init(const struct device *dev)
{
	SystemCoreClockUpdate();

	/* Use SoC PLL at configured frequency as core clock */
	sl_si91x_clock_manager_m4_set_core_clk(M4_SOCPLLCLK, CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC);

	/* Use interface PLL at configured frequency as peripheral clock */
	sl_si91x_clock_manager_set_pll_freq(INFT_PLL, CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC,
					    PLL_REF_CLK_VAL_XTAL);

	/* FIXME: Currently the clock consumer use clocks without power on them.
	 * This should be fixed in drivers. Meanwhile, get the list of required
	 * clocks using DT labels.
	 */
#if DT_NODE_HAS_STATUS(DT_NODELABEL(ulpi2c), okay)
	siwx91x_clock_on(dev, (clock_control_subsys_t)SIWX91X_CLK_ULP_I2C);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2c0), okay)
	siwx91x_clock_on(dev, (clock_control_subsys_t)SIWX91X_CLK_I2C0);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2c1), okay)
	siwx91x_clock_on(dev, (clock_control_subsys_t)SIWX91X_CLK_I2C1);
#endif

	return 0;
}

static DEVICE_API(clock_control, siwx91x_clock_api) = {
	.on = siwx91x_clock_on,
	.off = siwx91x_clock_off,
	.get_rate = siwx91x_clock_get_rate,
	.get_status = siwx91x_clock_get_status,
};

#define SIWX91X_CLOCK_INIT(p)                                                                \
	static struct siwx91x_clock_data siwx91x_clock_data_##p;                             \
	DEVICE_DT_INST_DEFINE(p, siwx91x_clock_init, NULL, &siwx91x_clock_data_##p, NULL,    \
			      PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY,              \
			      &siwx91x_clock_api);

DT_INST_FOREACH_STATUS_OKAY(SIWX91X_CLOCK_INIT)
