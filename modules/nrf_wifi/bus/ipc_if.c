/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief File containing API definitions for the
 * IPC bus layer of the nRF71 Wi-Fi driver.
 */
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(wifi_nrf_bus, CONFIG_WIFI_NRF70_BUSLIB_LOG_LEVEL);

#include "ipc_if.h"
#include "bal_structs.h"
#include "qspi.h"
#include "common/hal_structs_common.h"

/* Define addresses to use for the free queues */
#define EVENT_FREEQ_ADDR 0x200C2000
#define CMD_FREEQ_ADDR   0x200C3000

#define NUM_INSTANCES 3
#define NUM_ENDPOINTS 1

struct device *ipc_instances[NUM_INSTANCES];
struct ipc_ept ept[NUM_ENDPOINTS];
struct ipc_ept_cfg ept_cfg[NUM_ENDPOINTS];

static wifi_ipc_t wifi_event;
static wifi_ipc_t wifi_cmd;
static wifi_ipc_t wifi_tx;

static int (*callback_func)(void *data);

static void event_recv(void *data, void *priv)
{
	struct nrf_wifi_bus_qspi_dev_ctx *dev_ctx = NULL;
	struct nrf_wifi_bal_dev_ctx *bal_dev_ctx = NULL;
	struct nrf_wifi_hal_dev_ctx *hal_dev_ctx = NULL;

	dev_ctx = (struct nrf_wifi_bus_qspi_dev_ctx *)priv;
	bal_dev_ctx = (struct nrf_wifi_bal_dev_ctx *)dev_ctx->bal_dev_ctx;
	hal_dev_ctx = (struct nrf_wifi_hal_dev_ctx *)bal_dev_ctx->hal_dev_ctx;
	LOG_DBG("Event IPC received");

	hal_dev_ctx->ipc_msg = data;
	callback_func(priv);
	LOG_DBG("Event IPC callback completed");
}

int ipc_init(void)
{
	wifi_ipc_host_event_init(&wifi_event, EVENT_FREEQ_ADDR);
	LOG_DBG("Event IPC initialized");
	wifi_ipc_host_cmd_init(&wifi_cmd, CMD_FREEQ_ADDR);
	LOG_DBG("Command IPC initialized");
	return 0;
}

int ipc_deinit(void)
{
	return 0;
}

int ipc_recv(ipc_ctx_t ctx, void *data, int len)
{
	return 0;
}

int ipc_send(ipc_ctx_t ctx, const void *data, int len)
{

	int ret = 0;

	switch (ctx.inst) {
	case IPC_INSTANCE_CMD_CTRL:
		/* IPC service on RPU may not have been established. Keep trying. */
		do {
			ret = wifi_ipc_host_cmd_send_memcpy(&wifi_cmd, data, len);
		} while (ret == WIFI_IPC_STATUS_BUSYQ_NOTREADY);

		/* Critical error during IPC service transfer. Should never happen. */
		if (ret == WIFI_IPC_STATUS_BUSYQ_CRITICAL_ERR) {
			LOG_ERR("Critical error during IPC CMD busyq transfer");
			return -1;
		}
		break;
	case IPC_INSTANCE_CMD_TX:
		/* IPC service on RPU may not have been established. Keep trying. */
		do {
			ret = wifi_ipc_host_tx_send(&wifi_tx, data);
		} while (ret == WIFI_IPC_STATUS_BUSYQ_NOTREADY);

		/* Critical error during IPC service transfer. Should never happen. */
		if (ret == WIFI_IPC_STATUS_BUSYQ_CRITICAL_ERR) {
			LOG_ERR("Critical error during IPC TX busyq transfer");
			return -1;
		}
		break;
	case IPC_INSTANCE_RX:
		break;
	default:
		break;
	}

	LOG_DBG("IPC send completed: %d", ret);

	return ret;
}

int ipc_register_rx_cb(int (*rx_handler)(void *priv), void *data)
{
	int ret;

	callback_func = rx_handler;

	ret = wifi_ipc_bind_ipc_service_tx_rx(&wifi_cmd, &wifi_event,
					      DEVICE_DT_GET(DT_NODELABEL(ipc0)), event_recv, data);
	if (ret != WIFI_IPC_STATUS_OK) {
		LOG_ERR("Failed to bind IPC service: %d", ret);
		return -1;
	}

	ret = wifi_ipc_bind_ipc_service(&wifi_tx, DEVICE_DT_GET(DT_NODELABEL(ipc1)), event_recv,
					data);
	if (ret != WIFI_IPC_STATUS_OK) {
		LOG_ERR("Failed to bind IPC service: %d", ret);
		return -1;
	}

	return 0;
}
