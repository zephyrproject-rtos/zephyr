/*
 * RTC driver for PCF85363A
 * Copyright (c) 2025 Byteflies NV
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_pcf85363a

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(pcf85363a, CONFIG_RTC_LOG_LEVEL);

/*
 * RTC mode registers and Stopwatch registers share address space.
 * RTCM bit of the FUNCTION register selects between them.
 * 0: RTC mode (default after reset)
 * 1: Stopwatch mode
 * (Clock keeps running when we use stopwatch mode)
 */

/* PCF85363A register addresses */
enum pcf85363a_register {
// Time and date
	PCF85363A_CENTISECONDS = 0x00U,
	PCF85363A_SECONDS = 0x01U,
	PCF85363A_MINUTES = 0x02U,
	PCF85363A_HOURS = 0x03U,
	PCF85363A_DAYS = 0x04U,
	PCF85363A_WEEKDAYS = 0x05U,
	PCF85363A_MONTHS = 0x06U,
	PCF85363A_YEARS = 0x07U,
	// Alarm 1
	PCF85363A_SECOND_ALARM1 = 0x08U,
	PCF85363A_MINUTE_ALARM1 = 0x09U,
	PCF85363A_HOUR_ALARM1 = 0x0AU,
	PCF85363A_DAY_ALARM1 = 0x0BU,
	PCF85363A_MONTH_ALARM1 = 0x0CU,
	// Alarm 2
	PCF85363A_MINUTE_ALARM2 = 0x0DU,
	PCF85363A_HOUR_ALARM2 = 0x0EU,
	PCF85363A_WEEKDAY_ALARM2 = 0x0FU,
	// Alarm enables
	PCF85363A_ALARM_ENABLES = 0x10U,
	// RTC Timestamp 1
	PCF85363A_TSR1_SECONDS = 0x11U,
	PCF85363A_TSR1_MINUTES = 0x12U,
	PCF85363A_TSR1_HOURS = 0x13U,
	PCF85363A_TSR1_DAYS = 0x14U,
	PCF85363A_TSR1_MONTHS = 0x15U,
	PCF85363A_TSR1_YEARS = 0x16U,
	// RTC Timestamp 2
	PCF85363A_TSR2_SECONDS = 0x17U,
	PCF85363A_TSR2_MINUTES = 0x18U,
	PCF85363A_TSR2_HOURS = 0x19U,
	PCF85363A_TSR2_DAYS = 0x1AU,
	PCF85363A_TSR2_MONTHS = 0x1BU,
	PCF85363A_TSR2_YEARS = 0x1CU,
	// RTC Timestamp 3
	PCF85363A_TSR3_SECONDS = 0x1DU,
	PCF85363A_TSR3_MINUTES = 0x1EU,
	PCF85363A_TSR3_HOURS = 0x1FU,
	PCF85363A_TSR3_DAYS = 0x20U,
	PCF85363A_TSR3_MONTHS = 0x21U,
	PCF85363A_TSR3_YEARS = 0x22U,
	// RTC Timestamp Control
	PCF85363A_TSR_MODE = 0x23U,

// CONTROL REGISTERS start here
	PCF85363A_OFFSET = 0x24U,

	PCF85363A_OSCILLATOR = 0x25U,
	PCF85363A_BATTERY_SWITCH = 0x26U,
	PCF85363A_PIN_IO = 0x27U,
	PCF85363A_FUNCTION = 0x28U,
	PCF85363A_INTA_ENABLE = 0x29U,
	PCF85363A_INTB_ENABLE = 0x2AU,
	PCF85363A_FLAGS = 0x2BU,
	// Single RAM byte
	PCF85363A_RAM_BYTE = 0x2CU,

	PCF85363A_WATCHDOG = 0x2DU,

	PCF85363A_STOP_ENABLE = 0x2EU,
	PCF85363A_RESETS = 0x2FU,
	// From here on, there are 64 bytes of RAM [0x40 - 0x7F]
	PCF85363A_RAM_START = 0x40U,
	PCF85363A_RAM_END = 0x7FU,
};

