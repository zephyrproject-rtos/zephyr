/* veml7700.h - driver for high accuracy ALS */
/*
 * Copyright (c) 2022 innblue
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef VEML7700_H__
#define VEML7700_H__

#include <device.h>
#include <zephyr/types.h>
#include <drivers/sensor.h>
#include <drivers/gpio.h>

/** Registers */
#define VEML7700_ALS_CONFIG 0x00 ///< Light configuration register
#define VEML7700_ALS_THREHOLD_HIGH 0x01 ///< Light high threshold for irq
#define VEML7700_ALS_THREHOLD_LOW 0x02 ///< Light low threshold for irq
#define VEML7700_ALS_POWER_SAVE 0x03 ///< Power save regiester
#define VEML7700_ALS_DATA 0x04 ///< The light data output
#define VEML7700_WHITE_DATA 0x05 ///< The white light data output
#define VEML7700_INTERRUPTSTATUS 0x06 ///< What IRQ (if any)

/** REGISTER #0 */
enum veml7700_gain {
	VEML7700_GAIN_1 = 0x00, ///< ALS gain 1x
	VEML7700_GAIN_2 = 0x01, ///< ALS gain 2x
	VEML7700_GAIN_1_8 = 0x02, ///< ALS gain 1/8x
	VEML7700_GAIN_1_4 = 0x03 ///< ALS gain 1/4x
};

enum veml7700_it {
	VEML7700_IT_100MS = 0x00, ///< ALS integration time 100ms
	VEML7700_IT_200MS = 0x01, ///< ALS integration time 200ms
	VEML7700_IT_400MS = 0x02, ///< ALS integration time 400ms
	VEML7700_IT_800MS = 0x03, ///< ALS integration time 800ms
	VEML7700_IT_50MS = 0x08, ///< ALS integration time 50ms
	VEML7700_IT_25MS = 0x0C ///< ALS integration time 25ms
};

enum veml7700_pers {
	VEML7700_PERS_1 = 0x00, ///< ALS irq persistance protect 1 sample
	VEML7700_PERS_2 = 0x01, ///< ALS irq persistance protect 2 samples
	VEML7700_PERS_4 = 0x02, ///< ALS irq persistance protect 4 samples
	VEML7700_PERS_8 = 0x03 ///< ALS irq persistance protect 8 samples

};

/**
 * @brief  CONFIGURATION REGISTER #0
 * 	Register address = 00h
 * 	The command code #0 is for configuration of the ambient light measurements.
 *
 * @note
 * 	Light level [lx] is (ALS OUTPUT DATA [dec.] / ALS Gain x responsivity).
 * 	Please study also the application note
 */
#define VEMEL7700_GAIN_MASK(X) (X << 11)
#define VEMEL7700_CLEAR_GAIN_MASK ~VEMEL7700_GAIN_MASK(0b11)
#define VEMEL7700_SET_GAIN(X, VAL) (X | VEMEL7700_GAIN_MASK(VAL))
#define VEMEL7700_CLEAR_GAIN(X) (X & VEMEL7700_CLEAR_GAIN_MASK)

#define VEMEL7700_IT_MASK(X) (X << 6)
#define VEMEL7700_CLEAR_IT_MASK ~VEMEL7700_IT_MASK(0b1111)
#define VEMEL7700_SET_IT(X, VAL) (X | VEMEL7700_IT_MASK(VAL))
#define VEMEL7700_CLEAR_IT(X) (X & VEMEL7700_CLEAR_IT_MASK)

#define VEMEL7700_PERSISTENCE_MASK(X) (X << 4)
#define VEMEL7700_CLEAR_PERSISTENCE_MASK ~VEMEL7700_PERSISTENCE_MASK(0b11)
#define VEMEL7700_SET_PERSISTENCE_PROTECT(X, VAL) (X | VEMEL7700_PERSISTENCE_MASK(VAL))
#define VEMEL7700_CLEAR_PERSISTENCE_PROTECT(X) (X & ~VEMEL7700_CLEAR_PERSISTENCE_MASK)

