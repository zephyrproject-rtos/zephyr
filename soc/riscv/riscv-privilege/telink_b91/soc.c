/*
 * Copyright (c) 2021 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys.h>
#include <clock.h>
#include <gpio.h>
#include <ext_driver/ext_pm.h>
#include <zephyr/device.h>


/* Software reset defines */
#define reg_reset                   REG_ADDR8(0x1401ef)
#define SOFT_RESET                  0x20u

/* List of supported CCLK frequencies */
#define CLK_16MHZ                   16000000u
#define CLK_24MHZ                   24000000u
#define CLK_32MHZ                   32000000u
#define CLK_48MHZ                   48000000u
#define CLK_60MHZ                   60000000u
#define CLK_96MHZ                   96000000u

/* Power Mode value */
#if DT_ENUM_IDX(DT_NODELABEL(power), power_mode) == 0
	#define POWER_MODE      LDO_1P4_LDO_1P8
#elif DT_ENUM_IDX(DT_NODELABEL(power), power_mode) == 1
	#define POWER_MODE      DCDC_1P4_LDO_1P8
#elif DT_ENUM_IDX(DT_NODELABEL(power), power_mode) == 2
	#define POWER_MODE      DCDC_1P4_DCDC_1P8
#else
	#error "Wrong value for power-mode parameter"
#endif

/* Vbat Type value */
#if DT_ENUM_IDX(DT_NODELABEL(power), vbat_type) == 0
	#define VBAT_TYPE       VBAT_MAX_VALUE_LESS_THAN_3V6
#elif DT_ENUM_IDX(DT_NODELABEL(power), vbat_type) == 1
	#define VBAT_TYPE       VBAT_MAX_VALUE_GREATER_THAN_3V6
#else
	#error "Wrong value for vbat-type parameter"
#endif

/* Check System Clock value. */
#if ((DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency) != CLK_16MHZ) &&	 \
	(DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency) != CLK_24MHZ) && \
	(DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency) != CLK_32MHZ) && \
	(DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency) != CLK_48MHZ) && \
	(DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency) != CLK_60MHZ) && \
	(DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency) != CLK_96MHZ))
	#error "Unsupported clock-frequency. Supported values: 16, 24, 32, 48, 60 and 96 MHz"
#endif

/**
 * @brief Perform basic initialization at boot.
 *
 * @return 0
 */
static int soc_b91_init(const struct device *arg)
{
	unsigned int cclk = DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency);

	ARG_UNUSED(arg);

#if (defined(CONFIG_PM) && defined(CONFIG_BT_B91))
	/* Select internal 32K for BLE PM, ASAP after boot */
	blc_pm_select_internal_32k_crystal();
#endif /* CONFIG_PM && CONFIG_BT_B91 */

	/* system init */
	sys_init(POWER_MODE, VBAT_TYPE);

#if CONFIG_PM
	gpio_shutdown(GPIO_ALL);
#endif /* CONFIG_PM */

	/* clocks init: CCLK, HCLK, PCLK */
	switch (cclk) {
	case CLK_16MHZ:
		CCLK_16M_HCLK_16M_PCLK_16M;
		break;

	case CLK_24MHZ:
		CCLK_24M_HCLK_24M_PCLK_24M;
		break;

	case CLK_32MHZ:
		CCLK_32M_HCLK_32M_PCLK_16M;
		break;

	case CLK_48MHZ:
		CCLK_48M_HCLK_48M_PCLK_24M;
		break;

	case CLK_60MHZ:
		CCLK_60M_HCLK_30M_PCLK_15M;
		break;

	case CLK_96MHZ:
		CCLK_96M_HCLK_48M_PCLK_24M;
		break;
	}

	/* Init Machine Timer source clock: 32 KHz RC */
	clock_32k_init(CLK_32K_RC);
	clock_cal_32k_rc();

	return 0;
}

/**
 * @brief Reset the system.
 */
void sys_arch_reboot(int type)
{
	ARG_UNUSED(type);

	reg_reset = SOFT_RESET;
}

SYS_INIT(soc_b91_init, PRE_KERNEL_1, 0);
