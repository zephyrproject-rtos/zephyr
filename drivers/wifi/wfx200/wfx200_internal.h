/**
 * Copyright (c) 2022 grandcentrix GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ZEPHYR_DRIVERS_WIFI_SILABS_WFX200_INTERNAL_H_
#define __ZEPHYR_DRIVERS_WIFI_SILABS_WFX200_INTERNAL_H_

#define DT_DRV_COMPAT silabs_wfx200

#include <stdint.h>
#include <stdbool.h>

#include <device.h>
#include <drivers/gpio.h>
#include <drivers/spi.h>
#include <net/ethernet.h>
#include <net/wifi_mgmt.h>

#include <sl_wfx_constants.h>

#define INST_HAS_HIF_SEL_OR(inst) DT_INST_NODE_HAS_PROP(inst, hif_sel_gpios) ||
#define ANY_INST_HAS_HIF_SEL_GPIOS DT_INST_FOREACH_STATUS_OKAY(INST_HAS_HIF_SEL_OR) 0

#define INST_HAS_WAKE_OR(inst) DT_INST_NODE_HAS_PROP(inst, wake_gpios) ||
#define ANY_INST_HAS_WAKE_GPIOS DT_INST_FOREACH_STATUS_OKAY(INST_HAS_WAKE_OR) 0

struct wfx200_config {
	const struct spi_dt_spec bus;

	const struct gpio_dt_spec interrupt;
	const struct gpio_dt_spec reset;

#if ANY_INST_HAS_WAKE_GPIOS
	const struct gpio_dt_spec *wakeup;
#endif
#if ANY_INST_HAS_HIF_SEL_GPIOS
	const struct gpio_dt_spec *hif_select;
#endif
	const char *const *const pds;
	const size_t pds_length;
};

enum wfx200_event {
	WFX200_CONNECT_EVENT = 0,
	WFX200_CONNECT_FAILED_EVENT,
	WFX200_DISCONNECT_EVENT,
	WFX200_AP_START_EVENT,
	WFX200_AP_START_FAILED_EVENT,
	WFX200_AP_STOP_EVENT,
};

struct wfx200_queue_event {
	enum wfx200_event ev;
};

enum wfx200_state {
	WFX200_STATE_IDLE = 0,
	WFX200_STATE_INTERFACE_INITIALIZED,
	WFX200_STATE_AP_MODE,
	WFX200_STATE_STA_MODE,
};

static inline char *wfx200_state_to_str(enum wfx200_state state)
{
	switch (state) {
	case WFX200_STATE_IDLE:
		return "WFX200_STATE_IDLE";
	case WFX200_STATE_INTERFACE_INITIALIZED:
		return "WFX200_STATE_INTERFACE_INITIALIZED";
	case WFX200_STATE_AP_MODE:
		return "WFX200_STATE_AP_MODE";
	case WFX200_STATE_STA_MODE:
		return "WFX200_STATE_STA_MODE";
	default:
		return "unknown state";
	}
}

struct wfx200_data {
	struct net_if *iface;
	const struct device *dev;

	size_t firmware_pos;

	struct k_sem wakeup_sem;
	struct k_sem event_sem;

	struct k_mutex bus_mutex;
	struct k_mutex event_mutex;

	uint8_t waited_event_id;

	struct gpio_callback int_cb;

	scan_result_cb_t scan_cb;

	sl_wfx_context_t sl_context;

	enum wfx200_state state;

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

void wfx200_enable_interrupt(struct wfx200_data *context);
void wfx200_disable_interrupt(struct wfx200_data *context);

void wfx200_event_thread(void *p1, void *p2, void *p3);

#endif /* __ZEPHYR_SILABS_WFX200_INTERNAL_H_ */
