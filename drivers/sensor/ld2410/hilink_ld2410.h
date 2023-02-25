/*
 * Copyright (c) 2023 Fabian Blatz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_HILINK_LD2410_H_
#define ZEPHYR_DRIVERS_SENSOR_HILINK_LD2410_H_

#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/sensor/ld2410.h>
#include <stdint.h>

#define LD2410_MAX_FRAME_BODYLEN 40
#define FRAME_FOOTER_SIZE        sizeof(uint32_t)
#define FRAME_HEADER_SIZE        sizeof(uint32_t)

enum ld2410_frame_type {
	DATA_FRAME = 0,
	ACK_FRAME
};

struct ld2410_frame_data {
	uint32_t header;
	uint16_t body_len;
	uint8_t body[LD2410_MAX_FRAME_BODYLEN + FRAME_FOOTER_SIZE];
} __packed;

struct ld2410_frame {
	size_t byte_count;
	union {
		struct ld2410_frame_data data;
		uint8_t raw[sizeof(struct ld2410_frame_data)];
	};
} __packed;

struct ld2410_cyclic_data {
	uint8_t data_type;
	uint8_t header_byte;
	uint8_t target_type;
	uint16_t motion_target_distance;
	uint8_t motion_target_energy;
	uint16_t stationary_target_distance;
	uint8_t stationary_target_energy;
	uint16_t detection_distance;
} __packed;

struct ld2410_engineering_data {
	uint8_t max_motion_gate;
	uint8_t max_stationary_gate;
	uint8_t motion_energy_per_gate[LD2410_GATE_COUNT];
	uint8_t stationary_energy_per_gate[LD2410_GATE_COUNT];
	uint8_t max_motion_energy;
	uint8_t max_stationary_energy;
} __packed;

struct ld2410_settings {
	uint8_t maximum_distance_gate;
	uint8_t max_motion_gate;
	uint8_t max_stationary_gate;
	uint8_t motion_gate_sensitivity[LD2410_GATE_COUNT];
	uint8_t stationary_gate_sensitivity[LD2410_GATE_COUNT];
	uint16_t presence_timeout;
} __packed;

struct ld2410_config {
	const struct device *uart_dev;
#ifdef CONFIG_LD2410_TRIGGER
	struct gpio_dt_spec int_gpios;
#endif /* CONFIG_LD2410_TRIGGER */

	bool engineering_mode;
	enum ld2410_gate_resolution distance_resolution;
};

struct ld2410_data {
	enum ld2410_frame_type awaited_rx_frame_type;
	struct ld2410_frame rx_frame;
	struct ld2410_frame tx_frame;

	struct k_sem tx_sem;
	struct k_sem rx_sem;
	struct k_mutex lock;

	struct ld2410_cyclic_data cyclic_data;
	struct ld2410_engineering_data engineering_data;
	struct ld2410_settings settings;

#ifdef CONFIG_LD2410_TRIGGER
	const struct device *gpio_dev;
	struct gpio_callback gpio_cb;

	sensor_trigger_handler_t th_handler;
	const struct sensor_trigger *th_trigger;
#if defined(CONFIG_LD2410_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_LD2410_THREAD_STACK_SIZE);
	struct k_sem gpio_sem;
	struct k_thread thread;
#elif defined(CONFIG_LD2410_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif
#endif /* CONFIG_LD2410_TRIGGER */
};

#ifdef CONFIG_LD2410_TRIGGER
int ld2410_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler);

int ld2410_init_interrupt(const struct device *dev);
#endif /* CONFIG_LD2410_TRIGGER */

#endif /* ZEPHYR_DRIVERS_SENSOR_HILINK_LD2410_H_ */
