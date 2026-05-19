/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
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

/* Mock IPC initialization and cleanup */
void init_mock_ipc(void);
void deinit_mock_ipc(void);

/* Callback testing support */
void trigger_bound_callback(void);
void trigger_received_callback(const void *data, size_t len);
void trigger_error_callback(const char *error_msg);

void get_stored_message(uint8_t **data, size_t *len);

void clear_stored_ept_cfg(void);
void clear_stored_message(void);

#endif /* MOCK_IPC_H_ */
