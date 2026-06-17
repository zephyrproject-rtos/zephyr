/*
 * Copyright (c) 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "adis16607.h"

LOG_MODULE_REGISTER(ADIS16607, CONFIG_SENSOR_LOG_LEVEL);

/* ---------------------------------------------------------------------------
 * SPI half-duplex I/O helpers
 * ---------------------------------------------------------------------------
 *
 * Read protocol (16-bit register):
 *   Transfer 1: TX [reg|0x80, 0, 0, 0] - device latches command, CS toggles
 *   Transfer 2: TX [reg|0x80, 0, 0, 0] - device responds; data in RX[2:3]
 *
 * Write protocol (16-bit register):
 *   Transfer 1: TX [reg, val_hi, val_lo] - single 3-byte frame
 *
 * Burst read:
 *   Single transfer with CS held low: TX/RX 58 bytes (4 cmd + 54 data)
 */

static int adis16607_read_reg_16(const struct device *dev, uint8_t reg, uint16_t *val)
{
	const struct adis16607_config *cfg = dev->config;
	uint8_t tx[4] = {reg | BIT(7), 0U, 0U, 0U};
	uint8_t rx[4] = {0};
	const struct spi_buf tx_buf = {.buf = tx, .len = sizeof(tx)};
	struct spi_buf rx_buf = {.buf = rx, .len = sizeof(rx)};
	const struct spi_buf_set tx_set = {.buffers = &tx_buf, .count = 1};
	struct spi_buf_set rx_set = {.buffers = &rx_buf, .count = 1};
	int ret;

	/* First transfer: send command - device latches it, CS deasserts */
	ret = spi_transceive_dt(&cfg->spi, &tx_set, NULL);
	if (ret) {
		LOG_ERR("SPI read (cmd) failed: %d", ret);
		return ret;
	}

	/* Minimum stall time before the device is ready to respond. */
	k_busy_wait(ADIS16607_STALL_TIME_US);

	/* Second transfer: same command - device responds with data in [2:3] */
	ret = spi_transceive_dt(&cfg->spi, &tx_set, &rx_set);
	if (ret) {
		LOG_ERR("SPI read (data) failed: %d", ret);
		return ret;
	}

	*val = sys_get_be16(&rx[2]);
	return 0;
}

static int adis16607_write_reg_16(const struct device *dev, uint8_t reg, uint16_t val)
{
	const struct adis16607_config *cfg = dev->config;
	uint8_t tx[3] = {reg, (uint8_t)(val >> 8), (uint8_t)(val & 0xFF)};
	const struct spi_buf tx_buf = {.buf = tx, .len = sizeof(tx)};
	const struct spi_buf_set tx_set = {.buffers = &tx_buf, .count = 1};
	int ret;

	ret = spi_write_dt(&cfg->spi, &tx_set);
	if (ret) {
		LOG_ERR("SPI write failed: %d", ret);
	}
	return ret;
}

static int adis16607_update_reg_16(const struct device *dev, uint8_t reg, uint16_t mask,
				   uint16_t val)
{
	uint16_t cur;
	int ret;

	ret = adis16607_read_reg_16(dev, reg, &cur);
	if (ret) {
		return ret;
	}

	return adis16607_write_reg_16(dev, reg, (cur & ~mask) | (val & mask));
}

/*
 * Burst 32-bit word layout:
 *   byte 0,1 = MSW (bits 23..8 of sensor value)
 *   byte 2,3 = LSW (bits 7..0 of sensor value in bits [15:8])
 *   24-bit = (MSW << 8) | (LSW >> 8)
 */
static int32_t adis16607_parse_24bit(const uint8_t *p)
{
	uint32_t msw = sys_get_be16(p);
	uint32_t lsw = sys_get_be16(p + 2);
	uint32_t raw24 = (msw << 8) | (lsw >> 8);

	return sign_extend(raw24, 23);
}

