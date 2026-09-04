/*
 * Copyright Runtime.io 2018. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Shell transport for the mcumgr SMP protocol.
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/net_buf.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/smp/smp.h>
#include <zephyr/mgmt/mcumgr/transport/smp.h>
#include <zephyr/mgmt/mcumgr/transport/serial.h>
#include <zephyr/mgmt/mcumgr/transport/smp_shell.h>
#include <zephyr/syscalls/uart.h>
#include <string.h>

#include <mgmt/mcumgr/transport/smp_internal.h>

#ifdef CONFIG_MCUMGR_GRP_TRANSPORT
#include <zephyr/mgmt/mcumgr/grp/transport_mgmt/transport_mgmt.h>
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(smp_shell);

BUILD_ASSERT(CONFIG_MCUMGR_TRANSPORT_SHELL_MTU != 0,
	     "CONFIG_MCUMGR_TRANSPORT_SHELL_MTU must be > 0");

#ifdef CONFIG_MCUMGR_TRANSPORT_SHELL_INPUT_TIMEOUT
BUILD_ASSERT(CONFIG_MCUMGR_TRANSPORT_SHELL_INPUT_TIMEOUT_TIME != 0,
	     "CONFIG_MCUMGR_TRANSPORT_SHELL_INPUT_TIMEOUT_TIME must be > 0");
#endif

static struct smp_transport smp_shell_transport;

static struct mcumgr_serial_rx_ctxt smp_shell_rx_ctxt;

static const struct shell_uart_common *shell_uart;

#if defined(CONFIG_SMP_CLIENT) || defined(CONFIG_MCUMGR_GRP_TRANSPORT)
static struct smp_client_transport_entry smp_client_transport = {
	.smpt = &smp_shell_transport,
	.smpt_type = SMP_SHELL_TRANSPORT,
#ifdef CONFIG_MCUMGR_GRP_TRANSPORT_INFO_FUNCTIONS
	.name = "Shell",
#endif
};
#endif

/** SMP mcumgr frame fragments. */
enum smp_shell_esc_mcumgr {
	ESC_MCUMGR_PKT_1,
	ESC_MCUMGR_PKT_2,
	ESC_MCUMGR_FRAG_1,
	ESC_MCUMGR_FRAG_2,
};

/** These states indicate whether an mcumgr frame is being received. */
enum smp_shell_mcumgr_state {
	SMP_SHELL_MCUMGR_STATE_NONE,
	SMP_SHELL_MCUMGR_STATE_HEADER,
	SMP_SHELL_MCUMGR_STATE_PAYLOAD
};

#ifdef CONFIG_MCUMGR_TRANSPORT_SHELL_INPUT_TIMEOUT
static void smp_shell_input_timeout_handler(struct k_timer *timer)
{
	ARG_UNUSED(timer);
	struct smp_shell_data *const data = shell_uart_smp_shell_data_get_ptr();

	atomic_clear_bit(&data->esc_state, ESC_MCUMGR_PKT_1);
	atomic_clear_bit(&data->esc_state, ESC_MCUMGR_PKT_2);
	atomic_clear_bit(&data->esc_state, ESC_MCUMGR_FRAG_1);
	atomic_clear_bit(&data->esc_state, ESC_MCUMGR_FRAG_2);

	if (data->buf) {
		net_buf_reset(data->buf);
		net_buf_drop(&data->buf);
	}
}

K_TIMER_DEFINE(smp_shell_input_timer, smp_shell_input_timeout_handler, NULL);
#endif

