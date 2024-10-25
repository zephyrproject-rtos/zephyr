/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <assert.h>
#include <string.h>

#include "soc_flash_init.h"

#include <esp_err.h>
#include <esp_log.h>
#include <esp_rom_gpio.h>
#include <esp_rom_efuse.h>
#include "esp32/rom/spi_flash.h"
#include "esp32/rom/cache.h"
#include "soc/gpio_periph.h"
#include "soc/efuse_reg.h"
#include "soc/spi_reg.h"
#include "soc/soc_caps.h"
#include "soc/soc_pins.h"
#include "soc/chip_revision.h"
#include "hal/efuse_hal.h"
#include "hal/gpio_hal.h"
#include "flash_qio_mode.h"
#include "bootloader_common.h"
#include "bootloader_flash_config.h"
#include "bootloader_flash_priv.h"
#include "bootloader_init.h"

#include "hal/efuse_ll.h"

#define TAG "flash_init"
extern esp_image_header_t bootloader_image_hdr;

#define FLASH_CLK_IO   SPI_CLK_GPIO_NUM
#define FLASH_CS_IO    SPI_CS0_GPIO_NUM
#define FLASH_SPIQ_IO  SPI_Q_GPIO_NUM
#define FLASH_SPID_IO  SPI_D_GPIO_NUM
#define FLASH_SPIWP_IO SPI_WP_GPIO_NUM
#define FLASH_SPIHD_IO SPI_HD_GPIO_NUM

uint32_t get_chip_ver_pkg(void)
{
	return efuse_ll_get_chip_ver_pkg();
}

void flash_update_id(void)
{
	g_rom_flashchip.device_id = bootloader_read_flash_id();
}

void flash_update_size(uint32_t size)
{
	g_rom_flashchip.chip_size = size;
}

void flash_cs_timing_config(void)
{
	SET_PERI_REG_MASK(SPI_USER_REG(0), SPI_CS_HOLD_M | SPI_CS_SETUP_M);
	SET_PERI_REG_BITS(SPI_CTRL2_REG(0), SPI_HOLD_TIME_V, 1, SPI_HOLD_TIME_S);
	SET_PERI_REG_BITS(SPI_CTRL2_REG(0), SPI_SETUP_TIME_V, 0, SPI_SETUP_TIME_S);
	SET_PERI_REG_MASK(SPI_USER_REG(1), SPI_CS_HOLD_M | SPI_CS_SETUP_M);
	SET_PERI_REG_BITS(SPI_CTRL2_REG(1), SPI_HOLD_TIME_V, 1, SPI_HOLD_TIME_S);
	SET_PERI_REG_BITS(SPI_CTRL2_REG(1), SPI_SETUP_TIME_V, 0, SPI_SETUP_TIME_S);
}

void flash_clock_config(const esp_image_header_t *pfhdr)
{
	uint32_t spi_clk_div = 0;

	switch (pfhdr->spi_speed) {
	case ESP_IMAGE_SPI_SPEED_DIV_1:
		spi_clk_div = 1;
		break;
	case ESP_IMAGE_SPI_SPEED_DIV_2:
		spi_clk_div = 2;
		break;
	case ESP_IMAGE_SPI_SPEED_DIV_3:
		spi_clk_div = 3;
		break;
	case ESP_IMAGE_SPI_SPEED_DIV_4:
		spi_clk_div = 4;
		break;
	default:
		break;
	}
	esp_rom_spiflash_config_clk(spi_clk_div, 0);
	esp_rom_spiflash_config_clk(spi_clk_div, 1);
}