static int adis16607_burst_read(const struct device *dev)
{
	const struct adis16607_config *cfg = dev->config;
	struct adis16607_data *data = dev->data;
	uint8_t buf[ADIS16607_BURST_TOTAL] = {0};
	const uint8_t *d = buf + ADIS16607_BURST_CMD_SIZE;
	const struct spi_buf spi_buf = {.buf = buf, .len = sizeof(buf)};
	const struct spi_buf_set buf_set = {.buffers = &spi_buf, .count = 1};
	int ret;

	buf[0] = ADIS16607_BURST_CMD;

	ret = spi_transceive_dt(&cfg->spi, &buf_set, &buf_set);
	if (ret) {
		LOG_ERR("Burst read SPI transfer failed: %d", ret);
		return ret;
	}

	/*
	 * Verify burst checksum: 16-bit sum of all words except the checksum
	 * word itself (first ADIS16607_BURST_DATA_LEN-2 bytes = 26 words).
	 */
	uint16_t chk_calc = 0;

	for (int i = 0; i < ADIS16607_BURST_DATA_LEN - 2; i += 2) {
		chk_calc += sys_get_be16(d + i);
	}

	uint16_t chk_rx = sys_get_be16(d + ADIS16607_BURST_OFF_CHECKSUM);

	if (chk_calc != chk_rx) {
		LOG_ERR("Burst checksum error: calc 0x%04x != rx 0x%04x", chk_calc, chk_rx);
		return -EIO;
	}

	/* Track data counter to detect missed samples. */
	uint16_t data_cntr = sys_get_be16(d + ADIS16607_BURST_OFF_DATA_CNTR);

	if (data->data_cntr_valid && (uint16_t)(data_cntr - data->data_cntr) > 1U) {
		LOG_WRN("Missed %u sample(s) (cntr %u -> %u)",
			(unsigned int)((uint16_t)(data_cntr - data->data_cntr) - 1U),
			data->data_cntr, data_cntr);
	}
	data->data_cntr = data_cntr;
	data->data_cntr_valid = true;

	data->accel_x = adis16607_parse_24bit(d + ADIS16607_BURST_OFF_ACCEL_X);
	data->accel_y = adis16607_parse_24bit(d + ADIS16607_BURST_OFF_ACCEL_Y);
	data->accel_z = adis16607_parse_24bit(d + ADIS16607_BURST_OFF_ACCEL_Z);
	data->gyro_x = adis16607_parse_24bit(d + ADIS16607_BURST_OFF_GYRO_X);
	data->gyro_y = adis16607_parse_24bit(d + ADIS16607_BURST_OFF_GYRO_Y);
	data->gyro_z = adis16607_parse_24bit(d + ADIS16607_BURST_OFF_GYRO_Z);
	data->temp = (int16_t)sys_get_be16(d + ADIS16607_BURST_OFF_TEMP);

	return 0;
}

static int adis16607_sw_reset(const struct device *dev)
{
	return adis16607_write_reg_16(dev, ADIS16607_REG_SW_RES, BIT(0));
}

static int adis16607_check_status(const struct device *dev)
{
	uint16_t diag1, diag2, diag;
	int ret;

	/* First read reflects latched fault bits, then clears them. */
	ret = adis16607_read_reg_16(dev, ADIS16607_REG_DIAG_STAT, &diag1);
	if (ret) {
		return ret;
	}

	/* Second read reflects any fault still asserted after clearing. */
	ret = adis16607_read_reg_16(dev, ADIS16607_REG_DIAG_STAT, &diag2);
	if (ret) {
		return ret;
	}

	/* Combine both reads so a fault latched-and-cleared by the first read
	 * is not missed just because it did not persist into the second read.
	 */
	diag = diag1 | diag2;

	if (diag & ADIS16607_DIAG_ERROR_MASK) {
		if (diag & ADIS16607_DIAG_BOOT_MEM_FAIL) {
			LOG_ERR("Boot memory failure");
		}
		if (diag & ADIS16607_DIAG_PWR_FAIL) {
			LOG_ERR("Power supply failure");
		}
		if (diag & ADIS16607_DIAG_ACCEL_FAIL) {
			LOG_ERR("Accelerometer failure");
		}
		if (diag & ADIS16607_DIAG_GYRO_FAIL) {
			LOG_ERR("Gyroscope failure");
		}
		return -EIO;
	}

	return 0;
}

