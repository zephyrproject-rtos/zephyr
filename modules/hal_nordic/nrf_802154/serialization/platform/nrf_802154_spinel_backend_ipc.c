/*
 * Copyright (c) 2020 - 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <ipc/rpmsg_service.h>
#include <device.h>
#include <logging/log.h>

#include "nrf_802154_spinel_backend_callouts.h"
#include "nrf_802154_serialization_error.h"
#include "../../spinel_base/spinel.h"

#define LOG_LEVEL LOG_LEVEL_INFO
#define LOG_MODULE_NAME spinel_ipc_backend
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#define IPC_MASTER IS_ENABLED(CONFIG_RPMSG_SERVICE_MODE_MASTER)

static K_SEM_DEFINE(ready_sem, 0, 1);
static int endpoint_id;

static int endpoint_cb(struct rpmsg_endpoint *ept,
		       void                  *data,
		       size_t                 len,
		       uint32_t               src,
		       void                  *priv)
{
	LOG_DBG("Received message of %u bytes.", len);

	if (len) {
		nrf_802154_spinel_encoded_packet_received(data, len);
	}

	return RPMSG_SUCCESS;
}

nrf_802154_ser_err_t nrf_802154_backend_init(void)
{
#if IPC_MASTER
	while (!rpmsg_service_endpoint_is_bound(endpoint_id)) {
		k_sleep(K_MSEC(1));
	}
#endif
	return NRF_802154_SERIALIZATION_ERROR_OK;
}

/* Make sure we register endpoint before RPMsg Service is initialized. */
static int register_endpoint(const struct device *arg)
{
	int status;

	status = rpmsg_service_register_endpoint("nrf_spinel", endpoint_cb);

	if (status < 0) {
		LOG_ERR("Registering endpoint failed with %d", status);
		return status;
	}

	endpoint_id = status;
	return 0;
}

SYS_INIT(register_endpoint, POST_KERNEL, CONFIG_RPMSG_SERVICE_EP_REG_PRIORITY);

/* Send packet thread details */
#define RING_BUFFER_LEN 4
#define SEND_THREAD_STACK_SIZE 1024

static K_SEM_DEFINE(send_sem, 0, RING_BUFFER_LEN);
K_THREAD_STACK_DEFINE(send_thread_stack, SEND_THREAD_STACK_SIZE);
struct k_thread send_thread_data;

struct ringbuffer {
	uint32_t len;
	uint8_t data[SPINEL_FRAME_BUFFER_SIZE];
};

static struct ringbuffer ring_buffer[RING_BUFFER_LEN];
static uint8_t rd_idx;
static uint8_t wr_idx;

static uint8_t get_rb_idx_plus_1(uint8_t i)
{
	return (i + 1) % RING_BUFFER_LEN;
}

static nrf_802154_ser_err_t spinel_packet_from_thread_send(const uint8_t *data, uint32_t len)
{
	if (get_rb_idx_plus_1(wr_idx) == rd_idx) {
		LOG_ERR("No spinel buffer available to send a new packet");
		return NRF_802154_SERIALIZATION_ERROR_BACKEND_FAILURE;
	}

	LOG_DBG("Scheduling %u bytes for send thread", len);

	struct ringbuffer *buf = &ring_buffer[wr_idx];

	wr_idx = get_rb_idx_plus_1(wr_idx);
	buf->len = len;
	memcpy(buf->data, data, len);

	k_sem_give(&send_sem);
	return (nrf_802154_ser_err_t)len;
}

static void spinel_packet_send_thread_fn(void *arg1, void *arg2, void *arg3)
{
	LOG_DBG("Spinel backend send thread started");
	while (true) {
		k_sem_take(&send_sem, K_FOREVER);
		struct ringbuffer *buf = &ring_buffer[rd_idx];
		uint32_t expected_ret = buf->len;

		LOG_DBG("Sending %u bytes from send thread", buf->len);
		int ret = rpmsg_service_send(endpoint_id, buf->data, buf->len);

		rd_idx = get_rb_idx_plus_1(rd_idx);

		if (ret != expected_ret) {
			nrf_802154_ser_err_data_t err = {
				.reason = NRF_802154_SERIALIZATION_ERROR_BACKEND_FAILURE,
			};

			nrf_802154_serialization_error(&err);
		}
	}
}

K_THREAD_DEFINE(spinel_packet_send_thread, SEND_THREAD_STACK_SIZE,
		spinel_packet_send_thread_fn, NULL, NULL, NULL, K_PRIO_COOP(0), 0, 0);

nrf_802154_ser_err_t nrf_802154_spinel_encoded_packet_send(const void *p_data,
							   size_t      data_len)
{
	if (k_is_in_isr()) {
		return spinel_packet_from_thread_send(p_data, data_len);
	}

	LOG_DBG("Sending %u bytes directly", data_len);
	int ret = rpmsg_service_send(endpoint_id, p_data, data_len);

	return ((ret < 0) ? NRF_802154_SERIALIZATION_ERROR_BACKEND_FAILURE
			  : (nrf_802154_ser_err_t) ret);
}
