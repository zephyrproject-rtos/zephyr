/*
 * Copyright (c) 2023 Kurtis Dinelle
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_TSL2591_TSL2591_H_
#define ZEPHYR_DRIVERS_SENSOR_TSL2591_TSL2591_H_

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor/tsl2591.h>

/* Device Identification */
#define TSL2591_DEV_ID 0x50

/* Command: CMD:7 | TRANSACTION:6:5 | ADDR/SF:4:0 */
#define TSL2591_NORMAL_CMD    (BIT(7) | BIT(5))
#define TSL2591_SPECIAL_CMD   (BIT(7) | BIT(6) | BIT(5))
#define TSL2591_CLEAR_INT_CMD (TSL2591_SPECIAL_CMD | 0x7)

/* Enable: (0x00): NPIEN:7 | SAI:6 | Reserved:5 | AIEN:4 | Reserved:3:2 | AEN:1 | PON:0 */
#define TSL2591_POWER_MASK (BIT(1) | BIT(0))
#define TSL2591_POWER_ON   (BIT(1) | BIT(0))
#define TSL2591_POWER_OFF  (0)
#define TSL2591_AEN_MASK   (BIT(1))
#define TSL2591_AEN_ON     (BIT(1))
#define TSL2591_AEN_OFF    (0)
#define TSL2591_AIEN_MASK  (BIT(4))
#define TSL2591_AIEN_ON    (BIT(4))
#define TSL2591_AIEN_OFF   (0)

/* Config/Control: (0x01): SRESET:7 | Reserved:6 | AGAIN:5:4 | Reserved:3 | ATIME:2:0 */
#define TSL2591_SRESET     (BIT(7))
#define TSL2591_AGAIN_MASK (BIT(5) | BIT(4))
#define TSL2591_ATIME_MASK (BIT(2) | BIT(1) | BIT(0))

/* Status: (0x13): Reserved:7:6 | NPINTR:5 | AINT:4 | Reserved:3:1 | AVALID:0 */
#define TSL2591_AVALID_MASK (BIT(0))

/* Register Addresses */
#define TSL2591_REG_ENABLE  0x00
#define TSL2591_REG_CONFIG  0x01
#define TSL2591_REG_AILTL   0x04
#define TSL2591_REG_AILTH   0x05
#define TSL2591_REG_AIHTL   0x06
#define TSL2591_REG_AIHTH   0x07
#define TSL2591_REG_NPAILTL 0x08
#define TSL2591_REG_NPAILTH 0x09
#define TSL2591_REG_NPAIHTL 0x0A
#define TSL2591_REG_NPAIHTH 0x0B
#define TSL2591_REG_PERSIST 0x0C
#define TSL2591_REG_PID     0x11
#define TSL2591_REG_ID      0x12
#define TSL2591_REG_STATUS  0x13
#define TSL2591_REG_C0DATAL 0x14
#define TSL2591_REG_C0DATAH 0x15
#define TSL2591_REG_C1DATAL 0x16
#define TSL2591_REG_C1DATAH 0x17

/* Integration Time Modes */
#define TSL2591_INTEGRATION_100MS 0x00
#define TSL2591_INTEGRATION_200MS 0x01
#define TSL2591_INTEGRATION_300MS 0x02
#define TSL2591_INTEGRATION_400MS 0x03
#define TSL2591_INTEGRATION_500MS 0x04
#define TSL2591_INTEGRATION_600MS 0x05

/* Gain Modes */
#define TSL2591_GAIN_MODE_LOW  0x00
#define TSL2591_GAIN_MODE_MED  0x10
#define TSL2591_GAIN_MODE_HIGH 0x20
#define TSL2591_GAIN_MODE_MAX  0x30

/* Gain Scales (Typical Values)
 * See datasheet, used only for lux calculation.
 */
#define TSL2591_GAIN_SCALE_LOW  1U
#define TSL2591_GAIN_SCALE_MED  25U
#define TSL2591_GAIN_SCALE_HIGH 400U
#define TSL2591_GAIN_SCALE_MAX  9200U

/* Persistence Filters */
#define TSL2591_PERSIST_EVERY 0x00
#define TSL2591_PERSIST_1     0x01
#define TSL2591_PERSIST_2     0x02
#define TSL2591_PERSIST_3     0x03
#define TSL2591_PERSIST_5     0x04
#define TSL2591_PERSIST_10    0x05
#define TSL2591_PERSIST_15    0x06
#define TSL2591_PERSIST_20    0x07
#define TSL2591_PERSIST_25    0x08
#define TSL2591_PERSIST_30    0x09
#define TSL2591_PERSIST_35    0x0A
#define TSL2591_PERSIST_40    0x0B
#define TSL2591_PERSIST_45    0x0C
#define TSL2591_PERSIST_50    0x0D
#define TSL2591_PERSIST_55    0x0E
#define TSL2591_PERSIST_60    0x0F

/* Device factor coefficient for lux calculations */
#define TSL2591_LUX_DF 408

/* Max integration time (in ms) for single step */
#define TSL2591_MAX_TIME_STEP 105

/* Max ADC Counts */
#define TSL2591_MAX_ADC     65535
#define TSL2591_MAX_ADC_100 36863

struct tsl2591_config {
	const struct i2c_dt_spec i2c;
#ifdef CONFIG_TSL2591_TRIGGER
	const struct gpio_dt_spec int_gpio;
#endif
};

struct tsl2591_data {
	uint16_t vis_count;
	uint16_t ir_count;
	uint16_t again;
	uint16_t atime;
	bool powered_on;

#ifdef CONFIG_TSL2591_TRIGGER
	const struct device *dev;
	struct gpio_callback gpio_cb;
	sensor_trigger_handler_t th_handler;
	const struct sensor_trigger *th_trigger;

#if defined(CONFIG_TSL2591_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_TSL2591_THREAD_STACK_SIZE);
	struct k_sem trig_sem;
	struct k_thread thread;
#elif defined(CONFIG_TSL2591_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif

#endif
};

int tsl2591_reg_update(const struct device *dev, uint8_t reg, uint8_t mask, uint8_t val);

#ifdef CONFIG_TSL2591_TRIGGER
int tsl2591_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler);
int tsl2591_initialize_int(const struct device *dev);
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_TSL2591_TSL2591_H_ */
