/*
 * Copyright (c) 2018 STMicroelectronics
 *
 * LIS2MDL mag header file
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MAG_LIS2MDL_H
#define __MAG_LIS2MDL_H

#include <gpio.h>
#include <misc/util.h>
#include <i2c.h>

#define SYS_LOG_DOMAIN "LIS2MDL"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_SENSOR_LEVEL
#include <logging/sys_log.h>

#define LIS2MDL_EN_BIT                  1
#define LIS2MDL_DIS_BIT                 0
#define LIS2MDL_I2C_ADDR(__x)           (__x << 1)

#define LIS2MDL_AUTO_INCREMENT          0x80

/* Registers */
#define LIS2MDL_WHO_AM_I_REG            0x4f
#define LIS2MDL_WHOAMI_VAL              0x40

#define LIS2MDL_CFG_REG_A               0x60
/* Device supported modes */
#define LIS2MDL_MD_CONT_MODE            0x00
#define LIS2MDL_MD_SINGLE_MODE          0x01
#define LIS2MDL_MD_IDLE1_MODE           0x02
#define LIS2MDL_MD_IDLE2_MODE           0x03
#define LIS2MDL_MAG_MODE_MASK           0x03
/* Device supported ODRs */
#define LIS2MDL_ODR10_HZ                0x00
#define LIS2MDL_ODR20_HZ                0x04
#define LIS2MDL_ODR50_HZ                0x08
#define LIS2MDL_ODR100_HZ               0x0c
#define LIS2MDL_ODR_MASK                0x0c

#if defined(CONFIG_LIS2MDL_MAG_ODR_10) || \
	defined(CONFIG_LIS2MDL_MAG_ODR_RUNTIME)
	#define LIS2MDL_DEFAULT_ODR       LIS2MDL_ODR10_HZ
#elif defined(CONFIG_LIS2MDL_MAG_ODR_20)
	#define LIS2MDL_DEFAULT_ODR       LIS2MDL_ODR20_HZ
#elif defined(CONFIG_LIS2MDL_MAG_ODR_50)
	#define LIS2MDL_DEFAULT_ODR       LIS2MDL_ODR50_HZ
#elif defined(CONFIG_LIS2MDL_MAG_ODR_100)
	#define LIS2MDL_DEFAULT_ODR       LIS2MDL_ODR100_HZ
#endif

/* Sensors registers */
#define LIS2MDL_OFFSET_X_REG_L          0x45
#define LIS2MDL_OFFSET_X_REG_H          0x46
#define LIS2MDL_OFFSET_Y_REG_L          0x47
#define LIS2MDL_OFFSET_Y_REG_H          0x48
#define LIS2MDL_OFFSET_Z_REG_L          0x49
#define LIS2MDL_OFFSET_Z_REG_H          0x4A
#define LIS2MDL_CFG_REG_B               0x61
#define LIS2MDL_CFG_REG_C               0x62
#define LIS2MDL_INT_CTRL_REG            0x63
#define LIS2MDL_STATUS_REG              0x67
#define LIS2MDL_OUT_REG                 0x68
#define LIS2MDL_TEMP_OUT_L_REG          0x6E
#define LIS2MDL_TEMP_OUT_H_REG          0x6F

/* Registers bitmask */
#define LIS2MDL_STS_MDA_UP              0x08
#define LIS2MDL_REBOOT                  0x40
#define LIS2MDL_SOFT_RST                0x20
#define LIS2MDL_BDU_MASK                0x10
#define LIS2MDL_BDU_BIT                 0x10
#define LIS2MDL_INT_MAG_PIN             0x40
#define LIS2MDL_INT_MAG_PIN_MASK        0x40
#define LIS2MDL_INT_MAG                 0x01
#define LIS2MDL_INT_MAG_MASK            0x01
#define LIS2MDL_MODE_MASK               0x03
#define LIS2MDL_LP_MASK                 0x10
#define LIS2MDL_IEA                     0x04
#define LIS2MDL_IEL                     0x02
#define LIS2MDL_IEN                     0x01
#define LIS2MDL_COMP_TEMP_MASK          0x80

/* Output data size */
#define LIS2MDL_OUT_REG_SIZE            6
#define LIS2MDL_NUM_AXIS                3

/* Define ODR supported range */
#define LIS2MDL_MAX_ODR                 100
#define LIS2MDL_MIN_ODR                 10

/* Sensor sensitivity in uGa/LSB */
#define LIS2MDL_SENSITIVITY             1500

struct lis2mdl_device_config {
	char *master_dev_name;
#ifdef CONFIG_LIS2MDL_TRIGGER
	char *gpio_name;
	u32_t gpio_pin;
#endif  /* CONFIG_LIS2MDL_TRIGGER */
	u16_t i2c_addr_config;
};

/* Sensor data */
struct lis2mdl_data {
	struct device *i2c;
	u16_t i2c_addr;
	s16_t mag[3];
	s32_t temp_sample;

	/* save sensitivity */
	u16_t mag_fs_sensitivity;

#ifdef CONFIG_LIS2MDL_TRIGGER
	struct device *gpio;
	struct gpio_callback gpio_cb;

	sensor_trigger_handler_t handler_drdy;

#if defined(CONFIG_LIS2MDL_TRIGGER_OWN_THREAD)
	K_THREAD_STACK_MEMBER(thread_stack, CONFIG_LIS2MDL_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#elif defined(CONFIG_LIS2MDL_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
	struct device *dev;
#endif  /* CONFIG_LIS2MDL_TRIGGER_GLOBAL_THREAD */
#endif  /* CONFIG_LIS2MDL_TRIGGER */
};

#ifdef CONFIG_LIS2MDL_TRIGGER
int lis2mdl_init_interrupt(struct device *dev);
int lis2mdl_trigger_set(struct device *dev,
			  const struct sensor_trigger *trig,
			  sensor_trigger_handler_t handler);
#endif /* CONFIG_LIS2MDL_TRIGGER */

#endif /* __MAG_LIS2MDL_H */
