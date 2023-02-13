/*
 * Copyright (c) 2020 - 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ipc/ipc_service.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>

#include "nrf_802154_spinel_backend_callouts.h"
#include "nrf_802154_serialization_error.h"
#include "../../spinel_base/spinel.h"
#include "../../src/include/nrf_802154_spinel.h"

#define LOG_LEVEL LOG_LEVEL_INFO
#define LOG_MODULE_NAME spinel_ipc_backend
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#define IPC_BOUND_TIMEOUT_IN_MS K_MSEC(1000)

static K_SEM_DEFINE(edp_bound_sem, 0, 1);
static struct ipc_ept ept;

static void endpoint_bound(void *priv)
{
	k_sem_give(&edp_bound_sem);
}

static void endpoint_received(const void *data, size_t len, void *priv)
{
	LOG_DBG("Received message of %u bytes.", len);

	nrf_802154_spinel_encoded_packet_received(data, len);
}

static struct ipc_ept_cfg ept_cfg = {
	.name = "nrf_802154_spinel",
	.cb = {
		.bound = endpoint_bound,
		.received = endpoint_received
	},
};

nrf_802154_ser_err_t nrf_802154_backend_init(void)
{
	const struct device *const ipc_instance =
		DEVICE_DT_GET(DT_CHOSEN(nordic_802154_spinel_ipc));
	int err;

	err = ipc_service_open_instance(ipc_instance);
	if (err < 0 && err != -EALREADY) {
		LOG_ERR("Failed to open IPC instance: %d", err);
		return NRF_802154_SERIALIZATION_ERROR_INIT_FAILED;
	}

	err = ipc_service_register_endpoint(ipc_instance, &ept, &ept_cfg);
	if (err < 0) {
		LOG_ERR("Failed to register IPC endpoint: %d", err);
		return NRF_802154_SERIALIZATION_ERROR_INIT_FAILED;
	}

	err = k_sem_take(&edp_bound_sem, IPC_BOUND_TIMEOUT_IN_MS);
	if (err < 0) {
		LOG_ERR("IPC endpoint bind timed out");
		return NRF_802154_SERIALIZATION_ERROR_INIT_FAILED;
	}

	return NRF_802154_SERIALIZATION_ERROR_OK;
}

/* Send packet thread details */
#define RING_BUFFER_LEN 16
#define SEND_THREAD_STACK_SIZE 1024

static K_SEM_DEFINE(send_sem, 0, RING_BUFFER_LEN);
K_THREAD_STACK_DEFINE(send_thread_stack, SEND_THREAD_STACK_SIZE);
struct k_thread send_thread_data;

struct ringbuffer {
	uint32_t len;
	uint8_t data[NRF_802154_SPINEL_FRAME_MAX_SIZE];
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
		int ret = ipc_service_send(&ept, buf->data, buf->len);

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
	int ret = ipc_service_send(&ept, p_data, data_len);

	return ((ret < 0) ? NRF_802154_SERIALIZATION_ERROR_BACKEND_FAILURE
			  : (nrf_802154_ser_err_t) ret);
}
