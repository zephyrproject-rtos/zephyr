/*
 * Copyright Runtime.io 2018. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief UART transport for the mcumgr SMP protocol.
 */

#include <string.h>
#include <zephyr.h>
#include <init.h>
#include "net/buf.h"
#include <drivers/console/uart_mcumgr.h>
#include "mgmt/mgmt.h"
#include <mgmt/mcumgr/serial.h>
#include "mgmt/mcumgr/buf.h"
#include "mgmt/mcumgr/smp.h"

struct device;

static void smp_uart_process_rx_queue(struct k_work *work);

K_FIFO_DEFINE(smp_uart_rx_fifo);
K_WORK_DEFINE(smp_uart_work, smp_uart_process_rx_queue);

static struct mcumgr_serial_rx_ctxt smp_uart_rx_ctxt;
static struct zephyr_smp_transport smp_uart_transport;

/**
 * Processes a single line (fragment) coming from the mcumgr UART driver.
 */
static void smp_uart_process_frag(struct uart_mcumgr_rx_buf *rx_buf)
{
	struct net_buf *nb;

	/* Decode the fragment and write the result to the global receive
	 * context.
	 */
	nb = mcumgr_serial_process_frag(&smp_uart_rx_ctxt,
					rx_buf->data, rx_buf->length);

	/* Release the encoded fragment. */
	uart_mcumgr_free_rx_buf(rx_buf);

	/* If a complete packet has been received, pass it to SMP for
	 * processing.
	 */
	if (nb != NULL) {
		zephyr_smp_rx_req(&smp_uart_transport, nb);
	}
}

static void smp_uart_process_rx_queue(struct k_work *work)
{
	struct uart_mcumgr_rx_buf *rx_buf;

	while ((rx_buf = k_fifo_get(&smp_uart_rx_fifo, K_NO_WAIT)) != NULL) {
		smp_uart_process_frag(rx_buf);
	}
}

/**
 * Enqueues a received SMP fragment for later processing.  This function
 * executes in the interrupt context.
 */
static void smp_uart_rx_frag(struct uart_mcumgr_rx_buf *rx_buf)
{
	k_fifo_put(&smp_uart_rx_fifo, rx_buf);
	k_work_submit(&smp_uart_work);
}

static uint16_t smp_uart_get_mtu(const struct net_buf *nb)
{
	return CONFIG_MCUMGR_SMP_UART_MTU;
}

static int smp_uart_tx_pkt(struct zephyr_smp_transport *zst,
			   struct net_buf *nb)
{
	int rc;

	rc = uart_mcumgr_send(nb->data, nb->len);
	mcumgr_buf_free(nb);

	return rc;
}

static int smp_uart_init(struct device *dev)
{
	ARG_UNUSED(dev);

	zephyr_smp_transport_init(&smp_uart_transport, smp_uart_tx_pkt,
				  smp_uart_get_mtu, NULL, NULL);
	uart_mcumgr_register(smp_uart_rx_frag);

	return 0;
}

SYS_INIT(smp_uart_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
