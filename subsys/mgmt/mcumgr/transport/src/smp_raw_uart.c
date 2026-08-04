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

#ifdef CONFIG_MCUMGR_GRP_TRANSPORT
#include <zephyr/mgmt/mcumgr/grp/transport_mgmt/transport_mgmt.h>
#endif

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

#if defined(CONFIG_SMP_CLIENT) || defined(CONFIG_MCUMGR_GRP_TRANSPORT)
static struct smp_client_transport_entry smp_raw_uart_client_transport = {
	.smpt = &smp_raw_uart_transport,
	.smpt_type = SMP_RAW_SERIAL_TRANSPORT,
#ifdef CONFIG_MCUMGR_GRP_TRANSPORT_INFO_FUNCTIONS
	.name = "Raw UART",
#endif
};
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

#ifdef CONFIG_MCUMGR_GRP_TRANSPORT
static bool smp_raw_uart_bridge_connect(struct smp_transport_bridge *bridge, bool outgoing,
					uint32_t mode, bool same_transport,
					zcbor_state_t *input_data, zcbor_state_t *output_data)
{
	ARG_UNUSED(bridge);
	ARG_UNUSED(outgoing);
	ARG_UNUSED(input_data);
	ARG_UNUSED(output_data);

	if (mode != 0) {
		smp_add_cmd_err(output_data, MGMT_GROUP_ID_TRANSPORT,
				TRANSPORT_MGMT_ERR_INVALID_MODE);

		return false;
	}

	if (same_transport == true) {
		smp_add_cmd_err(output_data, MGMT_GROUP_ID_TRANSPORT,
				TRANSPORT_MGMT_ERR_SAME_BRIDGE_DEVICE_DISALLOWED);
		return false;
	}

	return true;
}

static void smp_raw_uart_bridge_disconnect(struct smp_transport_bridge *bridge, bool outgoing)
{
	ARG_UNUSED(bridge);
	ARG_UNUSED(outgoing);
}

static int smp_raw_uart_bridge_tx(const struct smp_transport_bridge *bridge, struct net_buf *nb,
				  bool outgoing)
{
	ARG_UNUSED(bridge);
	ARG_UNUSED(outgoing);

	return smp_raw_uart_tx_pkt(nb);
}

#if defined(CONFIG_MCUMGR_GRP_TRANSPORT_INFO_FUNCTIONS)
static bool smp_raw_uart_bridge_modes(zcbor_state_t *output_data, int *rc)
{
	bool ok;

	ok = zcbor_map_start_encode(output_data, 2) &&
	     zcbor_tstr_put_lit(output_data, "id") &&
	     zcbor_uint32_put(output_data, 0) &&
	     zcbor_tstr_put_lit(output_data, "description") &&
	     zcbor_tstr_put_lit(output_data, "UART") &&
	     zcbor_tstr_put_lit(output_data, "incoming") &&
	     zcbor_bool_put(output_data, true) &&
	     zcbor_tstr_put_lit(output_data, "outgoing") &&
	     zcbor_bool_put(output_data, true) &&
	     zcbor_map_end_encode(output_data, 2);

	*rc = MGMT_RETURN_CHECK(ok);
	return ok;
}

static bool smp_raw_uart_bridge_config_details(uint32_t mode, zcbor_state_t *output_data, int *rc)
{
	if (mode == 0) {
		return true;
	}

	smp_mgmt_reset_zse_writer(output_data);
	smp_add_cmd_err(output_data, MGMT_GROUP_ID_TRANSPORT, TRANSPORT_MGMT_ERR_INVALID_MODE);
	*rc = 0;

	return false;
}
#endif
#endif

static int smp_raw_uart_init(void)
{
	int rc;

	smp_raw_uart_transport.functions.output = smp_raw_uart_tx_pkt;
	smp_raw_uart_transport.functions.get_mtu = smp_raw_uart_get_mtu;

#ifdef CONFIG_MCUMGR_GRP_TRANSPORT
	smp_raw_uart_transport.functions.bridge_connect = smp_raw_uart_bridge_connect;
	smp_raw_uart_transport.functions.bridge_disconnect = smp_raw_uart_bridge_disconnect;
	smp_raw_uart_transport.functions.bridge_output = smp_raw_uart_bridge_tx;
#if defined(CONFIG_MCUMGR_GRP_TRANSPORT_INFO_FUNCTIONS)
	smp_raw_uart_transport.functions.bridge_modes = smp_raw_uart_bridge_modes;
	smp_raw_uart_transport.functions.bridge_config_details = smp_raw_uart_bridge_config_details;
#endif
#endif

	rc = smp_transport_init(&smp_raw_uart_transport);

	if (rc == 0) {
		uart_mcumgr_register(smp_raw_uart_process_frag);
#if defined(CONFIG_SMP_CLIENT) || defined(CONFIG_MCUMGR_GRP_TRANSPORT)
		smp_client_transport_register(&smp_raw_uart_client_transport);
#endif
	}

	return rc;
}

SYS_INIT(smp_raw_uart_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