static int adis16607_sensor_self_test(const struct device *dev)
{
	uint16_t st1[6], st2[6];
	const int16_t delta_max[6] = {
		ADIS16607_ACCEL_XY_DELTA_MAX, /* X accel */
		ADIS16607_ACCEL_XY_DELTA_MAX, /* Y accel */
		ADIS16607_ACCEL_Z_DELTA_MAX,  /* Z accel */
		ADIS16607_GYRO_DELTA_MAX,     /* X gyro  */
		ADIS16607_GYRO_DELTA_MAX,     /* Y gyro  */
		ADIS16607_GYRO_DELTA_MAX,     /* Z gyro  */
	};
	int ret;

	ret = adis16607_write_reg_16(dev, ADIS16607_REG_SELF_TEST, ADIS16607_SNSR_SELF_TEST_MASK);
	if (ret) {
		return ret;
	}

	/* Allow the sensor element to settle before reading reference data. */
	k_msleep(ADIS16607_SELF_TEST_DELAY_MS);

	for (int i = 0; i < 6; i++) {
		ret = adis16607_read_reg_16(dev, ADIS16607_REG_SELF_TEST_DATA(i), &st1[i]);
		if (ret) {
			return ret;
		}
	}

	ret = adis16607_write_reg_16(dev, ADIS16607_REG_SELF_TEST,
				     ADIS16607_SNSR_SELF_TEST_MASK | ADIS16607_ST_FORCE_MASK);
	if (ret) {
		return ret;
	}

	/* Allow the sensor element to settle under forced stimulus. */
	k_msleep(ADIS16607_SELF_TEST_DELAY_MS);

	for (int i = 0; i < 6; i++) {
		int32_t delta;

		ret = adis16607_read_reg_16(dev, ADIS16607_REG_SELF_TEST_DATA(i), &st2[i]);
		if (ret) {
			return ret;
		}

		delta = (int16_t)st1[i] - (int16_t)st2[i];
		if (delta < 0) {
			delta = -delta;
		}
		if (delta > delta_max[i]) {
			LOG_ERR("Self-test axis %d delta %d > limit %d", i, (int)delta,
				(int)delta_max[i]);
			return -EIO;
		}
	}

	return adis16607_write_reg_16(dev, ADIS16607_REG_SELF_TEST, 0U);
}

static int adis16607_initial_startup(const struct device *dev)
{
	const struct adis16607_config *cfg = dev->config;
	uint16_t digital_status;
	uint16_t gpio_cfg;
	int ret;

	if (cfg->reset_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&cfg->reset_gpio)) {
			LOG_ERR_DEVICE_NOT_READY(cfg->reset_gpio.port);
			return -ENODEV;
		}
		ret = gpio_pin_configure_dt(&cfg->reset_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret) {
			return ret;
		}
		k_usleep(ADIS16607_RESET_PULSE_US);
		gpio_pin_set_dt(&cfg->reset_gpio, 0);
		k_msleep(ADIS16607_RESET_DELAY_MS);
	} else {
		ret = adis16607_sw_reset(dev);
		if (ret) {
			return ret;
		}
		k_msleep(ADIS16607_SW_RESET_DELAY_MS);
	}

	k_msleep(ADIS16607_STARTUP_DELAY_MS);

	ret = adis16607_write_reg_16(dev, ADIS16607_REG_SPI_HALFDUPLEX,
				     ADIS16607_LOCK_SPI_HALFDUPLEX);
	if (ret) {
		LOG_ERR("Failed to lock SPI half-duplex mode: %d", ret);
		return ret;
	}

	{
		uint16_t half_duplex;

		ret = adis16607_read_reg_16(dev, ADIS16607_REG_SPI_HALFDUPLEX, &half_duplex);
		if (ret) {
			LOG_ERR("Failed to read back SPI half-duplex lock: %d", ret);
			return ret;
		}
		if (half_duplex != ADIS16607_LOCK_SPI_HALFDUPLEX) {
			LOG_ERR("SPI half-duplex lock not confirmed: 0x%04x", half_duplex);
			return -EIO;
		}
	}

	/*
	 * Clear any fault bits latched during the reset transient (e.g. a brief
	 * power-supply dip caused by the startup current surge).  Do not treat
	 * them as errors here — only check status after self-test.
	 */
	{
		uint16_t diag;

		adis16607_read_reg_16(dev, ADIS16607_REG_DIAG_STAT, &diag);
	}

	ret = adis16607_read_reg_16(dev, ADIS16607_REG_DIGITAL_STATUS, &digital_status);
	if (ret) {
		return ret;
	}
	if (digital_status & ADIS16607_BOOTLOADER_BUSY_MASK) {
		LOG_ERR("Bootloader busy");
		return -EBUSY;
	}

	/*
	 * Verify the device responds with the expected device ID as early as
	 * possible, but only once SPI half-duplex mode is locked in and the
	 * bootloader-busy gate above has cleared — reading before that point
	 * risks either the wrong SPI protocol or stale/garbage data from a
	 * device that is still booting.
	 */
	{
		uint16_t dev_id;

		ret = adis16607_read_reg_16(dev, ADIS16607_REG_DEV_ID, &dev_id);
		if (ret) {
			LOG_ERR("Failed to read device ID: %d", ret);
			return ret;
		}
		if (dev_id != cfg->chip_info->dev_id) {
			LOG_ERR("Device ID mismatch: expected 0x%04x, got 0x%04x",
				cfg->chip_info->dev_id, dev_id);
			return -ENODEV;
		}
	}

	gpio_cfg = ADIS16607_GPIO_ACTIVE_DATA_RDY;
	if (cfg->reset_gpio.port != NULL) {
		gpio_cfg |= ADIS16607_GPIO_ACTIVE_RESET;
	}
	ret = adis16607_write_reg_16(dev, ADIS16607_REG_USER_GPIO_CFG, gpio_cfg);
	if (ret) {
		return ret;
	}

	ret = adis16607_sensor_self_test(dev);
	if (ret) {
		LOG_ERR("Self-test failed: %d", ret);
		return ret;
	}

	ret = adis16607_check_status(dev);
	if (ret) {
		return ret;
	}

	ret = adis16607_update_reg_16(dev, ADIS16607_REG_USER_DATA_CFG,
				      ADIS16607_BURST32_MASK | ADIS16607_DATA_CNTR_EN_MASK,
				      ADIS16607_BURST32_MASK | ADIS16607_DATA_CNTR_EN_MASK);
	if (ret) {
		LOG_ERR("Failed to enable burst32/data-cntr: %d", ret);
		return ret;
	}

	return 0;
}

