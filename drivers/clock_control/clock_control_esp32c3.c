/*
 * Copyright (c) 2021 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_rtc

#include <zephyr/dt-bindings/clock/esp32c3_clock.h>
#include <hal/clk_gate_ll.h>
#include <soc/soc_caps.h>
#include <soc/soc.h>
#include <soc/rtc.h>
#include <rtc_clk_common.h>

#include <soc.h>
#include <zephyr/drivers/clock_control.h>
#include <driver/periph_ctrl.h>

static int clock_control_esp32_on(const struct device *dev,
				  clock_control_subsys_t sys)
{
	ARG_UNUSED(dev);
	periph_module_enable((periph_module_t)sys);
	return 0;
}

static int clock_control_esp32_off(const struct device *dev,
				   clock_control_subsys_t sys)
{
	ARG_UNUSED(dev);
	periph_module_disable((periph_module_t)sys);
	return 0;
}

static enum clock_control_status clock_control_esp32_get_status(const struct device *dev,
								clock_control_subsys_t sys)
{
	ARG_UNUSED(dev);
	uint32_t clk_en_reg = periph_ll_get_clk_en_reg((periph_module_t)sys);
	uint32_t clk_en_mask =  periph_ll_get_clk_en_mask((periph_module_t)sys);

	if (DPORT_GET_PERI_REG_MASK(clk_en_reg, clk_en_mask)) {
		return CLOCK_CONTROL_STATUS_ON;
	}
	return CLOCK_CONTROL_STATUS_OFF;
}

static int clock_control_esp32_get_rate(const struct device *dev,
					clock_control_subsys_t sub_system,
					uint32_t *rate)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(sub_system);

	uint32_t soc_clk_sel = REG_GET_FIELD(SYSTEM_SYSCLK_CONF_REG, SYSTEM_SOC_CLK_SEL);
	uint32_t cpuperiod_sel;
	uint32_t source_freq_mhz;
	uint32_t clk_div;

	switch (soc_clk_sel) {
	case DPORT_SOC_CLK_SEL_XTAL:
		clk_div = REG_GET_FIELD(SYSTEM_SYSCLK_CONF_REG, SYSTEM_PRE_DIV_CNT) + 1;
		source_freq_mhz = (uint32_t) rtc_clk_xtal_freq_get();
		*rate = MHZ(source_freq_mhz / clk_div);
		return 0;
	case DPORT_SOC_CLK_SEL_PLL:
		cpuperiod_sel = DPORT_REG_GET_FIELD(SYSTEM_CPU_PER_CONF_REG, SYSTEM_CPUPERIOD_SEL);
		if (cpuperiod_sel == DPORT_CPUPERIOD_SEL_80) {
			*rate = MHZ(80);
		} else if (cpuperiod_sel == DPORT_CPUPERIOD_SEL_160) {
			*rate = MHZ(160);
		} else {
			*rate = 0;
			return -ENOTSUP;
		}
		return 0;
	case DPORT_SOC_CLK_SEL_8M:
		*rate = MHZ(8);
		return 0;
	default:
		*rate = 0;
		return -ENOTSUP;
	}
}

static int clock_control_esp32_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static const struct clock_control_driver_api clock_control_esp32_api = {
	.on = clock_control_esp32_on,
	.off = clock_control_esp32_off,
	.async_on = NULL,
	.get_rate = clock_control_esp32_get_rate,
	.get_status = clock_control_esp32_get_status,
};

DEVICE_DT_DEFINE(DT_NODELABEL(rtc),
		 &clock_control_esp32_init,
		 NULL,
		 NULL,
		 NULL,
		 PRE_KERNEL_1,
		 CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		 &clock_control_esp32_api);