void flash_gpio_config(const esp_image_header_t *pfhdr)
{
	uint32_t drv = 2;

	if (pfhdr->spi_speed == ESP_IMAGE_SPI_SPEED_DIV_1) {
		drv = 3;
	}

	uint32_t pkg_ver = get_chip_ver_pkg();

	if (pkg_ver == EFUSE_RD_CHIP_VER_PKG_ESP32D2WDQ5 ||
	    pkg_ver == EFUSE_RD_CHIP_VER_PKG_ESP32PICOD2 ||
	    pkg_ver == EFUSE_RD_CHIP_VER_PKG_ESP32PICOD4 ||
	    pkg_ver == EFUSE_RD_CHIP_VER_PKG_ESP32PICOV302) {
		/* For ESP32D2WD or ESP32-PICO series,the SPI pins are already configured flash
		 * clock signal should come from IO MUX.
		 */
		gpio_hal_iomux_func_sel(PERIPHS_IO_MUX_SD_CLK_U, FUNC_SD_CLK_SPICLK);
		SET_PERI_REG_BITS(PERIPHS_IO_MUX_SD_CLK_U, FUN_DRV, drv, FUN_DRV_S);
	} else {
		const uint32_t spiconfig = esp_rom_efuse_get_flash_gpio_info();

		if (spiconfig == ESP_ROM_EFUSE_FLASH_DEFAULT_SPI) {
			esp_rom_gpio_connect_out_signal(SPI_IOMUX_PIN_NUM_CS, SPICS0_OUT_IDX, 0, 0);
			esp_rom_gpio_connect_out_signal(SPI_IOMUX_PIN_NUM_MISO, SPIQ_OUT_IDX, 0, 0);
			esp_rom_gpio_connect_in_signal(SPI_IOMUX_PIN_NUM_MISO, SPIQ_IN_IDX, 0);
			esp_rom_gpio_connect_out_signal(SPI_IOMUX_PIN_NUM_MOSI, SPID_OUT_IDX, 0, 0);
			esp_rom_gpio_connect_in_signal(SPI_IOMUX_PIN_NUM_MOSI, SPID_IN_IDX, 0);
			esp_rom_gpio_connect_out_signal(SPI_IOMUX_PIN_NUM_WP, SPIWP_OUT_IDX, 0, 0);
			esp_rom_gpio_connect_in_signal(SPI_IOMUX_PIN_NUM_WP, SPIWP_IN_IDX, 0);
			esp_rom_gpio_connect_out_signal(SPI_IOMUX_PIN_NUM_HD, SPIHD_OUT_IDX, 0, 0);
			esp_rom_gpio_connect_in_signal(SPI_IOMUX_PIN_NUM_HD, SPIHD_IN_IDX, 0);
			/* select pin function gpio */
			gpio_hal_iomux_func_sel(PERIPHS_IO_MUX_SD_DATA0_U, PIN_FUNC_GPIO);
			gpio_hal_iomux_func_sel(PERIPHS_IO_MUX_SD_DATA1_U, PIN_FUNC_GPIO);
			gpio_hal_iomux_func_sel(PERIPHS_IO_MUX_SD_DATA2_U, PIN_FUNC_GPIO);
			gpio_hal_iomux_func_sel(PERIPHS_IO_MUX_SD_DATA3_U, PIN_FUNC_GPIO);
			gpio_hal_iomux_func_sel(PERIPHS_IO_MUX_SD_CMD_U, PIN_FUNC_GPIO);
			/* flash clock signal should come from IO MUX. Set drive ability for clock
			 */
			gpio_hal_iomux_func_sel(PERIPHS_IO_MUX_SD_CLK_U, FUNC_SD_CLK_SPICLK);
			SET_PERI_REG_BITS(PERIPHS_IO_MUX_SD_CLK_U, FUN_DRV, drv, FUN_DRV_S);

			uint32_t flash_id = g_rom_flashchip.device_id;

			if (flash_id == FLASH_ID_GD25LQ32C) {
				/* Set drive ability for 1.8v flash in 80Mhz. */
				SET_PERI_REG_BITS(PERIPHS_IO_MUX_SD_DATA0_U, FUN_DRV, 3, FUN_DRV_S);
				SET_PERI_REG_BITS(PERIPHS_IO_MUX_SD_DATA1_U, FUN_DRV, 3, FUN_DRV_S);
				SET_PERI_REG_BITS(PERIPHS_IO_MUX_SD_DATA2_U, FUN_DRV, 3, FUN_DRV_S);
				SET_PERI_REG_BITS(PERIPHS_IO_MUX_SD_DATA3_U, FUN_DRV, 3, FUN_DRV_S);
				SET_PERI_REG_BITS(PERIPHS_IO_MUX_SD_CMD_U, FUN_DRV, 3, FUN_DRV_S);
				SET_PERI_REG_BITS(PERIPHS_IO_MUX_SD_CLK_U, FUN_DRV, 3, FUN_DRV_S);
			}
		}
	}
}

