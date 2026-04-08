/*
 * Copyright (c) 2026 Charles Dias
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adafruit_seesaw_gamepad

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

LOG_MODULE_REGISTER(input_adafruit_seesaw_gamepad, CONFIG_INPUT_LOG_LEVEL);

/* Seesaw product code for the gamepad */
#define SEESAW_PRODUCT_CODE     5743

/* Seesaw register addresses (16-bit big-endian) */
#define SEESAW_STATUS_HW_ID     0x0001
#define SEESAW_STATUS_VERSION   0x0002
#define SEESAW_STATUS_SWRST     0x007F
#define SEESAW_GPIO_DIRCLR_BULK 0x0103
#define SEESAW_GPIO_BULK        0x0104
#define SEESAW_GPIO_BULK_SET    0x0105
#define SEESAW_GPIO_PULLENSET   0x010B
#define SEESAW_ADC_BASE         0x0900
#define SEESAW_ADC_OFFSET       0x07

/* Seesaw ADC channel indices for the joystick */
#define SEESAW_ANALOG_X 0x0E
#define SEESAW_ANALOG_Y 0x0F
#define SEESAW_ADC_X    (SEESAW_ADC_BASE | (SEESAW_ADC_OFFSET + SEESAW_ANALOG_X))
#define SEESAW_ADC_Y    (SEESAW_ADC_BASE | (SEESAW_ADC_OFFSET + SEESAW_ANALOG_Y))

/* Button pin assignments on the seesaw */
#define SEESAW_BUTTON_SELECT BIT(0)
#define SEESAW_BUTTON_B      BIT(1)
#define SEESAW_BUTTON_Y      BIT(2)
#define SEESAW_BUTTON_A      BIT(5)
#define SEESAW_BUTTON_X      BIT(6)
#define SEESAW_BUTTON_START  BIT(16)

#define SEESAW_BUTTON_MASK                                                                         \
	(SEESAW_BUTTON_A | SEESAW_BUTTON_B | SEESAW_BUTTON_X | SEESAW_BUTTON_Y |                   \
	 SEESAW_BUTTON_SELECT | SEESAW_BUTTON_START)

/* 10-bit ADC full-scale value */
#define SEESAW_JOYSTICK_AXIS_MAX 1023U

/* Time to wait after a software reset before communicating */
#define SEESAW_RESET_DELAY_MS 15

struct seesaw_gamepad_config {
	struct i2c_dt_spec bus;
	int polling_interval_ms;
};

struct seesaw_gamepad_data {
	const struct device *dev;
	struct k_work_delayable work;
	uint32_t btn_state;
	uint16_t axis_x;
	uint16_t axis_y;
};

struct seesaw_button_map {
	uint32_t bit;
	uint16_t code;
};

static const struct seesaw_button_map seesaw_button_map[] = {
	{SEESAW_BUTTON_SELECT, INPUT_BTN_SELECT}, {SEESAW_BUTTON_B, INPUT_BTN_EAST},
	{SEESAW_BUTTON_Y, INPUT_BTN_Y},           {SEESAW_BUTTON_A, INPUT_BTN_SOUTH},
	{SEESAW_BUTTON_X, INPUT_BTN_X},           {SEESAW_BUTTON_START, INPUT_BTN_START},
};

static int seesaw_reg_read(const struct i2c_dt_spec *bus, uint16_t reg, void *buf, size_t len)
{
	uint8_t reg_buf[2];

	sys_put_be16(reg, reg_buf);

	return i2c_write_read_dt(bus, reg_buf, sizeof(reg_buf), buf, len);
}

static int seesaw_reg_write_u8(const struct i2c_dt_spec *bus, uint16_t reg, uint8_t val)
{
	uint8_t buf[3];

	sys_put_be16(reg, buf);
	buf[2] = val;

	return i2c_write_dt(bus, buf, sizeof(buf));
}

static int seesaw_reg_write_u32(const struct i2c_dt_spec *bus, uint16_t reg, uint32_t val)
{
	uint8_t buf[6];

	sys_put_be16(reg, buf);
	sys_put_be32(val, &buf[2]);

	return i2c_write_dt(bus, buf, sizeof(buf));
}

