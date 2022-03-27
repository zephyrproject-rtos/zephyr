/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 * Copyright (c) 2022 Grzegorz Blach
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <drivers/gpio.h>

#ifndef ZEPHYR_DRIVERS_SENSOR_BH1745_H_
#define ZEPHYR_DRIVERS_SENSOR_BH1745_H_

/* Registers */
#define BH1745_SYSTEM_CONTROL                           0x40
#define BH1745_MODE_CONTROL1                            0x41
#define BH1745_MODE_CONTROL2                            0x42
#define BH1745_MODE_CONTROL3                            0x44
#define BH1745_RED_DATA_LSB                             0x50
#define BH1745_RED_DATA_MSB                             0x51
#define BH1745_GREEN_DATA_LSB                           0x52
#define BH1745_GREEN_DATA_MSB                           0x53
#define BH1745_BLUE_DATA_LSB                            0x54
#define BH1745_BLUE_DATA_MSB                            0x55
#define BH1745_LIGHT_DATA_LSB                           0x56
#define BH1745_LIGHT_DATA_MSB                           0x57
#define BH1745_DINT_DATA_LSB                            0x58
#define BH1745_DINT_DATA_MSB                            0x59
#define BH1745_INTERRUPT                                0x60
#define BH1745_PERSISTENCE                              0x61
#define BH1745_TH_HIGH_LSB                              0x62
#define BH1745_TH_HIGH_MSB                              0x63
#define BH1745_TH_LOW_LSB                               0x64
#define BH1745_TH_LOW_MSB                               0x65
#define BH1745_MANUFACTURER_ID                          0x92

/* BH1745_SYSTEM_CONTROL */
#define BH1745_SYSTEM_CONTROL_PART_ID_Msk               GENMASK(5, 0)
#define BH1745_SYSTEM_CONTROL_PART_ID                   0x0B
#define BH1745_SYSTEM_CONTROL_SW_RESET_Msk              BIT(7)
#define BH1745_SYSTEM_CONTROL_SW_RESET                  BIT(7)
#define BH1745_SYSTEM_CONTROL_INT_RESET_Msk             BIT(6)
#define BH1745_SYSTEM_CONTROL_INT_RESET                 BIT(6)

/* BH1745_MODE_CONTROL1 */
/* Measurement mode: 160ms mode */
#define BH1745_MODE_CONTROL1_DEFAULTS                   0x00
#define BH1745_MODE_CONTROL1_MEAS_MODE_Msk              GENMASK(2, 0)
#define BH1745_MODE_CONTROL1_MEAS_MODE_160MS            0x00
#define BH1745_MODE_CONTROL1_MEAS_MODE_320MS            0x01
#define BH1745_MODE_CONTROL1_MEAS_MODE_640MS            0x02
#define BH1745_MODE_CONTROL1_MEAS_MODE_1280MS           0x03
#define BH1745_MODE_CONTROL1_MEAS_MODE_2560MS           0x04
#define BH1745_MODE_CONTROL1_MEAS_MODE_5120MS           0x05

#define BH1745_MODE_CONTROL1_RGB_GAIN_Msk               GENMASK(4, 3)
#define BH1745_MODE_CONTROL1_RGB_GAIN_1X                ((0x01) << 3)
#define BH1745_MODE_CONTROL1_RGB_GAIN_32X               ((0x03) << 3)

#define BH1745_MODE_CONTROL1_LIGHT_GAIN_Msk             GENMASK(6, 5)
#define BH1745_MODE_CONTROL1_LIGHT_GAIN_1X              ((0x01) << 5)
#define BH1745_MODE_CONTROL1_LIGHT_GAIN_32X             ((0x03) << 5)

/* BH1745_MODE_CONTROL2 */
#define BH1745_MODE_CONTROL2_RGB_EN_Msk                BIT(4)
#define BH1745_MODE_CONTROL2_RGB_EN_ENABLE             BIT(4)
#define BH1745_MODE_CONTROL2_RGB_EN_DISABLE            0x00