void flash_dummy_config(const esp_image_header_t *pfhdr)
{
	int spi_cache_dummy = 0;
	uint32_t modebit = READ_PERI_REG(SPI_CTRL_REG(0));

	if (modebit & SPI_FASTRD_MODE) {
		if (modebit & SPI_FREAD_QIO) {
			/* SPI mode is QIO */
			spi_cache_dummy = SPI0_R_QIO_DUMMY_CYCLELEN;
		} else if (modebit & SPI_FREAD_DIO) {
			/* SPI mode is DIO */
			spi_cache_dummy = SPI0_R_DIO_DUMMY_CYCLELEN;
			SET_PERI_REG_BITS(SPI_USER1_REG(0), SPI_USR_ADDR_BITLEN_V,
					  SPI0_R_DIO_ADDR_BITSLEN, SPI_USR_ADDR_BITLEN_S);
		} else if (modebit & (SPI_FREAD_QUAD | SPI_FREAD_DUAL)) {
			/* SPI mode is QOUT or DIO */
			spi_cache_dummy = SPI0_R_FAST_DUMMY_CYCLELEN;
		}
	}

	switch (pfhdr->spi_speed) {
	case ESP_IMAGE_SPI_SPEED_DIV_1:
		g_rom_spiflash_dummy_len_plus[0] = ESP_ROM_SPIFLASH_DUMMY_LEN_PLUS_80M;
		g_rom_spiflash_dummy_len_plus[1] = ESP_ROM_SPIFLASH_DUMMY_LEN_PLUS_80M;
		break;
	case ESP_IMAGE_SPI_SPEED_DIV_2:
		g_rom_spiflash_dummy_len_plus[0] = ESP_ROM_SPIFLASH_DUMMY_LEN_PLUS_40M;
		g_rom_spiflash_dummy_len_plus[1] = ESP_ROM_SPIFLASH_DUMMY_LEN_PLUS_40M;
		break;
	case ESP_IMAGE_SPI_SPEED_DIV_3:
	case ESP_IMAGE_SPI_SPEED_DIV_4:
		g_rom_spiflash_dummy_len_plus[0] = ESP_ROM_SPIFLASH_DUMMY_LEN_PLUS_20M;
		g_rom_spiflash_dummy_len_plus[1] = ESP_ROM_SPIFLASH_DUMMY_LEN_PLUS_20M;
		break;
	default:
		break;
	}

	SET_PERI_REG_BITS(SPI_USER1_REG(0), SPI_USR_DUMMY_CYCLELEN_V,
			  spi_cache_dummy + g_rom_spiflash_dummy_len_plus[0],
			  SPI_USR_DUMMY_CYCLELEN_S);
}

/* ESP32-D2WD & ESP32-PICO-D4 has this GPIO wired to WP pin of flash  */
#define ESP32_D2WD_WP_GPIO 7
/* ESP32-PICO-V3* use this GPIO for WP pin of flash */
#define ESP32_PICO_V3_GPIO 18

int flash_get_wp_pin(void)
{
	switch (get_chip_ver_pkg()) {
	case EFUSE_RD_CHIP_VER_PKG_ESP32U4WDH:
	case EFUSE_RD_CHIP_VER_PKG_ESP32D2WDQ5:
		return ESP32_D2WD_WP_GPIO;
	case EFUSE_RD_CHIP_VER_PKG_ESP32PICOD4:
		/* Same package IDs are used for ESP32-PICO-V3 and ESP32-PICO-D4, silicon version
		 * differentiates
		 */
		return !ESP_CHIP_REV_ABOVE(efuse_hal_chip_revision(), 300) ? ESP32_D2WD_WP_GPIO
									   : ESP32_PICO_V3_GPIO;
	case EFUSE_RD_CHIP_VER_PKG_ESP32PICOV302:
		return ESP32_PICO_V3_GPIO;
	default:
		return SPI_WP_GPIO_NUM;
	}
}

static void update_flash_config(const esp_image_header_t *bootloader_hdr)
{
	volatile uint32_t size;

	switch (bootloader_hdr->spi_size) {
	case ESP_IMAGE_FLASH_SIZE_1MB:
		size = 1;
		break;
	case ESP_IMAGE_FLASH_SIZE_2MB:
		size = 2;
		break;
	case ESP_IMAGE_FLASH_SIZE_4MB:
		size = 4;
		break;
	case ESP_IMAGE_FLASH_SIZE_8MB:
		size = 8;
		break;
	case ESP_IMAGE_FLASH_SIZE_16MB:
		size = 16;
		break;
	default:
		size = 2;
	}
	Cache_Read_Disable(0);
	/* Set flash chip size */
	esp_rom_spiflash_config_param(g_rom_flashchip.device_id, size * 0x100000, 0x10000, 0x1000,
				      0x100, 0xffff);
	Cache_Flush(0);
	Cache_Read_Enable(0);
}