enum pcf85363a_rtc_bits {
	// PCF85363A_SECONDS
	PCF85363A_OSCILLATOR_STOP = BIT(7),
	// PCF85363A_MINUTES
	PCF85363A_EMON = BIT(7),
	// PCF85363A_HOURS
	PCF85363A_AMPM = BIT(5),
	// WEEKDAYS
	PCF85363A_SUNDAY = 0U,
	PCF85363A_MONDAY = 1U,
	PCF85363A_TUESDAY = 2U,
	PCF85363A_WEDNESDAY = 3U,
	PCF85363A_THURSDAY = 4U,
	PCF85363A_FRIDAY = 5U,
	PCF85363A_SATURDAY = 6U,
	// MONTHS (BCD encoded)
	PCF85363A_JANUARY = 0x01U,
	PCF85363A_FEBRUARY = 0x02U,
	PCF85363A_MARCH = 0x03U,
	PCF85363A_APRIL = 0x04U,
	PCF85363A_MAY = 0x05U,
	PCF85363A_JUNE = 0x06U,
	PCF85363A_JULY = 0x07U,
	PCF85363A_AUGUST = 0x08U,
	PCF85363A_SEPTEMBER = 0x09U,
	PCF85363A_OCTOBER = 0x10U,
	PCF85363A_NOVEMBER = 0x11U,
	PCF85363A_DECEMBER = 0x12U,
	// PCF85363A_ALARM_ENABLES
	PCF85363A_SEC_A1E = BIT(0),
	PCF85363A_MIN_A1E = BIT(1),
	PCF85363A_HR_A1E = BIT(2),
	PCF85363A_DAY_A1E = BIT(3),
	PCF85363A_MON_A1E = BIT(4),
	PCF85363A_MIN_A2E = BIT(5),
	PCF85363A_HR_A2E = BIT(6),
	PCF85363A_WDAY_A2E = BIT(7),
	// PCF85363A_TSR_MODE
	PCF85363A_TSR1_MASK = BIT_MASK(2),
	PCF85363A_TSR2_MASK = BIT_MASK(3) << 2,
	PCF85363A_TSR2_SHIFT = 2,
	// Bit 6 is unused
	PCF85363A_TSR3_MASK = BIT_MASK(2) << 6,
	PCF85363A_TSR3_SHIFT = 6,
}



enum pcf85363a_control_bits {
	// PCF85363A_OSCILLATOR
	PCF85363A_OSC_CL_MASK = GENMASK(1, 0),
	PCF85363A_OSC_OSCD_MASK = GENMASK(3, 2),
	PCF85363A_OSC_LOWJ = BIT(4),
	PCF85363A_OSC_12_24 = BIT(5),
	PCF85363A_OSC_OFFM = BIT(6),
	PCF85363A_OSC_CLKIV = BIT(7),
	// PCF85363A_BATTERY_SWITCH
	PCF85363A_BATTERY_SWITCH_BSTH = BIT(0),
	PCF85363A_BATTERY_SWITCH_BSM = GENMASK(2, 1),
	PCF85363A_BATTERY_SWITCH_BSRR = BIT(3),
	PCF85363A_BATTERY_SWITCH_BSOFF = BIT(4),
	// PCF85363A_PIN_IO
	PCF85363A_PIN_IO_INTAPM = GENMASK(1, 0),
	PCF85363A_PIN_IO_TSPM = GENMASK(3, 2),
	PCF85363A_PIN_IO_TSIM = BIT(4),
	PCF85363A_PIN_IO_TSLE = BIT(5),
	PCF85363A_PIN_IO_TSPULL = BIT(6),
	PCF85363A_PIN_IO_CLKPM = BIT(7),
	// PCF85363A_FUNCTION
	PCF85363A_FUNCTION_COF = GENMASK(2, 0),
	PCF85363A_FUNCTION_STOPM = BIT(3),
	PCF85363A_FUNCTION_RTCM = BIT(4),
	PCF85363A_FUNCTION_PI = GENMASK(6, 5),
	PCF85363A_FUNCTION_100TH = BIT(7),
	// PCF85363A_INTA_ENABLE
	PCF85363A_INTA_ENABLE_WDIEA = BIT(0),
	PCF85363A_INTA_ENABLE_BSIEA = BIT(1),
	PCF85363A_INTA_ENABLE_TSRIEA = BIT(2),
	PCF85363A_INTA_ENABLE_A2IEA = BIT(3),
	PCF85363A_INTA_ENABLE_A1IEA = BIT(4),
	PCF85363A_INTA_ENABLE_OIEA = BIT(5),
	PCF85363A_INTA_ENABLE_PIEA = BIT(6),
	PCF85363A_INTA_ENABLE_ILPA = BIT(7),
	// PCF85363A_INTB_ENABLE
	PCF85363A_INTB_ENABLE_WDIEB = BIT(0),
	PCF85363A_INTB_ENABLE_BSIEB = BIT(1),
	PCF85363A_INTB_ENABLE_TSRIEB = BIT(2),
	PCF85363A_INTB_ENABLE_A2IEB = BIT(3),
	PCF85363A_INTB_ENABLE_A1IEB = BIT(4),
	PCF85363A_INTB_ENABLE_OIEB = BIT(5),
	PCF85363A_INTB_ENABLE_PIEB = BIT(6),
	PCF85363A_INTB_ENABLE_ILPB = BIT(7),
	// PCF85363A_FLAGS
	PCF85363A_FLAGS_TSR1F = BIT(0),
	PCF85363A_FLAGS_TSR2F = BIT(1),
	PCF85363A_FLAGS_TSR3F = BIT(2),
	PCF85363A_FLAGS_BSF = BIT(3),
	PCF85363A_FLAGS_WDF = BIT(4),
	PCF85363A_FLAGS_A1F = BIT(5),
	PCF85363A_FLAGS_A2F = BIT(6),
	PCF85363A_FLAGS_PIF = BIT(7),
	// PCF85363A_WATCHDOG
	PCF85363A_WATCHDOG_WDS_MASK = GENMASK(1, 0),
	PCF85363A_WATCHDOG_WDR = GENMASK(6, 2),
	PCF85363A_WATCHDOG_WDM = BIT(7),
	// PCF85363A_STOP_ENABLE
	PCF85363A_STOP_ENABLE_STOP = BIT(0),
	// PCF85363A_RESETS
	PCF85363A_RESETS_CTSR = BIT(0),
	PCF85363A_RESETS_SR = BIT(3),
	PCF85363A_RESETS_CPR = BIT(7)
};

