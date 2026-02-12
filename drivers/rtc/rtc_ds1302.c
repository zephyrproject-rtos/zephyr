/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2026 Harpreet Saini
 * Author: Harpreet Saini <sainiharpreet29@yahoo.com>
 */

#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/util.h>
#include "rtc_utils.h"

#define DT_DRV_COMPAT maxim_ds1302

LOG_MODULE_REGISTER(ds1302, CONFIG_RTC_LOG_LEVEL);

/* DS1302 registers */

#define RTC_CMD_READ  0x81 /* Read command */
#define RTC_CMD_WRITE 0x80 /* Write command */

#define RTC_CMD_WRITE_ENABLE  0x00 /* Write enable */
#define RTC_CMD_WRITE_DISABLE 0x80 /* Write disable */

#define RTC_ADDR_RAM0  0x20 /* Address of RAM0 */
#define RTC_ADDR_TCR   0x08 /* Address of trickle charge register */
#define RTC_CLCK_BURST 0x1F /* Address of clock burst */
#define RTC_CLCK_LEN   0x08 /* Size of clock burst */
#define RTC_ADDR_CTRL  0x07 /* Address of control register */
#define RTC_ADDR_YEAR  0x06 /* Address of year register */
#define RTC_ADDR_DAY   0x05 /* Address of day of week register */
#define RTC_ADDR_MON   0x04 /* Address of month register */
#define RTC_ADDR_DATE  0x03 /* Address of day of month register */
#define RTC_ADDR_HOUR  0x02 /* Address of hour register */
#define RTC_ADDR_MIN   0x01 /* Address of minute register */
#define RTC_ADDR_SEC   0x00 /* Address of second register */

#define DS1302_RTC_TIME_MASK                                                                       \
	(RTC_ALARM_TIME_MASK_SECOND | RTC_ALARM_TIME_MASK_MINUTE | RTC_ALARM_TIME_MASK_HOUR |      \
	 RTC_ALARM_TIME_MASK_MONTH | RTC_ALARM_TIME_MASK_MONTHDAY | RTC_ALARM_TIME_MASK_YEAR |     \
	 RTC_ALARM_TIME_MASK_WEEKDAY)

struct ds1302_config {
	struct spi_dt_spec spi_bus;
};

struct ds1302_data {
	struct k_sem sem;
};

static int ds1302_set_time(const struct device *dev, const struct rtc_time *timeptr)
{
	int err;
	uint8_t buf[RTC_CLCK_LEN + 1] = {0};

	struct ds1302_data *data = dev->data;
	const struct ds1302_config *config = dev->config;

	if ((timeptr == NULL) || !rtc_utils_validate_rtc_time(timeptr, DS1302_RTC_TIME_MASK)) {
		LOG_ERR("Invalid timer pointer or time values\r\n");
		return -EINVAL;
	}

	k_sem_take(&data->sem, K_FOREVER);

	LOG_DBG("set time: year = %d, mon = %d, mday = %d, wday = %d, hour = %d, min = %d, sec = "
		"%d",
		timeptr->tm_year, timeptr->tm_mon, timeptr->tm_mday, timeptr->tm_wday,
		timeptr->tm_hour, timeptr->tm_min, timeptr->tm_sec);

	buf[0] = RTC_ADDR_CTRL << 1 | RTC_CMD_WRITE;
	buf[1] = RTC_CMD_WRITE_ENABLE;

	struct spi_buf buffer = {
		.buf = buf,
		.len = 2,
	};
	struct spi_buf_set tx_buf = {
		.buffers = &buffer,
		.count = 1,
	};
	err = spi_write_dt(&config->spi_bus, &tx_buf);
	if (err != 0) {
		goto unlock;
	}

	buf[0] = RTC_CLCK_BURST << 1 | RTC_CMD_WRITE;
	buf[1] = bin2bcd(timeptr->tm_sec);
	buf[2] = bin2bcd(timeptr->tm_min);
	buf[3] = bin2bcd(timeptr->tm_hour);
	buf[4] = bin2bcd(timeptr->tm_mday);
	buf[5] = bin2bcd(timeptr->tm_mon + 1);
	buf[6] = timeptr->tm_wday + 1;
	buf[7] = bin2bcd(timeptr->tm_year % 100);
	buf[8] = RTC_CMD_WRITE_DISABLE;
	buffer.len = RTC_CLCK_LEN + 1;

	err = spi_write_dt(&config->spi_bus, &tx_buf);

unlock:
	k_sem_give(&data->sem);
	return err;
}

