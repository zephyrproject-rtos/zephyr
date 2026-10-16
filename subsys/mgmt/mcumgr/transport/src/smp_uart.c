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
#include <mgmt/mcumgr/transport/smp_uart_dt.h>

#ifdef CONFIG_MCUMGR_GRP_TRANSPORT
#include <zephyr/mgmt/mcumgr/grp/transport_mgmt/transport_mgmt.h>
#endif

#define DT_DRV_COMPAT zephyr_smp_uart

BUILD_ASSERT(CONFIG_MCUMGR_TRANSPORT_UART_MTU != 0, "CONFIG_MCUMGR_TRANSPORT_UART_MTU must be > 0");

/* The chosen UART uses SMP over console framing unless raw framing is selected for it */
#if defined(CONFIG_UART_MCUMGR_RAW_PROTOCOL)
#define SMP_UART_CHOSEN 0
#else
#define SMP_UART_CHOSEN SMP_UART_CHOSEN_IN_USE
#endif

#define SMP_UART_COUNT (DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) + SMP_UART_CHOSEN)

BUILD_ASSERT(SMP_UART_COUNT > 0,
	     "CONFIG_MCUMGR_TRANSPORT_UART needs a zephyr,smp-uart node, or the zephyr,uart-mcumgr "
	     "chosen UART with CONFIG_UART_MCUMGR_RAW_PROTOCOL disabled and no zephyr,smp-uart-raw "
	     "node of an enabled raw UART transport");

#if defined(CONFIG_SMP_CLIENT) || defined(CONFIG_MCUMGR_GRP_TRANSPORT)
/* The SMP client registry holds one transport per type */
BUILD_ASSERT(SMP_UART_COUNT == 1,
	     "CONFIG_SMP_CLIENT and CONFIG_MCUMGR_GRP_TRANSPORT support one zephyr,smp-uart node");
#endif

struct device;

/** An SMP over console transport on one UART. */
struct smp_uart {
	struct smp_transport transport;
	struct uart_mcumgr *uart;
	struct mcumgr_serial_rx_ctxt rx_ctxt;
	/* Lines received in interrupt context, decoded on the system workqueue */
	struct k_fifo rx_fifo;
	struct k_work rx_work;
#if defined(CONFIG_SMP_CLIENT) || defined(CONFIG_MCUMGR_GRP_TRANSPORT)
	struct smp_client_transport_entry client_entry;
#endif
};

/** What differs between transports; the rest is set up at init. */
struct smp_uart_config {
	struct smp_uart *inst;
	struct uart_mcumgr *uart;
	smp_transport_out_fn output;
};

/* Unused when no transport is defined, which the assertion above reports */
static __maybe_unused int smp_uart_tx_pkt(struct smp_uart *inst, struct net_buf *nb);

