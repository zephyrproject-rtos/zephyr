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
#include <mgmt/mcumgr/transport/smp_uart_dt.h>

#ifdef CONFIG_MCUMGR_GRP_TRANSPORT
#include <zephyr/mgmt/mcumgr/grp/transport_mgmt/transport_mgmt.h>
#endif

#ifdef CONFIG_MCUMGR_TRANSPORT_RAW_UART_INPUT_TIMEOUT_TIME_MS
BUILD_ASSERT(CONFIG_MCUMGR_TRANSPORT_RAW_UART_INPUT_TIMEOUT_TIME_MS != 0,
	     "CONFIG_MCUMGR_TRANSPORT_RAW_UART_INPUT_TIMEOUT_TIME_MS must be > 0");
#endif

#define DT_DRV_COMPAT zephyr_smp_uart_raw

/* The chosen UART uses raw framing if selected for it */
#if defined(CONFIG_UART_MCUMGR_RAW_PROTOCOL)
#define SMP_RAW_UART_CHOSEN SMP_UART_CHOSEN_IN_USE
#else
#define SMP_RAW_UART_CHOSEN 0
#endif

#define SMP_RAW_UART_COUNT (DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) + SMP_RAW_UART_CHOSEN)

BUILD_ASSERT(SMP_RAW_UART_COUNT > 0,
	     "CONFIG_MCUMGR_TRANSPORT_RAW_UART needs a zephyr,smp-uart-raw node, or the "
	     "zephyr,uart-mcumgr chosen UART with CONFIG_UART_MCUMGR_RAW_PROTOCOL enabled and no "
	     "zephyr,smp-uart node of an enabled UART transport");

#if defined(CONFIG_SMP_CLIENT) || defined(CONFIG_MCUMGR_GRP_TRANSPORT)
/* The SMP client registry holds one transport per type */
BUILD_ASSERT(SMP_RAW_UART_COUNT == 1,
	     "CONFIG_SMP_CLIENT and CONFIG_MCUMGR_GRP_TRANSPORT support one "
	     "zephyr,smp-uart-raw node");
#endif

/** A raw transport on one UART. */
struct smp_raw_uart {
	struct smp_transport transport;
	struct uart_mcumgr *uart;
	struct mcumgr_serial_rx_ctxt rx_ctxt;
#ifdef CONFIG_MCUMGR_TRANSPORT_RAW_UART_INPUT_TIMEOUT
	struct k_timer input_timer;
	bool clear_buffer;
#endif
#if defined(CONFIG_SMP_CLIENT) || defined(CONFIG_MCUMGR_GRP_TRANSPORT)
	struct smp_client_transport_entry client_entry;
#endif
};

/** What differs between transports; the rest is set up at init. */
struct smp_raw_uart_config {
	struct smp_raw_uart *inst;
	struct uart_mcumgr *uart;
	smp_transport_out_fn output;
};

/* Unused when no transport is defined, which the assertion above reports */
static __maybe_unused int smp_raw_uart_tx_pkt(struct smp_raw_uart *inst, struct net_buf *nb);

/*
 * The output function has no context, so each transport gets its own. When serial_util.c is
 * built for both framings, the receive context is marked raw, which can only be done here.
 */
