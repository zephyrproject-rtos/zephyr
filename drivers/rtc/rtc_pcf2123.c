/**
 * Copyright (c) 2025 Lukas Gunnarsson <lukasgunnarsson@protonmail.com>
 * SPDX-License-Identifier: Apache-2.0
 *
 * Supported features:
 * - Base RTC functionality (set/get time)
 * - Alarm interrupts
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "rtc_utils.h"

#define DT_DRV_COMPAT nxp_pcf2123

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int1_gpios) &&                                                \
	(defined(CONFIG_RTC_ALARM) || defined(CONFIG_RTC_UPDATE))
#define PCF2123_INT1_GPIOS_IN_USE 1
#endif

LOG_MODULE_REGISTER(pcf2123, CONFIG_RTC_LOG_LEVEL);

/**
 * Datasheet: https://www.nxp.com/docs/en/data-sheet/PCF2123.pdf
 */

/* Control registers. */
#define PCF2123_REG_CTRL_1        0x00
#define PCF2123_REG_CTRL_2        0x01
/* Time and date registers. */
#define PCF2123_REG_SECONDS       0x02
#define PCF2123_REG_MINUTES       0x03
#define PCF2123_REG_HOURS         0x04
#define PCF2123_REG_DAYS          0x05
#define PCF2123_REG_WEEKDAYS      0x06
#define PCF2123_REG_MONTHS        0x07
#define PCF2123_REG_YEARS         0x08
/* Alarm registers. */
#define PCF2123_REG_ALARM_MINUTE  0x09
#define PCF2123_REG_ALARM_HOUR    0x0A
#define PCF2123_REG_ALARM_DAY     0x0B
#define PCF2123_REG_ALARM_WEEKDAY 0x0C
/* Offset register. */
#define PCF2123_REG_OFFSET        0x0D
/* Timer registers. */
#define PCF2123_TIMER_CLKOUT      0x0E
#define PCF2123_TIMER_COUNTDOWN   0x0F

/**
 * Control register 1 bits (page 9 in the datasheet).
 * Bits 6, 3 and 0 are unused.
 */
#define PCF2123_CTRL_1_EXT_TEST BIT(7)
#define PCF2123_CTRL_1_STOP     BIT(5)
#define PCF2123_CTRL_1_SR       BIT(4)
#define PCF2123_CTRL_1_12_24    BIT(2)
#define PCF2123_CTRL_1_CIE      BIT(1)

/**
 * Control register 2 bits (page 11 in the datasheet).
 */
#define PCF2123_CTRL_2_MI    BIT(7)
#define PCF2123_CTRL_2_SI    BIT(6)
#define PCF2123_CTRL_2_MSF   BIT(5)
#define PCF2123_CTRL_2_TI_TP BIT(4)
#define PCF2123_CTRL_2_AF    BIT(3)
#define PCF2123_CTRL_2_TF    BIT(2)
#define PCF2123_CTRL_2_AIE   BIT(1)
#define PCF2123_CTRL_2_TIE   BIT(0)

/**
 * Alarm register bits (page 17 in the datasheet).
 */
#define PCF2123_ALARM_MINUTE_AE_M  (0UL << 7)
#define PCF2123_ALARM_HOUR_AE_H    (0UL << 7)
#define PCF2123_ALARM_DAY_AE_D     (0UL << 7)
#define PCF2123_ALARM_WEEKDAY_AE_W (0UL << 7)

/**
 * The masks have been gathered from the register overview (page 8 in the datasheet).
 */
#define PCF2123_SECONDS_MASK  GENMASK(6, 0)
#define PCF2123_MINUTES_MASK  GENMASK(6, 0)
#define PCF2123_HOURS_MASK    GENMASK(5, 0)
#define PCF2123_DAYS_MASK     GENMASK(5, 0)
#define PCF2123_WEEKDAYS_MASK GENMASK(2, 0)
#define PCF2123_MONTHS_MASK   GENMASK(4, 0)
#define PCF2123_YEARS_MASK    GENMASK(7, 0)

#define PCF2123_RTC_TIME_MASK                                                                      \
	(RTC_ALARM_TIME_MASK_SECOND | RTC_ALARM_TIME_MASK_MINUTE | RTC_ALARM_TIME_MASK_HOUR |      \
	 RTC_ALARM_TIME_MASK_MONTHDAY | RTC_ALARM_TIME_MASK_WEEKDAY | RTC_ALARM_TIME_MASK_MONTH |  \
	 RTC_ALARM_TIME_MASK_YEAR)

