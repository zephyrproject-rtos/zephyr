/*
 * Copyright (c) 2025 Baylibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MFD_RV3032_H_
#define ZEPHYR_INCLUDE_DRIVERS_MFD_RV3032_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>

/* RV3032 RAM register addresses */
#define RV3032_REG_100TH_SECONDS	0x00
#define RV3032_REG_SECONDS		0x01
#define RV3032_REG_MINUTES		0x02
#define RV3032_REG_HOURS		0x03
#define RV3032_REG_WEEKDAY		0x04
#define RV3032_REG_DATE			0x05
#define RV3032_REG_MONTH		0x06
#define RV3032_REG_YEAR			0x07
#define RV3032_REG_ALARM_MINUTES	0x08
#define RV3032_REG_ALARM_HOURS		0x09
#define RV3032_REG_ALARM_DATE		0x0A
#define RV3032_REG_TIMER_VALUE_0	0x0B
#define RV3032_REG_TIMER_VALUE_1	0x0C
#define RV3032_REG_STATUS		0x0D
#define RV3032_REG_TEMPERATURE_LSB	0x0E
#define RV3032_REG_TEMPERATURE_MSB	0x0F
#define RV3032_REG_CONTROL1		0x10
#define RV3032_REG_CONTROL2		0x11
#define RV3032_REG_CONTROL3		0x12
#define RV3032_TS_CTRL			0x13
#define RV3032_CLK_INT_MASK		0x14
#define RV3032_EVI_CONTROL		0x15
#define RV3032_REG_TEMP_LOW_THLD	0x16
#define RV3032_REG_TEMP_HIGH_THLD	0x17

/* EEPROM register addresses */
#define RV3032_REG_EEPROM_ADDRESS	0x3D
#define RV3032_REG_EEPROM_DATA		0x3E
#define RV3032_REG_EEPROM_COMMAND	0x3F
#define RV3032_REG_EEPROM_PMU		0xC0
#define RV3032_REG_EEPROM_RAM1		0x40
#define RV3032_REG_EEPROM_OFFSET	0xC1
#define RV3032_REG_EEPROM_CLKOUT1	0xC2
#define RV3032_REG_EEPROM_CLKOUT2	0xC3
#define RV3032_REG_EEPROM_TREF0		0xC4
#define RV3032_REG_EEPROM_TREF1		0xC5

/* Registers masks and bits */
#define RV3032_CONTROL1_TD		GENMASK(1, 0)
#define RV3032_CONTROL1_EERD		BIT(2)
#define RV3032_CONTROL1_TE		BIT(3)
#define RV3032_CONTROL1_USEL		BIT(4)
#define RV3032_CONTROL1_GP0		BIT(5)

#define RV3032_CONTROL2_STOP		BIT(0)
#define RV3032_CONTROL2_GP1		BIT(1)
#define RV3032_CONTROL2_EIE		BIT(2)
#define RV3032_CONTROL2_AIE		BIT(3)
#define RV3032_CONTROL2_TIE		BIT(4)
#define RV3032_CONTROL2_UIE		BIT(5)
#define RV3032_CONTROL2_CLKIE		BIT(6)

#define RV3032_CONTROL3_TLIE		BIT(0)
#define RV3032_CONTROL3_THIE		BIT(1)
#define RV3032_CONTROL3_TLE		BIT(2)
#define RV3032_CONTROL3_THE		BIT(3)
#define RV3032_CONTROL3_BSIE		BIT(4)

#define RV3032_100TH_SECONDS_MASK	GENMASK(7, 0)
#define RV3032_SECONDS_MASK		GENMASK(6, 0)
#define RV3032_MINUTES_MASK		GENMASK(6, 0)
#define RV3032_HOURS_AMPM		BIT(5)
#define RV3032_HOURS_12H_MASK		GENMASK(4, 0)
#define RV3032_HOURS_24H_MASK		GENMASK(5, 0)
#define RV3032_DATE_MASK		GENMASK(5, 0)
#define RV3032_WEEKDAY_MASK		GENMASK(2, 0)
#define RV3032_MONTH_MASK		GENMASK(4, 0)
#define RV3032_YEAR_MASK		GENMASK(7, 0)

