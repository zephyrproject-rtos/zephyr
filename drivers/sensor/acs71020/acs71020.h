/*
 *
 * Copyright (c) 2020 SER Consulting LLC, Steven Riedl
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ACS71020_ACS71020_H_
#define ZEPHYR_DRIVERS_SENSOR_ACS71020_ACS71020_H_

#include <errno.h>

#include <zephyr/types.h>
#include <device.h>
#include <drivers/sensor.h>
#include <sys/util.h>
#include <drivers/gpio.h>

#define ACS71020_REG_I_ADJUST   	  0x0B
#define ACS71020_REG_V_RMS_ADJUST     0x0C
#define ACS71020_REG_P_ADJUST         0x0D
#define ACS71020_REG_V_FLAG_ADJUST    0x0E
#define ACS71020_REG_I2C_ADJUST       0x0F
#define ACS71020_REG_S_I_ADJUST       0x1B
#define ACS71020_REG_S_V_RMS_ADJUST   0x1C
#define ACS71020_REG_S_P_ADJUST       0x1D
#define ACS71020_REG_S_V_FLAG_ADJUST  0x1E
#define ACS71020_REG_S_I2C_ADJUST     0x1F
#define ACS71020_REG_IV               0x20
#define ACS71020_REG_P_ACT            0x21
#define ACS71020_REG_P_APP            0x22
#define ACS71020_REG_P_IMAG           0x23
#define ACS71020_REG_P_FACT           0x24
#define ACS71020_REG_IV_SAMP          0x25
#define ACS71020_REG_IV_SEC           0x26
#define ACS71020_REG_IV_MIN           0x27
#define ACS71020_REG_P_SEC            0x28
#define ACS71020_REG_P_MIN            0x29
#define ACS71020_REG_V_CODES          0x2A
#define ACS71020_REG_I_CODES          0x2B
#define ACS71020_REG_P_INSTANT        0x2C
#define ACS71020_REG_STATUS           0x2D
#define ACS71020_REG_U1               0x2E
#define ACS71020_REG_ACCESS           0x2F
#define ACS71020_REG_CUST             0x30
#define ACS71020_REG_U2               0x31

#define ACS71020_VOLTAGE        305
#define ACS71020_CURRENT        30

#define ACS71020_CFG_ALERT_ENA       BIT(0)
#define ACS71020_CFG_ALERT_MODE_INT	BIT(1)
#define ACS71020_CFG_ALERT_HI_LO     BIT(2)
#define ACS71020_CFG_ALERT_RIS_FAL   BIT(3)
#define ACS71020_CFG_ALERT_TH_TC     BIT(4)
#define ACS71020_CFG_INT_CLEAR		BIT(7)

struct acs71020_data {
	const struct device *i2c_master;

    uint16_t qvo_fine;
    uint16_t sns_fine;
    uint16_t crs_sns;
    uint16_t iavgselen;
    uint16_t rms_avg_1;
    uint16_t rms_avg_2;
    uint16_t pacc_trim;
    uint16_t ichan_del_en;
    uint16_t chan_del_sel;
    uint16_t fault;
    uint16_t fltdly;
    uint16_t halfcycle_en;
    uint16_t squarewave_en;
    uint16_t vevent_cycs;
    uint16_t vadc_rate_set;
    uint16_t overvreg;
    uint16_t undervreg;
    uint16_t delaycnt_sel;
    uint16_t i2c_slv_addr;
    uint16_t i2c_dis_slv_addr;
    uint16_t dio_0_sel;
    uint16_t dio_1_sel;
    uint16_t vrms;
    uint16_t irms;
    int32_t pactive;
    uint16_t papparent;
    uint16_t pimag;
    uint16_t pfactor;
    uint16_t numptsout;
    uint16_t vrmsavgonesec;
    uint16_t irmsavgonesec;
    uint16_t vrmsavgonemin;
    uint16_t irmsavgonemin;
    uint32_t pactavgonesec;
    uint32_t pactavgonemin;
    uint32_t vcodes;
    uint32_t icodes;
    uint32_t pinstant;
    uint16_t vzerocrossout;
    uint16_t faultout;
    uint16_t faultlatched;
    uint16_t overvoltage;
    uint16_t undervoltage;
    uint16_t posangle;
    uint16_t pospf;
    uint32_t access_code;
    uint16_t customer_access;

#ifdef CONFIG_ACS71020_TRIGGER
	struct device *alert_gpio;
	struct gpio_callback alert_cb;

	struct device *dev;

	struct sensor_trigger trig;
	sensor_trigger_handler_t trigger_handler;
#endif

#ifdef CONFIG_ACS71020_TRIGGER_OWN_THREAD
	struct k_sem sem;
#endif

#ifdef CONFIG_ACS71020_TRIGGER_GLOBAL_THREAD
	struct k_work work;
#endif
};

struct acs71020_config {
	const char *i2c_bus;
	uint16_t i2c_addr;
#ifdef CONFIG_ACS71020_TRIGGER
	uint8_t alert_pin;
	uint8_t alert_flags;
	const char *alert_controller;
#endif /* CONFIG_ACS71020_TRIGGER */
};

static int acs71020_reg_read(const struct device *dev, uint8_t start, uint8_t *buf, int size);

#ifdef CONFIG_ACS71020_TRIGGER
int acs71020_attr_set(struct device *dev, enum sensor_channel chan,
		     enum sensor_attribute attr,
		     const struct sensor_value *val);
int acs71020_trigger_set(struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler);
int acs71020_setup_interrupt(struct device *dev);
#endif /* CONFIG_ACS71020_TRIGGER */

#endif /* ZEPHYR_DRIVERS_SENSOR_ACS71020_ACS71020_H_ */
