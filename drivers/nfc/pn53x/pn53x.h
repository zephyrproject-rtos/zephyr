/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_NFC_PN53X_PN53X_H_
#define ZEPHYR_DRIVERS_NFC_PN53X_PN53X_H_

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/nfc.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>

#define PN53X_FRAME_MAXLEN 262U
struct pn53x_transport_api {
	int (*write)(const struct device *dev, const uint8_t *buf, size_t len);
	int (*read)(const struct device *dev, uint8_t *buf, size_t len);
	int (*wait_ready)(const struct device *dev, k_timeout_t timeout);
};

struct pn53x_config {
	const struct pn53x_transport_api *transport;
	const void *bus;
	struct gpio_dt_spec reset_gpio;
	struct gpio_dt_spec irq_gpio;
	k_thread_stack_t *poll_stack;
	size_t poll_stack_size;
};

struct pn53x_data {
	const struct device *dev;
	struct k_mutex lock;
	struct k_thread poll_thread;
	struct k_sem poll_kick;
	struct k_sem irq_sem;
	struct gpio_callback irq_cb;
	atomic_t polling;
	nfc_proto_t poll_protos;
	nfc_target_cb_t target_cb;
	void *target_ud;
	uint8_t buf[PN53X_FRAME_MAXLEN];
	uint8_t xchg_cmd[PN53X_FRAME_MAXLEN];
	uint8_t xchg_resp[PN53X_FRAME_MAXLEN];
};

extern const struct nfc_driver_api pn53x_nfc_api;

int pn53x_init_common(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_NFC_PN53X_PN53X_H_ */