#define SMP_RAW_UART_DEFINE(_name, _dev)						\
	UART_MCUMGR_DEFINE(_name##_mcumgr, _dev, UART_MCUMGR_FRAMING_RAW);		\
	static struct smp_raw_uart _name =						\
		COND_CODE_1(CONFIG_MCUMGR_TRANSPORT_SERIAL_HAS_SMP_OVER_CONSOLE,	\
			    ({ .rx_ctxt.raw_transport = true }), ({ 0 }));		\
	static int _name##_output(struct net_buf *nb)					\
	{										\
		return smp_raw_uart_tx_pkt(&_name, nb);					\
	}

#define SMP_RAW_UART_CONFIG(_name)							\
	{										\
		.inst = &_name,								\
		.uart = &_name##_mcumgr,						\
		.output = _name##_output,						\
	},

#define SMP_RAW_UART_INST_DEFINE(inst)							\
	SMP_UART_NODE_CHECK(DT_DRV_INST(inst))						\
	SMP_RAW_UART_DEFINE(smp_raw_uart_##inst, DEVICE_DT_GET(DT_INST_PARENT(inst)))

#define SMP_RAW_UART_INST_CONFIG(inst) SMP_RAW_UART_CONFIG(smp_raw_uart_##inst)

DT_INST_FOREACH_STATUS_OKAY(SMP_RAW_UART_INST_DEFINE)

#if SMP_RAW_UART_CHOSEN
SMP_RAW_UART_DEFINE(smp_raw_uart_chosen, DEVICE_DT_GET(DT_CHOSEN(zephyr_uart_mcumgr)))
#endif

static const struct smp_raw_uart_config smp_raw_uart_configs[] = {
	DT_INST_FOREACH_STATUS_OKAY(SMP_RAW_UART_INST_CONFIG)
#if SMP_RAW_UART_CHOSEN
	SMP_RAW_UART_CONFIG(smp_raw_uart_chosen)
#endif
};

#ifdef CONFIG_MCUMGR_TRANSPORT_RAW_UART_INPUT_TIMEOUT
static void smp_raw_uart_input_timeout_handler(struct k_timer *timer)
{
	struct smp_raw_uart *inst = CONTAINER_OF(timer, struct smp_raw_uart, input_timer);

	inst->clear_buffer = true;
}
#endif

/**
 * Processes a single line (fragment) coming from the MCUmgr UART driver.
 */
static void smp_raw_uart_process_frag(struct uart_mcumgr_rx_buf *rx_buf, void *user_data)
{
	struct smp_raw_uart *inst = user_data;
	struct mcumgr_serial_rx_ctxt *rx_ctxt = &inst->rx_ctxt;
	struct net_buf *nb;

#ifdef CONFIG_MCUMGR_TRANSPORT_RAW_UART_INPUT_TIMEOUT
	bool first_receive = true;

	if (inst->clear_buffer == true) {
		if (rx_ctxt->nb != NULL) {
			smp_packet_free(rx_ctxt->nb);
			rx_ctxt->nb = NULL;
		}

		inst->clear_buffer = false;
	} else if (rx_ctxt->nb != NULL) {
		first_receive = false;
	} else {
		/* Empty else that does nothing to stop a code checker pointlessly complaining. */
	}
#endif

	/* Decode the fragment and write the result to the transport's receive context. */
	nb = mcumgr_serial_process_frag(rx_ctxt, rx_buf->data, rx_buf->length);

	/* Release the encoded fragment. */
	uart_mcumgr_free_rx_buf(inst->uart, rx_buf);

	/* If a complete packet has been received, pass it to SMP for processing. */
	if (nb != NULL) {
#ifdef CONFIG_MCUMGR_TRANSPORT_RAW_UART_INPUT_TIMEOUT
		k_timer_stop(&inst->input_timer);
#endif
		smp_rx_req(&inst->transport, nb);
#ifdef CONFIG_MCUMGR_TRANSPORT_RAW_UART_INPUT_TIMEOUT
	} else if (rx_ctxt->nb != NULL && rx_ctxt->nb->len > 0 && first_receive == true) {
		/*
		 * Upon timer timeout, a flag will be set which will clear the buffer on the next
		 * invocation of this function, which could be right away or could be a long time
		 * away, this avoids having to deal with synchronisation inside of ISRs.
		 */
		k_timer_start(&inst->input_timer,
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

static int smp_raw_uart_tx_pkt(struct smp_raw_uart *inst, struct net_buf *nb)
{
	int rc;

	rc = uart_mcumgr_send(inst->uart, nb->data, nb->len);
	smp_packet_free(nb);

	return rc;
}

#ifdef CONFIG_MCUMGR_GRP_TRANSPORT
static bool smp_raw_uart_bridge_connect(struct smp_transport_bridge *bridge, bool direction,
					uint32_t mode, bool same_transport,
					zcbor_state_t *input_data, zcbor_state_t *output_data)
{
	ARG_UNUSED(bridge);
	ARG_UNUSED(direction);
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

	return true;
}

static void smp_raw_uart_bridge_disconnect(struct smp_transport_bridge *bridge, bool direction)
{
	ARG_UNUSED(bridge);
	ARG_UNUSED(direction);
}

static int smp_raw_uart_bridge_tx(const struct smp_transport_bridge *bridge, struct net_buf *nb,
				  bool direction)
{
	struct smp_transport *smpt = direction == TRANSPORT_MGMT_DIRECTION_OUTGOING
					     ? bridge->outgoing_transport
					     : bridge->incoming_transport;

	return smp_raw_uart_tx_pkt(CONTAINER_OF(smpt, struct smp_raw_uart, transport), nb);
}

#if defined(CONFIG_MCUMGR_GRP_TRANSPORT_INFO_FUNCTIONS)
static bool smp_raw_uart_bridge_modes(zcbor_state_t *output_data, int *rc)
{
	bool ok;

	ok = zcbor_map_start_encode(output_data, 4) &&
	     zcbor_tstr_put_lit(output_data, "id") &&
	     zcbor_uint32_put(output_data, 0) &&
	     zcbor_tstr_put_lit(output_data, "description") &&
	     zcbor_tstr_put_lit(output_data, "UART") &&
	     zcbor_tstr_put_lit(output_data, "incoming") &&
	     zcbor_bool_put(output_data, true) &&
	     zcbor_tstr_put_lit(output_data, "outgoing") &&
	     zcbor_bool_put(output_data, true) &&
	     zcbor_map_end_encode(output_data, 4);

	*rc = MGMT_RETURN_CHECK(ok);
	return ok;
}

static bool smp_raw_uart_bridge_config_details(uint32_t mode, zcbor_state_t *output_data, int *rc)
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

static int smp_raw_uart_init_one(const struct smp_raw_uart_config *config)
{
	struct smp_raw_uart *inst = config->inst;
	int rc;

	inst->uart = config->uart;
#ifdef CONFIG_MCUMGR_TRANSPORT_RAW_UART_INPUT_TIMEOUT
	k_timer_init(&inst->input_timer, smp_raw_uart_input_timeout_handler, NULL);
#endif

	inst->transport.functions.output = config->output;
	TOOLCHAIN_DISABLE_WARNING(TOOLCHAIN_WARNING_DEPRECATED_DECLARATIONS);
	inst->transport.functions.get_mtu = smp_raw_uart_get_mtu;
	TOOLCHAIN_ENABLE_WARNING(TOOLCHAIN_WARNING_DEPRECATED_DECLARATIONS);

#ifdef CONFIG_MCUMGR_GRP_TRANSPORT
	inst->transport.functions.bridge_connect = smp_raw_uart_bridge_connect;
	inst->transport.functions.bridge_disconnect = smp_raw_uart_bridge_disconnect;
	inst->transport.functions.bridge_output = smp_raw_uart_bridge_tx;
#if defined(CONFIG_MCUMGR_GRP_TRANSPORT_INFO_FUNCTIONS)
	inst->transport.functions.bridge_modes = smp_raw_uart_bridge_modes;
	inst->transport.functions.bridge_config_details = smp_raw_uart_bridge_config_details;
#endif
#endif

	rc = smp_transport_init(&inst->transport);
	if (rc != 0) {
		return rc;
	}

	rc = uart_mcumgr_register(inst->uart, smp_raw_uart_process_frag, inst);
	if (rc != 0) {
		return rc;
	}

#if defined(CONFIG_SMP_CLIENT) || defined(CONFIG_MCUMGR_GRP_TRANSPORT)
	inst->client_entry.smpt = &inst->transport;
	inst->client_entry.smpt_type = SMP_RAW_SERIAL_TRANSPORT;
#ifdef CONFIG_MCUMGR_GRP_TRANSPORT_INFO_FUNCTIONS
	inst->client_entry.name = "Raw UART";
#endif
	smp_client_transport_register(&inst->client_entry);
#endif

	return 0;
}

static int smp_raw_uart_init(void)
{
	int ret = 0;

	/* A UART that fails to set up does not stop the others */
	ARRAY_FOR_EACH_PTR(smp_raw_uart_configs, config) {
		int rc = smp_raw_uart_init_one(config);

		if (rc != 0 && ret == 0) {
			ret = rc;
		}
	}

	return ret;
}

SYS_INIT(smp_raw_uart_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