static int read_mcumgr_byte(struct smp_shell_data *data, uint8_t byte)
{
	bool frag_1;
	bool frag_2;
	bool pkt_1;
	bool pkt_2;

	pkt_1 = atomic_test_bit(&data->esc_state, ESC_MCUMGR_PKT_1);
	pkt_2 = atomic_test_bit(&data->esc_state, ESC_MCUMGR_PKT_2);
	frag_1 = atomic_test_bit(&data->esc_state, ESC_MCUMGR_FRAG_1);
	frag_2 = atomic_test_bit(&data->esc_state, ESC_MCUMGR_FRAG_2);

	if (pkt_2 || frag_2) {
		/* Already fully framed. */
		return SMP_SHELL_MCUMGR_STATE_PAYLOAD;
	}

	if (pkt_1) {
		if (byte == MCUMGR_SERIAL_HDR_PKT_2) {
			/* Final framing byte received. */
			atomic_set_bit(&data->esc_state, ESC_MCUMGR_PKT_2);
#ifdef CONFIG_MCUMGR_TRANSPORT_SHELL_INPUT_TIMEOUT
			k_timer_start(&smp_shell_input_timer,
				      K_MSEC(CONFIG_MCUMGR_TRANSPORT_SHELL_INPUT_TIMEOUT_TIME),
				      K_NO_WAIT);
#endif
			return SMP_SHELL_MCUMGR_STATE_PAYLOAD;
		}
	} else if (frag_1) {
		if (byte == MCUMGR_SERIAL_HDR_FRAG_2) {
			/* Final framing byte received. */
			atomic_set_bit(&data->esc_state, ESC_MCUMGR_FRAG_2);
#ifdef CONFIG_MCUMGR_TRANSPORT_SHELL_INPUT_TIMEOUT
			k_timer_start(&smp_shell_input_timer,
				      K_MSEC(CONFIG_MCUMGR_TRANSPORT_SHELL_INPUT_TIMEOUT_TIME),
				      K_NO_WAIT);
#endif
			return SMP_SHELL_MCUMGR_STATE_PAYLOAD;
		}
	} else {
		if (byte == MCUMGR_SERIAL_HDR_PKT_1) {
			/* First framing byte received. */
			atomic_set_bit(&data->esc_state, ESC_MCUMGR_PKT_1);
			return SMP_SHELL_MCUMGR_STATE_HEADER;
		} else if (byte == MCUMGR_SERIAL_HDR_FRAG_1) {
			/* First framing byte received. */
			atomic_set_bit(&data->esc_state, ESC_MCUMGR_FRAG_1);
			return SMP_SHELL_MCUMGR_STATE_HEADER;
		}
	}

	/* Non-mcumgr byte received. */
	return SMP_SHELL_MCUMGR_STATE_NONE;
}

size_t smp_shell_rx_bytes(struct smp_shell_data *data, const uint8_t *bytes,
			  size_t size)
{
	size_t consumed = 0;		/* Number of bytes consumed by SMP */

	/* Process all bytes that are accepted as SMP commands. */
	while (size != consumed) {
		uint8_t byte = bytes[consumed];
		int mcumgr_state = read_mcumgr_byte(data, byte);

		if (mcumgr_state == SMP_SHELL_MCUMGR_STATE_NONE) {
			break;
		} else if (mcumgr_state == SMP_SHELL_MCUMGR_STATE_HEADER &&
			   !data->buf) {
			data->buf = net_buf_alloc(data->buf_pool, K_NO_WAIT);
			if (!data->buf) {
				LOG_WRN("Failed to alloc SMP buf");
			}
		}

		if (data->buf && net_buf_tailroom(data->buf) > 0) {
			net_buf_add_u8(data->buf, byte);
		}

		/* Newline in payload means complete frame */
		if (mcumgr_state == SMP_SHELL_MCUMGR_STATE_PAYLOAD &&
		    byte == '\n') {
			if (data->buf) {
				k_fifo_put(&data->buf_ready, data->buf);
				data->buf = NULL;
			}
			atomic_clear_bit(&data->esc_state, ESC_MCUMGR_PKT_1);
			atomic_clear_bit(&data->esc_state, ESC_MCUMGR_PKT_2);
			atomic_clear_bit(&data->esc_state, ESC_MCUMGR_FRAG_1);
			atomic_clear_bit(&data->esc_state, ESC_MCUMGR_FRAG_2);

#ifdef CONFIG_MCUMGR_TRANSPORT_SHELL_INPUT_TIMEOUT
			k_timer_stop(&smp_shell_input_timer);
#endif
		}

		++consumed;
	}

	return consumed;
}

void smp_shell_process(struct smp_shell_data *data)
{
	struct net_buf *buf;
	struct net_buf *nb;

	while (true) {
		buf = k_fifo_get(&data->buf_ready, K_NO_WAIT);
		if (!buf) {
			break;
		}

		nb = mcumgr_serial_process_frag(&smp_shell_rx_ctxt,
						buf->data,
						buf->len);
		if (nb != NULL) {
			smp_rx_req(&smp_shell_transport, nb);
		}

		net_buf_unref(buf);
	}
}

static uint16_t smp_shell_get_mtu(const struct net_buf *nb)
{
	return CONFIG_MCUMGR_TRANSPORT_SHELL_MTU;
}

static int smp_shell_tx_raw(const void *data, int len)
{
	const uint8_t *out = data;

	while ((out != NULL) && (len != 0)) {
		uart_poll_out(shell_uart->dev, *out);
		++out;
		--len;
	}

	return 0;
}

