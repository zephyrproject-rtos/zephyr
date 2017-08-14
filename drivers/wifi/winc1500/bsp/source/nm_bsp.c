/**
 *
 * \file
 *
 * \brief This module contains SAMD21 BSP APIs implementation.
 *
 * Copyright (c) 2016 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 */

#include <board.h>
#include <gpio.h>

#include "bsp/include/nm_bsp.h"
#include "common/include/nm_common.h"
#include "conf_winc.h"

/**
 *
 */
struct winc1500_device winc1500;

void (*isr_function)(void);

static inline void chip_isr(struct device *port,
			    struct gpio_callback *cb,
			    uint32_t pins)
{
	if (isr_function) {
		isr_function();
	}
}

/*
 *	@fn		nm_bsp_init
 *	@brief	Initialize BSP
 *	@return	0 in case of success and -1 in case of failure
 */
s8_t nm_bsp_init(void)
{
	isr_function = NULL;

	/* Initialize chip IOs. */
	winc1500.gpios = winc1500_configure_gpios();

	winc1500.spi_dev = device_get_binding(CONFIG_WINC1500_SPI_DRV_NAME);
	if (!winc1500.spi_dev) {
		return -1;
	}

	/* Perform chip reset. */
	nm_bsp_reset();

	return 0;
}

/**
 *	@fn		nm_bsp_deinit
 *	@brief	De-iInitialize BSP
 *	@return	0 in case of success and -1 in case of failure
 */
s8_t nm_bsp_deinit(void)
{
	/* TODO */
	return 0;
}

/**
 *	@fn		nm_bsp_reset
 *	@brief	Reset NMC1500 SoC by setting CHIP_EN and RESET_N signals low,
 *           CHIP_EN high then RESET_N high
 */
void nm_bsp_reset(void)
{
	gpio_pin_write(winc1500.gpios[WINC1500_GPIO_IDX_CHIP_EN],
		       CONFIG_WINC1500_GPIO_CHIP_EN, 0);
	gpio_pin_write(winc1500.gpios[WINC1500_GPIO_IDX_RESET_N],
		       CONFIG_WINC1500_GPIO_RESET_N, 0);
	nm_bsp_sleep(100);
	gpio_pin_write(winc1500.gpios[WINC1500_GPIO_IDX_CHIP_EN],
		       CONFIG_WINC1500_GPIO_CHIP_EN, 1);
	nm_bsp_sleep(10);
	gpio_pin_write(winc1500.gpios[WINC1500_GPIO_IDX_RESET_N],
		       CONFIG_WINC1500_GPIO_RESET_N, 1);
	nm_bsp_sleep(10);
}

/*
 *	@fn		nm_bsp_sleep
 *	@brief	Sleep in units of mSec
 *	@param[IN]	time_msec
 *				Time in milliseconds
 */
void nm_bsp_sleep(u32_t time_msec)
{
	k_busy_wait(time_msec * MSEC_PER_SEC);
}

/*
 *	@fn		nm_bsp_register_isr
 *	@brief	Register interrupt service routine
 *	@param[IN]	isr_fun
 *				Pointer to ISR handler
 */
void nm_bsp_register_isr(void (*isr_fun)(void))
{

	isr_function = isr_fun;

	gpio_init_callback(&winc1500.gpio_cb,
			   chip_isr,
			   BIT(CONFIG_WINC1500_GPIO_IRQN));

	gpio_add_callback(winc1500.gpios[WINC1500_GPIO_IDX_IRQN],
			  &winc1500.gpio_cb);
}

/*
 *	@fn		nm_bsp_interrupt_ctrl
 *	@brief	Enable/Disable interrupts
 *	@param[IN]	enable
 *				'0' disable interrupts. '1' enable interrupts
 */
void nm_bsp_interrupt_ctrl(u8_t enable)
{
	if (enable) {
		gpio_pin_enable_callback(winc1500.gpios[WINC1500_GPIO_IDX_IRQN],
					 CONFIG_WINC1500_GPIO_IRQN);
	} else {
		gpio_pin_disable_callback(winc1500.gpios[WINC1500_GPIO_IDX_IRQN],
					  CONFIG_WINC1500_GPIO_IRQN);
	}
}
