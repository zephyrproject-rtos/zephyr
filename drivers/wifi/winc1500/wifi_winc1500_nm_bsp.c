/**
 * Copyright (c) 2017 IpTronix
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmel_winc1500

#include "wifi_winc1500_nm_bsp_internal.h"

#include <bsp/include/nm_bsp.h>
#include <common/include/nm_common.h>

#include "wifi_winc1500_config.h"

const struct winc1500_cfg winc1500_config = {
	.spi = SPI_DT_SPEC_INST_GET(0, SPI_WORD_SET(8) | SPI_TRANSFER_MSB, 0),
	.chip_en_gpio = GPIO_DT_SPEC_INST_GET(0, enable_gpios),
	.irq_gpio = GPIO_DT_SPEC_INST_GET(0, irq_gpios),
	.reset_gpio = GPIO_DT_SPEC_INST_GET(0, reset_gpios),
};

struct winc1500_device winc1500;

void (*isr_function)(void);

static inline void chip_isr(const struct device *port,
			    struct gpio_callback *cb,
			    gpio_port_pins_t pins)
{
	if (isr_function) {
		isr_function();
	}
}

int8_t nm_bsp_init(void)
{
	isr_function = NULL;

	/* Perform chip reset. */
	nm_bsp_reset();

	return 0;
}

int8_t nm_bsp_deinit(void)
{
	/* TODO */
	return 0;
}

void nm_bsp_reset(void)
{
	gpio_pin_set_dt(&winc1500_config.chip_en_gpio, 0);
	gpio_pin_set_dt(&winc1500_config.reset_gpio, 0);
	nm_bsp_sleep(100);

	gpio_pin_set_dt(&winc1500_config.chip_en_gpio, 1);
	nm_bsp_sleep(10);

	gpio_pin_set_dt(&winc1500_config.reset_gpio, 1);
	nm_bsp_sleep(10);
}

void nm_bsp_sleep(uint32 u32TimeMsec)
{
	k_busy_wait(u32TimeMsec * MSEC_PER_SEC);
}

void nm_bsp_register_isr(void (*isr_fun)(void))
{
	isr_function = isr_fun;

	gpio_init_callback(&winc1500.gpio_cb,
			   chip_isr,
			   BIT(winc1500_config.irq_gpio.pin));

	(void)gpio_add_callback(winc1500_config.irq_gpio.port, &winc1500.gpio_cb);
}

void nm_bsp_interrupt_ctrl(uint8_t enable)
{
	gpio_pin_interrupt_configure_dt(&winc1500_config.irq_gpio,
		enable ? GPIO_INT_EDGE_TO_ACTIVE : GPIO_INT_DISABLE);
}