#define PCF2123_RTC_ALARM_TIME_MASK                                                                \
	(RTC_ALARM_TIME_MASK_MINUTE | RTC_ALARM_TIME_MASK_HOUR | RTC_ALARM_TIME_MASK_MONTHDAY |    \
	 RTC_ALARM_TIME_MASK_WEEKDAY)

/* The PCF2123 supports two-digit years (0-99). */
#define PCF2123_YEARS_OFFSET  (2000 - 1900)
#define PCF2123_MONTHS_OFFSET 1

/**
 * Command byte definition (page 36 in the datasheet).
 *  Bit 7:   R/W (0 write, 1 read)
 *  Bit 6-4: SA  (Subaddress, required to be 0b001)
 *  Bit 3-0: RA  (Register address range)
 *
 * The PCF2123 starts from the address RA and auto-increments for every byte.
 */
#define PCF2123_CMD_READ  ((1U << 7) | (0b001 << 4))
#define PCF2123_CMD_WRITE ((0U << 7) | (0b001 << 4))

#ifdef PCF2123_INT1_GPIOS_IN_USE
void callback_work_handler(struct k_work *work);
K_WORK_DEFINE(callback_work, callback_work_handler);

static void gpio_int1_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins);
#endif

struct pcf2123_config {
	const struct spi_dt_spec spi;
#ifdef PCF2123_INT1_GPIOS_IN_USE
	struct gpio_dt_spec int1;
#endif
};

struct pcf2123_data {
#ifdef PCF2123_INT1_GPIOS_IN_USE
	const struct device *dev;
	rtc_alarm_callback alarm_callback;
	void *alarm_user_data;
	struct gpio_callback int1_callback;
	struct k_work callback_work;
#endif
};

/**
 * @brief Read from a single or multiple PCF2123 registers.
 *
 * @param dev The RTC device.
 * @param start_addr The first memory address to start reading from.
 * @param buf Buffer to store read data.
 * @param len Number of bytes to read.
 * @retval 0 on success, else -ERRNO.
 *
 * @note The PCF2123 auto-increments the selected memory address for every byte sent.
 */
static int pcf2123_reg_read(const struct device *dev, const uint8_t start_addr, uint8_t *buf,
			    const size_t len)
{
	if (!buf) {
		return -EINVAL;
	}
	if (len <= 0) {
		return -EINVAL;
	}

	const struct pcf2123_config *config = dev->config;
	int ret;

	uint8_t cmd_byte = PCF2123_CMD_READ | start_addr;
	struct spi_buf tx_buf = {.buf = &cmd_byte, .len = sizeof(cmd_byte)};
	const struct spi_buf_set tx_bufs = {.buffers = &tx_buf, .count = 1};

	uint8_t rx_dummy; /* Dummy byte. */
	struct spi_buf rx_buf[2] = {{.buf = &rx_dummy, .len = 1}, {.buf = buf, .len = len}};
	const struct spi_buf_set rx_bufs = {.buffers = rx_buf, .count = 2};

	ret = spi_transceive_dt(&config->spi, &tx_bufs, &rx_bufs);
	if (ret < 0) {
		LOG_ERR("Failed to read from register with start address %d (err %d)", start_addr,
			ret);
		return ret;
	}

	return 0;
}

/**
 * @brief Write to a single or multiple PCF2123 registers.
 *
 * @param dev The RTC device.
 * @param start_addr The first memory address to start writing towards.
 * @param buf Data to write.
 * @param len Data size in bytes.
 * @retval 0 on success, else -ERRNO.
 *
 * @note The PCF2123 auto-increments the selected memory address for every byte sent.
 */
static int pcf2123_reg_write(const struct device *dev, const uint8_t start_addr, uint8_t *buf,
			     const size_t len)
{
	if (!buf) {
		return -EINVAL;
	}
	if (len <= 0) {
		return -EINVAL;
	}

	const struct pcf2123_config *config = dev->config;
	int ret;

	uint8_t cmd_byte = PCF2123_CMD_WRITE | start_addr;
	struct spi_buf cmd_buf = {.buf = &cmd_byte, .len = sizeof(cmd_byte)};
	struct spi_buf data_buf = {.buf = buf, .len = len};
	struct spi_buf tx_buf[] = {cmd_buf, data_buf};
	const struct spi_buf_set tx_bufs = {.buffers = tx_buf, .count = 2};

	ret = spi_write_dt(&config->spi, &tx_bufs);
	if (ret < 0) {
		LOG_ERR("Failed to set RTC time (err %d)", ret);
		return ret;
	}

	return 0;
}