static int adis16607_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	/*
	 * The ADIS16607 always performs a full burst read regardless of the
	 * requested channel. Individual axis channels are accepted here for
	 * API compatibility so callers can use sensor_sample_fetch_chan()
	 * with any supported channel.
	 */
	switch (chan) {
	case SENSOR_CHAN_ALL:
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
	case SENSOR_CHAN_GYRO_XYZ:
	case SENSOR_CHAN_DIE_TEMP:
		return adis16607_burst_read(dev);
	default:
		return -ENOTSUP;
	}
}

/*
 * Convert a desired ODR (in milli-Hz, i.e. Hz * 1000) into a DEC_RATE value,
 * rounding to the nearest achievable rate. Rejects frequencies that are zero
 * or above the base clock, which would otherwise underflow the "- 1U" below
 * and silently program the slowest possible rate instead of failing.
 */
static int adis16607_calc_dec_rate(uint32_t freq_mhz, uint16_t *dec_rate)
{
	uint32_t base_mhz = ADIS16607_BASE_CLK_HZ * 1000U;

	if (freq_mhz == 0U || freq_mhz > base_mhz) {
		return -EINVAL;
	}

	*dec_rate = (uint16_t)CLAMP((base_mhz + freq_mhz / 2U) / freq_mhz - 1U, 0U, 65535U);

	return 0;
}

static int adis16607_attr_set_odr(const struct device *dev, const struct sensor_value *val)
{
	uint64_t freq_mhz_64;
	uint32_t freq_mhz;
	uint16_t dec;
	int ret;

	if (val->val1 < 0 || (val->val1 == 0 && val->val2 <= 0)) {
		return -EINVAL;
	}

	/* Compute in 64-bit so an out-of-range val1 is rejected below instead
	 * of silently wrapping around in 32-bit arithmetic.
	 */
	freq_mhz_64 = (uint64_t)val->val1 * 1000U + (uint64_t)(val->val2 / 1000U);
	if (freq_mhz_64 > UINT32_MAX) {
		return -EINVAL;
	}
	freq_mhz = (uint32_t)freq_mhz_64;

	ret = adis16607_calc_dec_rate(freq_mhz, &dec);
	if (ret) {
		return ret;
	}

	return adis16607_write_reg_16(dev, ADIS16607_REG_DEC_RATE, dec);
}

