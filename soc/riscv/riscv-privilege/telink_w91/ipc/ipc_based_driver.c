/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "ipc_based_driver.h"
#include <zephyr/logging/log.h>

#define LOG_LEVEL LOG_LEVEL_INFO
#define LOG_MODULE_NAME ipc_based_driver
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

static uint32_t ipc_events;

void ipc_based_driver_init(struct ipc_based_driver *drv)
{
	(void)k_mutex_init(&drv->mutex);
	(void)k_sem_init(&drv->resp_sem, 0, 1);
}

int ipc_based_driver_send(struct ipc_based_driver *drv,
	const void *tx_data, size_t tx_len,
	struct ipc_based_driver_ctx *ctx, uint32_t timeout_ms)
{
	int result = 0;

	struct ipc_based_driver_response_data {
		struct k_sem *resp_sem;
		struct ipc_based_driver_ctx *ctx;
	} response_data = {
		.resp_sem = &drv->resp_sem,
		.ctx = ctx,
	};

void resp_cb(const void *data, size_t len, void *param)
{
	struct ipc_based_driver_response_data *response_data =
		(struct ipc_based_driver_response_data *)param;

	if (response_data->ctx && response_data->ctx->unpack) {
		response_data->ctx->unpack(response_data->ctx->result, data, len);
	}
	k_sem_give(response_data->resp_sem);
}

	k_mutex_lock(&drv->mutex, K_FOREVER);
	uint32_t id = *(uint32_t *)tx_data;

	k_sem_reset(&drv->resp_sem);
	ipc_dispatcher_add(id, resp_cb, &response_data);

	ipc_events++;

	do {
		if (ipc_dispatcher_send(tx_data, tx_len) != tx_len) {
			LOG_ERR("IPC data send error (id=%x)", id);
			result = -ECANCELED;
			break;
		}

		if (k_sem_take(&drv->resp_sem, K_MSEC(timeout_ms))) {
			LOG_ERR("IPC response timeout (id=%x)", id);
			result = -ETIMEDOUT;
			break;
		}
	} while (0);

	ipc_events--;

	ipc_dispatcher_rm(id);
	k_mutex_unlock(&drv->mutex);

	return result;
}

uint32_t ipc_based_driver_get_ipc_events(void)
{
	return ipc_events;
}
