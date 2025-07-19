/*
 * Copyright (c) 2025, Nathan Winslow <natelostintimeandspace@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_FUELGAUGE_LTC2959_GAUGE_H_
#define ZEPHYR_DRIVERS_FUELGAUGE_LTC2959_GAUGE_H_

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/fuel_gauge.h>

/* LTC2959 Register Map — from datasheet (Rev A) */

/* Status and Control */
#define LTC2959_REG_STATUS                  (0x00)
#define LTC2959_REG_ADC_CONTROL             (0x01)
#define LTC2959_REG_CC_CONTROL              (0x02)

/* Accumulated Charge (uint32_t) */
#define LTC2959_REG_ACC_CHARGE_3            (0x03)
#define LTC2959_REG_ACC_CHARGE_2            (0x04)
#define LTC2959_REG_ACC_CHARGE_1            (0x05)
#define LTC2959_REG_ACC_CHARGE_0            (0x06)

/* Charge Thresholds (Low and High, uint32_t each) */
#define LTC2959_REG_CHG_THRESH_LOW_3        (0x07)
#define LTC2959_REG_CHG_THRESH_LOW_2        (0x08)
#define LTC2959_REG_CHG_THRESH_LOW_1        (0x09)
#define LTC2959_REG_CHG_THRESH_LOW_0        (0x0A)

#define LTC2959_REG_CHG_THRESH_HIGH_3       (0x0B)
#define LTC2959_REG_CHG_THRESH_HIGH_2       (0x0C)
#define LTC2959_REG_CHG_THRESH_HIGH_1       (0x0D)
#define LTC2959_REG_CHG_THRESH_HIGH_0       (0x0E)

/* Voltage (uint16_t) */
#define LTC2959_REG_VOLTAGE_MSB             (0x0F)
#define LTC2959_REG_VOLTAGE_LSB             (0x10)
#define LTC2959_REG_VOLT_THRESH_HIGH_MSB    (0x11)
#define LTC2959_REG_VOLT_THRESH_HIGH_LSB    (0x12)
#define LTC2959_REG_VOLT_THRESH_LOW_MSB     (0x13)
#define LTC2959_REG_VOLT_THRESH_LOW_LSB     (0x14)

#define LTC2959_REG_MAX_VOLTAGE_MSB         (0x15)
#define LTC2959_REG_MAX_VOLTAGE_LSB         (0x16)
#define LTC2959_REG_MIN_VOLTAGE_MSB         (0x17)
#define LTC2959_REG_MIN_VOLTAGE_LSB         (0x18)

/* Current (int16_t) */
#define LTC2959_REG_CURRENT_MSB             (0x19)
#define LTC2959_REG_CURRENT_LSB             (0x1A)

#define LTC2959_REG_CURR_THRESH_HIGH_MSB    (0x1B)
#define LTC2959_REG_CURR_THRESH_HIGH_LSB    (0x1C)

#define LTC2959_REG_CURR_THRESH_LOW_MSB     (0x1D)
#define LTC2959_REG_CURR_THRESH_LOW_LSB     (0x1E)

#define LTC2959_REG_MAX_CURRENT_MSB         (0x1F)
#define LTC2959_REG_MAX_CURRENT_LSB         (0x20)
#define LTC2959_REG_MIN_CURRENT_MSB         (0x21)
#define LTC2959_REG_MIN_CURRENT_LSB         (0x22)

/* Temperature (uint16_t) */
#define LTC2959_REG_TEMP_MSB                (0x23)
#define LTC2959_REG_TEMP_LSB                (0x24)

#define LTC2959_REG_TEMP_THRESH_HIGH_MSB    (0x25)
#define LTC2959_REG_TEMP_THRESH_HIGH_LSB    (0x26)

#define LTC2959_REG_TEMP_THRESH_LOW_MSB     (0x27)
#define LTC2959_REG_TEMP_THRESH_LOW_LSB     (0x28)

/* GPIO */
#define LTC2959_REG_GPIO_VOLTAGE_MSB        (0x29)
#define LTC2959_REG_GPIO_VOLTAGE_LSB        (0x2A)

#define LTC2959_REG_GPIO_THRESH_HIGH_MSB    (0x2B)
#define LTC2959_REG_GPIO_THRESH_HIGH_LSB    (0x2C)

#define LTC2959_REG_GPIO_THRESH_LOW_MSB     (0x2D)
#define LTC2959_REG_GPIO_THRESH_LOW_LSB     (0x2E)

/* STATUS Register Bit Definitions (0x00) */
#define LTC2959_STATUS_GPIO_ALERT         BIT(7)
#define LTC2959_STATUS_CURRENT_ALERT      BIT(6)
#define LTC2959_STATUS_CHARGE_OVER_UNDER  BIT(5)
#define LTC2959_STATUS_TEMP_ALERT         BIT(4)
#define LTC2959_STATUS_CHARGE_HIGH        BIT(3)
#define LTC2959_STATUS_CHARGE_LOW         BIT(2)
#define LTC2959_STATUS_VOLTAGE_ALERT      BIT(1)
#define LTC2959_STATUS_UVLO               BIT(0)

/* CONTROL Register Bit Masks */
#define LTC2959_CTRL_ADC_MODE_MASK      GENMASK(7, 5)
#define LTC2959_CTRL_GPIO_MODE_MASK     GENMASK(4, 3)
#define LTC2959_CTRL_VIN_SEL_BIT        BIT(2)
#define LTC2959_CTRL_RESERVED_MASK      GENMASK(1, 0)

