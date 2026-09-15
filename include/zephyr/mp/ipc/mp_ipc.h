/*
 * Copyright (c) 2026 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_MP_PLUGINS_MP_IPC_H_
#define ZEPHYR_INCLUDE_MP_PLUGINS_MP_IPC_H_

#include <zephyr/mp/core/mp_sink.h>
#include <zephyr/mp/core/mp_src.h>
#include <zephyr/ipc/ipc_service.h>

#define MP_IPC_MAX_BUFFERS 16

/**
 * @defgroup mp_ipc IPC Plugin
 * @ingroup mp_plugins
 * @{
 */

/**
 * @struct mp_ipc_sink
 * IPC Sink element structure.
 */
struct mp_ipc_sink {
	struct mp_sink base;
	struct ipc_ept ept;
	bool bound;
	char endpoint_name[32];
	
	/* Hold references to buffers until remote releases them */
	struct net_buf *pending_buffers[MP_IPC_MAX_BUFFERS];

	/* Semaphore for synchronous queries */
	struct k_sem query_sem;
	
	/* Buffer for holding query response */
	struct mp_structure *last_query_resp;
	bool last_query_success;
};

#define MP_IPC_SINK(obj) ((struct mp_ipc_sink *)(obj))

/**
 * Initialize an IPC Sink element.
 *
 * @param self Pointer to the mp_element to initialize as an IPC sink
 */
void mp_ipc_sink_init(struct mp_element *self);

/**
 * @struct mp_ipc_src
 * IPC Source element structure.
 */
struct mp_ipc_src {
	struct mp_src base;
	struct ipc_ept ept;
	bool bound;
	char endpoint_name[32];

	/* Semaphore for synchronous queries */
	struct k_sem query_sem;
	
	/* Buffer for holding query response */
	struct mp_structure *last_query_resp;
	bool last_query_success;
};

#define MP_IPC_SRC(obj) ((struct mp_ipc_src *)(obj))

/**
 * Initialize an IPC Source element.
 *
 * @param self Pointer to the mp_element to initialize as an IPC source
 */
void mp_ipc_src_init(struct mp_element *self);

/**
 * Translate a local virtual memory pointer to a physical bus address
 */
uintptr_t mp_ipc_virt_to_phys(void *virt_addr);

/**
 * Translate a physical bus address back to a local virtual pointer
 */
void *mp_ipc_phys_to_virt(uintptr_t phys_addr);

/** @} */

#endif /* ZEPHYR_INCLUDE_MP_PLUGINS_MP_IPC_H_ */