static int pcf2123_set_time(const struct device *dev, const struct rtc_time *timeptr)
{
	LOG_DBG("Set time: year=%d mon=%d mday=%d wday=%d hour=%d min=%d sec=%d", timeptr->tm_year,
		timeptr->tm_mon, timeptr->tm_mday, timeptr->tm_wday, timeptr->tm_hour,
		timeptr->tm_min, timeptr->tm_sec);

	if (!rtc_utils_validate_rtc_time(timeptr, PCF2123_RTC_TIME_MASK)) {
		LOG_ERR("RTC time validation failed");
		return -EINVAL;
	}

	if ((timeptr->tm_year < PCF2123_YEARS_OFFSET) ||
	    (timeptr->tm_year > PCF2123_YEARS_OFFSET + 99)) {
		LOG_ERR("Invalid tm_year value: %d", timeptr->tm_year);
		return -EINVAL;
	}

	/* There are 7 time registers ranging from 02h-08h. */
	uint8_t regs[7];

	/* Set seconds. */
	regs[0] = bin2bcd(timeptr->tm_sec) & PCF2123_SECONDS_MASK;

	/* Set minutes. */
	regs[1] = bin2bcd(timeptr->tm_min) & PCF2123_MINUTES_MASK;

	/* Set hours. */
	regs[2] = bin2bcd(timeptr->tm_hour) & PCF2123_HOURS_MASK;

	/* Set days. */
	regs[3] = bin2bcd(timeptr->tm_mday) & PCF2123_DAYS_MASK;

	/* Set weekdays. The values are not BCD encoded. */
	regs[4] = timeptr->tm_wday;

	/* Set month. */
	regs[5] = bin2bcd(timeptr->tm_mon + PCF2123_MONTHS_OFFSET) & PCF2123_MONTHS_MASK;

	/* Set year. */
	regs[6] = bin2bcd(timeptr->tm_year - PCF2123_YEARS_OFFSET) & PCF2123_YEARS_MASK;

	pcf2123_reg_write(dev, PCF2123_REG_SECONDS, regs, sizeof(regs));

	return 0;
}

static int pcf2123_get_time(const struct device *dev, struct rtc_time *timeptr)
{
	uint8_t regs[7];
	int ret;

	ret = pcf2123_reg_read(dev, PCF2123_REG_SECONDS, regs, sizeof(regs));
	if (ret < 0) {
		return -ENODATA;
	}

	/**
	 * Validate clock integrity.
	 * The seconds register use bit 7 for checking integrity (page 12 in the datasheet).
	 */
	if (bcd2bin(regs[0]) & BIT(7)) {
		LOG_WRN("Clock integrity failed");
		LOG_HEXDUMP_DBG(regs, sizeof(regs), "Read data");
		return -ENODATA;
	}

	/* Nanoseconds, unused. */
	timeptr->tm_nsec = 0;

	/* Seconds. */
	timeptr->tm_sec = bcd2bin(regs[0] & PCF2123_SECONDS_MASK);

	/* Minutes. */
	timeptr->tm_min = bcd2bin(regs[1] & PCF2123_MINUTES_MASK);

	/* Hours. */
	timeptr->tm_hour = bcd2bin(regs[2] & PCF2123_HOURS_MASK);

	/* Days. */
	timeptr->tm_mday = bcd2bin(regs[3] & PCF2123_DAYS_MASK);

	/* Weekdays. */
	timeptr->tm_wday = bcd2bin(regs[4] & PCF2123_WEEKDAYS_MASK);

	/* Year days, unused. */
	timeptr->tm_yday = -1;

	/**
	 * Months range from 1-12 (table 5, page 8 in the datasheet).
	 * The struct rtc_time expects months in the 0-11 range therefore
	 * we need to offset the month by 1.
	 */
	timeptr->tm_mon = bcd2bin(regs[5] & PCF2123_MONTHS_MASK) - PCF2123_MONTHS_OFFSET;

	/* Years range from 0-99 (table 5, page 8 in the datasheet). */
	timeptr->tm_year = bcd2bin(regs[6] & PCF2123_YEARS_MASK) + PCF2123_YEARS_OFFSET;

	timeptr->tm_isdst = -1; /* Unused. */

	LOG_DBG("Get time: year=%d mon=%d mday=%d wday=%d hour=%d min=%d sec=%d", timeptr->tm_year,
		timeptr->tm_mon, timeptr->tm_mday, timeptr->tm_wday, timeptr->tm_hour,
		timeptr->tm_min, timeptr->tm_sec);

	return 0;
}

