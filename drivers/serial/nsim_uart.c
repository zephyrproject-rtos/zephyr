/*
 * Copyright (c) 2014-2015, Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <nanokernel.h>
#include <arch/cpu.h>
#include <sections.h>
#include <misc/__assert.h>
#include <stdint.h>
#include <misc/util.h>
#include <string.h>
#include <board.h>
#include <uart.h>

#define NSIM_UART_DATA 0
#define NSIM_UART_STATUS 1

#define DEV_CFG(dev) \
	((struct uart_device_config * const)(dev)->config->config_info)

#define DATA_REG(dev) (DEV_CFG(dev)->regs + NSIM_UART_DATA)
#define STATUS_REG(dev) (DEV_CFG(dev)->regs + NSIM_UART_STATUS)

#define TXEMPTY 0x80 /* Transmit FIFO empty and next character can be sent */

static struct uart_driver_api nsim_uart_driver_api;

/*
 * @brief Initialize fake serial port
 *
 * @param dev UART device struct (of type struct uart_device_config)
 * @param init_info Pointer to initialization information
 */
void nsim_uart_port_init(struct device *dev,
	       const struct uart_init_info * const init_info)
{
	int key = irq_lock();

	DEV_CFG(dev)->regs = init_info->regs;
	irq_unlock(key);
}

/*
 * @brief Output a character to serial port
 *
 * @param dev UART device struct (of type struct uart_device_config)
 * @param c character to output
 */
unsigned char nsim_uart_poll_out(struct device *dev, unsigned char c)
{

	/* wait for transmitter to ready to accept a character */
	while ((_arc_v2_aux_reg_read(STATUS_REG(dev)) & TXEMPTY) == 0)
		;
	_arc_v2_aux_reg_write(DATA_REG(dev), c);
	return c;
}

static int nsim_uart_poll_in(struct device *dev, unsigned char *c)
{
	return -DEV_INVALID_OP;

}

static struct uart_driver_api nsim_uart_driver_api = {
	.poll_out = nsim_uart_poll_out,
	.poll_in = nsim_uart_poll_in,
};