static int adis16607_attr_get_odr(const struct device *dev, struct sensor_value *val)
{
	uint32_t base_mhz = ADIS16607_BASE_CLK_HZ * 1000U;
	uint32_t freq_mhz;
	uint16_t dec;
	int ret;

	ret = adis16607_read_reg_16(dev, ADIS16607_REG_DEC_RATE, &dec);
	if (ret) {
		return ret;
	}

	freq_mhz = base_mhz / ((uint32_t)dec + 1U);

	val->val1 = (int32_t)(freq_mhz / 1000U);
	val->val2 = (int32_t)((freq_mhz % 1000U) * 1000U);

	return 0;
}

static int adis16607_attr_set(const struct device *dev, enum sensor_channel chan,
			      enum sensor_attribute attr, const struct sensor_value *val)
{
	switch (attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return adis16607_attr_set_odr(dev, val);
	default:
		return -ENOTSUP;
	}
}

static int adis16607_attr_get(const struct device *dev, enum sensor_channel chan,
			      enum sensor_attribute attr, struct sensor_value *val)
{
	switch (attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return adis16607_attr_get_odr(dev, val);
	default:
		return -ENOTSUP;
	}
}

static void adis16607_convert_accel(struct sensor_value *val, int32_t raw)
{
	/*
	 * 1 LSB = ADIS16607_ACCEL_SCALE_MICRO / ADIS16607_ACCEL_DENOM um/s^2
	 *       = 392266000 / 8388608 um/s^2, ~= 46.77 um/s^2
	 */
	int64_t micro = (int64_t)raw * ADIS16607_ACCEL_SCALE_MICRO / ADIS16607_ACCEL_DENOM;

	val->val1 = (int32_t)(micro / 1000000LL);
	val->val2 = (int32_t)(micro % 1000000LL);
}

static void adis16607_convert_gyro(struct sensor_value *val, int32_t raw, int32_t scale_micro)
{
	/*
	 * micro_rad_s = raw * scale_micro / ADIS16607_GYRO_DENOM
	 *   scale_micro: full-scale value in urad/s at 2^23 LSB.
	 */
	int64_t micro = (int64_t)raw * scale_micro / ADIS16607_GYRO_DENOM;

	val->val1 = (int32_t)(micro / 1000000LL);
	val->val2 = (int32_t)(micro % 1000000LL);
}

static void adis16607_convert_temp(struct sensor_value *val, int16_t raw)
{
	/*
	 * degC = (raw + OFFSET) * SCALE / 1000000
	 *   raw = 0 at 25 degC; scale = 5 mdegC/LSB = 5000 udegC/LSB.
	 */
	int64_t micro_c = (int64_t)(raw + ADIS16607_TEMP_OFFSET) * ADIS16607_TEMP_SCALE_MICRO_C;

	val->val1 = (int32_t)(micro_c / 1000000LL);
	val->val2 = (int32_t)(micro_c % 1000000LL);
}

static int adis16607_channel_get(const struct device *dev, enum sensor_channel chan,
				 struct sensor_value *val)
{
	const struct adis16607_config *cfg = dev->config;
	const struct adis16607_data *data = dev->data;
	int32_t gyro_scale = cfg->chip_info->gyro_scale_micro;

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
		adis16607_convert_accel(val, data->accel_x);
		break;
	case SENSOR_CHAN_ACCEL_Y:
		adis16607_convert_accel(val, data->accel_y);
		break;
	case SENSOR_CHAN_ACCEL_Z:
		adis16607_convert_accel(val, data->accel_z);
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
		adis16607_convert_accel(&val[0], data->accel_x);
		adis16607_convert_accel(&val[1], data->accel_y);
		adis16607_convert_accel(&val[2], data->accel_z);
		break;
	case SENSOR_CHAN_GYRO_X:
		adis16607_convert_gyro(val, data->gyro_x, gyro_scale);
		break;
	case SENSOR_CHAN_GYRO_Y:
		adis16607_convert_gyro(val, data->gyro_y, gyro_scale);
		break;
	case SENSOR_CHAN_GYRO_Z:
		adis16607_convert_gyro(val, data->gyro_z, gyro_scale);
		break;
	case SENSOR_CHAN_GYRO_XYZ:
		adis16607_convert_gyro(&val[0], data->gyro_x, gyro_scale);
		adis16607_convert_gyro(&val[1], data->gyro_y, gyro_scale);
		adis16607_convert_gyro(&val[2], data->gyro_z, gyro_scale);
		break;
	case SENSOR_CHAN_DIE_TEMP:
		adis16607_convert_temp(val, data->temp);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

#ifdef CONFIG_ADIS16607_TRIGGER

static void adis16607_thread_cb(const struct device *dev)
{
	struct adis16607_data *data = dev->data;
	const struct adis16607_config *cfg = dev->config;
	sensor_trigger_handler_t handler;
	const struct sensor_trigger *trigger;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);
	handler = data->drdy_handler;
	trigger = data->drdy_trigger;
	k_spin_unlock(&data->lock, key);

