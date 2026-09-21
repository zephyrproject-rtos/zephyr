/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 * Copyright (c) 2020 Linaro Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Puts the mx25r8035f external flash into Deep Power-Down (DPD) at boot by
 * bit-banging its SPI command over raw GPIO.
 * Boards/apps that need the flash for real
 * must re-enable the external flash node.
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>

#define GPIO_PORT DT_NODELABEL(gpio0)

#define EXT_FLASH_SPI_CLK  DT_PROP_BY_IDX(DT_NODELABEL(spi0_sck_default), pinmux, 0)
#define EXT_FLASH_SPI_MOSI DT_PROP_BY_IDX(DT_NODELABEL(spi0_mosi_default), pinmux, 0)
#define EXT_FLASH_SPI_MISO DT_PROP_BY_IDX(DT_NODELABEL(spi0_miso_default), pinmux, 0)
#define EXT_FLASH_SPI_CS   DT_PROP_BY_IDX(DT_NODELABEL(spi0_cs_default), pinmux, 0)

#define EXT_FLASH_CMD_DEEP_POWER_DOWN 0xB9

static void ext_flash_send_byte(const struct device *gpio, uint8_t byte)
{
	gpio_pin_set(gpio, EXT_FLASH_SPI_CS, 0);

	for (int i = 0; i < 8; i++) {
		gpio_pin_set(gpio, EXT_FLASH_SPI_CLK, 0);
		gpio_pin_set(gpio, EXT_FLASH_SPI_MOSI, (byte >> (7 - i)) & 0x01);
		gpio_pin_set(gpio, EXT_FLASH_SPI_CLK, 1);
		k_busy_wait(1);
	}

	gpio_pin_set(gpio, EXT_FLASH_SPI_CLK, 0);
	gpio_pin_set(gpio, EXT_FLASH_SPI_CS, 1);

	/* Keep CS high at least 40 us before the next command. */
	k_busy_wait(44);
}

static void ext_flash_wake_up(const struct device *gpio)
{
	/* Toggling CS for >= 20 ns then waiting >= 35 us wakes the flash. */
	gpio_pin_set(gpio, EXT_FLASH_SPI_CS, 0);
	k_busy_wait(1);
	gpio_pin_set(gpio, EXT_FLASH_SPI_CS, 1);
	k_busy_wait(35);
}

static int ext_flash_dpd_init(void)
{
	const struct device *gpio = DEVICE_DT_GET(GPIO_PORT);

	if (!device_is_ready(gpio)) {
		return -ENODEV;
	}

	gpio_pin_configure(gpio, EXT_FLASH_SPI_CS, GPIO_OUTPUT_ACTIVE);
	gpio_pin_configure(gpio, EXT_FLASH_SPI_CLK, GPIO_OUTPUT);
	gpio_pin_configure(gpio, EXT_FLASH_SPI_MOSI, GPIO_OUTPUT);
	gpio_pin_configure(gpio, EXT_FLASH_SPI_MISO, GPIO_INPUT | GPIO_PULL_DOWN);

	/*
	 * The flash's power-on state is unknown (it may already be asleep),
	 * so wake it up before sending the shutdown command to guarantee the
	 * command is actually received.
	 */
	ext_flash_wake_up(gpio);
	ext_flash_send_byte(gpio, EXT_FLASH_CMD_DEEP_POWER_DOWN);

	return 0;
}

SYS_INIT(ext_flash_dpd_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
