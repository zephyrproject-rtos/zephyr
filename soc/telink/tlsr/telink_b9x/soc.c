/*
 * Copyright (c) 2023-2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys.h>
#include <clock.h>
#include <gpio.h>
#include <ext_driver/ext_pm.h>
#include "rf.h"

#include "flash.h"
#include <watchdog.h>
#include <zephyr/device.h>
#include <zephyr/storage/flash_map.h>

#if (defined(CONFIG_BT_B9X) || defined(CONFIG_IEEE802154))
#include "b9x_bt_flash.h"
#endif

#if CONFIG_SOC_RISCV_TELINK_B91
#undef rand
#endif /* CONFIG_SOC_RISCV_TELINK_B91 */
#include <stdlib.h>

/* Softre reset defines */
#define reg_reset                   REG_ADDR8(0x1401ef)
#define SOFT_RESET                  0x20u

/* List of supported CCLK frequencies */
#define CLK_16MHZ                   16000000u
#define CLK_24MHZ                   24000000u
#define CLK_32MHZ                   32000000u
#define CLK_48MHZ                   48000000u
#define CLK_60MHZ                   60000000u
#define CLK_80MHZ                   80000000u
#define CLK_96MHZ                   96000000u
#define CLK_120MHZ                  120000000u

/* MID register flash size */
#define FLASH_MID_SIZE_OFFSET       16
#define FLASH_MID_SIZE_MASK         0x00ff0000

/* Power Mode value */
#if CONFIG_SOC_RISCV_TELINK_B91
	#if DT_ENUM_IDX(DT_NODELABEL(power), power_mode) == 0
		#define POWER_MODE      LDO_1P4_LDO_1P8
	#elif DT_ENUM_IDX(DT_NODELABEL(power), power_mode) == 1
		#define POWER_MODE      DCDC_1P4_LDO_1P8
	#elif DT_ENUM_IDX(DT_NODELABEL(power), power_mode) == 2
		#define POWER_MODE      DCDC_1P4_DCDC_1P8
	#else
		#error "Wrong value for power-mode parameter"
	#endif
#elif CONFIG_SOC_RISCV_TELINK_B92
	#if DT_ENUM_IDX(DT_NODELABEL(power), power_mode) == 0
		#define POWER_MODE      LDO_1P4_LDO_2P0
	#elif DT_ENUM_IDX(DT_NODELABEL(power), power_mode) == 1
		#define POWER_MODE      DCDC_1P4_LDO_2P0
	#elif DT_ENUM_IDX(DT_NODELABEL(power), power_mode) == 2
		#define POWER_MODE      DCDC_1P4_DCDC_2P0
	#else
		#error "Wrong value for power-mode parameter"
	#endif
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
	(DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency) != CLK_80MHZ) && \
	(DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency) != CLK_96MHZ) && \
	(DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency) != CLK_120MHZ))
	#error "Invalid clock-frequency. Supported values: 16, 24, 32, 48, 60, 80, 96 and 120 MHz"
#endif

#if (defined(CONFIG_BT_B9X) || defined(CONFIG_IEEE802154))
/* SOC Parameters structure */
_attribute_data_retention_sec_ struct {
	unsigned char	cap_freq_offset_en;
	unsigned char	cap_freq_offset_value;
} soc_nvParam;

/**
 * @brief Perform SOC calibration at boot time (normal boot)
 */
void soc_load_rf_parameters_normal(void)
{
	if (!blt_miscParam.ext_cap_en) {
		unsigned char cap_freq_ofset;

		flash_read_page(FIXED_PARTITION_OFFSET(vendor_partition) +
		B9X_CALIBRATION_ADDR_OFFSET, 1, &cap_freq_ofset);
		if (cap_freq_ofset != 0xff) {
			soc_nvParam.cap_freq_offset_en = 1;
			soc_nvParam.cap_freq_offset_value = cap_freq_ofset;
			rf_update_internal_cap(soc_nvParam.cap_freq_offset_value);
		}
	}
}

/**
 * @brief Perform SOC calibration at boot time (deep retention)
 */
void soc_load_rf_parameters_deep_retention(void)
{
	if (soc_nvParam.cap_freq_offset_value) {
		rf_update_internal_cap(soc_nvParam.cap_freq_offset_value);
	}
}
#endif

/**
 * @brief Perform basic initialization at boot.
 *
 * @return 0
 */
void soc_early_init_hook(void)
{
unsigned int cclk = DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency);

#ifdef CONFIG_PM
	/* Select internal 32K for BLE PM, ASAP after boot */
	blc_pm_select_internal_32k_crystal();
#endif /* CONFIG_PM  */

	/* system init */
