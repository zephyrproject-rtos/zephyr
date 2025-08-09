/*
 * Copyright (c) 2025 Alex Bucknall
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT plantower_pmsa003i

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(PMSA003I, CONFIG_SENSOR_LOG_LEVEL);

/* PMSA003I response buffer offsets */
#define PMSA003I_OFFSET_PM_1_0 0x04
#define PMSA003I_OFFSET_PM_2_5 0x06
#define PMSA003I_OFFSET_PM_10  0x08

/* PMSA003I start bytes */
#define PMSA003I_START_BYTE_1 0x42
#define PMSA003I_START_BYTE_2 0x4d

/* PMSA003I data length */
#define PMSA003I_DATA_LEN 32

/* PMSA003I checksum length */
#define PMSA003I_CHECKSUM_LEN 30

/* PMSA003I timeout */
#define CFG_PMSA003I_TIMEOUT 1000

struct pmsa003i_data {
	uint16_t pm_1_0;
	uint16_t pm_2_5;
	uint16_t pm_10;
};

struct pmsa003i_config {
	struct i2c_dt_spec i2c;
};

static uint16_t pmsa003i_calculate_checksum(uint8_t *buf)
{
	uint16_t sum = 0;

	for (size_t i = 0; i < PMSA003I_CHECKSUM_LEN; i++) {
		sum += buf[i];
	}
	return sum;
}

static int pmsa003i_read_with_timeout(const struct pmsa003i_config *cfg,
				     uint8_t *buf, size_t len)
{
	int64_t timeout_ms = CFG_PMSA003I_TIMEOUT;
	int64_t start_time;
	int rc;

	start_time = k_uptime_get();

	while (k_uptime_get() - start_time < timeout_ms) {
		rc = i2c_read_dt(&cfg->i2c, buf, len);
		if (rc < 0) {
			return rc;
		}

		if (buf[0] == PMSA003I_START_BYTE_1 &&
		    buf[1] == PMSA003I_START_BYTE_2) {
			return 0;
		}

		k_sleep(K_MSEC(10));
	}

	return -ETIMEDOUT;
}

static int pmsa003i_sample_fetch(const struct device *dev,
				enum sensor_channel chan)
{
	const struct pmsa003i_config *cfg = dev->config;
	struct pmsa003i_data *data = dev->data;

	uint8_t read_buf[PMSA003I_DATA_LEN];
	uint16_t checksum;
	int rc;

	rc = pmsa003i_read_with_timeout(cfg, read_buf, sizeof(read_buf));
	if (rc < 0) {
		LOG_WRN("Failed to read valid data (err: %d)", rc);
		return rc;
	}

	checksum = pmsa003i_calculate_checksum(read_buf);
	uint16_t recv_checksum = sys_get_be16(&read_buf[30]);

	if (checksum != recv_checksum) {
		LOG_WRN("checksum mismatch (calc: 0x%04X, recv: 0x%04X)",
			checksum, recv_checksum);
		return -EIO;
	}

	data->pm_1_0 = sys_get_be16(&read_buf[PMSA003I_OFFSET_PM_1_0]);
	data->pm_2_5 = sys_get_be16(&read_buf[PMSA003I_OFFSET_PM_2_5]);
	data->pm_10 = sys_get_be16(&read_buf[PMSA003I_OFFSET_PM_10]);

	return 0;
}

static int pmsa003i_channel_get(const struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	const struct pmsa003i_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_PM_1_0:
		val->val1 = data->pm_1_0;
		val->val2 = 0;
		break;
	case SENSOR_CHAN_PM_2_5:
		val->val1 = data->pm_2_5;
		val->val2 = 0;
		break;
	case SENSOR_CHAN_PM_10:
		val->val1 = data->pm_10;
		val->val2 = 0;
		break;
	default:
		LOG_ERR("Unsupported channel: %d", chan);
		return -ENOTSUP;
	}

	return 0;
}

static DEVICE_API(sensor, pmsa003i_api) = {
	.sample_fetch = pmsa003i_sample_fetch,
	.channel_get = pmsa003i_channel_get,
};

static int pmsa003i_init(const struct device *dev)
{
	const struct pmsa003i_config *cfg = dev->config;

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

	return 0;
}

#define PMSA003I_DEFINE(inst)                                          \
                                                                       \
	static struct pmsa003i_data pmsa003i_data_##inst;              \
	static const struct pmsa003i_config pmsa003i_config_##inst = { \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                     \
	};                                                             \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, pmsa003i_init, NULL,       \
				     &pmsa003i_data_##inst,            \
				     &pmsa003i_config_##inst,          \
				     POST_KERNEL,                      \
				     CONFIG_SENSOR_INIT_PRIORITY,      \
				     &pmsa003i_api);

DT_INST_FOREACH_STATUS_OKAY(PMSA003I_DEFINE)
