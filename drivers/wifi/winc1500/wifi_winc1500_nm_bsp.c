/**
 * Copyright (c) 2017 IpTronix
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "wifi_winc1500_nm_bsp_internal.h"

#include <bsp/include/nm_bsp.h>
#include <common/include/nm_common.h>

#include "wifi_winc1500_config.h"

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
	gpio_pin_set_raw(winc1500.gpios[WINC1500_GPIO_IDX_CHIP_EN].dev,
			 winc1500.gpios[WINC1500_GPIO_IDX_CHIP_EN].pin, 0);
	gpio_pin_set_raw(winc1500.gpios[WINC1500_GPIO_IDX_RESET_N].dev,
			 winc1500.gpios[WINC1500_GPIO_IDX_RESET_N].pin, 0);
	nm_bsp_sleep(100);

	gpio_pin_set_raw(winc1500.gpios[WINC1500_GPIO_IDX_CHIP_EN].dev,
			 winc1500.gpios[WINC1500_GPIO_IDX_CHIP_EN].pin, 1);
	nm_bsp_sleep(10);

	gpio_pin_set_raw(winc1500.gpios[WINC1500_GPIO_IDX_RESET_N].dev,
			 winc1500.gpios[WINC1500_GPIO_IDX_RESET_N].pin, 1);
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
			   BIT(winc1500.gpios[WINC1500_GPIO_IDX_IRQN].pin));

	gpio_add_callback(winc1500.gpios[WINC1500_GPIO_IDX_IRQN].dev,
			  &winc1500.gpio_cb);
}

void nm_bsp_interrupt_ctrl(uint8_t enable)
{
	gpio_pin_interrupt_configure(
		winc1500.gpios[WINC1500_GPIO_IDX_IRQN].dev,
		winc1500.gpios[WINC1500_GPIO_IDX_IRQN].pin,
		enable ? GPIO_INT_EDGE_TO_ACTIVE : GPIO_INT_DISABLE);
}
