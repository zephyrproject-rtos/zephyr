/*!
 * Copyright (c) 2025 TQ-Systems GmbH <oss@ew.tq-group.com>
 * SPDX-License-Identifier: Apache 2.0
 * Author: Isaac L. L. Yuki
 */

/*******************************************************************************
 * Includes
 ******************************************************************************/

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/platform/hooks.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include "tm070.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/* Check if devicetree node identifier is defined */
#if DT_NODE_HAS_STATUS(DT_ALIAS(i2c_peripheral_5), okay)
#define I2C_NODE DT_ALIAS(i2c_peripheral_5)
#else
#error "I2C isn't set."
#endif

#define LCD_CTR_NODE DT_ALIAS(lcd_control)
#if !DT_NODE_HAS_STATUS_OKAY(LCD_CTR_NODE)
#error "key1 devicetree alias is not defined"
#endif

#define SN65_ADDRESS 0x2C

#define TEST_MODE 0

/*******************************************************************************
 * Variables
 ******************************************************************************/

static const struct device *i2c_dev = DEVICE_DT_GET(I2C_NODE);

static const struct gpio_dt_spec backlight = GPIO_DT_SPEC_GET_BY_IDX(LCD_CTR_NODE, gpios, 0);
static const struct gpio_dt_spec lcd_rst = GPIO_DT_SPEC_GET_BY_IDX(LCD_CTR_NODE, gpios, 1);
static const struct gpio_dt_spec pwr_enb = GPIO_DT_SPEC_GET_BY_IDX(LCD_CTR_NODE, gpios, 2);
static const struct gpio_dt_spec mipi_select = GPIO_DT_SPEC_GET_BY_IDX(LCD_CTR_NODE, gpios, 3);

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

static uint32_t SN65DSI83_WriteCSR(uint8_t const addr, uint8_t const data)
{
	return (uint32_t)i2c_burst_write(i2c_dev, SN65_ADDRESS, addr, &data, 1);
}

static uint32_t SN65DSI83_ReadCSR(uint8_t const addr, uint8_t *data)
{
	return (uint32_t)i2c_burst_read(i2c_dev, SN65_ADDRESS, addr, data, 1);
}

static uint32_t BOARD_ReadBridgeStatus(void)
{
	uint32_t status;
	uint8_t data = 0x00u;

	status = (uint32_t)SN65DSI83_ReadCSR(0xE5u, &data);
	if (status == kStatus_Success) {
		if (data != 0x00u) {
			printk("Error in SN65DSI83 status 0xE5: 0x%02X\r\n", data);
			status = (uint32_t)SN65DSI83_WriteCSR(0xE5u, 0xFFu);
			if (status != kStatus_Success) {
				printk("failed to clear SN65DSI83 status\r\n");
			}
		}
	} else {
		printk("failed to read SN65DSI83 status\r\n");
	}

	return status;
}

/*******************************************************************************
 * Code
 ******************************************************************************/

