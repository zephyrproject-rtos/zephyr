/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __IPC_IF_H__
#define __IPC_IF_H__

#include <zephyr/kernel.h>
#include "ipc_service.h"

typedef enum {
	IPC_INSTANCE_CMD_CTRL = 0,
	IPC_INSTANCE_CMD_TX,
	IPC_INSTANCE_EVT,
	IPC_INSTANCE_RX
} ipc_instances_nrf71_t;

typedef enum {
	IPC_EPT_UMAC = 0,
	IPC_EPT_LMAC
} ipc_epts_nrf71_t;

typedef struct ipc_ctx {
	ipc_instances_nrf71_t inst;
	ipc_epts_nrf71_t ept;
} ipc_ctx_t;

struct rpu_dev {
	int (*init)();
	int (*deinit)(void);
	int (*send)(ipc_ctx_t ctx, const void *data, int len);
	int (*recv)(ipc_ctx_t ctx, void *data, int len);
	int (*register_rx_cb)(int (*rx_handler)(void *priv), void *data);
};

struct rpu_dev *rpu_dev(void);

int ipc_init(void);
int ipc_deinit(void);
int ipc_send(ipc_ctx_t ctx, const void *data, int len);
/* Blocking Receive */
int ipc_recv(ipc_ctx_t ctx, void *data, int len);
/* Non-blocking Receive (global, not per instance) */
int ipc_register_rx_cb(int (*rx_handler)(void *priv), void *data);

#endif /* __IPC_IF_H__ */