#define RV3032_ALARM_MINUTES_AE_M	BIT(7)
#define RV3032_ALARM_MINUTES_MASK	GENMASK(6, 0)
#define RV3032_ALARM_HOURS_AE_H		BIT(7)
#define RV3032_ALARM_HOURS_AMPM		BIT(5)
#define RV3032_ALARM_HOURS_12H_MASK	GENMASK(4, 0)
#define RV3032_ALARM_HOURS_24H_MASK	GENMASK(5, 0)
#define RV3032_ALARM_DATE_AE_D		BIT(7)
#define RV3032_ALARM_DATE_MASK		GENMASK(5, 0)

#define RV3032_STATUS_VLF		BIT(0)
#define RV3032_STATUS_PORF		BIT(1)
#define RV3032_STATUS_EVF		BIT(2)
#define RV3032_STATUS_AF		BIT(3)
#define RV3032_STATUS_TF		BIT(4)
#define RV3032_STATUS_UF		BIT(5)
#define RV3032_STATUS_TLF		BIT(6)
#define RV3032_STATUS_THF		BIT(7)

#define RV3032_EEPROM_CLKOUT1_HFD_LOW	GENMASK(7, 0)
#define RV3032_TEMPERATURE_BSF		BIT(0)
#define RV3032_TEMPERATURE_CLKF		BIT(1)
#define RV3032_TEMPERATURE_EEBUSY	BIT(2)
#define RV3032_TEMPERATURE_EEF		BIT(3)
#define RV3032_TEMPERATURE_TEMP_LSB	GENMASK(7, 4)

#define RV3032_EEPROM_PMU_NCLKE		BIT(6)
#define RV3032_EEPROM_PMU_BSM		GENMASK(5, 4)
#define RV3032_EEPROM_PMU_TCR		GENMASK(3, 2)
#define RV3032_EEPROM_PMU_TCM		GENMASK(1, 0)

#define RV3032_EEPROM_CLKOUT2_OS	BIT(7)
#define RV3032_EEPROM_CLKOUT2_FD	GENMASK(6, 5)
#define RV3032_EEPROM_CLKOUT2_HFD_HIGH	GENMASK(4, 0)

/* The RV3032 only supports two-digit years. Leap years are correctly handled from 2000 to 2099 */
#define RV3032_YEAR_OFFSET (2000 - 1900)

/* The RV3032 enumerates months 1 to 12 */
#define RV3032_MONTH_OFFSET 1

typedef void (*child_isr_t)(const struct device *dev);

/**
 * @brief Child device of rv3032
 */
enum child_dev {
	RV3032_DEV_REG = 0,
	RV3032_DEV_RTC_ALARM,
	RV3032_DEV_RTC_UPDATE,
	RV3032_DEV_COUNTER,
	RV3032_DEV_SENSOR,
	RV3032_DEV_MAX,     /** Maximum number of child devices */
};

int mfd_rv3032_read_regs(const struct device *dev, uint8_t addr, void *buf, size_t len);
int mfd_rv3032_read_reg8(const struct device *dev, uint8_t addr, uint8_t *val);
int mfd_rv3032_write_regs(const struct device *dev, uint8_t addr, void *buf, size_t len);
int mfd_rv3032_write_reg8(const struct device *dev, uint8_t addr, uint8_t val);
int mfd_rv3032_update_reg8(const struct device *dev, uint8_t addr, uint8_t mask, uint8_t val);
void mfd_rv3032_set_irq_handler(const struct device *dev, const struct device *child_dev,
				  enum child_dev child_idx, child_isr_t handler);

#define mfd_rv3032_set_status(dev, mask) mfd_rv3032_update_status(dev, mask, 0xff)
#define mfd_rv3032_clear_status(dev, mask) mfd_rv3032_update_status(dev, mask, 0x00)

int mfd_rv3032_update_status(const struct device *dev, uint8_t mask, uint8_t val);


#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MFD_RV3032_H_ */