	if (handler != NULL) {
		handler(dev, trigger);
	}

	/*
	 * Re-check after invoking the handler: it may have unregistered itself
	 * via sensor_trigger_set(dev, trig, NULL), which already disabled the
	 * GPIO interrupt. Re-arming unconditionally here would leave it armed.
	 */
	key = k_spin_lock(&data->lock);
	handler = data->drdy_handler;
	k_spin_unlock(&data->lock, key);

	if (handler != NULL) {
		gpio_pin_interrupt_configure_dt(&cfg->drdy_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	}
}

static void adis16607_gpio_callback(const struct device *gpio_dev, struct gpio_callback *cb,
				    uint32_t pins)
{
	ARG_UNUSED(gpio_dev);
	ARG_UNUSED(pins);

	struct adis16607_data *data = CONTAINER_OF(cb, struct adis16607_data, gpio_cb);
	const struct adis16607_config *cfg = data->dev->config;

	gpio_pin_interrupt_configure_dt(&cfg->drdy_gpio, GPIO_INT_DISABLE);

#if defined(CONFIG_ADIS16607_TRIGGER_OWN_THREAD)
	k_sem_give(&data->gpio_sem);
#elif defined(CONFIG_ADIS16607_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->work);
#endif
}

#if defined(CONFIG_ADIS16607_TRIGGER_OWN_THREAD)
static void adis16607_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct adis16607_data *data = p1;

	while (true) {
		k_sem_take(&data->gpio_sem, K_FOREVER);
		adis16607_thread_cb(data->dev);
	}
}
#elif defined(CONFIG_ADIS16607_TRIGGER_GLOBAL_THREAD)
static void adis16607_work_cb(struct k_work *work)
{
	struct adis16607_data *data = CONTAINER_OF(work, struct adis16607_data, work);

	adis16607_thread_cb(data->dev);
}
#endif

int adis16607_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			  sensor_trigger_handler_t handler)
{
	struct adis16607_data *data = dev->data;
	const struct adis16607_config *cfg = dev->config;
	k_spinlock_key_t key;

	if (cfg->drdy_gpio.port == NULL) {
		LOG_ERR("Trigger requires drdy-gpios in DT");
		return -ENOTSUP;
	}

	if (trig->type != SENSOR_TRIG_DATA_READY) {
		return -ENOTSUP;
	}

	gpio_pin_interrupt_configure_dt(&cfg->drdy_gpio, GPIO_INT_DISABLE);

	key = k_spin_lock(&data->lock);
	data->drdy_handler = handler;
	data->drdy_trigger = trig;
	k_spin_unlock(&data->lock, key);

	if (handler != NULL) {
		gpio_pin_interrupt_configure_dt(&cfg->drdy_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	}

	return 0;
}

int adis16607_init_interrupt(const struct device *dev)
{
	struct adis16607_data *data = dev->data;
	const struct adis16607_config *cfg = dev->config;
	int ret;

	data->dev = dev;

	if (!gpio_is_ready_dt(&cfg->drdy_gpio)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->drdy_gpio.port);
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&cfg->drdy_gpio, GPIO_INPUT);
	if (ret) {
		LOG_ERR("Failed to configure DRDY GPIO: %d", ret);
		return ret;
	}

	gpio_init_callback(&data->gpio_cb, adis16607_gpio_callback, BIT(cfg->drdy_gpio.pin));
	ret = gpio_add_callback(cfg->drdy_gpio.port, &data->gpio_cb);
	if (ret) {
		LOG_ERR("Failed to add DRDY GPIO callback: %d", ret);
		return ret;
	}

#if defined(CONFIG_ADIS16607_TRIGGER_OWN_THREAD)
	k_sem_init(&data->gpio_sem, 0, K_SEM_MAX_LIMIT);
	k_thread_create(&data->thread, data->thread_stack,
			K_KERNEL_STACK_SIZEOF(data->thread_stack), adis16607_thread, data, NULL,
			NULL, CONFIG_ADIS16607_THREAD_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&data->thread, "adis16607");
#elif defined(CONFIG_ADIS16607_TRIGGER_GLOBAL_THREAD)
	k_work_init(&data->work, adis16607_work_cb);
#endif

	return 0;
}
#endif /* CONFIG_ADIS16607_TRIGGER */