static void print_flash_info(const esp_image_header_t *bootloader_hdr)
{
	ESP_EARLY_LOGD(TAG, "magic %02x", bootloader_hdr->magic);
	ESP_EARLY_LOGD(TAG, "segments %02x", bootloader_hdr->segment_count);
	ESP_EARLY_LOGD(TAG, "spi_mode %02x", bootloader_hdr->spi_mode);
	ESP_EARLY_LOGD(TAG, "spi_speed %02x", bootloader_hdr->spi_speed);
	ESP_EARLY_LOGD(TAG, "spi_size %02x", bootloader_hdr->spi_size);

	const char *str;

	switch (bootloader_hdr->spi_speed) {
	case ESP_IMAGE_SPI_SPEED_DIV_2:
		str = "40MHz";
		break;
	case ESP_IMAGE_SPI_SPEED_DIV_3:
		str = "26.7MHz";
		break;
	case ESP_IMAGE_SPI_SPEED_DIV_4:
		str = "20MHz";
		break;
	case ESP_IMAGE_SPI_SPEED_DIV_1:
		str = "80MHz";
		break;
	default:
		str = "20MHz";
		break;
	}
	ESP_EARLY_LOGI(TAG, "SPI Speed      : %s", str);

	/* SPI mode could have been set to QIO during boot already,
	 * so test the SPI registers not the flash header
	 */
	uint32_t spi_ctrl = REG_READ(SPI_CTRL_REG(0));

	if (spi_ctrl & SPI_FREAD_QIO) {
		str = "QIO";
	} else if (spi_ctrl & SPI_FREAD_QUAD) {
		str = "QOUT";
	} else if (spi_ctrl & SPI_FREAD_DIO) {
		str = "DIO";
	} else if (spi_ctrl & SPI_FREAD_DUAL) {
		str = "DOUT";
	} else if (spi_ctrl & SPI_FASTRD_MODE) {
		str = "FAST READ";
	} else {
		str = "SLOW READ";
	}
	ESP_EARLY_LOGI(TAG, "SPI Mode       : %s", str);

	switch (bootloader_hdr->spi_size) {
	case ESP_IMAGE_FLASH_SIZE_1MB:
		str = "1MB";
		break;
	case ESP_IMAGE_FLASH_SIZE_2MB:
		str = "2MB";
		break;
	case ESP_IMAGE_FLASH_SIZE_4MB:
		str = "4MB";
		break;
	case ESP_IMAGE_FLASH_SIZE_8MB:
		str = "8MB";
		break;
	case ESP_IMAGE_FLASH_SIZE_16MB:
		str = "16MB";
		break;
	default:
		str = "2MB";
		break;
	}
	ESP_EARLY_LOGI(TAG, "SPI Flash Size : %s", str);
}

static void init_flash_configure(void)
{
	flash_gpio_config(&bootloader_image_hdr);
	flash_dummy_config(&bootloader_image_hdr);
	flash_cs_timing_config();
}

esp_err_t init_spi_flash(void)
{
	init_flash_configure();
#ifndef CONFIG_SPI_FLASH_ROM_DRIVER_PATCH
	const uint32_t spiconfig = esp_rom_efuse_get_flash_gpio_info();

	if (spiconfig != ESP_ROM_EFUSE_FLASH_DEFAULT_SPI &&
	    spiconfig != ESP_ROM_EFUSE_FLASH_DEFAULT_HSPI) {
		ESP_EARLY_LOGE(TAG, "SPI flash pins are overridden. Enable "
				    "CONFIG_SPI_FLASH_ROM_DRIVER_PATCH in menuconfig");
		return ESP_FAIL;
	}
#endif

	bootloader_flash_unlock();

#if CONFIG_ESPTOOLPY_FLASHMODE_QIO || CONFIG_ESPTOOLPY_FLASHMODE_QOUT
	bootloader_enable_qio_mode();
#endif

	print_flash_info(&bootloader_image_hdr);
	update_flash_config(&bootloader_image_hdr);
	/* ensure the flash is write-protected */
	bootloader_enable_wp();
	return ESP_OK;
}