static void seesaw_gamepad_poll(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct seesaw_gamepad_data *data = CONTAINER_OF(dwork, struct seesaw_gamepad_data, work);
	const struct device *dev = data->dev;
	const struct seesaw_gamepad_config *cfg = dev->config;
	uint8_t buf[4];
	uint32_t btn_state;
	uint32_t changed;
	uint16_t new_x, new_y;
	bool y_changed;
	bool sync_flag;
	int ret;

	/* Read the 32-bit GPIO bulk register for button state */
	ret = seesaw_reg_read(&cfg->bus, SEESAW_GPIO_BULK, buf, sizeof(buf));
	if (ret < 0) {
		LOG_ERR("Failed to read GPIO bulk: %d", ret);
		goto reschedule;
	}

	/* Buttons are active-low: invert and mask to get pressed bits */
	btn_state = ~sys_get_be32(buf) & SEESAW_BUTTON_MASK;
	changed = btn_state ^ data->btn_state;
	data->btn_state = btn_state;

	for (size_t i = 0; i < ARRAY_SIZE(seesaw_button_map); i++) {
		if (changed & seesaw_button_map[i].bit) {
			input_report_key(dev, seesaw_button_map[i].code,
					 (btn_state & seesaw_button_map[i].bit) != 0, true,
					 K_FOREVER);
		}
	}

	/* Read joystick X axis */
	ret = seesaw_reg_read(&cfg->bus, SEESAW_ADC_X, buf, 2);
	if (ret < 0) {
		LOG_ERR("Failed to read ADC X: %d", ret);
		goto reschedule;
	}
	/* Hardware reads left=1023, right=0; invert so left=0, right=1023 */
	new_x = SEESAW_JOYSTICK_AXIS_MAX - sys_get_be16(buf);

	/* Read joystick Y axis */
	ret = seesaw_reg_read(&cfg->bus, SEESAW_ADC_Y, buf, 2);
	if (ret < 0) {
		LOG_ERR("Failed to read ADC Y: %d", ret);
		goto reschedule;
	}
	new_y = sys_get_be16(buf);
	y_changed = new_y != data->axis_y;

	if (new_x != data->axis_x) {
		data->axis_x = new_x;
		sync_flag = !y_changed;
		input_report_abs(dev, INPUT_ABS_X, new_x, sync_flag, K_FOREVER);
	}
	if (y_changed) {
		data->axis_y = new_y;
		input_report_abs(dev, INPUT_ABS_Y, new_y, true, K_FOREVER);
	}

reschedule:
	k_work_reschedule(dwork, K_MSEC(cfg->polling_interval_ms));
}

static int seesaw_gamepad_init(const struct device *dev)
{
	const struct seesaw_gamepad_config *cfg = dev->config;
	struct seesaw_gamepad_data *data = dev->data;
	uint8_t hw_id;
	uint8_t buf[2];
	uint16_t prod_code;
	int ret;

	data->dev = dev;

	if (!i2c_is_ready_dt(&cfg->bus)) {
		LOG_ERR("I2C bus not ready");
		return -ENODEV;
	}

	/* Software reset. Seesaw requires a delay before further access */
	ret = seesaw_reg_write_u8(&cfg->bus, SEESAW_STATUS_SWRST, 0xFF);
	if (ret < 0) {
		LOG_ERR("Reset failed: %d", ret);
		return ret;
	}
	k_msleep(SEESAW_RESET_DELAY_MS);

	/* Verify the chip is responding */
	ret = seesaw_reg_read(&cfg->bus, SEESAW_STATUS_HW_ID, &hw_id, sizeof(hw_id));
	if (ret < 0) {
		LOG_ERR("Failed to read HW ID: %d", ret);
		return ret;
	}

	LOG_DBG("Seesaw HW ID: 0x%02x", hw_id);

	/* Read upper 2 bytes of the VERSION register to get the product code */
	ret = seesaw_reg_read(&cfg->bus, SEESAW_STATUS_VERSION, buf, sizeof(buf));
	if (ret < 0) {
		LOG_ERR("Failed to read product code: %d", ret);
		return ret;
	}
	prod_code = sys_get_be16(buf);

	if (prod_code != SEESAW_PRODUCT_CODE) {
		LOG_ERR("Unexpected product code: %u (expected %u)", prod_code,
			SEESAW_PRODUCT_CODE);
		return -ENODEV;
	}

	/* Set button pins as inputs */
	ret = seesaw_reg_write_u32(&cfg->bus, SEESAW_GPIO_DIRCLR_BULK, SEESAW_BUTTON_MASK);
	if (ret < 0) {
		LOG_ERR("Failed to set GPIO direction: %d", ret);
		return ret;
	}

	/* Enable internal pull-up resistors on button pins */
	ret = seesaw_reg_write_u32(&cfg->bus, SEESAW_GPIO_PULLENSET, SEESAW_BUTTON_MASK);
	if (ret < 0) {
		LOG_ERR("Failed to enable pull-ups: %d", ret);
		return ret;
	}

	/* Assert pull-ups high so buttons are idle-high / active-low */
	ret = seesaw_reg_write_u32(&cfg->bus, SEESAW_GPIO_BULK_SET, SEESAW_BUTTON_MASK);
	if (ret < 0) {
		LOG_ERR("Failed to set GPIO bulk high: %d", ret);
		return ret;
	}

	k_work_init_delayable(&data->work, seesaw_gamepad_poll);
	k_work_reschedule(&data->work, K_MSEC(cfg->polling_interval_ms));

	return 0;
}

#define SEESAW_GAMEPAD_INIT(inst)                                                                  \
	static const struct seesaw_gamepad_config seesaw_cfg_##inst = {                            \
		.bus = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.polling_interval_ms = DT_INST_PROP(inst, polling_interval_ms),                    \
	};                                                                                         \
	BUILD_ASSERT(DT_INST_PROP(inst, polling_interval_ms) > 8);                                 \
                                                                                                   \
	static struct seesaw_gamepad_data seesaw_data_##inst;                                      \
	DEVICE_DT_INST_DEFINE(inst, &seesaw_gamepad_init, NULL, &seesaw_data_##inst,               \
			      &seesaw_cfg_##inst, POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(SEESAW_GAMEPAD_INIT)