/* CONTROL Register Values for ADC Mode */
#define LTC2959_CTRL_VIN_VDD            (0x0 << 2)
#define LTC2959_CTRL_GPIO_ALERT         (0x0 << 3)

/* ADC mode values (bits 7:5 of CONTROL register 0x01) */
#define LTC2959_ADC_MODE_SLEEP         (0x0 << 5)
#define LTC2959_ADC_MODE_SMART_SLEEP   (0x1 << 5)
#define LTC2959_ADC_MODE_CONT_V        (0x2 << 5)
#define LTC2959_ADC_MODE_CONT_I        (0x3 << 5)
#define LTC2959_ADC_MODE_CONT_VI       (0x4 << 5)
#define LTC2959_ADC_MODE_SINGLE_SHOT   (0x5 << 5)
#define LTC2959_ADC_MODE_CONT_VIT      (0x6 << 5)  // recommended for full telemetry

/* GPIO mode bits (bits 4:3) */
#define LTC2959_GPIO_MODE_ALERT       (0x00)
#define LTC2959_GPIO_MODE_CHGCOMP     (0x08)
#define LTC2959_GPIO_MODE_BIPOLAR     (0x10)
#define LTC2959_GPIO_MODE_UNIPOLAR    (0x18)

/* Voltage source selection (bit 2) */
#define LTC2959_VIN_VDD                (0x0 << 2)
#define LTC2959_VIN_SENSEN             (0x1 << 2)

/* CC Control bits (bits 7:5 of CC register 0x02)*/
#define LTC2959_CC_ACCUM_CLR_BIT        BIT(7)
#define LTC2959_CC_THRESH_EN_BIT        BIT(6)
#define LTC2959_CC_HOLD_BIT             BIT(5)
#define LTC2959_CC_ENABLE_THRESH        LTC2959_CC_THRESH_EN_BIT
#define LTC2959_CC_HOLD                 LTC2959_CC_HOLD_BIT
#define LTC2959_CC_CLEAR                LTC2959_CC_ACCUM_CLR_BIT
#define LTC2959_CC_DEADBAND_0UV     (0b00 << 6)
#define LTC2959_CC_DEADBAND_20UV    (0b01 << 6)
#define LTC2959_CC_DEADBAND_40UV    (0b10 << 6)
#define LTC2959_CC_DEADBAND_80UV    (0b11 << 6)
#define LTC2959_CC_DO_NOT_COUNT     (BIT(3))
#define LTC2959_CC_RESERVED_MASK    (BIT(4))  // Bit 4 = 1, Bit 5 = 0 — reserved pattern: 0b01
#define LTC2959_CC_RESERVED_VALUE   (0b01 << 4)
#define LTC2959_CC_WRITABLE_MASK (LTC2959_CC_ACCUM_CLR_BIT | \
                                   LTC2959_CC_THRESH_EN_BIT | \
                                   LTC2959_CC_DO_NOT_COUNT)

/* Used when ACR is controlled via firmware */
#define LTC2959_ACR_CLR (0xFFFFFFFF)

#ifdef CONFIG_FUEL_GAUGE_LTC2959_RSENSE_MOHMS
/* Use Kconfig-defined value if available */
#define LTC2959_RSENSE_mOHMS CONFIG_FUEL_GAUGE_LTC2959_RSENSE_MOHMS
#else
#define LTC2959_RSENSE_mOHMS (80)
#endif

/* LSB in µV across the sense resistor */
#define LTC2959_CURRENT_LSB_UV (97500000 / (LTC2959_RSENSE_mOHMS * 32768))

// NOTE: For 3.8V → raw = (3800 * 1049) >> 4
#define LTC2959_VOLT_TO_RAW_SF (1049)
#define LTC2959_RAW_TO_UVOLT_SF (955) // µV per LSB

// NOTE: QLSB = 533 nAh = ~0.533 µAh → raw × (34952 / 65536)
#define LTC2959_CHARGE_UAH_SF (34952)
#define LTC2959_TEMP_K_SF (8250)
#define LTC2959_VOLT_MV_SF (62600)
#define LTC2959_RAW_TO_MV_SF (68607)
#define LTC2959_GPIO_BIPOLAR_UV_SF (97500)
#define LTC2959_GPIO_UNIPOLAR_UV_SF (1560000)

enum ltc2959_custom_props {
	FUEL_GAUGE_ADC_MODE = FUEL_GAUGE_CUSTOM_BEGIN, //0x8000
	FUEL_GAUGE_VOLTAGE_HIGH_THRESHOLD,
	FUEL_GAUGE_VOLTAGE_LOW_THRESHOLD,
	FUEL_GAUGE_CURRENT_HIGH_THRESHOLD,
	FUEL_GAUGE_CURRENT_LOW_THRESHOLD,
	FUEL_GAUGE_TEMPERATURE_HIGH_THRESHOLD,
	FUEL_GAUGE_TEMPERATURE_LOW_THRESHOLD,
	FUEL_GAUGE_GPIO_VOLTAGE_UV,
	FUEL_GAUGE_GPIO_HIGH_THRESHOLD,
	FUEL_GAUGE_GPIO_LOW_THRESHOLD,
	FUEL_GAUGE_CC_CONFIG,
	FUEL_GAUGE_CC_CLEAR,
	FUEL_GAUGE_CC_HOLD
};

struct ltc2959_config {
	struct i2c_dt_spec i2c;
};

#endif
