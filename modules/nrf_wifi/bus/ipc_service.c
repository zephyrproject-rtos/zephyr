/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief File containing API definitions for the
 * IPC service layer of the nrf71 Wi-Fi driver.
 */
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(wifi_nrf_bus, CONFIG_WIFI_NRF70_BUSLIB_LOG_LEVEL);

#include "ipc_service.h"

static void wifi_ipc_ep_bound(void *priv)
{
	wifi_ipc_t *context = (wifi_ipc_t *)priv;

	context->busy_q.ipc_ready = true;
}

static void wifi_ipc_recv_callback(const void *data, size_t len, void *priv)
{
	(void)len;
	uint32_t global_addr = *((uint32_t *)data);

	wifi_ipc_t *context = (wifi_ipc_t *)priv;

	context->busy_q.recv_cb((void *)global_addr, context->busy_q.priv);

	if (context->free_q != NULL) {
		while (!spsc32_push(context->free_q, global_addr)) {
		};
	}
}

static void wifi_ipc_busyq_init(wifi_ipc_busyq_t *busyq, const ipc_device_wrapper_t *ipc_inst,
				void *rx_cb, void *priv)
{
	busyq->ipc_inst = ipc_inst;
	busyq->ipc_ep_cfg.cb.bound = wifi_ipc_ep_bound;
	busyq->ipc_ep_cfg.cb.received = wifi_ipc_recv_callback;
	busyq->recv_cb = rx_cb;
	busyq->ipc_ready = false;
	busyq->priv = priv;
}

/**
 * Register the IPC service on the busy_queue
 */
static wifi_ipc_status_t wifi_ipc_busyq_register(wifi_ipc_t *context)
{
	int ret;
	const struct device *ipc_instance = GET_IPC_INSTANCE(context->busy_q.ipc_inst);

	ret = ipc_service_open_instance(ipc_instance);
	if (ret < 0) {
		return WIFI_IPC_STATUS_INIT_ERR;
	}

	context->busy_q.ipc_ep_cfg.name = "ep";
	context->busy_q.ipc_ep_cfg.priv = context;

	ret = ipc_service_register_endpoint(ipc_instance, &context->busy_q.ipc_ep,
						&context->busy_q.ipc_ep_cfg);
	if (ret < 0 && ret != -EALREADY) {
		return WIFI_IPC_STATUS_INIT_ERR;
	}

	LOG_INF("IPC busy queue registered");

	return WIFI_IPC_STATUS_OK;
}

wifi_ipc_status_t wifi_ipc_bind_ipc_service(wifi_ipc_t *context,
						const ipc_device_wrapper_t *ipc_inst,
						void (*rx_cb)(void *data, void *priv), void *priv)
{
	wifi_ipc_busyq_init(&context->busy_q, ipc_inst, rx_cb, priv);
	return wifi_ipc_busyq_register(context);
}

wifi_ipc_status_t wifi_ipc_bind_ipc_service_tx_rx(wifi_ipc_t *tx, wifi_ipc_t *rx,
						  const ipc_device_wrapper_t *ipc_inst,
						  void (*rx_cb)(void *data, void *priv), void *priv)
{
	wifi_ipc_busyq_init(&rx->busy_q, ipc_inst, rx_cb, priv);

	/**
	 * When initialising an IPC service, both TX and RX mailboxes need to be
	 * registered at the same time using a single function call. Both tx and
	 * rx need to refer to the same IPC instance.
	 */
	tx->linked_ipc = &rx->busy_q;

	return wifi_ipc_busyq_register(rx);
}

wifi_ipc_status_t wifi_ipc_freeq_get(wifi_ipc_t *context, uint32_t *data)
{
	if (context->free_q == NULL) {
		LOG_ERR("Free queue is not initialised");
		return WIFI_IPC_STATUS_FREEQ_UNINIT_ERR;
	}

	if (spsc32_is_empty(context->free_q)) {
		LOG_DBG("Free queue is empty");
		return WIFI_IPC_STATUS_FREEQ_EMPTY;
	}

	if (!spsc32_read_head(context->free_q, data)) {
		LOG_DBG("Free queue is empty");
		return WIFI_IPC_STATUS_FREEQ_EMPTY;
	}

	return WIFI_IPC_STATUS_OK;
}

wifi_ipc_status_t wifi_ipc_freeq_send(wifi_ipc_t *context, uint32_t data)
{
	return (spsc32_push(context->free_q, data) == true ? WIFI_IPC_STATUS_OK
								 : WIFI_IPC_STATUS_FREEQ_FULL);
}

wifi_ipc_status_t wifi_ipc_busyq_send(wifi_ipc_t *context, uint32_t *data)
{
	/* Get correct linked endpoint */
	wifi_ipc_busyq_t *busyq =
		context->linked_ipc ? context->linked_ipc : &context->busy_q;

	if (!busyq->ipc_ready) {
		LOG_ERR("IPC service is not ready");
		return WIFI_IPC_STATUS_BUSYQ_NOTREADY;
	}

	int ret = ipc_service_send(&busyq->ipc_ep, data, sizeof(*data));

	if (ret == -ENOMEM) {
		LOG_ERR("No space in the buffer");
		/* No space in the buffer */
		return WIFI_IPC_STATUS_BUSYQ_FULL;
	} else if (ret < 0) {
		LOG_ERR("Critical IPC failure: %d", ret);
		/* Critical IPC failure */
		return WIFI_IPC_STATUS_BUSYQ_CRITICAL_ERR;
	}

	if (context->free_q != NULL) {
		/* Free up global address pointer from the free queue */
		uint32_t data_out;

		return (spsc32_pop(context->free_q, &data_out) == true
				? (*data == data_out ? WIFI_IPC_STATUS_OK
							: WIFI_IPC_STATUS_FREEQ_INVALID)
				: WIFI_IPC_STATUS_FREEQ_EMPTY);
	}

	return WIFI_IPC_STATUS_OK;
}

wifi_ipc_status_t wifi_ipc_host_cmd_init(wifi_ipc_t *context, uint32_t addr_freeq)
{
	context->free_q = (void *)addr_freeq;
	return WIFI_IPC_STATUS_OK;
}

wifi_ipc_status_t wifi_ipc_host_event_init(wifi_ipc_t *context, uint32_t addr_freeq)
{
	context->free_q = (void *)addr_freeq;
	return WIFI_IPC_STATUS_OK;
}

wifi_ipc_status_t wifi_ipc_host_cmd_get(wifi_ipc_t *context, uint32_t *data)
{
	return wifi_ipc_freeq_get(context, data);
}

wifi_ipc_status_t wifi_ipc_host_cmd_send(wifi_ipc_t *context, uint32_t *data)
{
	return wifi_ipc_busyq_send(context, data);
}

wifi_ipc_status_t wifi_ipc_host_cmd_send_memcpy(wifi_ipc_t *context, const void *msg,
						size_t len)
{
	int ret;
	uint32_t gdram_addr;

	ret = wifi_ipc_host_cmd_get(context, &gdram_addr);
	if (ret != WIFI_IPC_STATUS_OK) {
		LOG_ERR("Failed to get command location from free queue: %d", ret);
		return ret;
	}

	memcpy((void *)gdram_addr, msg, len);

	return wifi_ipc_host_cmd_send(context, &gdram_addr);
}

wifi_ipc_status_t wifi_ipc_host_tx_send(wifi_ipc_t *context, const void *msg)
{
	return wifi_ipc_host_cmd_send(context, (uint32_t *)&msg);
}