#if CONFIG_SOC_RISCV_TELINK_B91
	sys_init(POWER_MODE, VBAT_TYPE);
#elif CONFIG_SOC_RISCV_TELINK_B92
	sys_init(POWER_MODE, VBAT_TYPE, GPIO_VOLTAGE_3V3);
#endif

#if CONFIG_PM
	gpio_shutdown(GPIO_ALL);
#endif /* CONFIG_PM */

#if (defined(CONFIG_BT_B9X) || defined(CONFIG_IEEE802154))
	soc_load_rf_parameters_normal();
#endif

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

#if CONFIG_SOC_RISCV_TELINK_B91
	case CLK_60MHZ:
		CCLK_60M_HCLK_30M_PCLK_15M;
		break;
#endif

	case CLK_96MHZ:
		CCLK_96M_HCLK_48M_PCLK_24M;
		break;
	}

	/* Init Machine Timer source clock: 32 KHz RC */
	clock_32k_init(CLK_32K_RC);
	clock_cal_32k_rc();

#if CONFIG_SOC_RISCV_TELINK_B92
	/* Stop 32k watchdog */
	wd_32k_stop();
#endif
}

/**
 * @brief Reset the system.
 */
void sys_arch_reboot(int type)
{
	ARG_UNUSED(type);
	reg_reset = SOFT_RESET;
}

/**
 * @brief Restore SOC after deep-sleep.
 */
void soc_b9x_restore(void)
{
	unsigned int cclk = DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency);

	/* system init */
#if CONFIG_SOC_RISCV_TELINK_B91
	sys_init(POWER_MODE, VBAT_TYPE);
#elif CONFIG_SOC_RISCV_TELINK_B92
	sys_init(POWER_MODE, VBAT_TYPE, GPIO_VOLTAGE_3V3);
#endif

#if CONFIG_PM
	gpio_shutdown(GPIO_ALL);
#endif /* CONFIG_PM */

#if (defined(CONFIG_BT_B9X) || defined(CONFIG_IEEE802154))
	soc_load_rf_parameters_deep_retention();
#endif

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

#if CONFIG_SOC_RISCV_TELINK_B91
	case CLK_60MHZ:
		CCLK_60M_HCLK_30M_PCLK_15M;
		break;
#endif

	case CLK_96MHZ:
		CCLK_96M_HCLK_48M_PCLK_24M;
		break;
	}
}

#if CONFIG_SOC_RISCV_TELINK_B92

#include "flash/flash_common.h"
#include "flash_base.h"

/**
 * @brief       This function is used to set the use of four lines when reading and writing flash.
 * @param[in]   flash_mid   - the mid of flash.
 * @return      1: success, 0: error, 2: mid is not supported.
 */
unsigned char flash_set_4line_read_write(unsigned int flash_mid)
{

	unsigned char status = flash_4line_en(flash_mid);

	if (status == 1) {
		flash_read_page = flash_4read;
		flash_set_xip_config(FLASH_X4READ_CMD);
		flash_write_page = flash_quad_page_program;
	}

	return status;
}
#endif


/**
 * @brief Check mounted flash size (should be greater than in .dts).
 */
static int soc_b9x_check_flash(void)
{
	static const size_t dts_flash_size = DT_REG_SIZE(DT_CHOSEN(zephyr_flash));
	size_t hw_flash_size = 0;
	unsigned int  mid = flash_read_mid();

	const flash_capacity_e hw_flash_cap = (mid & FLASH_MID_SIZE_MASK) >> FLASH_MID_SIZE_OFFSET;

#if CONFIG_SOC_RISCV_TELINK_B92
	/* Enable 4x SPI read and write */
	if (flash_set_4line_read_write(mid) != 1) {

	}
#endif

	switch (hw_flash_cap) {
	case FLASH_SIZE_1M:
		hw_flash_size = 1 * 1024 * 1024;
		break;
	case FLASH_SIZE_2M:
		hw_flash_size = 2 * 1024 * 1024;
		break;
	case FLASH_SIZE_4M:
		hw_flash_size = 4 * 1024 * 1024;
		break;
	case FLASH_SIZE_8M:
		hw_flash_size = 8 * 1024 * 1024;
		break;
#if CONFIG_SOC_RISCV_TELINK_B92
	case FLASH_SIZE_16M:
		hw_flash_size = 16 * 1024 * 1024;
		break;
#endif
	default:
		break;
	}

	if (hw_flash_size < dts_flash_size) {
		printk("!!! flash error: expected (.dts) %u, actually %u\n",
			dts_flash_size, hw_flash_size);
		abort();
	}

	return 0;
}

SYS_INIT(soc_b9x_check_flash, POST_KERNEL, 0);