#ifdef CONFIG_RTC_ALARM
static int pcf2123_clear_alarm_flag(const struct device *dev)
{
	/* Clearing of the alarm flag can be found in page 19 in the datasheet. */
	uint8_t ctrl2;
	int ret;

	ret = pcf2123_reg_read(dev, PCF2123_REG_CTRL_2, &ctrl2, 1);
	if (ret < 0) {
		LOG_ERR("Failed to read from control register 2 to clear AF bit (err %d)", ret);
		return ret;
	}

	/* Clear the AF bit. */
	ctrl2 &= ~PCF2123_CTRL_2_AF;
	ret = pcf2123_reg_write(dev, PCF2123_REG_CTRL_2, &ctrl2, 1);
	if (ret < 0) {
		LOG_ERR("Failed write to control register 2 to clear the AF bit (err %d)", ret);
		return ret;
	}

	return 0;
}

static int pcf2123_alarm_get_supported_fields(const struct device *dev, uint16_t id, uint16_t *mask)
{
	ARG_UNUSED(dev);

	if (id != 0U) {
		LOG_ERR("Invalid ID: %d", id);
		return -EINVAL;
	}

	*mask = PCF2123_RTC_ALARM_TIME_MASK;

	return 0;
}

static int pcf2123_alarm_set_time(const struct device *dev, uint16_t id, uint16_t mask,
				  const struct rtc_time *timeptr)
{
	if (id != 0) {
		LOG_ERR("Invalid ID: %d", id);
		return -EINVAL;
	}

	if ((mask & ~(PCF2123_RTC_ALARM_TIME_MASK)) != 0) {
		LOG_ERR("Invalid alarm mask: 0x%04X", mask);
		return -EINVAL;
	}

	if (!rtc_utils_validate_rtc_time(timeptr, mask)) {
		LOG_ERR("Failed to validate the RTC time");
		return -EINVAL;
	}

	uint8_t regs[4];
	int ret;

	/* The alarms are disabled if bit 7 is 1 (page 17 in the datasheet). */

	/* Alarm register: MINUTE */
	if ((mask & RTC_ALARM_TIME_MASK_MINUTE) != 0) {
		regs[0] = bin2bcd(timeptr->tm_min) & PCF2123_MINUTES_MASK;
	} else {
		regs[0] = BIT(7);
	}

	/* Alarm register: HOUR */
	if ((mask & RTC_ALARM_TIME_MASK_HOUR) != 0) {
		regs[1] = bin2bcd(timeptr->tm_hour) & PCF2123_HOURS_MASK;
	} else {
		regs[1] = BIT(7);
	}

	/* Alarm register: DAY */
	if ((mask & RTC_ALARM_TIME_MASK_MONTHDAY) != 0) {
		regs[2] = bin2bcd(timeptr->tm_mday) & PCF2123_DAYS_MASK;
	} else {
		regs[2] = BIT(7);
	}

	/* Alarm register: WEEKDAY */
	if ((mask & RTC_ALARM_TIME_MASK_WEEKDAY) != 0) {
		regs[3] = bin2bcd(timeptr->tm_wday) & PCF2123_WEEKDAYS_MASK;
	} else {
		regs[3] = BIT(7);
	}

	/* Clear the alarm flag. */
	ret = pcf2123_clear_alarm_flag(dev);
	if (ret < 0) {
		LOG_ERR("Failed to clear alarm flag (err %d)", ret);
		return -ENOMSG;
	}

	ret = pcf2123_reg_write(dev, PCF2123_REG_ALARM_MINUTE, regs, sizeof(regs));
	if (ret) {
		LOG_ERR("Failed to write to alarm registers (err %d)", ret);
		return -ENOMSG;
	}

	/* Make sure interrupts are enabled. */
	uint8_t ctrl2;

	ret = pcf2123_reg_read(dev, PCF2123_REG_CTRL_2, &ctrl2, 1);
	if (ret < 0) {
		LOG_ERR("Failed to read from control register 2 (err %d)", ret);
		return ret;
	}

	if ((ctrl2 & PCF2123_CTRL_2_AIE) == 0) {
		ctrl2 |= PCF2123_CTRL_2_AIE;
		ret = pcf2123_reg_write(dev, PCF2123_REG_CTRL_2, &ctrl2, 1);
		if (ret < 0) {
			LOG_ERR("Failed to enable interrupts in control register 2 (err %d)", ret);
			return ret;
		}
	}

	return 0;
}