/* The output function has no context, so each transport gets its own */
#define SMP_UART_DEFINE(_name, _dev)							\
	UART_MCUMGR_DEFINE(_name##_mcumgr, _dev, UART_MCUMGR_FRAMING_SMP_OVER_CONSOLE);	\
	static struct smp_uart _name;							\
	static int _name##_output(struct net_buf *nb)					\
	{										\
		return smp_uart_tx_pkt(&_name, nb);					\
	}

#define SMP_UART_CONFIG(_name)								\
	{										\
		.inst = &_name,								\
		.uart = &_name##_mcumgr,						\
		.output = _name##_output,						\
	},

#define SMP_UART_INST_DEFINE(inst)							\
	SMP_UART_NODE_CHECK(DT_DRV_INST(inst))						\
	SMP_UART_DEFINE(smp_uart_##inst, DEVICE_DT_GET(DT_INST_PARENT(inst)))

#define SMP_UART_INST_CONFIG(inst) SMP_UART_CONFIG(smp_uart_##inst)

DT_INST_FOREACH_STATUS_OKAY(SMP_UART_INST_DEFINE)

#if SMP_UART_CHOSEN
SMP_UART_DEFINE(smp_uart_chosen, DEVICE_DT_GET(DT_CHOSEN(zephyr_uart_mcumgr)))
#endif

static const struct smp_uart_config smp_uart_configs[] = {
	DT_INST_FOREACH_STATUS_OKAY(SMP_UART_INST_CONFIG)
#if SMP_UART_CHOSEN
	SMP_UART_CONFIG(smp_uart_chosen)
#endif
};

/**
 * Processes a single line (fragment) coming from the mcumgr UART driver.
 */
static void smp_uart_process_frag(struct smp_uart *inst, struct uart_mcumgr_rx_buf *rx_buf)
{
	struct net_buf *nb;

	/* Decode the fragment and write the result to the transport's receive
	 * context.
	 */
	nb = mcumgr_serial_process_frag(&inst->rx_ctxt, rx_buf->data, rx_buf->length);

	/* Release the encoded fragment. */
	uart_mcumgr_free_rx_buf(inst->uart, rx_buf);

	/* If a complete packet has been received, pass it to SMP for
	 * processing.
	 */
	if (nb != NULL) {
		smp_rx_req(&inst->transport, nb);
	}
}

static void smp_uart_process_rx_queue(struct k_work *work)
{
	struct smp_uart *inst = CONTAINER_OF(work, struct smp_uart, rx_work);
	struct uart_mcumgr_rx_buf *rx_buf;

	while ((rx_buf = k_fifo_get(&inst->rx_fifo, K_NO_WAIT)) != NULL) {
		smp_uart_process_frag(inst, rx_buf);
	}
}

/**
 * Enqueues a received SMP fragment for later processing.  This function
 * executes in the interrupt context.
 */
static void smp_uart_rx_frag(struct uart_mcumgr_rx_buf *rx_buf, void *user_data)
{
	struct smp_uart *inst = user_data;

	k_fifo_put(&inst->rx_fifo, rx_buf);
	k_work_submit(&inst->rx_work);
}

static uint16_t smp_uart_get_mtu(const struct net_buf *nb)
{
	return CONFIG_MCUMGR_TRANSPORT_UART_MTU;
}

static int smp_uart_tx_pkt(struct smp_uart *inst, struct net_buf *nb)
{
	int rc;

	rc = uart_mcumgr_send(inst->uart, nb->data, nb->len);
	smp_packet_free(nb);

	return rc;
}

#ifdef CONFIG_MCUMGR_GRP_TRANSPORT
static bool smp_uart_bridge_connect(struct smp_transport_bridge *bridge, bool direction,
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

static void smp_uart_bridge_disconnect(struct smp_transport_bridge *bridge, bool direction)
{
	ARG_UNUSED(bridge);
	ARG_UNUSED(direction);
}

static int smp_uart_bridge_tx(const struct smp_transport_bridge *bridge, struct net_buf *nb,
			      bool direction)
{
	struct smp_transport *smpt = direction == TRANSPORT_MGMT_DIRECTION_OUTGOING
					     ? bridge->outgoing_transport
					     : bridge->incoming_transport;

	return smp_uart_tx_pkt(CONTAINER_OF(smpt, struct smp_uart, transport), nb);
}

#if defined(CONFIG_MCUMGR_GRP_TRANSPORT_INFO_FUNCTIONS)
static bool smp_uart_bridge_modes(zcbor_state_t *output_data, int *rc)
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

static bool smp_uart_bridge_config_details(uint32_t mode, zcbor_state_t *output_data, int *rc)
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

static int smp_uart_init_one(const struct smp_uart_config *config)
{
	struct smp_uart *inst = config->inst;
	int rc;

	inst->uart = config->uart;
	k_fifo_init(&inst->rx_fifo);
	k_work_init(&inst->rx_work, smp_uart_process_rx_queue);

	inst->transport.functions.output = config->output;
	TOOLCHAIN_DISABLE_WARNING(TOOLCHAIN_WARNING_DEPRECATED_DECLARATIONS);
	inst->transport.functions.get_mtu = smp_uart_get_mtu;
	TOOLCHAIN_ENABLE_WARNING(TOOLCHAIN_WARNING_DEPRECATED_DECLARATIONS);

#ifdef CONFIG_MCUMGR_GRP_TRANSPORT
	inst->transport.functions.bridge_connect = smp_uart_bridge_connect;
	inst->transport.functions.bridge_disconnect = smp_uart_bridge_disconnect;
	inst->transport.functions.bridge_output = smp_uart_bridge_tx;
#if defined(CONFIG_MCUMGR_GRP_TRANSPORT_INFO_FUNCTIONS)
	inst->transport.functions.bridge_modes = smp_uart_bridge_modes;
	inst->transport.functions.bridge_config_details = smp_uart_bridge_config_details;
#endif
#endif

	rc = smp_transport_init(&inst->transport);
	if (rc != 0) {
		return rc;
	}

	rc = uart_mcumgr_register(inst->uart, smp_uart_rx_frag, inst);
	if (rc != 0) {
		return rc;
	}

#if defined(CONFIG_SMP_CLIENT) || defined(CONFIG_MCUMGR_GRP_TRANSPORT)
	inst->client_entry.smpt = &inst->transport;
	inst->client_entry.smpt_type = SMP_SERIAL_TRANSPORT;
#ifdef CONFIG_MCUMGR_GRP_TRANSPORT_INFO_FUNCTIONS
	inst->client_entry.name = "UART";
#endif
	smp_client_transport_register(&inst->client_entry);
#endif

	return 0;
}

static int smp_uart_init(void)
{
	int ret = 0;

	/* A UART that fails to set up does not stop the others */
	ARRAY_FOR_EACH_PTR(smp_uart_configs, config) {
		int rc = smp_uart_init_one(config);

		if (rc != 0 && ret == 0) {
			ret = rc;
		}
	}

	return ret;
}

SYS_INIT(smp_uart_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