#define VEMEL7700_DISABLE_INT_MASK 0xFFFD
#define VEMEL7700_ENABLE_INT(X) (X | ~VEMEL7700_DISABLE_INT_MASK)
#define VEMEL7700_DISABLE_INT(X) (X & VEMEL7700_DISABLE_INT_MASK)

#define VEMEL7700_TURN_ON_MASK 0xFFFE
#define VEMEL7700_TURN_ON(X) (X & VEMEL7700_TURN_ON_MASK)
#define VEMEL7700_SHUT_DOWN(X) (X | ~VEMEL7700_TURN_ON_MASK)

#define VEMEL7700_GET_GAIN(X) ((X >> 11) & 0b11)
#define VEMEL7700_GET_IT(X) ((X >> 6) & 0b1111)
#define VEMEL7700_GET_PERSISTENCE_PROTECT(X) ((X >> 4) & 0b11)
#define VEMEL7700_GET_INT(X) ((X >> 1) & 0b1)
#define VEMEL7700_GET_SD(X) (X & 0b1)

/** REGISTER #3 */
enum veml7700_psm {
	VEML7700_POWERSAVE_MODE1 = 0x00, ///< Power saving mode 1
	VEML7700_POWERSAVE_MODE2 = 0x01, ///< Power saving mode 2
	VEML7700_POWERSAVE_MODE3 = 0x02, ///< Power saving mode 3
	VEML7700_POWERSAVE_MODE4 = 0x03 ///< Power saving mode 4

};

union veml7700_reg3 {
	struct {
		/*
		 * Power saving mode enable setting
		 * 	0 = disable
		 * 	1 = enable
		 */
		uint8_t psm_en : 1;
		enum veml7700_psm psm : 2;
		uint8_t rfu_1 : 5;
		uint8_t rfu_2;
	} reg3_bits;

	uint8_t reg3_val[2];
};

/** REGISTER #6 */
/**
 * @brief
 * 	Command code address = 06h.
 * 	Bit 15 defines interrupt flag while trigger occurred due to data
 * 		crossing low threshold windows.
 * 	Bit 14 defines interrupt flag while trigger occurred due to data
 * 		crossing high threshold windows.
 */
enum veml7700_interrupt {
	VEML7700_INTERRUPT_HIGH = 0x4000, ///< Interrupt status for high threshold
	VEML7700_INTERRUPT_LOW = 0x8000 ///< Interrupt status for low threshold
};

struct veml7700_data {
	const struct device *i2c;
	uint16_t i2c_addr;
#ifdef CONFIG_VEML7700_TRIGGER
	const struct device *int_gpio;
	uint8_t int_gpio_pin;
	struct gpio_callback int_gpio_cb;

	struct sensor_trigger trig_als_thrs;
	sensor_trigger_handler_t handler_als_thrs;
#endif

#ifdef CONFIG_VEML7700_TRIGGER_OWN_THREAD
	struct k_sem sem;
#endif

#ifdef CONFIG_VEML7700_TRIGGER_GLOBAL_THREAD
	struct k_work work;
#endif

	/* Sensor runtime configuration */
	enum veml7700_it als_it;
	enum veml7700_gain als_gain;

	/* Calculated sensor values. */
	uint16_t als;
	uint16_t white_channel;
};

struct veml7700_config {
	const char *i2c_name;
	uint8_t i2c_addr;
#ifdef CONFIG_VEML7700_TRIGGER
	const char *int_gpio;
	uint8_t int_gpio_pin;
	uint8_t int_gpio_flags;
#endif
};

#ifdef CONFIG_VEML7700_TRIGGER
int veml7700_interrupt_init(const struct device *dev);
int veml7700_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler);
#endif

#endif /* VEML7700_H__ */