static int pcf2123_alarm_get_time(const struct device *dev, uint16_t id, uint16_t *mask,
				  struct rtc_time *timeptr)
{
	uint8_t regs[4];
	int ret;

	ret = pcf2123_reg_read(dev, PCF2123_REG_ALARM_MINUTE, regs, sizeof(regs));
	if (ret < 0) {
		LOG_ERR("Failed to read alarm registers (err %d)", ret);
		return ret;
	}

	*mask = 0;

	/* Alarm register: MINUTE */
	if ((regs[0] & BIT(7)) == 0) {
		*mask |= RTC_ALARM_TIME_MASK_MINUTE;
		timeptr->tm_min = bcd2bin(regs[0] & PCF2123_MINUTES_MASK);
	} else {
		timeptr->tm_min = 0;
	}

	/* Alarm register: HOUR */
	if ((regs[1] & BIT(7)) == 0) {
		*mask |= RTC_ALARM_TIME_MASK_HOUR;
		timeptr->tm_hour = bcd2bin(regs[1] & PCF2123_HOURS_MASK);
	} else {
		timeptr->tm_hour = 0;
	}

	/* Alarm register: DAY */
	if ((regs[2] & BIT(7)) == 0) {
		*mask |= RTC_ALARM_TIME_MASK_MONTHDAY;
		timeptr->tm_mday = bcd2bin(regs[2] & PCF2123_DAYS_MASK);
	} else {
		timeptr->tm_mday = 1;
	}

	/* Alarm register: WEEKDAY */
	if ((regs[3] & BIT(7)) == 0) {
		*mask |= RTC_ALARM_TIME_MASK_WEEKDAY;
		timeptr->tm_wday = bcd2bin(regs[3] & PCF2123_WEEKDAYS_MASK);
	} else {
		timeptr->tm_wday = -1;
	}

	return 0;
}

static int pcf2123_alarm_is_pending(const struct device *dev, uint16_t id)
{
	int ret;
	uint8_t ctrl2;

	if (id != 0) {
		LOG_ERR("Invalid ID: %d", id);
		return -EINVAL;
	}

	ret = pcf2123_reg_read(dev, PCF2123_REG_CTRL_2, &ctrl2, 1);
	if (ret < 0) {
		LOG_ERR("Failed to read from control register 2 (err %d)", ret);
		return ret;
	}

	/* Clear the alarm flag. */
	if (ctrl2 & PCF2123_CTRL_2_AF) {
		ret = pcf2123_clear_alarm_flag(dev);
		if (ret < 0) {
			LOG_ERR("Failed to clear alarm flag (err %d)", ret);
			return -ENOMSG;
		}

		return 1;
	}

	return 0;
}
#endif /* CONFIG_RTC_ALARM */

