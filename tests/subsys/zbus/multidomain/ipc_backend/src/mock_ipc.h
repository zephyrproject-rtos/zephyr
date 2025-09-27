/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MOCK_IPC_H_
#define MOCK_IPC_H_

#include <zephyr/device.h>
#include <zephyr/ipc/ipc_service_backend.h>
#include <zephyr/fff.h>

DECLARE_FAKE_VALUE_FUNC1(int, fake_ipc_open_instance, const struct device *);
DECLARE_FAKE_VALUE_FUNC1(int, fake_ipc_close_instance, const struct device *);
DECLARE_FAKE_VALUE_FUNC4(int, fake_ipc_send, const struct device *, void *, const void *, size_t);
DECLARE_FAKE_VALUE_FUNC3(int, fake_ipc_register_endpoint, const struct device *, void **,
			 const struct ipc_ept_cfg *);
DECLARE_FAKE_VALUE_FUNC2(int, fake_ipc_deregister_endpoint, const struct device *, void *);

extern const struct ipc_service_backend fake_backend_ops;

/* Callback testing support */
void trigger_bound_callback(void);
void trigger_unbound_callback(void);
void trigger_received_callback(const void *data, size_t len);
void trigger_error_callback(const char *error_msg);
void clear_stored_ept_cfg(void);

/* Bound callback verification */
bool was_bound_callback_triggered(void);
void reset_bound_callback_flag(void);

#endif /* MOCK_IPC_H_ */