static int smp_shell_tx_pkt(struct net_buf *nb)
{
	int rc;

	shell_uart = (struct shell_uart_common *)shell_backend_uart_get_ptr()->iface->ctx;
	rc = mcumgr_serial_tx_pkt(nb->data, nb->len, smp_shell_tx_raw);
	smp_packet_free(nb);

	return rc;
}

#ifdef CONFIG_MCUMGR_GRP_TRANSPORT
static bool smp_shell_bridge_connect(struct smp_transport_bridge *bridge, bool direction,
				     uint32_t mode, bool same_transport,
				     zcbor_state_t *input_data, zcbor_state_t *output_data)
{
	ARG_UNUSED(bridge);
	ARG_UNUSED(input_data);
	ARG_UNUSED(output_data);

	if (mode != 0) {
		smp_add_cmd_err(output_data, MGMT_GROUP_ID_TRANSPORT,
				TRANSPORT_MGMT_ERR_INVALID_MODE);

		return false;
	}

	if (same_transport) {
		smp_add_cmd_err(output_data, MGMT_GROUP_ID_TRANSPORT,
				TRANSPORT_MGMT_ERR_SAME_BRIDGE_DEVICE_DISALLOWED);
		return false;
	}

	if (direction == TRANSPORT_MGMT_DIRECTION_OUTGOING) {
		smp_add_cmd_err(output_data, MGMT_GROUP_ID_TRANSPORT,
				TRANSPORT_MGMT_ERR_TRANSPORT_OUTGOING_NOT_SUPPORTED);
		return false;
	}

	return true;
}

static void smp_shell_bridge_disconnect(struct smp_transport_bridge *bridge, bool direction)
{
	ARG_UNUSED(bridge);
	ARG_UNUSED(direction);
}

static int smp_shell_bridge_tx(const struct smp_transport_bridge *bridge, struct net_buf *nb,
			       bool direction)
{
	ARG_UNUSED(bridge);
	ARG_UNUSED(direction);

	return smp_shell_tx_pkt(nb);
}

#if defined(CONFIG_MCUMGR_GRP_TRANSPORT_INFO_FUNCTIONS)
static bool smp_shell_bridge_modes(zcbor_state_t *output_data, int *rc)
{
	bool ok;

	ok = zcbor_map_start_encode(output_data, 3) &&
	     zcbor_tstr_put_lit(output_data, "id") &&
	     zcbor_uint32_put(output_data, 0) &&
	     zcbor_tstr_put_lit(output_data, "description") &&
	     zcbor_tstr_put_lit(output_data, "Shell") &&
	     zcbor_tstr_put_lit(output_data, "incoming") &&
	     zcbor_bool_put(output_data, true) &&
	     zcbor_map_end_encode(output_data, 3);

	*rc = MGMT_RETURN_CHECK(ok);
	return ok;
}

static bool smp_shell_bridge_config_details(uint32_t mode, zcbor_state_t *output_data, int *rc)
{
	if (mode == 0) {
		return true;
	}

	smp_mgmt_reset_writer(output_data);
	smp_add_cmd_err(output_data, MGMT_GROUP_ID_TRANSPORT, TRANSPORT_MGMT_ERR_INVALID_MODE);
	*rc = 0;

	return false;
}
#endif
#endif

int smp_shell_init(void)
{
	int rc;

	smp_shell_transport.functions.output = smp_shell_tx_pkt;
	smp_shell_transport.functions.get_mtu = smp_shell_get_mtu;

#ifdef CONFIG_MCUMGR_GRP_TRANSPORT
	smp_shell_transport.functions.bridge_connect = smp_shell_bridge_connect;
	smp_shell_transport.functions.bridge_disconnect = smp_shell_bridge_disconnect;
	smp_shell_transport.functions.bridge_output = smp_shell_bridge_tx;
#if defined(CONFIG_MCUMGR_GRP_TRANSPORT_INFO_FUNCTIONS)
	smp_shell_transport.functions.bridge_modes = smp_shell_bridge_modes;
	smp_shell_transport.functions.bridge_config_details = smp_shell_bridge_config_details;
#endif
#endif

	rc = smp_transport_init(&smp_shell_transport);
#if defined(CONFIG_SMP_CLIENT) || defined(CONFIG_MCUMGR_GRP_TRANSPORT)
	if (rc == 0) {
		smp_client_transport_register(&smp_client_transport);
	}
#endif

	return rc;
}
