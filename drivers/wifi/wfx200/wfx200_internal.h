/**
 * Copyright (c) 2023 grandcentrix GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ZEPHYR_DRIVERS_WIFI_SILABS_WFX200_INTERNAL_H_
#define __ZEPHYR_DRIVERS_WIFI_SILABS_WFX200_INTERNAL_H_

#define DT_DRV_COMPAT silabs_wfx200

#include <stdint.h>
#include <stdbool.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/wifi_mgmt.h>

#include <sl_wfx_constants.h>

#define HAS_HIF_SEL_GPIO DT_INST_NODE_HAS_PROP(0, hif_sel_gpios)
#define HAS_WAKE_GPIO    DT_INST_NODE_HAS_PROP(0, wake_gpios)

struct wfx200_config {
	const struct spi_dt_spec bus;

	const struct gpio_dt_spec interrupt;
	const struct gpio_dt_spec reset;

#if HAS_WAKE_GPIO
	const struct gpio_dt_spec wakeup;
#endif
#if HAS_HIF_SEL_GPIO
	const struct gpio_dt_spec hif_select;
#endif
	const char *const *const pds;
	const size_t pds_length;
};

enum wfx200_event {
	WFX200_CONNECT_EVENT = 0,
	WFX200_CONNECT_FAILED_EVENT,
	WFX200_DISCONNECT_EVENT,
};

struct wfx200_queue_event {
	enum wfx200_event ev;
};

struct wfx200_data {
	const struct device *dev;

	struct net_if *iface;

	size_t firmware_pos;

	struct k_sem event_sem;

	struct k_mutex bus_mutex;
	struct k_mutex event_mutex;

	uint8_t waited_event_id;

	struct gpio_callback int_cb;

	scan_result_cb_t scan_cb;

	sl_wfx_context_t sl_context;

	bool sta_enabled;

	struct k_work_q incoming_work_q;
	struct k_work incoming_work;

	struct k_queue event_queue;
	struct k_thread event_thread;

	K_KERNEL_STACK_MEMBER(wfx200_stack_area, CONFIG_WIFI_WFX200_STACK_SIZE);
	K_KERNEL_STACK_MEMBER(wfx200_event_stack_area, CONFIG_WIFI_WFX200_EVENT_STACK_SIZE);

	struct k_heap heap;
	char heap_buffer[CONFIG_WIFI_WFX200_HEAP_SIZE];
};

BUILD_ASSERT(CONFIG_WIFI_WFX200_HEAP_SIZE >= 1508, "WFX200 heap must be at least 1508 bytes");

extern const uint8_t __sl_wfx_firmware_start;
extern const uint8_t __sl_wfx_firmware_end;

void wfx200_enable_interrupt(struct wfx200_data *context);
void wfx200_disable_interrupt(struct wfx200_data *context);

void wfx200_event_thread(void *p1, void *p2, void *p3);

#endif /* __ZEPHYR_SILABS_WFX200_INTERNAL_H_ */