struct pcf85363a_config {
	struct i2c_dt_spec i2c;
	struct gpio_dt_spec int_a;
	struct gpio_dt_spec int_b;
};

struct pcf85363a_data {

};


/*
 * @brief Read one or more registers from the device
 *
 * The registers are 8-bit wide, and the device uses auto-increment
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param addr Starting register address
 * @param data Pointer to the buffer to store read data
 * @param len Number of bytes to read
 * @return 0 if successful, negative error code otherwise
 */
static int pcf85363a_read_regs(const struct device *dev, uint8_t addr, uint8_t *data, size_t len)
{
	const struct pcf85363a_config *config = dev->config;
	uint8_t reg_addr = addr;
	int err;

	err = i2c_write_dt(&config->i2c, &reg_addr, sizeof(reg_addr));
	if (err != 0) {
		LOG_ERR("failed to write reg addr 0x%02x (err %d)", addr, err);
		return err;
	}

	err = i2c_read_dt(&config->i2c, data, len);
	if (err != 0) {
		LOG_ERR("failed to read reg addr 0x%02x, len %d (err %d)", addr, len, err);
		return err;
	}

	return 0;
}

static int pcf85363a_write_regs(const struct device *dev, uint8_t addr, const uint8_t *data, size_t len)
{
	const struct pcf85363a_config *config = dev->config;
	uint8_t block[1 + len];
	int err;

	block[0] = addr;
	memcpy(&block[1], data, len);

	err = i2c_write_dt(&config->i2c, block, sizeof(block));
	if (err != 0) {
		LOG_ERR("failed to write reg addr 0x%02x, len %d (err %d)", addr, len, err);
		return err;
	}

	return 0;
}

/*
 * @brief Check if the RTC is running
 *
 * @param dev Pointer to the device structure for the driver instance
 * @return true if the RTC is running, false otherwise
 */
static bool pcf85363a_is_rtc_running(const struct device *dev)
{
	uint8_t sec_reg;
	int err;
	err = pcf85363a_read_regs(dev, PCF85363A_SECONDS, &sec_reg, 1);
	if (err != 0) {
		return false; // Assume not running if read fails
	}
	return !(sec_reg & PCF85363A_OSCILLATOR_STOP);
}


/*
 * @brief Write time to the RTC
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param timeptr Pointer to the rtc_time structure containing the time to set
 * @return 0 if successful, negative error code otherwise
 */
static int pcf85363a_set_time(const struct device *dev, const struct rtc_time *timeptr)
{
	uint8_t regs[8];
	int err;

	int csec = timeptr->tm_nsec / 10000000; // Convert nanoseconds to centiseconds

	// Prepare time data in BCD format
	regs[0] = bin2bcd(csec) & 0x7FU; // Centiseconds
	regs[1] = bin2bcd(timeptr->tm_sec) & 0x7FU; // Seconds
	regs[2] = bin2bcd(timeptr->tm_min) & 0x7FU; // Minutes
	regs[3] = bin2bcd(timeptr->tm_hour) & 0x3FU; // Hours (24-hour format)
	regs[4] = bin2bcd(timeptr->tm_mday) & 0x3FU; // Days
	regs[5] = (timeptr->tm_wday % 7) & 0x07U; // Weekdays (0=Sunday)
	regs[6] = bin2bcd((timeptr->tm_mon + 1)) & 0x1FU; // Months (1-12)
	regs[7] = bin2bcd(timeptr->tm_year % 100); // Years (00-99)

	// Write time registers
	err = pcf85363a_write_regs(dev, PCF85363A_CENTISECONDS, regs, sizeof(regs));
	if (err != 0) {
		return err;
	}

	// Verify if the oscillator stop flag is cleared
	if(!pcf85363a_is_rtc_running(dev)) {
		LOG_ERR("Oscillator stop flag is still set after setting time");
		return -EIO;
	}

	return 0;
}

