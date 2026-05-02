/*
 * Copyright Runtime.io 2018. All rights reserved.
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Raw UART transport for the MCUmgr (non-SMP over console) protocol.
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

#ifdef CONFIG_MCUMGR_TRANSPORT_RAW_UART_INPUT_TIMEOUT_TIME_MS
BUILD_ASSERT(CONFIG_MCUMGR_TRANSPORT_RAW_UART_INPUT_TIMEOUT_TIME_MS != 0,
	     "CONFIG_MCUMGR_TRANSPORT_RAW_UART_INPUT_TIMEOUT_TIME_MS must be > 0");
#endif

static struct mcumgr_serial_rx_ctxt mcumgr_raw_uart_rx_ctxt = {
#if defined(CONFIG_MCUMGR_TRANSPORT_SERIAL_HAS_SMP_OVER_CONSOLE) && \
	defined(CONFIG_MCUMGR_TRANSPORT_SERIAL_HAS_RAW_BINARY_NON_SMP_OVER_CONSOLE)
	.raw_transport = true,
#endif
};
static struct smp_transport smp_raw_uart_transport;

#ifdef CONFIG_SMP_CLIENT
static struct smp_client_transport_entry smp_raw_uart_client_transport;
#endif

#ifdef CONFIG_MCUMGR_TRANSPORT_RAW_UART_INPUT_TIMEOUT
static bool clear_buffer;

static void smp_raw_uart_input_timeout_handler(struct k_timer *timer)
{
	ARG_UNUSED(timer);

	clear_buffer = true;
}

K_TIMER_DEFINE(smp_raw_uart_input_timer, smp_raw_uart_input_timeout_handler, NULL);
#endif

/**
 * Processes a single line (fragment) coming from the MCUmgr UART driver.
 */
static void smp_raw_uart_process_frag(struct uart_mcumgr_rx_buf *rx_buf)
{
	struct net_buf *nb;

#ifdef CONFIG_MCUMGR_TRANSPORT_RAW_UART_INPUT_TIMEOUT
	bool first_receive = true;

	if (clear_buffer == true) {
		if (mcumgr_raw_uart_rx_ctxt.nb != NULL) {
			smp_packet_free(mcumgr_raw_uart_rx_ctxt.nb);
			mcumgr_raw_uart_rx_ctxt.nb = NULL;
		}

		clear_buffer = false;
	} else if (mcumgr_raw_uart_rx_ctxt.nb != NULL) {
		first_receive = false;
	} else {
		/* Empty else that does nothing to stop a code checker pointlessly complaining. */
	}
#endif

	/* Decode the fragment and write the result to the global receive context. */
	nb = mcumgr_serial_process_frag(&mcumgr_raw_uart_rx_ctxt,
					rx_buf->data, rx_buf->length);

	/* Release the encoded fragment. */
	uart_mcumgr_free_rx_buf(rx_buf);

	/* If a complete packet has been received, pass it to SMP for processing. */
	if (nb != NULL) {
#ifdef CONFIG_MCUMGR_TRANSPORT_RAW_UART_INPUT_TIMEOUT
		k_timer_stop(&smp_raw_uart_input_timer);
#endif
		smp_rx_req(&smp_raw_uart_transport, nb);
#ifdef CONFIG_MCUMGR_TRANSPORT_RAW_UART_INPUT_TIMEOUT
	} else if (mcumgr_raw_uart_rx_ctxt.nb != NULL && mcumgr_raw_uart_rx_ctxt.nb->len > 0 &&
		   first_receive == true) {
		/*
		 * Upon timer timeout, a flag will be set which will clear the buffer on the next
		 * invocation of this function, which could be right away or could be a long time
		 * away, this avoids having to deal with synchronisation inside of ISRs.
		 */
		k_timer_start(&smp_raw_uart_input_timer,
			      K_MSEC(CONFIG_MCUMGR_TRANSPORT_RAW_UART_INPUT_TIMEOUT_TIME_MS),
			      K_NO_WAIT);
#endif
	} else {
		/* Empty else that does nothing to stop a code checker pointlessly complaining. */
	}
}

static uint16_t smp_raw_uart_get_mtu(const struct net_buf *nb)
{
	return CONFIG_MCUMGR_TRANSPORT_NETBUF_SIZE;
}

static int smp_raw_uart_tx_pkt(struct net_buf *nb)
{
	int rc;

	rc = uart_mcumgr_send(nb->data, nb->len);
	smp_packet_free(nb);

	return rc;
}

static int smp_raw_uart_init(void)
{
	int rc;

	smp_raw_uart_transport.functions.output = smp_raw_uart_tx_pkt;
	smp_raw_uart_transport.functions.get_mtu = smp_raw_uart_get_mtu;

	rc = smp_transport_init(&smp_raw_uart_transport);

	if (rc == 0) {
		uart_mcumgr_register(smp_raw_uart_process_frag);
#ifdef CONFIG_SMP_CLIENT
		smp_client_transport.smpt = &smp_raw_uart_transport;
		smp_client_transport.smpt_type = SMP_RAW_SERIAL_TRANSPORT;
		smp_client_transport_register(&smp_raw_client_transport);
#endif
	}

	return rc;
}

SYS_INIT(smp_raw_uart_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
