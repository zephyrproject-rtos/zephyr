/*
 * Copyright (c) 2026 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_EXCELITAS_TPI1S1385_TPI1S1385_H_
#define ZEPHYR_DRIVERS_SENSOR_EXCELITAS_TPI1S1385_TPI1S1385_H_

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/tpi1s1385.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#ifdef CONFIG_TPI1S1385_TRIGGER
#include <zephyr/drivers/gpio.h>
#endif

/* Register map. */
#define TPI1S1385_REG_GENERAL_CALL		0x00

#define TPI1S1385_REG_TP_OBJECT_MSB		0x01

#define TPI1S1385_REG_TP_PRESENCE		0x0F
#define TPI1S1385_REG_TP_MOTION			0x10

/* Reading this register clears the latched interrupt flags. */
#define TPI1S1385_REG_INTERRUPT_STATUS		0x12

#define TPI1S1385_REG_SLP12			0x14
#define TPI1S1385_REG_SLP3			0x15

#define TPI1S1385_REG_PRESENCE_THRESHOLD	0x16
#define TPI1S1385_REG_MOTION_THRESHOLD		0x17
#define TPI1S1385_REG_AMB_SHOCK_THRESHOLD	0x18

#define TPI1S1385_REG_INTERRUPT_MASK		0x19
#define TPI1S1385_REG_SRC_SELECT		0x1A

#define TPI1S1385_REG_EEPROM_CONTROL		0x1F

/* Calibration constants stored in EEPROM. */
#define TPI1S1385_REG_EEPROM_LOOKUP		41
#define TPI1S1385_REG_EEPROM_PTAT25_MSB		42
#define TPI1S1385_REG_EEPROM_PTAT25_LSB		43
#define TPI1S1385_REG_EEPROM_M_MSB		44
#define TPI1S1385_REG_EEPROM_M_LSB		45
#define TPI1S1385_REG_EEPROM_U0_MSB		46
#define TPI1S1385_REG_EEPROM_U0_LSB		47
#define TPI1S1385_REG_EEPROM_UOUT1_MSB		48
#define TPI1S1385_REG_EEPROM_UOUT1_LSB		49
#define TPI1S1385_REG_EEPROM_TOBJ1		50

/*
 * Bit layout of the 17 bit TP_OBJECT and 15 bit TP_AMBIENT samples inside
 * the raw register bytes 0x01, 0x02, 0x03 and 0x04.
 */
#define TPI1S1385_TP_OBJECT_MSB_SHIFT		9
#define TPI1S1385_TP_OBJECT_MID_SHIFT		1
#define TPI1S1385_TP_OBJECT_LSB_MASK		BIT(7)
#define TPI1S1385_TP_OBJECT_LSB_SHIFT		7
#define TPI1S1385_TP_AMBIENT_MSB_MASK		GENMASK(6, 0)
#define TPI1S1385_TP_AMBIENT_MSB_SHIFT		8

/* PTAT25 is a 15 bit value stored across two registers, bit 7 of the MSB is reserved. */
#define TPI1S1385_EEPROM_PTAT25_MSB_MASK	GENMASK(6, 0)

/* Field layouts for the packed configuration registers. */
#define TPI1S1385_SLP_FIELD_MASK		GENMASK(3, 0)
#define TPI1S1385_SLP2_SHIFT			4
#define TPI1S1385_CYCLE_TIME_MASK		GENMASK(1, 0)
#define TPI1S1385_SRC_SELECT_MASK		GENMASK(1, 0)
#define TPI1S1385_SRC_SELECT_SHIFT		2
/* Set to fire the TPOT interrupt when TPobject exceeds the threshold. */
#define TPI1S1385_TPOT_DIRECTION_EXCEEDS	BIT(4)

/* Interrupt status and interrupt mask bits, valid in registers 0x12 and 0x19. */
#define TPI1S1385_INT_TP_PRESENCE		BIT(3)
#define TPI1S1385_INT_TP_MOTION			BIT(2)

/* Calibration constants read once from the on chip EEPROM at init time. */
struct tpi1s1385_eeprom {
	uint16_t ptat25;
	uint16_t m;
	uint16_t u0;
	uint16_t uout1;
	uint8_t  tobj1;
	uint8_t  lookup;
};

struct tpi1s1385_config {
	struct i2c_dt_spec i2c;
#ifdef CONFIG_TPI1S1385_TRIGGER
	struct gpio_dt_spec int_gpio;
#endif
	uint8_t slp1;
	uint8_t slp2;
	uint8_t slp3;
	uint8_t src_select;
	uint8_t cycle_time;
	bool tpot_direction_falls_below;
	uint8_t presence_threshold;
	uint8_t motion_threshold;
	uint8_t amb_shock_threshold;
};

struct tpi1s1385_data {
	int32_t tp_object;
	int16_t tp_ambient;
	uint8_t tp_presence;
	uint8_t tp_motion;
	uint8_t interrupt_status;

	/*
	 * Serializes I2C transactions, the cached sample state and the
	 * trigger handlers.
	 */
	struct k_mutex lock;

	struct tpi1s1385_eeprom eeprom;

#ifdef CONFIG_TPI1S1385_TRIGGER
	const struct device *dev;
	struct gpio_callback gpio_cb;
	sensor_trigger_handler_t presence_handler;
	const struct sensor_trigger *presence_trig;
	sensor_trigger_handler_t motion_handler;
	const struct sensor_trigger *motion_trig;

	/* Work item that processes the interrupt outside of interrupt context. */
	struct k_work work;
#ifdef CONFIG_TPI1S1385_TRIGGER_OWN_THREAD
	/* Private queue used to run the work item on a thread of our own. */
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_TPI1S1385_THREAD_STACK_SIZE);
	struct k_work_q work_q;
#endif
#endif /* CONFIG_TPI1S1385_TRIGGER */
};

int tpi1s1385_reg_read(const struct device *dev, uint8_t reg, uint8_t *val);
int tpi1s1385_reg_write(const struct device *dev, uint8_t reg, uint8_t val);
int tpi1s1385_reg_update(const struct device *dev, uint8_t reg,
			 uint8_t mask, uint8_t val);

#ifdef CONFIG_TPI1S1385_TRIGGER
int tpi1s1385_trigger_init(const struct device *dev);
int tpi1s1385_trigger_set(const struct device *dev,
			  const struct sensor_trigger *trig,
			  sensor_trigger_handler_t handler);
int tpi1s1385_attr_set(const struct device *dev,
		       enum sensor_channel chan,
		       enum sensor_attribute attr,
		       const struct sensor_value *val);
#endif /* CONFIG_TPI1S1385_TRIGGER */

#endif /* ZEPHYR_DRIVERS_SENSOR_EXCELITAS_TPI1S1385_TPI1S1385_H_ */
