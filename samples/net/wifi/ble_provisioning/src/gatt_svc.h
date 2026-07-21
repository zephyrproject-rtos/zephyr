/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef GATT_SVC_H_
#define GATT_SVC_H_

#include <stddef.h>
#include <stdint.h>

#define GATT_SVC_CTRL_CONNECT 0x01
#define GATT_SVC_CTRL_ERASE   0x02

enum gatt_svc_status {
	GATT_SVC_STATUS_IDLE = 0,
	GATT_SVC_STATUS_PROVISIONED,
	GATT_SVC_STATUS_CONNECTING,
	GATT_SVC_STATUS_CONNECTED,
	GATT_SVC_STATUS_FAILED,
};

struct gatt_svc_callbacks {
	void (*ssid_written)(const uint8_t *data, size_t len);
	void (*password_written)(const uint8_t *data, size_t len);
	void (*security_written)(uint8_t security);
	void (*control_written)(uint8_t cmd);
};

int gatt_svc_init(const struct gatt_svc_callbacks *cb);

void gatt_svc_set_status(enum gatt_svc_status status);

int gatt_svc_adv_start(void);
int gatt_svc_adv_stop(void);

#endif
