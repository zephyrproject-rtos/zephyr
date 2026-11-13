/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/irq_offload.h>
#include <zephyr/ztest.h>

#define EXPANDER_NODE DT_NODELABEL(expander)
#define INPUT_LOW      0U
#define INPUT_HIGH     1U
#define OUTPUT_LOW     2U
#define OUTPUT_HIGH    3U
#define DIRECTION_LOW  6U
#define DIRECTION_HIGH 7U

struct pca_series_fixture {
	uint8_t registers[8];
	uint8_t cursor;
	int error;
	uint32_t transfers;
};

static int pca9555_emul_transfer(const struct emul *target, struct i2c_msg *msgs, int num_msgs,
				 int addr)
{
	struct pca_series_fixture *data = target->data;

	ARG_UNUSED(addr);
	data->transfers++;
	if (data->error != 0) {
		int error = data->error;

		data->error = 0;
		return error;
	}

	for (int32_t i = 0; i < num_msgs; i++) {
		struct i2c_msg *msg = &msgs[i];
		uint32_t offset = 0U;

		if ((msg->flags & I2C_MSG_READ) == 0U &&
		    (i == 0 || (msg->flags & I2C_MSG_RESTART) != 0U ||
		     (msgs[i - 1].flags & I2C_MSG_STOP) != 0U)) {
			if (msg->len == 0U) {
				return -EINVAL;
			}
			data->cursor = msg->buf[0];
			offset = 1U;
		}
		if (data->cursor + msg->len - offset > sizeof(data->registers)) {
			return -EINVAL;
		}
		for (; offset < msg->len; offset++) {
			if ((msg->flags & I2C_MSG_READ) != 0U) {
				msg->buf[offset] = data->registers[data->cursor++];
			} else {
				data->registers[data->cursor++] = msg->buf[offset];
			}
		}
	}

	return 0;
}

static int pca9555_emul_init(const struct emul *target, const struct device *parent)
{
	struct pca_series_fixture *data = target->data;

	ARG_UNUSED(parent);
	memset(data->registers, 0, sizeof(data->registers));
	data->registers[OUTPUT_LOW] = UINT8_MAX;
	data->registers[OUTPUT_HIGH] = UINT8_MAX;
	data->registers[DIRECTION_LOW] = UINT8_MAX;
	data->registers[DIRECTION_HIGH] = UINT8_MAX;
	return 0;
}

static const struct i2c_emul_api pca9555_emul_api = {
	.transfer = pca9555_emul_transfer,
};

static struct pca_series_fixture emul_data;

EMUL_DT_DEFINE(EXPANDER_NODE, pca9555_emul_init, &emul_data, NULL, &pca9555_emul_api, NULL);

static void *pca_series_setup(void)
{
	return &emul_data;
}

static void pca_series_before(void *fixture)
{
	struct pca_series_fixture *data = fixture;
	const struct device *dev = DEVICE_DT_GET(EXPANDER_NODE);

	zassert_true(device_is_ready(dev));
	data->error = 0;
	data->registers[INPUT_LOW] = 0U;
	data->registers[INPUT_HIGH] = 0U;
	zassert_ok(gpio_port_set_masked_raw(dev, UINT16_MAX, 0U));
	for (gpio_pin_t pin = 0U; pin < 16U; pin++) {
		zassert_ok(gpio_pin_configure(dev, pin, GPIO_INPUT));
	}
}

ZTEST_F(pca_series, test_outputs)
{
	struct pca_series_fixture *data = fixture;
	const struct device *dev = DEVICE_DT_GET(EXPANDER_NODE);

	zassert_ok(gpio_pin_configure(dev, 1U, GPIO_OUTPUT_INACTIVE));
	zassert_ok(gpio_pin_configure(dev, 10U, GPIO_OUTPUT_ACTIVE));
	zassert_equal(data->registers[DIRECTION_LOW], UINT8_MAX & ~BIT(1));
	zassert_equal(data->registers[DIRECTION_HIGH], UINT8_MAX & ~BIT(2));
	zassert_equal(data->registers[OUTPUT_LOW], 0U);
	zassert_equal(data->registers[OUTPUT_HIGH], BIT(2));
	zassert_ok(gpio_port_set_bits_raw(dev, BIT(1)));
	zassert_equal(data->registers[OUTPUT_LOW], BIT(1));
	zassert_ok(gpio_port_clear_bits_raw(dev, BIT(10)));
	zassert_equal(data->registers[OUTPUT_HIGH], 0U);
	zassert_ok(gpio_port_toggle_bits(dev, BIT(1) | BIT(10)));
	zassert_equal(data->registers[OUTPUT_LOW], 0U);
	zassert_equal(data->registers[OUTPUT_HIGH], BIT(2));
}

ZTEST_F(pca_series, test_inputs)
{
	struct pca_series_fixture *data = fixture;
	const struct device *dev = DEVICE_DT_GET(EXPANDER_NODE);
	gpio_port_value_t value;

	data->registers[INPUT_LOW] = 0xa5U;
	data->registers[INPUT_HIGH] = 0x5aU;
	zassert_ok(gpio_port_get_raw(dev, &value));
	zassert_equal(value & UINT16_MAX, 0x5aa5U);
}

ZTEST_F(pca_series, test_i2c_error_recovery)
{
	struct pca_series_fixture *data = fixture;
	const struct device *dev = DEVICE_DT_GET(EXPANDER_NODE);
	gpio_port_value_t value;

	data->error = -EIO;
	zassert_equal(gpio_port_get_raw(dev, &value), -EIO);
	zassert_ok(gpio_port_get_raw(dev, &value));
	data->error = -EIO;
	zassert_equal(gpio_port_set_bits_raw(dev, BIT(1)), -EIO);
	zassert_ok(gpio_port_clear_bits_raw(dev, BIT(1)));
}

struct isr_result {
	int read;
	int write;
	int configure;
};

static void pca_series_from_isr(const void *arg)
{
	struct isr_result *result = (struct isr_result *)arg;
	const struct device *dev = DEVICE_DT_GET(EXPANDER_NODE);
	gpio_port_value_t value;

	result->read = gpio_port_get_raw(dev, &value);
	result->write = gpio_port_set_bits_raw(dev, BIT(1));
	result->configure = gpio_pin_configure(dev, 1U, GPIO_OUTPUT);
}

ZTEST_F(pca_series, test_reject_isr_access)
{
	struct pca_series_fixture *data = fixture;
	struct isr_result result = {0};
	uint32_t transfers = data->transfers;

	irq_offload(pca_series_from_isr, &result);
	zassert_equal(result.read, -EWOULDBLOCK);
	zassert_equal(result.write, -EWOULDBLOCK);
	zassert_equal(result.configure, -EWOULDBLOCK);
	zassert_equal(data->transfers, transfers);
}

ZTEST_SUITE(pca_series, NULL, pca_series_setup, pca_series_before, NULL, NULL);
