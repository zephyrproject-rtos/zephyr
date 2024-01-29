/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef IPC_DISPATCHER_H
#define IPC_DISPATCHER_H

#include <stdint.h>
#include <stddef.h>

/* Data types */

typedef void (*ipc_dispatcher_cb_t)(const void *data, size_t len, void *param);

/* Public APIs */

void ipc_dispatcher_add(uint32_t id, ipc_dispatcher_cb_t cb, void *param);
void ipc_dispatcher_rm(uint32_t id);
int ipc_dispatcher_send(const void *data, size_t len);

#endif /* IPC_DISPATCHER_H */
