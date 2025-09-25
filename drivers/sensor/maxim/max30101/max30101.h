/*
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>

#define MAX30101_REG_INT_STS1		0x00
#define MAX30101_REG_INT_STS2		0x01
#define MAX30101_REG_INT_EN1		0x02
#define MAX30101_REG_INT_EN2		0x03
#define MAX30101_REG_FIFO_WR		0x04
#define MAX30101_REG_FIFO_OVF		0x05
#define MAX30101_REG_FIFO_RD		0x06
#define MAX30101_REG_FIFO_DATA		0x07
#define MAX30101_REG_FIFO_CFG		0x08
#define MAX30101_REG_MODE_CFG		0x09
#define MAX30101_REG_SPO2_CFG		0x0a
#define MAX30101_REG_LED1_PA		0x0c
#define MAX30101_REG_LED2_PA		0x0d
#define MAX30101_REG_LED3_PA		0x0e
#define MAX30101_REG_LED4_PA            0x0f
#define MAX30101_REG_PILOT_PA		0x10
#define MAX30101_REG_MULTI_LED		0x11
#define MAX30101_REG_TINT		0x1f
#define MAX30101_REG_TFRAC		0x20
#define MAX30101_REG_TEMP_CFG		0x21
#define MAX30101_REG_PROX_INT		0x30
#define MAX30101_REG_REV_ID		0xfe
#define MAX30101_REG_PART_ID		0xff

#define MAX30101_INT_PPG_MASK		(1 << 6)

#define MAX30101_FIFO_CFG_SMP_AVE_SHIFT		5
#define MAX30101_FIFO_CFG_ROLLOVER_EN_SHIFT     4
#define MAX30101_FIFO_CFG_FIFO_FULL_SHIFT	0
#define MAX30101_FIFO_CFG_ROLLOVER_EN_MASK	(1 << 4)

#define MAX30101_MODE_CFG_SHDN_MASK	(1 << 7)
#define MAX30101_MODE_CFG_RESET_MASK	(1 << 6)

#define MAX30101_SPO2_ADC_RGE_SHIFT	5
#define MAX30101_SPO2_SR_SHIFT		2
#define MAX30101_SPO2_PW_SHIFT		0

#define MAX30101_PART_ID		0x15

#define MAX30101_BYTES_PER_CHANNEL	3
#define MAX30101_MAX_NUM_CHANNELS	3
#define MAX30101_MAX_BYTES_PER_SAMPLE	(MAX30101_MAX_NUM_CHANNELS * \
					 MAX30101_BYTES_PER_CHANNEL)

#define MAX30101_SLOT_LED_MASK		0x03

#define MAX30101_FIFO_DATA_BITS		18
#define MAX30101_FIFO_DATA_MASK		((1 << MAX30101_FIFO_DATA_BITS) - 1)

enum max30101_mode {
	MAX30101_MODE_HEART_RATE = 2,
	MAX30101_MODE_SPO2 = 3,
	MAX30101_MODE_MULTI_LED = 7,
};

static const uint8_t max30101_mode_convert[3] = {7, 2, 3};

enum max30101_slot {
	MAX30101_SLOT_DISABLED = 0,
	MAX30101_SLOT_RED_LED = 1,
	MAX30101_SLOT_IR_LED = 2,
	MAX30101_SLOT_GREEN_LED = 3,
};

#define MAX30101_HR_SLOTS                                                                          \
	{MAX30101_SLOT_RED_LED, MAX30101_SLOT_DISABLED, MAX30101_SLOT_DISABLED,                    \
	 MAX30101_SLOT_DISABLED}

#define MAX30101_SPO2_SLOTS                                                                        \
	{MAX30101_SLOT_RED_LED, MAX30101_SLOT_IR_LED, MAX30101_SLOT_DISABLED,                      \
	 MAX30101_SLOT_DISABLED}

#define MAX30101_MULTI_LED(n)                                                                      \
	{                                                                                          \
		DT_INST_PROP_BY_IDX(n, led_slot, 0),                                               \
		DT_INST_PROP_BY_IDX(n, led_slot, 1),                                               \
		DT_INST_PROP_BY_IDX(n, led_slot, 2),                                               \
		DT_INST_PROP_BY_IDX(n, led_slot, 3),                                               \
	}

enum max30101_led_channel {
	MAX30101_LED_CHANNEL_RED = 0,
	MAX30101_LED_CHANNEL_IR = 1,
	MAX30101_LED_CHANNEL_GREEN = 2,
};

struct max30101_config {
	struct i2c_dt_spec i2c;
	uint8_t fifo;
	uint8_t spo2;
	uint8_t led_pa[MAX30101_MAX_NUM_CHANNELS];
	uint8_t mode;
	uint8_t slot[4];
};

struct max30101_data {
	uint32_t raw[MAX30101_MAX_NUM_CHANNELS];
	uint8_t map[MAX30101_MAX_NUM_CHANNELS];
	uint8_t num_channels;
};
