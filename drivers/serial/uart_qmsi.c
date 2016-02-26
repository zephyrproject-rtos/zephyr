/*
 * Copyright (c) 2016 Intel Corporation.
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

#include <device.h>
#include <uart.h>

#include "qm_uart.h"
#include "qm_scss.h"

/* Convenient macro to get the controller instance. */
#define GET_CONTROLLER_INSTANCE(dev) \
	(((struct uart_qmsi_config_info *)dev->config->config_info)->instance)

struct uart_qmsi_config_info {
	qm_uart_t instance;
	clk_periph_t clock_gate;
};

static int uart_qmsi_init(struct device *dev);

#ifdef CONFIG_UART_QMSI_0

static struct uart_qmsi_config_info config_info_0 = {
	.instance = QM_UART_0,
	.clock_gate = CLK_PERIPH_UARTA_REGISTER,
};

DEVICE_INIT(uart_0, CONFIG_UART_QMSI_0_NAME, uart_qmsi_init, NULL,
	    &config_info_0, PRIMARY, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
#endif /* CONFIG_UART_QMSI_0 */

#ifdef CONFIG_UART_QMSI_1
static struct uart_qmsi_config_info config_info_1 = {
	.instance = QM_UART_1,
	.clock_gate = CLK_PERIPH_UARTB_REGISTER,
};

DEVICE_INIT(uart_1, CONFIG_UART_QMSI_1_NAME, uart_qmsi_init, NULL,
	    &config_info_1, PRIMARY, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
#endif /* CONFIG_UART_QMSI_1 */

static int uart_qmsi_poll_in(struct device *dev, unsigned char *data)
{
	qm_uart_t instance = GET_CONTROLLER_INSTANCE(dev);
	qm_uart_status_t status = qm_uart_get_status(instance);

	/* In order to check if there is any data to read from UART
	 * controller we should check if the QM_UART_RX_BUSY bit from
	 * 'status' is not set. This bit is set only if there is any
	 * pending character to read.
	 */
	if (!(status & QM_UART_RX_BUSY))
		return -1;

	qm_uart_read(instance, data);
	return 0;
}

static unsigned char uart_qmsi_poll_out(struct device *dev,
					unsigned char data)
{
	qm_uart_t instance = GET_CONTROLLER_INSTANCE(dev);

	qm_uart_write(instance, data);
	return data;
}

static int uart_qmsi_err_check(struct device *dev)
{
	qm_uart_t instance = GET_CONTROLLER_INSTANCE(dev);

	/* QMSI and Zephyr use the same bits to represent UART errors
	 * so we don't need to translate each error bit from QMSI API
	 * to Zephyr API.
	 */
	return qm_uart_get_status(instance) & QM_UART_LSR_ERROR_BITS;
}

static struct uart_driver_api api = {
	.poll_in = uart_qmsi_poll_in,
	.poll_out = uart_qmsi_poll_out,
	.err_check = uart_qmsi_err_check,
};

static int uart_qmsi_init(struct device *dev)
{
	struct uart_qmsi_config_info *config = dev->config->config_info;
	qm_uart_config_t cfg;

	cfg.line_control = QM_UART_LC_8N1;
	cfg.baud_divisor = QM_UART_CFG_BAUD_DL_PACK(0, 17, 6); /* 115200 */
	cfg.hw_fc = false;
	cfg.int_en = false;

	clk_periph_enable(config->clock_gate);

	qm_uart_set_config(config->instance, &cfg);

	dev->driver_api = &api;

	return DEV_OK;
}