static int pcf2123_alarm_set_callback(const struct device *dev, uint16_t id,
				      rtc_alarm_callback callback, void *user_data)
{
#ifndef PCF2123_INT1_GPIOS_IN_USE
	ARG_UNUSED(dev);
	ARG_UNUSED(id);
	ARG_UNUSED(callback);
	ARG_UNUSED(user_data);

	return -ENOTSUP;
#else
	const struct pcf2123_config *config = dev->config;
	struct pcf2123_data *data = dev->data;
	int ret;

	if (config->int1.port == NULL) {
		LOG_ERR("The int1 port is NULL");
		return -ENOTSUP;
	}

	if (id != 0U) {
		LOG_ERR("Invalid ID %d", id);
		return -EINVAL;
	}

	data->dev = dev;
	data->alarm_callback = callback;
	data->alarm_user_data = user_data;

	ret = gpio_pin_configure_dt(&config->int1, GPIO_INPUT | GPIO_PULL_UP);
	if (ret < 0) {
		LOG_ERR("Failed to configure int1 (err %d)", ret);
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&config->int1, GPIO_INT_EDGE_FALLING);
	if (ret < 0) {
		LOG_ERR("Failed to configure edge on int1 (err %d)", ret);
		return ret;
	}

	gpio_init_callback(&data->int1_callback, gpio_int1_callback, BIT(config->int1.pin));
	ret = gpio_add_callback(config->int1.port, &data->int1_callback);
	if (ret < 0) {
		LOG_ERR("Failed to add callback to int1 (err %d)", ret);
		return ret;
	}

	return 0;
#endif
}

#ifdef PCF2123_INT1_GPIOS_IN_USE
static void gpio_int1_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	struct pcf2123_data *data = CONTAINER_OF(cb, struct pcf2123_data, int1_callback);

	LOG_DBG("PCF2123 interrupt detected");
	k_work_submit(&(data->callback_work));
}

void callback_work_handler(struct k_work *work)
{
	struct pcf2123_data *data = CONTAINER_OF(work, struct pcf2123_data, callback_work);

	if (data->alarm_callback) {
		data->alarm_callback(data->dev, 0, data->alarm_user_data);
	} else {
		LOG_WRN("Missing PCF2123 alarm callback");
	}
}
#endif /* PCF2123_INT1_GPIOS_IN_USE */

static DEVICE_API(rtc, pcf2123_driver_api) = {
	.set_time = pcf2123_set_time,
	.get_time = pcf2123_get_time,
#ifdef CONFIG_RTC_ALARM
	.alarm_get_supported_fields = pcf2123_alarm_get_supported_fields,
	.alarm_set_time = pcf2123_alarm_set_time,
	.alarm_get_time = pcf2123_alarm_get_time,
	.alarm_is_pending = pcf2123_alarm_is_pending,
	.alarm_set_callback = pcf2123_alarm_set_callback,
#endif
};

int pcf2123_init(const struct device *dev)
{
	const struct pcf2123_config *config = dev->config;
	int ret;
	uint8_t dummy_check;

#ifdef PCF2123_INT1_GPIOS_IN_USE
	struct pcf2123_data *data = dev->data;

	data->callback_work = callback_work;
#endif

	if (!device_is_ready(config->spi.bus)) {
		LOG_ERR("SPI device not ready: %s", config->spi.bus->name);
		return -ENODEV;
	}

	/* Determine if the SPI device is alive by attempting to read from control register 1. */
	ret = pcf2123_reg_read(dev, PCF2123_REG_CTRL_1, &dummy_check, 1);
	if (ret < 0) {
		LOG_ERR("Failed to communicate with PCF2123 (err %d)", ret);
		return -ENODEV;
	}

#ifdef PCF2123_INT1_GPIOS_IN_USE
	uint8_t ctrl2 = PCF2123_CTRL_2_AIE; /* Enable alarm interrupts (AIE). */

	ret = pcf2123_reg_write(dev, PCF2123_REG_CTRL_2, &ctrl2, 1);
	if (ret < 0) {
		LOG_ERR("Failed to write to control register 2 during initialization");
		return -ENODEV;
	}
#endif

	LOG_DBG("%s initialized", dev->name);

	return 0;
}

#define PCF2123_INIT(inst)                                                                         \
	static const struct pcf2123_config pcf2123_config_##inst = {                               \
		.spi = SPI_DT_SPEC_INST_GET(inst, SPI_WORD_SET(8) | SPI_TRANSFER_MSB, 0U),         \
		IF_ENABLED(PCF2123_INT1_GPIOS_IN_USE,\
			(.int1 = GPIO_DT_SPEC_INST_GET_OR(inst, int1_gpios, {0})) \
		)};                \
                                                                                                   \
	static struct pcf2123_data pcf2123_data_##inst;                                            \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, &pcf2123_init, NULL, &pcf2123_data_##inst,                     \
			      &pcf2123_config_##inst, POST_KERNEL, CONFIG_RTC_INIT_PRIORITY,       \
			      &pcf2123_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PCF2123_INIT)
