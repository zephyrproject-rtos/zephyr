/*
 * Copyright 2024 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MFD_ADP5585_H_
#define ZEPHYR_INCLUDE_DRIVERS_MFD_ADP5585_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/device.h>
#define ADP5585_ID                 0x00
#define ADP5585_INT_STATUS         0x01
#define ADP5585_STATUS             0x02
#define ADP5585_FIFO_1             0x03
#define ADP5585_FIFO_2             0x04
#define ADP5585_FIFO_3             0x05
#define ADP5585_FIFO_4             0x06
#define ADP5585_FIFO_5             0x07
#define ADP5585_FIFO_6             0x08
#define ADP5585_FIFO_7             0x09
#define ADP5585_FIFO_8             0x0A
#define ADP5585_FIFO_9             0x0B
#define ADP5585_FIFO_10            0x0C
#define ADP5585_FIFO_11            0x0D
#define ADP5585_FIFO_12            0x0E
#define ADP5585_FIFO_13            0x0F
#define ADP5585_FIFO_14            0x10
#define ADP5585_FIFO_15            0x11
#define ADP5585_FIFO_16            0x12
#define ADP5585_GPI_INT_STAT_A     0x13
#define ADP5585_GPI_INT_STAT_B     0x14
#define ADP5585_GPI_STATUS_A       0x15
#define ADP5585_GPI_STATUS_B       0x16
#define ADP5585_RPULL_CONFIG_A     0x17
#define ADP5585_RPULL_CONFIG_B     0x18
#define ADP5585_RPULL_CONFIG_C     0x19
#define ADP5585_RPULL_CONFIG_D     0x1A
#define ADP5585_GPI_INT_LEVEL_A    0x1B
#define ADP5585_GPI_INT_LEVEL_B    0x1C
#define ADP5585_GPI_EVENT_EN_A     0x1D
#define ADP5585_GPI_EVENT_EN_B     0x1E
#define ADP5585_GPI_INTERRUPT_EN_A 0x1F
#define ADP5585_GPI_INTERRUPT_EN_B 0x20
#define ADP5585_DEBOUNCE_DIS_A     0x21
#define ADP5585_DEBOUNCE_DIS_B     0x22
#define ADP5585_GPO_DATA_OUT_A     0x23
#define ADP5585_GPO_DATA_OUT_B     0x24
#define ADP5585_GPO_OUT_MODE_A     0x25
#define ADP5585_GPO_OUT_MODE_B     0x26
#define ADP5585_GPIO_DIRECTION_A   0x27
#define ADP5585_GPIO_DIRECTION_B   0x28
#define ADP5585_RESET1_EVENT_A     0x29
#define ADP5585_RESET1_EVENT_B     0x2A
#define ADP5585_RESET1_EVENT_C     0x2B
#define ADP5585_RESET2_EVENT_A     0x2C
#define ADP5585_RESET2_EVENT_B     0x2D
#define ADP5585_RESET_CFG          0x2E
#define ADP5585_PWM_OFFT_LOW       0x2F
#define ADP5585_PWM_OFFT_HIGH      0x30
#define ADP5585_PWM_ONT_LOW        0x31
#define ADP5585_PWM_ONT_HIGH       0x32
#define ADP5585_PWM_CFG            0x33
#define ADP5585_LOGIC_CFG          0x34
#define ADP5585_LOGIC_FF_CFG       0x35
#define ADP5585_LOGIC_INT_EVENT_EN 0x36
#define ADP5585_POLL_PTIME_CFG     0x37
#define ADP5585_PIN_CONFIG_A       0x38
#define ADP5585_PIN_CONFIG_B       0x39
#define ADP5585_PIN_CONFIG_C       0x3A
#define ADP5585_GENERAL_CFG        0x3B
#define ADP5585_INT_EN             0x3C

/* ID Register */
#define ADP5585_DEVICE_ID_MASK 0xF
#define ADP5585_MAN_ID_MASK    0xF
#define ADP5585_MAN_ID_SHIFT   4
#define ADP5585_MAN_ID         0x02

#define ADP5585_PWM_CFG_EN         0x1
#define ADP5585_PWM_CFG_MODE       0x2
#define ADP5585_PIN_CONFIG_R3_PWM  0x8
#define ADP5585_PIN_CONFIG_R3_MASK 0xC
#define ADP5585_GENERAL_CFG_OSC_EN 0x80

/* INT_EN and INT_STATUS Register */
#define ADP5585_INT_EVENT    (1U << 0)
#define ADP5585_INT_GPI      (1U << 1)
#define ADP5585_INT_OVERFLOW (1U << 2)
#define ADP5585_INT_LOGIC    (1U << 4)

#define ADP5585_REG_MASK 0xFF

struct mfd_adp5585_config {
	struct gpio_dt_spec reset_gpio;
	struct gpio_dt_spec nint_gpio;
	struct i2c_dt_spec i2c_bus;
};

struct mfd_adp5585_data {
	struct k_work work;
	struct k_sem lock;
	const struct device *dev;
	struct {
#ifdef CONFIG_GPIO_ADP5585
		const struct device *gpio_dev;
#endif /* CONFIG_GPIO_ADP5585 */
	} child;
	struct gpio_callback int_gpio_cb;
};

/**
 * @brief Forward declaration of child device interrupt
 *      handler
 */
#ifdef CONFIG_GPIO_ADP5585
void gpio_adp5585_irq_handler(const struct device *dev);
#endif /* CONFIG_GPIO_ADP5585 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MFD_AD5952_H_ */