/*
 * @brief Read time from the RTC
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param timeptr Pointer to the rtc_time structure to store the read time
 * @return 0 if successful, negative error code otherwise
 */
static int pcf85363a_get_time(const struct device *dev, struct rtc_time *timeptr)
{
	uint8_t regs[8];
	int err;

	if (!timeptr)
	{
		LOG_ERR("Invalid timeptr");
		return -EINVAL;
	}

	// Read time registers
	err = pcf85363a_read_regs(dev, PCF85363A_CENTISECONDS, regs, sizeof(regs));
	if (err != 0) {
		return err;
	}

	if (regs[1] & PCF85363A_OSCILLATOR_STOP) {
		LOG_ERR("Oscillator stop flag is set, time is invalid");
		return -EIO;
	}

	timeptr->tm_sec = bcd2bin(regs[1] & 0x7FU);
	timeptr->tm_min = bcd2bin(regs[2] & 0x7FU);
	timeptr->tm_hour = bcd2bin(regs[3] & 0x3FU);
	timeptr->tm_mday = bcd2bin(regs[4] & 0x3FU);
	timeptr->tm_wday = regs[5] & 0x07U;
	timeptr->tm_mon = bcd2bin(regs[6] & 0x1FU) - 1; // tm_mon is 0-11
	timeptr->tm_year = bcd2bin(regs[7]) + 2000 - 1900; // tm_year is years since 1900

	timeptr->tm_isdst = -1; // Not supported
	timeptr->tm_yday = -1; // Not supported
	timeptr->tm_nsec = 0; // Not supported

	return 0;
}

static int pcf85363a_init(const struct device *dev)
{
	const struct pcf85363a_config *config = dev->config;
	uint8_t func_reg;
	int err;

	if (!device_is_ready(config->i2c.bus)) {
		LOG_ERR("I2C bus %s not ready", config->i2c.bus->name);
		return -ENODEV;
	}

	if (config->int_a.port && !device_is_ready(config->int_a.port)) {
		LOG_ERR("INTA GPIO port %s not ready", config->int_a.port->name);
		return -ENODEV;
	}

	if (config->int_b.port && !device_is_ready(config->int_b.port)) {
		LOG_ERR("INTB GPIO port %s not ready", config->int_b.port->name);
		return -ENODEV;
	}

	// Configure INTA pin if defined
	if (config->int_a.port) {
		err = gpio_pin_configure_dt(&config->int_a, GPIO_INPUT);
		if (err != 0) {
			LOG_ERR("Failed to configure INTA pin");
			return err;
		}
	}

	// Configure INTB pin if defined
	if (config->int_b.port) {
		err = gpio_pin_configure_dt(&config->int_b, GPIO_INPUT);
		if (err != 0) {
			LOG_ERR("Failed to configure INTB pin");
			return err;
		}
	}

	// Disable unnecessary IO features for basic RTC operation
	uint8_t pinio_val = PCF85363A_PIN_IO_CLKPM	// CLKOUT disabled (fixed at 0v)
						| PCF85363A_PIN_IO_INTAPM; // INTA = Hi-z
	err = pcf85363a_write_regs(dev, PCF85363A_PIN_IO, &pinio_val, 1);
	if (err != 0) {
		LOG_ERR("Failed to configure pin I/O settings");
		return err;
	}

	return 0;
}

static const struct rtc_driver_api pcf85363a_api = {
	.set_time = pcf85363a_set_time,
	.get_time = pcf85363a_get_time,
	// Alarm and calibration functions can be added here
};

#define PCF85363A_INIT(inst)                                                \
	static const struct pcf85363a_config pcf85363a_config_##inst = {        \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                 \
		.int_a = GPIO_DT_SPEC_INST_GET_OR(inst, int_a, {0}),               \
		.int_b = GPIO_DT_SPEC_INST_GET_OR(inst, int_b, {0}),               \
	};                                                                     \
																		   \
	static struct pcf85363a_data pcf85363a_data_##inst;                    \
																		   \
	DEVICE_DT_INST_DEFINE(inst,                                            \
						  pcf85363a_init,                                  \
						  NULL,                                           \
						  &pcf85363a_data_##inst,                         \
						  &pcf85363a_config_##inst,                       \
						  POST_KERNEL,                                    \
						  CONFIG_RTC_INIT_PRIORITY,                       \
						  &pcf85363a_api);

DT_INST_FOREACH_STATUS_OKAY(PCF85363A_INIT)
