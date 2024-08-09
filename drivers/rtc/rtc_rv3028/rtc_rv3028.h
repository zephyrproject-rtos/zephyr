/*
 * Copyright (c) 2024 ANITRA system s.r.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_RTC_RTC_RV3028_RTC_RV3028_H_
#define ZEPHYR_DRIVERS_RTC_RTC_RV3028_RTC_RV3028_H_

#include <zephyr/device.h>
#include <sys/types.h>

void rv3028_lock_sem(const struct device *dev);

void rv3028_unlock_sem(const struct device *dev);

int rv3028_read_regs(const struct device *dev, uint8_t addr, void *buf, size_t len);

int rv3028_read_reg8(const struct device *dev, uint8_t addr, uint8_t *val);

int rv3028_write_regs(const struct device *dev, uint8_t addr, const void *buf, size_t len);

int rv3028_write_reg8(const struct device *dev, uint8_t addr, uint8_t val);

int rv3028_update_reg8(const struct device *dev, uint8_t addr, uint8_t mask, uint8_t val);

int rv3028_eeprom_wait_busy(const struct device *dev, int poll_ms);

int rv3028_exit_eerd(const struct device *dev);

int rv3028_enter_eerd(const struct device *dev);

int rv3028_eeprom_command(const struct device *dev, uint8_t command);

/* RV3028 RAM register addresses */
#define RV3028_REG_SECONDS              0x00
#define RV3028_REG_MINUTES              0x01
#define RV3028_REG_HOURS                0x02
#define RV3028_REG_WEEKDAY              0x03
#define RV3028_REG_DATE                 0x04
#define RV3028_REG_MONTH                0x05
#define RV3028_REG_YEAR                 0x06
#define RV3028_REG_ALARM_MINUTES        0x07
#define RV3028_REG_ALARM_HOURS          0x08
#define RV3028_REG_ALARM_WEEKDAY        0x09
#define RV3028_REG_STATUS               0x0E
#define RV3028_REG_CONTROL1             0x0F
#define RV3028_REG_CONTROL2             0x10
#define RV3028_REG_EVENT_CONTROL        0x13
#define RV3028_REG_TS_COUNT             0x14
#define RV3028_REG_TS_SECONDS           0x15
#define RV3028_REG_TS_MINUTES           0x16
#define RV3028_REG_TS_HOURS             0x17
#define RV3028_REG_TS_DATE              0x18
#define RV3028_REG_TS_MONTH             0x19
#define RV3028_REG_TS_YEAR              0x1A
#define RV3028_REG_UNIXTIME0            0x1B
#define RV3028_REG_UNIXTIME1            0x1C
#define RV3028_REG_UNIXTIME2            0x1D
#define RV3028_REG_UNIXTIME3            0x1E
#define RV3028_REG_USER_RAM1            0x1F
#define RV3028_REG_USER_RAM2            0x20
#define RV3028_REG_EEPROM_ADDRESS       0x25
#define RV3028_REG_EEPROM_DATA          0x26
#define RV3028_REG_EEPROM_COMMAND       0x27
#define RV3028_REG_ID                   0x28
#define RV3028_REG_CLKOUT               0x35
#define RV3028_REG_OFFSET               0x36
#define RV3028_REG_BACKUP               0x37

/* RV3028 EE command register values */
#define RV3028_EEPROM_CMD_INIT          0x00
#define RV3028_EEPROM_CMD_UPDATE        0x11
#define RV3028_EEPROM_CMD_REFRESH       0x12
#define RV3028_EEPROM_CMD_WRITE         0x21
#define RV3028_EEPROM_CMD_READ          0x22

#define RV3028_EEBUSY_READ_POLL_MS      1
#define RV3028_EEBUSY_WRITE_POLL_MS     10
#define RV3028_EEBUSY_TIMEOUT_MS        100

/* RV3028 Status register bit masks */
#define RV3028_STATUS_PORF              BIT(0)
#define RV3028_STATUS_EVF               BIT(1)
#define RV3028_STATUS_AF                BIT(2)
#define RV3028_STATUS_TF                BIT(3)
#define RV3028_STATUS_UF                BIT(4)
#define RV3028_STATUS_BSF               BIT(5)
#define RV3028_STATUS_CLKF              BIT(6)
#define RV3028_STATUS_EEBUSY            BIT(7)

#endif /* ZEPHYR_DRIVERS_RTC_RTC_RV3028_RTC_RV3028_H_ */
