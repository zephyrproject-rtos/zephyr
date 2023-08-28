/*
 * Copyright (c) 2023 Alvaro Garcia Gomez <maxpowel@gmail.com>
 * Copyright (c) 2023 Fraunhofer AICOS
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef ZEPHYR_DRIVERS_SENSOR_MAX17048_MAX17048_H_
#define ZEPHYR_DRIVERS_SENSOR_MAX17048_MAX17048_H_

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/fuel_gauge/max17048.h>

#define REGISTER_VCELL      0x02
#define REGISTER_SOC        0x04
#define REGISTER_MODE       0x06
#define REGISTER_VERSION    0x08
#define REGISTER_HIBRT      0x0A
#define REGISTER_CONFIG     0x0C
#define REGISTER_VALRT      0x14
#define REGISTER_CRATE      0x16
#define REGISTER_VRESET     0x18
#define REGISTER_CHIP_ID    0x19
#define REGISTER_STATUS     0x1A
#define REGISTER_TABLE      0x40
#define REGISTER_COMMAND    0xFE

#define RESET_COMMAND       0x5400
#define QUICKSTART_MODE     0x4000

/* CONFIG register */
#define MAX17048_CONFIG_ALRT  BIT(5)

/* STATUS register */
#define MAX17048_STATUS_RI   BIT(8)
#define MAX17048_STATUS_VH   BIT(9)
#define MAX17048_STATUS_VL   BIT(10)
#define MAX17048_STATUS_VR   BIT(11)
#define MAX17048_STATUS_HD   BIT(12)
#define MAX17048_STATUS_SC   BIT(13)
#define MAX17048_STATUS_EnVr BIT(14)

#define MAX17048_OVERVOLTAGE_THRESHOLD_MAX (0xFF * 20) /* 1LSB = 20mV */
#define MAX17048_SOC_THRESHOLD_MAX 32
#define MAX17048_SOC_THRESHOLD_POR 4

/**
 * Storage for the fuel gauge private data
 */
struct max17048_data {
	/* Charge as percentage */
	uint8_t charge;
	/* Voltage as mV */
	uint16_t voltage;

	/* Time in minutes */
	uint16_t time_to_full;
	uint16_t time_to_empty;
	/* True if battery chargin, false if discharging */
	bool charging;

#ifdef CONFIG_MAX17048_TRIGGER
	const struct device *dev;
	struct gpio_callback gpio_cb;
	struct k_work work;
	max17048_trigger_handler_t trigger_overvoltage_handler;
	max17048_trigger_handler_t trigger_undervoltage_handler;
	max17048_trigger_handler_t trigger_low_soc_handler;
#endif
};

/**
 * Storage for the fuel gauge configuration
 */
struct max17048_config {
	struct i2c_dt_spec i2c;
	struct gpio_dt_spec int_gpio;
	/* Under-voltage threshold in mV */
	uint16_t undervoltage_threshold;
	/* Over-voltage threshold in mV */
	uint16_t overvoltage_threshold;
	/* Low SoC value in % */
	uint8_t low_soc_threshold;
};

int max17048_trigger_init(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_SENSOR_MAX17048_MAX17048_H_ */