uint32_t BOARD_PrepareDisplayController(void)
{
	uint32_t status;

	/*select LVDS*/

	status = gpio_pin_configure_dt(&mipi_select, GPIO_OUTPUT_ACTIVE);
	if (status < 0) {
		printk("GPIO_0 output failed to configure.\n");
		return -1;
	}

	status = gpio_pin_configure_dt(&pwr_enb, GPIO_OUTPUT_ACTIVE);
	if (status < 0) {
		printk("GPIO_0 output failed to configure.\n");
		return -1;
	}

	status = gpio_pin_configure_dt(&lcd_rst, GPIO_OUTPUT_ACTIVE);
	if (status < 0) {
		printk("GPIO_0 output failed to configure.\n");
		return -1;
	}

	status = gpio_pin_configure_dt(&backlight, GPIO_OUTPUT_ACTIVE);
	if (status < 0) {
		printk("GPIO_0 output failed to configure.\n");
		return -1;
	}

	// init seq 2: enable LP11 on inactive DSI lanes
	DSI_HOST_DPHY_INTFC->AUTO_PD_EN = DSI_HOST_NXP_FDSOI28_DPHY_INTFC_AUTO_PD_EN_AUTO_PD_EN(0u);
	DSI_HOST->CFG_NONCONTINUOUS_CLK = DSI_HOST_CFG_NONCONTINUOUS_CLK_CLK_MODE(0U);

	// wait for it
	k_sleep(K_MSEC(10));

	status = SN65DSI83_WriteCSR(0x0Au, 0x05u);
	status = SN65DSI83_WriteCSR(0x0Bu, 0x28u); // div= 6
	status = SN65DSI83_WriteCSR(0x10u, 0x30u); // 2 lanes
	status = SN65DSI83_WriteCSR(0x12u, 0x58u); // 88u == 440 - 445 MHz
	status = SN65DSI83_WriteCSR(0x18u, 0x7Au);

	status = SN65DSI83_WriteCSR(0x20u, 0x00u); // CHA_ACTIVE_LINE_LENGTH_LOW
	status = SN65DSI83_WriteCSR(0x21u, 0x05u); // CHA_ACTIVE_LINE_LENGTH_HIGH
	status = SN65DSI83_WriteCSR(0x28u, 0x21u); // CHA_SYNC_DELAY_LOW
	status = SN65DSI83_WriteCSR(0x29u, 0x00u); // CHA_SYNC_DELAY_HIGH

#if (defined(TEST_MODE) && (TEST_MODE == 1))
	status = SN65DSI83_WriteCSR(0x24u, 0x20u); // CHA_VERTICAL_DISPLAY_SIZE_LOW
	status = SN65DSI83_WriteCSR(0x25u, 0x03u); // CHA_VERTICAL_DISPLAY_SIZE_HIGH
	status = SN65DSI83_WriteCSR(0x2Cu, 0x01u); // CHA_HSYNC_PULSE_WIDTH_LOW
	status = SN65DSI83_WriteCSR(0x2Du, 0x00u); // CHA_HSYNC_PULSE_WIDTH_HIGH
	status = SN65DSI83_WriteCSR(0x30u, 0x01u); // CHA_VSYNC_PULSE_WIDTH_LOW
	status = SN65DSI83_WriteCSR(0x31u, 0x00u); // CHA_VSYNC_PULSE_WIDTH_HIGH
	status = SN65DSI83_WriteCSR(0x34u, 0x05u); // CHA_HORIZONTAL_BACK_PORCH
	status = SN65DSI83_WriteCSR(0x36u, 0x02u); // CHA_VERTICAL_BACK_PORCH
	status = SN65DSI83_WriteCSR(0x38u, 0x40u); // CHA_HORIZONTAL_FRONT_PORCH
	status = SN65DSI83_WriteCSR(0x3Au, 0x02u); // CHA_VERTICAL_FRONT_PORCH
	status = SN65DSI83_WriteCSR(0x3Cu, 0x10u); // test mode
#endif

	k_sleep(K_MSEC(3));
	// init seq 6: set PLL_EN bit in CSR
	SN65DSI83_WriteCSR(0x0Du, 0x01u);
	// wait for it
	k_sleep(K_MSEC(10));
	// init seq 7: set SOFT_RESET bit in CSR
	SN65DSI83_WriteCSR(0x09u, 0x01u);
	// wait for it
	k_sleep(K_MSEC(10));
	// init seq 8: change DSI data lanes to HS and start video stream
	// init seq 9: overstept
	// init seq 10: clear all errors in CSR
	status = (status_t)SN65DSI83_WriteCSR(0xE5u, 0xFFu);
	// wait for it
	k_sleep(K_MSEC(1));
	// init seq 11: verify no error in CSR
	status = BOARD_ReadBridgeStatus();

	return status;
}
