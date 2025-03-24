/*
 * Copyright (c) 2025 Quectel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <string.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>

#include "quec_uhc_driver.h"

LOG_MODULE_REGISTER(quec_uhc_memory, LOG_LEVEL_ERR);

/*===========================================================================
 * 							  variables
 ===========================================================================*/
K_MSGQ_DEFINE(uhc_sys_msgq, sizeof(quec_uhc_msg_t), 20, 4);
K_MSGQ_DEFINE(uhc_at_rx_msgq, sizeof(quec_trans_status_t), 20, 4);
K_MSGQ_DEFINE(uhc_at_tx_msgq, sizeof(quec_trans_status_t), 20, 4);
K_MSGQ_DEFINE(uhc_modem_rx_msgq, sizeof(quec_trans_status_t), 20, 4);
K_MSGQ_DEFINE(uhc_modem_tx_msgq, sizeof(quec_trans_status_t), 20, 4);


__attribute__((section("SRAM3"))) uint8_t quec_at_rx_buf[USB_FIFO_SIZE];
__attribute__((section("SRAM3"))) uint8_t quec_at_tx_buf[USB_FIFO_SIZE];
__attribute__((section("SRAM3"))) uint8_t quec_modem_rx_buf[USB_FIFO_SIZE];
__attribute__((section("SRAM3"))) uint8_t quec_modem_tx_buf[USB_FIFO_SIZE];

__attribute__((section("SRAM3"))) uint8_t quec_sys_task_stack[QUEC_RX_STACK_SIZE];
__attribute__((section("SRAM3"))) uint8_t quec_at_rx_task_stack[QUEC_RX_STACK_SIZE];
__attribute__((section("SRAM3"))) uint8_t quec_at_tx_task_stack[QUEC_TX_STACK_SIZE];
__attribute__((section("SRAM3"))) uint8_t quec_modem_tx_task_stack[QUEC_TX_STACK_SIZE];
__attribute__((section("SRAM3"))) uint8_t quec_modem_rx_task_stack[QUEC_RX_STACK_SIZE];


/*===========================================================================
 * 							  functions
 ===========================================================================*/
void quec_uhc_cdc_memory_init(quec_uhc_dev_t *dev, uint8_t port)
{
	switch(port)
	{
		case QUEC_AT_PORT:
			dev->rx_port.msgq = &uhc_at_rx_msgq;
			dev->tx_port.msgq = &uhc_at_tx_msgq;
			dev->rx_port.task_stack = quec_at_rx_task_stack;
			dev->tx_port.task_stack = quec_at_tx_task_stack;
			ring_buffer_init(&dev->rx_port.fifo, quec_at_rx_buf, USB_FIFO_SIZE);
			ring_buffer_init(&dev->tx_port.fifo, quec_at_tx_buf, USB_FIFO_SIZE);
		break;

		case QUEC_MODEM_PORT:
			dev->rx_port.msgq = &uhc_modem_rx_msgq;
			dev->tx_port.msgq = &uhc_modem_tx_msgq;
			dev->rx_port.task_stack = quec_modem_rx_task_stack;
			dev->tx_port.task_stack = quec_modem_tx_task_stack;
			ring_buffer_init(&dev->rx_port.fifo, quec_modem_rx_buf, USB_FIFO_SIZE);
			ring_buffer_init(&dev->tx_port.fifo, quec_modem_tx_buf, USB_FIFO_SIZE);
		break;

		default:

		break;
	}

	quec_print("cdc port %d memory init ok", port);
}

void quec_uhc_sys_memory_init(quec_uhc_pmg_t *port)
{
	port->task_stack = quec_sys_task_stack;
	port->msgq = &uhc_sys_msgq;

	quec_print("sys port memory init ok");
}

