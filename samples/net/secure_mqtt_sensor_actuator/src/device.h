/*
 * Copyright (c) 2024 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __DEVICE_H__
#define __DEVICE_H__

/** @brief Sensor sample structure */
struct sensor_sample {
	const char *unit;
	int value;
};

/** @brief Available board LEDs */
enum led_id {
	LED_NET, /* Network status LED*/
	LED_USER /* User LED */
};

/** @brief LED states */
enum led_state {
	LED_OFF,
	LED_ON
};

/** @brief Device command consisting of a command string
 *         and associated handler function pointer
 */
struct device_cmd {
	const char *command;
	void (*handler)();
};

/**
 *  @brief Check if all devices are ready
 */
bool devices_ready(void);

/**
 *  @brief Read sample from the sensor
 */
int device_read_sensor(struct sensor_sample *sample);

/**
 *  @brief Write to a board LED
 */
int device_write_led(enum led_id led_idx, enum led_state state);

/**
 *  @brief Handler function for commands received over MQTT
 */
void device_command_handler(uint8_t *command);

#endif /* __DEVICE_H__ */
