/*
 * Copyright (c) 2014-2015, Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <nanokernel.h>
#include <arch/cpu.h>
#include <sections.h>
#include <misc/__assert.h>
#include <stdint.h>
#include <misc/util.h>
#include <string.h>
#include <board.h>
#include <drivers/uart.h>

#define NSIM_UART_DATA 0
#define NSIM_UART_STATUS 1

#define DEV_CFG(dev) \
	((struct uart_device_config_t * const)(dev)->config->config_info)

#define DATA_REG(dev) (DEV_CFG(dev)->regs + NSIM_UART_DATA)
#define STATUS_REG(dev) (DEV_CFG(dev)->regs + NSIM_UART_STATUS)

#define TXEMPTY 0x80 /* Transmit FIFO empty and next character can be sent */

static struct uart_driver_api nsim_uart_driver_api;

/*
 * @brief Initialize fake serial port
 *
 * @param dev UART device struct (of type struct uart_device_config_t)
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
 * @param dev UART device struct (of type struct uart_device_config_t)
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