static int ds1302_get_time(const struct device *dev, struct rtc_time *timeptr)
{
	int err;
	uint8_t buf[RTC_CLCK_LEN - 1] = {0};
	struct ds1302_data *data = dev->data;
	const struct ds1302_config *config = dev->config;

	if (timeptr == NULL) {
		LOG_ERR("Invalid timer pointer\r\n");
		return -EINVAL;
	}

	LOG_DBG("get time: year = %d, mon = %d, mday = %d, wday = %d, hour = %d, min = %d, sec = "
		"%d\r\n",
		timeptr->tm_year, timeptr->tm_mon, timeptr->tm_mday, timeptr->tm_wday,
		timeptr->tm_hour, timeptr->tm_min, timeptr->tm_sec);

	k_sem_take(&data->sem, K_FOREVER);

	uint8_t addr = RTC_CLCK_BURST << 1 | RTC_CMD_READ;
	struct spi_buf buffer[2] = {
		{
			.buf = &addr,
			.len = 1,
		},
		{
			.buf = buf,
			.len = ARRAY_SIZE(buf),
		},
	};

	struct spi_buf_set tx_buf = {
		.buffers = &buffer[0],
		.count = 1,
	};

	struct spi_buf_set rx_buf = {
		.buffers = &buffer[1],
		.count = 1,
	};

	err = spi_transceive_dt(&config->spi_bus, &tx_buf, &rx_buf);
	if (err != 0) {
		goto unlock;
	}

	/* Decode the registers */
	timeptr->tm_sec = bcd2bin(buf[RTC_ADDR_SEC]);
	timeptr->tm_min = bcd2bin(buf[RTC_ADDR_MIN]);
	timeptr->tm_hour = bcd2bin(buf[RTC_ADDR_HOUR]);
	timeptr->tm_wday = buf[RTC_ADDR_DAY] - 1;
	timeptr->tm_mday = bcd2bin(buf[RTC_ADDR_DATE]);
	timeptr->tm_mon = bcd2bin(buf[RTC_ADDR_MON]) - 1;
	timeptr->tm_year = bcd2bin(buf[RTC_ADDR_YEAR]) + 100;

unlock:
	k_sem_give(&data->sem);
	return err;
}

static const struct rtc_driver_api ds1302_driver_api = {
	.set_time = ds1302_set_time,
	.get_time = ds1302_get_time,
};

static int ds1302_init(const struct device *dev)
{
	int err;
	uint8_t addr = RTC_ADDR_CTRL << 1 | RTC_CMD_READ;
	uint8_t buff = 0;
	const struct ds1302_config *config = dev->config;
	struct spi_dt_spec spi = config->spi_bus;
	struct ds1302_data *data = dev->data;

	k_sem_init(&data->sem, 1, 1);

	struct spi_buf_set tx_buf = {
		.buffers = &(struct spi_buf){.buf = &addr, .len = 1},
		.count = 1,
	};

	struct spi_buf_set rx_buf = {
		.buffers = &(struct spi_buf){.buf = &buff, .len = 1},
		.count = 1,
	};
	k_sem_take(&data->sem, K_FOREVER);
	if (!spi_is_ready_dt(&spi)) {
		LOG_ERR("SPI bus not ready\r\n");
		err = -ENODEV;
		goto unlock;
	}
	err = spi_transceive_dt(&spi, &tx_buf, &rx_buf);

unlock:
	k_sem_give(&data->sem);
	return err;
}

#define RTC_DS1302_SPI_CFG SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_LSB | SPI_HALF_DUPLEX

#define DS1302_DEFINE(inst)                                                                        \
	static struct ds1302_data ds1302_data_##inst;                                              \
	static const struct ds1302_config ds1302_config_##inst = {                                 \
		.spi_bus = SPI_DT_SPEC_INST_GET(inst, RTC_DS1302_SPI_CFG),                         \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, &ds1302_init, NULL, &ds1302_data_##inst,                       \
			      &ds1302_config_##inst, POST_KERNEL, CONFIG_RTC_INIT_PRIORITY,        \
			      &ds1302_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DS1302_DEFINE)
