/*
 * Copyright Runtime.io 2018. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief UART transport for the mcumgr SMP protocol.
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/net_buf.h>
#include <zephyr/drivers/console/uart_mcumgr.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/smp/smp.h>
#include <zephyr/mgmt/mcumgr/transport/smp.h>
#include <zephyr/mgmt/mcumgr/transport/serial.h>

#include <mgmt/mcumgr/transport/smp_internal.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(smp_uart, 4);

BUILD_ASSERT(CONFIG_MCUMGR_TRANSPORT_UART_MTU != 0, "CONFIG_MCUMGR_TRANSPORT_UART_MTU must be > 0");

#if defined(CONFIG_MCUMGR_TRANSPORT_FORWARD_TREE)
#define DT_DRV_COMPAT zephyr_smpmgr_transport

#define ENTRY_SERIAL_TRANSPORTS(inst)						\
{										\
	.dev = DEVICE_DT_GET(DT_INST_PHANDLE(inst, transport)),			\
},

#define INST_SERIAL_TRANSPORTS(inst)						\
	COND_CODE_1(DT_INST_ENUM_HAS_VALUE(inst, type, serial),			\
	(ENTRY_SERIAL_TRANSPORTS(inst)), ())

static struct smp_transport smp_uart_transport[] = {
	DT_INST_FOREACH_STATUS_OKAY(INST_SERIAL_TRANSPORTS)
};
#else
static struct smp_transport smp_uart_transport[1] = {};
static const struct device *const uart_mcumgr_dev =
	DEVICE_DT_GET(DT_CHOSEN(zephyr_uart_mcumgr));
#endif

struct device;

static void smp_uart_process_rx_queue(struct k_work *work);

K_FIFO_DEFINE(smp_uart_rx_fifo);
K_WORK_DEFINE(smp_uart_work, smp_uart_process_rx_queue);

static struct mcumgr_serial_rx_ctxt smp_uart_rx_ctxt;
#ifdef CONFIG_SMP_CLIENT
static struct smp_client_transport_entry smp_client_transport;
#endif

/**
 * Processes a single line (fragment) coming from the mcumgr UART driver.
 */
static void smp_uart_process_frag(struct uart_mcumgr_rx_buf *rx_buf)
{
	struct net_buf *nb;

#if defined(CONFIG_MCUMGR_TRANSPORT_FORWARD_TREE)
	struct smp_transport *transport = NULL;
	int inst;

	LOG_DBG("RX from %s", rx_buf->dev->name);
#endif
	LOG_HEXDUMP_DBG(rx_buf->data, rx_buf->length, "RX");

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
#if defined(CONFIG_MCUMGR_TRANSPORT_FORWARD_TREE)
		for (inst = 0; inst < ARRAY_SIZE(smp_uart_transport); ++inst)
		{
			if (smp_uart_transport[inst].dev == rx_buf->dev) {
				transport = &smp_uart_transport[inst];
				break;
			}
		}

		if (transport) {
			smp_rx_req(transport, nb);
		}
#else
		smp_rx_req(smp_uart_transport, nb);
#endif
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
	return CONFIG_MCUMGR_TRANSPORT_UART_MTU;
}

static int smp_uart_tx_pkt(
#if defined(CONFIG_MCUMGR_TRANSPORT_FORWARD_TREE)
	const struct device *dev,
#endif
	struct net_buf *nb)
{
	int rc;

	LOG_HEXDUMP_DBG(nb->data, nb->len, "TX");

	rc = uart_mcumgr_send(
#if defined(CONFIG_MCUMGR_TRANSPORT_FORWARD_TREE)
		dev,
#else
		uart_mcumgr_dev,
#endif
		nb->data, nb->len);

	smp_packet_free(nb);

	return rc;
}

static int smp_uart_init(void)
{
	int rc;
	int inst;

	for (inst = 0; inst < ARRAY_SIZE(smp_uart_transport); ++inst)
	{
		smp_uart_transport[inst].functions.output = smp_uart_tx_pkt;
		smp_uart_transport[inst].functions.get_mtu = smp_uart_get_mtu;

		rc = smp_transport_init(&smp_uart_transport[inst]);

		if (rc) {
			return rc;
		}

#if defined(CONFIG_MCUMGR_TRANSPORT_FORWARD_TREE)
		LOG_DBG("uart dev[%d]: %s", inst, smp_uart_transport[inst].dev->name);
		uart_mcumgr_register(smp_uart_transport[inst].dev, smp_uart_rx_frag);
#else
		LOG_DBG("uart dev: %s", uart_mcumgr_dev->name);
		uart_mcumgr_register(uart_mcumgr_dev, smp_uart_rx_frag);
#endif
	}

#ifdef CONFIG_SMP_CLIENT
	smp_client_transport.smpt = &smp_uart_transport;
	smp_client_transport.smpt_type = SMP_SERIAL_TRANSPORT;
	smp_client_transport_register(&smp_client_transport);
#endif

	return 0;
}

SYS_INIT(smp_uart_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
