/*
 * Copyright (c) 2023 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys.h>
#include <clock.h>
#include <gpio.h>
#include <ext_driver/ext_pm.h>

#include "flash.h"
#include <watchdog.h>
#include <zephyr/device.h>
#include <zephyr/storage/flash_map.h>

#if (defined(CONFIG_BT_TLX) || defined(CONFIG_IEEE802154))
#include "b9x_bt_flash.h"
#endif

/* List of supported CCLK frequencies */
#define CLK_24MHZ                   24000000u
#define CLK_48MHZ                   48000000u
#define CLK_96MHZ                   96000000u

/* MID register flash size */
#define FLASH_MID_SIZE_OFFSET       16
#define FLASH_MID_SIZE_MASK         0x00ff0000

/* Power Mode value */
#if DT_ENUM_IDX(DT_NODELABEL(power), power_mode) == 0
	#define POWER_MODE      LDO_1P25_LDO_1P8
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
#if ((DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency) != CLK_24MHZ) && \
	(DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency) != CLK_48MHZ) && \
	(DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency) != CLK_96MHZ))
	#error "Invalid clock-frequency. Supported values: 24, 48, 96 MHz"
#endif

#if (defined(CONFIG_BT_TLX) || defined(CONFIG_IEEE802154))
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
static int soc_b9x_init(void)
{
	unsigned int cclk = DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency);

#ifdef CONFIG_PM
	/* Select internal 32K for BLE PM, ASAP after boot */
	blc_pm_select_internal_32k_crystal();
#endif /* CONFIG_PM  */

	/* system init */
	sys_init(VBAT_TYPE, INTERNAL_CAP_XTAL24M);

#if CONFIG_PM
	gpio_shutdown(GPIO_ALL);
#endif /* CONFIG_PM */

#if (defined(CONFIG_BT_TLX) || defined(CONFIG_IEEE802154))
	soc_load_rf_parameters_normal();
#endif

	/* clocks init: CCLK, HCLK, PCLK */
	switch (cclk) {
	case CLK_24MHZ:
		PLL_192M_CCLK_24M_HCLK_24M_PCLK_24M_MSPI_48M;
		break;

	case CLK_48MHZ:
		PLL_192M_CCLK_48M_HCLK_48M_PCLK_48M_MSPI_48M;
		break;

	case CLK_96MHZ:
		PLL_192M_CCLK_96M_HCLK_48M_PCLK_48M_MSPI_48M;
		break;
	}

	/* Init Machine Timer source clock: 32 KHz RC */
	clock_32k_init(CLK_32K_RC);
	clock_cal_32k_rc();

	/* Stop 32k watchdog */
	wd_32k_stop();

	return 0;
}

/**
 * @brief Reset the system.
 */
void sys_arch_reboot(int type)
{
	ARG_UNUSED(type);
	// fixme
	// sys_reboot_lib();
}

/**
 * @brief Restore SOC after deep-sleep.
 */
void soc_b9x_restore(void)
{
	unsigned int cclk = DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency);

	/* system init */
	sys_init(VBAT_TYPE, INTERNAL_CAP_XTAL24M);

#if CONFIG_PM
	gpio_shutdown(GPIO_ALL);
#endif /* CONFIG_PM */

#if (defined(CONFIG_BT_TLX) || defined(CONFIG_IEEE802154))
	soc_load_rf_parameters_deep_retention();
#endif

	/* clocks init: CCLK, HCLK, PCLK */
	switch (cclk) {
	case CLK_24MHZ:
		PLL_192M_CCLK_24M_HCLK_24M_PCLK_24M_MSPI_48M;
		break;

	case CLK_48MHZ:
		PLL_192M_CCLK_48M_HCLK_48M_PCLK_48M_MSPI_48M;
		break;

	case CLK_96MHZ:
		PLL_192M_CCLK_96M_HCLK_48M_PCLK_48M_MSPI_48M;
		break;
	}
}

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

/**
 * @brief Check mounted flash size (should be greater than in .dts).
 */
static int soc_b9x_check_flash(void)
{
	static const size_t dts_flash_size = DT_REG_SIZE(DT_CHOSEN(zephyr_flash));
	size_t hw_flash_size = 0;
	flash_capacity_e hw_flash_cap;
	uint32_t mid;

	mid = flash_read_mid();
	hw_flash_cap = (flash_capacity_e)((mid & FLASH_MID_SIZE_MASK) >> FLASH_MID_SIZE_OFFSET);

	/* Enable 4x SPI read and write */
	if (flash_set_4line_read_write(mid) != 1) {
	}

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
	case FLASH_SIZE_16M:
		hw_flash_size = 16 * 1024 * 1024;
		break;
	default:
		break;
	}

	if (hw_flash_size < dts_flash_size) {
		printk("!!! flash error: expected (.dts) %u, actually %u\n",
			dts_flash_size, hw_flash_size);
		extern void abort(void);
		abort();
	}

	return 0;
}

#ifdef CONFIG_BT_TLX
/**
 * @brief bt mac initialization
 */
__attribute__((noinline)) void telink_bt_blc_mac_init(uint8_t *bt_mac)
{
	b9x_bt_blc_mac_init(bt_mac);
}
#endif

SYS_INIT(soc_b9x_init, PRE_KERNEL_1, 0);

// SYS_INIT(soc_b9x_check_flash, POST_KERNEL, 0);
