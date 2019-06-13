/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <gpio.h>

#ifndef ZEPHYR_DRIVERS_SENSOR_BH1749_H_
#define ZEPHYR_DRIVERS_SENSOR_BH1749_H_

/* Registers */
#define BH1749_SYSTEM_CONTROL                           0x40
#define BH1749_MODE_CONTROL1                            0x41
#define BH1749_MODE_CONTROL2                            0x42
#define BH1749_RED_DATA_LSB                             0x50
#define BH1749_RED_DATA_MSB                             0x51
#define BH1749_GREEN_DATA_LSB                           0x52
#define BH1749_GREEN_DATA_MSB                           0x53
#define BH1749_BLUE_DATA_LSB                            0x54
#define BH1749_BLUE_DATA_MSB                            0x55
#define BH1749_IR_DATA_LSB                              0x58
#define BH1749_IR_DATA_MSB                              0x59
#define BH1749_GREEN2_DATA_LSB                          0x5A
#define BH1749_GREEN2_DATA_MSB                          0x5B
#define BH1749_INTERRUPT                                0x60
#define BH1749_PERSISTENCE                              0x61
#define BH1749_TH_HIGH_LSB                              0x62
#define BH1749_TH_HIGH_MSB                              0x63
#define BH1749_TH_LOW_LSB                               0x64
#define BH1749_TH_LOW_MSB                               0x65
#define BH1749_MANUFACTURER_ID                          0x92

/* BH1749_SYSTEM_CONTROL */
#define BH1749_SYSTEM_CONTROL_PART_ID_Msk               GENMASK(5, 0)
#define BH1749_SYSTEM_CONTROL_PART_ID                   0x0D
#define BH1749_SYSTEM_CONTROL_SW_RESET_Msk              BIT(6)
#define BH1749_SYSTEM_CONTROL_SW_RESET                  BIT(6)
#define BH1749_SYSTEM_CONTROL_INT_RESET_Msk             BIT(7)
#define BH1749_SYSTEM_CONTROL_INT_RESET                 BIT(7)

/* BH1749_MODE_CONTROL1 */
#define BH1749_MODE_CONTROL1_MEAS_MODE_Msk              GENMASK(2, 0)
#define BH1749_MODE_CONTROL1_MEAS_MODE_120MS            0x02
#define BH1749_MODE_CONTROL1_MEAS_MODE_240MS            0x03
#define BH1749_MODE_CONTROL1_MEAS_MODE_35MS             0x05

#define BH1749_MODE_CONTROL1_RGB_GAIN_Msk               GENMASK(4, 3)
#define BH1749_MODE_CONTROL1_RGB_GAIN_1X                ((0x01) << 3)
#define BH1749_MODE_CONTROL1_RGB_GAIN_32X               ((0x03) << 3)

#define BH1749_MODE_CONTROL1_IR_GAIN_Msk                GENMASK(6, 5)
#define BH1749_MODE_CONTROL1_IR_GAIN_1X                 ((0x01) << 5)
#define BH1749_MODE_CONTROL1_IR_GAIN_32X                ((0x03) << 5)

/* BH1749_MODE_CONTROL2 */
#define BH1749_MODE_CONTROL2_RGB_EN_Msk                 BIT(4)
#define BH1749_MODE_CONTROL2_RGB_EN_ENABLE              BIT(4)
#define BH1749_MODE_CONTROL2_RGB_EN_DISABLE             0x00

#define BH1749_MODE_CONTROL2_VALID_Msk                  BIT(7)

/* BH1749_INTERRUPT */
#define BH1749_INTERRUPT_ENABLE_Msk                     BIT(0)
#define BH1749_INTERRUPT_ENABLE_DISABLE                 0x00
#define BH1749_INTERRUPT_ENABLE_ENABLE                  BIT(0)

#define BH1749_INTERRUPT_LATCH                          BIT(4)

#define BH1749_INTERRUPT_INT_SOURCE_Msk                 GENMASK(3, 2)
#define BH1749_INTERRUPT_INT_SOURCE_RED                 ((0x00) << 2)
#define BH1749_INTERRUPT_INT_SOURCE_GREEN               ((0x01) << 2)
#define BH1749_INTERRUPT_INT_SOURCE_BLUE                ((0x02) << 2)

#define BH1749_INTERRUPT_INT_STATUS_Msk                 BIT(7)

/* BH1749_PERSISTENCE */
#define BH1749_PERSISTENCE_PERSISTENCE_Msk              GENMASK(1, 0)
#define BH1749_PERSISTENCE_PERSISTENCE_ACTIVE_END       0x00
#define BH1749_PERSISTENCE_PERSISTENCE_UPDATE_END       0x01
#define BH1749_PERSISTENCE_PERSISTENCE_4_SAMPLES        0x02
#define BH1749_PERSISTENCE_PERSISTENCE_8_SAMPLES        0x03

/* BH1749 RGB/IR SAMPLE POSITIONS  */
#define BH1749_RGB_BYTE_POS_RED                         0
#define BH1749_RGB_BYTE_POS_GREEN                       1
#define BH1749_RGB_BYTE_POS_BLUE                        2
#define BH1749_RGB_BYTE_POS_IR                          4

/* Manufacturer ID */
#define BH1749_MANUFACTURER_ID_DEFAULT                  0xE0

struct bh1749_data {
	struct device *i2c;
	struct device *gpio;
	struct gpio_callback gpio_cb;
	struct k_work work;
	struct device *dev;
	u16_t sample_rgb_ir[5];
	u8_t pdata;

#ifdef CONFIG_BH1749_TRIGGER
	sensor_trigger_handler_t trg_handler;
	struct sensor_trigger trigger;
	struct k_sem data_sem;
#endif
};

#ifdef CONFIG_BH1749_TRIGGER
void bh1749_work_cb(struct k_work *work);

int bh1749_attr_set(struct device *dev,
		    enum sensor_channel chan,
		    enum sensor_attribute attr,
		    const struct sensor_value *val);

int bh1749_trigger_set(struct device *dev,
		       const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler);

int bh1749_gpio_interrupt_init(struct device *dev);
#endif  /* CONFIG_BH1749_TRIGGER */

#endif  /* ZEPHYR_DRIVERS_SENSOR_BH1749_H_ */