static int adis16607_init(const struct device *dev)
{
	const struct adis16607_config *cfg = dev->config;
	int ret;

	if (!spi_is_ready_dt(&cfg->spi)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->spi.bus);
		return -ENODEV;
	}

	ret = adis16607_initial_startup(dev);
	if (ret) {
		LOG_ERR("Initial startup failed: %d", ret);
		return ret;
	}

	if (cfg->odr > 0) {
		uint16_t dec_rate;

		ret = adis16607_calc_dec_rate((uint32_t)cfg->odr * 1000U, &dec_rate);
		if (ret) {
			LOG_ERR("Invalid odr %u Hz in devicetree", cfg->odr);
			return ret;
		}

		ret = adis16607_write_reg_16(dev, ADIS16607_REG_DEC_RATE, dec_rate);
		if (ret) {
			LOG_ERR("Failed to set decimation rate: %d", ret);
			return ret;
		}
	}

#ifdef CONFIG_ADIS16607_TRIGGER
	if (cfg->drdy_gpio.port != NULL) {
		ret = adis16607_init_interrupt(dev);
		if (ret) {
			LOG_ERR("Failed to initialise trigger: %d", ret);
			return ret;
		}
	}
#endif

	LOG_INF("ADIS16607 initialised (dev_id=0x%04x)", cfg->chip_info->dev_id);
	return 0;
}

static DEVICE_API(sensor, adis16607_driver_api) = {
	.sample_fetch = adis16607_sample_fetch,
	.channel_get = adis16607_channel_get,
	.attr_set = adis16607_attr_set,
	.attr_get = adis16607_attr_get,
#ifdef CONFIG_ADIS16607_TRIGGER
	.trigger_set = adis16607_trigger_set,
#endif
};

#if DT_HAS_COMPAT_STATUS_OKAY(adi_adis16607_2)
static const struct adis16607_chip_info adis16607_2_info = {
	.gyro_scale_micro = ADIS16607_2_GYRO_SCALE_MICRO,
	.dev_id = 0x6000,
};
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(adi_adis16607_3)
static const struct adis16607_chip_info adis16607_3_info = {
	.gyro_scale_micro = ADIS16607_3_GYRO_SCALE_MICRO,
	.dev_id = 0x6000,
};
#endif

#define ADIS16607_SPI_CFG SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_MODE_CPOL | SPI_MODE_CPHA

#ifdef CONFIG_ADIS16607_TRIGGER
#define ADIS16607_CFG_TRIGGER(inst) .drdy_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, drdy_gpios, {})
#else
#define ADIS16607_CFG_TRIGGER(inst)
#endif

#define ADIS16607_INIT(inst, name, chip_info_ptr)                                                  \
	static struct adis16607_data adis16607_data_##name##_##inst;                               \
	\
	static const struct adis16607_config adis16607_cfg_##name##_##inst = {                    \
		.spi = SPI_DT_SPEC_INST_GET(inst, ADIS16607_SPI_CFG),                              \
		.chip_info = chip_info_ptr,                                                        \
		.odr = DT_INST_PROP(inst, odr),                                                    \
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {}),                     \
		ADIS16607_CFG_TRIGGER(inst)};                                                      \
	\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, adis16607_init, NULL,                                   \
				     &adis16607_data_##name##_##inst,                              \
				     &adis16607_cfg_##name##_##inst, POST_KERNEL,                  \
				     CONFIG_SENSOR_INIT_PRIORITY, &adis16607_driver_api)

#if DT_HAS_COMPAT_STATUS_OKAY(adi_adis16607_2)
#define DT_DRV_COMPAT adi_adis16607_2
DT_INST_FOREACH_STATUS_OKAY_VARGS(ADIS16607_INIT, DT_DRV_COMPAT, &adis16607_2_info)
#undef DT_DRV_COMPAT
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(adi_adis16607_3)
#define DT_DRV_COMPAT adi_adis16607_3
DT_INST_FOREACH_STATUS_OKAY_VARGS(ADIS16607_INIT, DT_DRV_COMPAT, &adis16607_3_info)
#undef DT_DRV_COMPAT
#endif