#define BH1745_MODE_CONTROL2_ADC_GAIN_Msk               GENMASK(1, 0)
#define BH1745_MODE_CONTROL2_ADC_GAIN_1X                0x00
#define BH1745_MODE_CONTROL2_ADC_GAIN_2X                0x01
#define BH1745_MODE_CONTROL2_ADC_GAIN_32X               0x10

#define BH1745_MODE_CONTROL2_VALID_Msk                  BIT(7)

/* BH1745_INTERRUPT */
#define BH1745_INTERRUPT_ENABLE_Msk                     BIT(0)
#define BH1745_INTERRUPT_ENABLE_DISABLE                 0x00
#define BH1745_INTERRUPT_ENABLE_ENABLE                  BIT(0)

#define BH1745_INTERRUPT_LATCH                          BIT(4)

#define BH1745_INTERRUPT_INT_SOURCE_Msk                 GENMASK(3, 2)
#define BH1745_INTERRUPT_INT_SOURCE_RED                 ((0x00) << 2)
#define BH1745_INTERRUPT_INT_SOURCE_GREEN               ((0x01) << 2)
#define BH1745_INTERRUPT_INT_SOURCE_BLUE                ((0x02) << 2)
#define BH1745_INTERRUPT_INT_SOURCE_LIGHT               ((0x03) << 2)

#define BH1745_INTERRUPT_INT_STATUS_Msk                 BIT(7)

/* BH1745_PERSISTENCE */
#define BH1745_PERSISTENCE_PERSISTENCE_Msk              GENMASK(1, 0)
#define BH1745_PERSISTENCE_PERSISTENCE_ACTIVE_END       0x00
#define BH1745_PERSISTENCE_PERSISTENCE_UPDATE_END       0x01
#define BH1745_PERSISTENCE_PERSISTENCE_4_SAMPLES        0x02
#define BH1745_PERSISTENCE_PERSISTENCE_8_SAMPLES        0x03

/* BH1745 RGB/LIGHT SAMPLE POSITIONS  */
#define BH1745_SAMPLE_POS_RED                   0
#define BH1745_SAMPLE_POS_GREEN                 1
#define BH1745_SAMPLE_POS_BLUE                  2
#define BH1745_SAMPLE_POS_LIGHT                 3
#define BH1745_SAMPLES_TO_FETCH                 4

/* Manufacturer ID */
#define BH1745_MANUFACTURER_ID_DEFAULT                  0xE0

enum async_init_step {
	ASYNC_INIT_STEP_RESET_CHECK,
	ASYNC_INIT_RGB_ENABLE,
	ASYNC_INIT_STEP_CONFIGURE,

	ASYNC_INIT_STEP_COUNT
};

struct bh1745_data {
	struct gpio_callback gpio_cb;
	struct k_work_delayable init_work;
	struct k_work work;
	const struct device *dev;
	uint16_t sample_rgb_light[BH1745_SAMPLES_TO_FETCH];
	enum async_init_step async_init_step;
	int err;
	bool ready;

#ifdef CONFIG_BH1745_TRIGGER
	sensor_trigger_handler_t trg_handler;
	struct sensor_trigger trigger;
#endif
};

struct bh1745_config {
	const struct i2c_dt_spec i2c;
	const struct gpio_dt_spec int_gpio;
};

#ifdef CONFIG_BH1745_TRIGGER
int bh1745_attr_set(const struct device *dev,
		    enum sensor_channel chan,
		    enum sensor_attribute attr,
		    const struct sensor_value *val);

int bh1745_trigger_set(const struct device *dev,
		       const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler);

int bh1745_gpio_interrupt_init(const struct device *dev);
#endif  /* CONFIG_BH1745_TRIGGER */

#endif  /* ZEPHYR_DRIVERS_SENSOR_BH1745_H_ */
