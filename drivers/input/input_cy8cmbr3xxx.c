/*
 * Copyright (c) 2025 Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT cypress_cy8cmbr3xxx

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/input/cy8cmbr3xxx.h>
#include <zephyr/input/input.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(cy8cmbr3xxx, CONFIG_INPUT_LOG_LEVEL);

#define CY8CMBR3XXX_SENSOR_EN                 0x00
#define CY8CMBR3XXX_FSS_EN                    0x02
#define CY8CMBR3XXX_TOGGLE_EN                 0x04
#define CY8CMBR3XXX_LED_ON_EN                 0x06
#define CY8CMBR3XXX_SENSITIVITY0              0x08
#define CY8CMBR3XXX_SENSITIVITY1              0x09
#define CY8CMBR3XXX_SENSITIVITY2              0x0A
#define CY8CMBR3XXX_SENSITIVITY3              0x0B
#define CY8CMBR3XXX_BASE_THRESHOLD0           0x0C
#define CY8CMBR3XXX_BASE_THRESHOLD1           0x0D
#define CY8CMBR3XXX_FINGER_THRESHOLD2         0x0E
#define CY8CMBR3XXX_FINGER_THRESHOLD3         0x0F
#define CY8CMBR3XXX_FINGER_THRESHOLD4         0x10
#define CY8CMBR3XXX_FINGER_THRESHOLD5         0x11
#define CY8CMBR3XXX_FINGER_THRESHOLD6         0x12
#define CY8CMBR3XXX_FINGER_THRESHOLD7         0x13
#define CY8CMBR3XXX_FINGER_THRESHOLD8         0x14
#define CY8CMBR3XXX_FINGER_THRESHOLD9         0x15
#define CY8CMBR3XXX_FINGER_THRESHOLD10        0x16
#define CY8CMBR3XXX_FINGER_THRESHOLD11        0x17
#define CY8CMBR3XXX_FINGER_THRESHOLD12        0x18
#define CY8CMBR3XXX_FINGER_THRESHOLD13        0x19
#define CY8CMBR3XXX_FINGER_THRESHOLD14        0x1A
#define CY8CMBR3XXX_FINGER_THRESHOLD15        0x1B
#define CY8CMBR3XXX_SENSOR_DEBOUNCE           0x1C
#define CY8CMBR3XXX_BUTTON_HYS                0x1D
#define CY8CMBR3XXX_BUTTON_LBR                0x1F
#define CY8CMBR3XXX_BUTTON_NNT                0x20
#define CY8CMBR3XXX_BUTTON_NT                 0x21
#define CY8CMBR3XXX_PROX_EN                   0x26
#define CY8CMBR3XXX_PROX_CFG                  0x27
#define CY8CMBR3XXX_PROX_CFG2                 0x28
#define CY8CMBR3XXX_PROX_TOUCH_TH0            0x2A
#define CY8CMBR3XXX_PROX_TOUCH_TH1            0x2C
#define CY8CMBR3XXX_PROX_RESOLUTION0          0x2E
#define CY8CMBR3XXX_PROX_RESOLUTION1          0x2F
#define CY8CMBR3XXX_PROX_HYS                  0x30
#define CY8CMBR3XXX_PROX_LBR                  0x32
#define CY8CMBR3XXX_PROX_NNT                  0x33
#define CY8CMBR3XXX_PROX_NT                   0x34
#define CY8CMBR3XXX_PROX_POSITIVE_TH0         0x35
#define CY8CMBR3XXX_PROX_POSITIVE_TH1         0x36
#define CY8CMBR3XXX_PROX_NEGATIVE_TH0         0x39
#define CY8CMBR3XXX_PROX_NEGATIVE_TH1         0x3A
#define CY8CMBR3XXX_LED_ON_TIME               0x3D
#define CY8CMBR3XXX_BUZZER_CFG                0x3E
#define CY8CMBR3XXX_BUZZER_ON_TIME            0x3F
#define CY8CMBR3XXX_GPO_CFG                   0x40
#define CY8CMBR3XXX_PWM_DUTYCYCLE_CFG0        0x41
#define CY8CMBR3XXX_PWM_DUTYCYCLE_CFG1        0x42
#define CY8CMBR3XXX_PWM_DUTYCYCLE_CFG2        0x43
#define CY8CMBR3XXX_PWM_DUTYCYCLE_CFG3        0x44
#define CY8CMBR3XXX_PWM_DUTYCYCLE_CFG4        0x45
#define CY8CMBR3XXX_PWM_DUTYCYCLE_CFG5        0x46
#define CY8CMBR3XXX_PWM_DUTYCYCLE_CFG6        0x47
#define CY8CMBR3XXX_PWM_DUTYCYCLE_CFG7        0x48
#define CY8CMBR3XXX_SPO_CFG                   0x4C
#define CY8CMBR3XXX_DEVICE_CFG0               0x4D
#define CY8CMBR3XXX_DEVICE_CFG1               0x4E
#define CY8CMBR3XXX_DEVICE_CFG2               0x4F
#define CY8CMBR3XXX_DEVICE_CFG3               0x50
#define CY8CMBR3XXX_I2C_ADDR                  0x51
#define CY8CMBR3XXX_REFRESH_CTRL              0x52
#define CY8CMBR3XXX_STATE_TIMEOUT             0x55
#define CY8CMBR3XXX_SLIDER_CFG                0x5D
#define CY8CMBR3XXX_SLIDER1_CFG               0x61
#define CY8CMBR3XXX_SLIDER1_RESOLUTION        0x62
#define CY8CMBR3XXX_SLIDER1_THRESHOLD         0x63
#define CY8CMBR3XXX_SLIDER2_CFG               0x67
#define CY8CMBR3XXX_SLIDER2_RESOLUTION        0x68
#define CY8CMBR3XXX_SLIDER2_THRESHOLD         0x69
#define CY8CMBR3XXX_SLIDER_LBR                0x71
#define CY8CMBR3XXX_SLIDER_NNT                0x72
#define CY8CMBR3XXX_SLIDER_NT                 0x73
#define CY8CMBR3XXX_SCRATCHPAD0               0x7A
#define CY8CMBR3XXX_SCRATCHPAD1               0x7B
#define CY8CMBR3XXX_CONFIG_CRC                0x7E
#define CY8CMBR3XXX_GPO_OUTPUT_STATE          0x80
#define CY8CMBR3XXX_SENSOR_ID                 0x82
#define CY8CMBR3XXX_CTRL_CMD                  0x86
#define CY8CMBR3XXX_CTRL_CMD_STATUS           0x88
#define CY8CMBR3XXX_CTRL_CMD_ERR              0x89
#define CY8CMBR3XXX_SYSTEM_STATUS             0x8A
#define CY8CMBR3XXX_PREV_CTRL_CMD_CODE        0x8C
#define CY8CMBR3XXX_FAMILY_ID                 0x8F
#define CY8CMBR3XXX_DEVICE_ID                 0x90
#define CY8CMBR3XXX_DEVICE_REV                0x92
#define CY8CMBR3XXX_CALC_CRC                  0x94
#define CY8CMBR3XXX_TOTAL_WORKING_SNS         0x97
#define CY8CMBR3XXX_SNS_CP_HIGH               0x98
#define CY8CMBR3XXX_SNS_VDD_SHORT             0x9A
#define CY8CMBR3XXX_SNS_GND_SHORT             0x9C
#define CY8CMBR3XXX_SNS_SNS_SHORT             0x9E
#define CY8CMBR3XXX_CMOD_SHIELD_TEST          0xA0
#define CY8CMBR3XXX_BUTTON_STAT               0xAA
#define CY8CMBR3XXX_LATCHED_BUTTON_STAT       0xAC
#define CY8CMBR3XXX_PROX_STAT                 0xAE
#define CY8CMBR3XXX_LATCHED_PROX_STAT         0xAF
#define CY8CMBR3XXX_SLIDER1_POSITION          0xB0
#define CY8CMBR3XXX_LIFTOFF_SLIDER1_POSITION  0xB1
#define CY8CMBR3XXX_SLIDER2_POSITION          0xB2
#define CY8CMBR3XXX_LIFTOFF_SLIDER2_POSITION  0xB3
#define CY8CMBR3XXX_SYNC_COUNTER0             0xB9
#define CY8CMBR3XXX_DIFFERENCE_COUNT_SENSOR0  0xBA
#define CY8CMBR3XXX_DIFFERENCE_COUNT_SENSOR1  0xBC
#define CY8CMBR3XXX_DIFFERENCE_COUNT_SENSOR2  0xBE
#define CY8CMBR3XXX_DIFFERENCE_COUNT_SENSOR3  0xC0
#define CY8CMBR3XXX_DIFFERENCE_COUNT_SENSOR4  0xC2
#define CY8CMBR3XXX_DIFFERENCE_COUNT_SENSOR5  0xC4
#define CY8CMBR3XXX_DIFFERENCE_COUNT_SENSOR6  0xC6
#define CY8CMBR3XXX_DIFFERENCE_COUNT_SENSOR7  0xC8
#define CY8CMBR3XXX_DIFFERENCE_COUNT_SENSOR8  0xCA
#define CY8CMBR3XXX_DIFFERENCE_COUNT_SENSOR9  0xCC
#define CY8CMBR3XXX_DIFFERENCE_COUNT_SENSOR10 0xCE
#define CY8CMBR3XXX_DIFFERENCE_COUNT_SENSOR11 0xD0
#define CY8CMBR3XXX_DIFFERENCE_COUNT_SENSOR12 0xD2
#define CY8CMBR3XXX_DIFFERENCE_COUNT_SENSOR13 0xD4
#define CY8CMBR3XXX_DIFFERENCE_COUNT_SENSOR14 0xD6
#define CY8CMBR3XXX_DIFFERENCE_COUNT_SENSOR15 0xD8
#define CY8CMBR3XXX_GPO_DATA                  0xDA
#define CY8CMBR3XXX_SYNC_COUNTER1             0xDB
#define CY8CMBR3XXX_DEBUG_SENSOR_ID           0xDC
#define CY8CMBR3XXX_DEBUG_CP                  0xDD
#define CY8CMBR3XXX_DEBUG_DIFFERENCE_COUNT0   0xDE
#define CY8CMBR3XXX_DEBUG_BASELINE0           0xE0
#define CY8CMBR3XXX_DEBUG_RAW_COUNT0          0xE2
#define CY8CMBR3XXX_DEBUG_AVG_RAW_COUNT0      0xE4
#define CY8CMBR3XXX_SYNC_COUNTER2             0xE7

#define CY8CMBR3XXX_CTRL_CMD_CALC_CRC 0x02
#define CY8CMBR3XXX_CTRL_CMD_RESET    0xFF

/* The controller wakes up from the low-power state on an address match but sends NACK
 * until it transitions into the Active state. When the device NACKs a transaction, the host is
 * expected to retry the transaction until it receives an ACK. Typically, no more than 3 retries are
 * necessary, depending on time between the interrupt and the first i2c transfer or if
 * no interrupt has caused the initiation of the communication.
 */
#define CY8CMBR3XXX_I2C_RETRIES 5

struct cy8cmbr3xxx_config {
	struct i2c_dt_spec i2c;
	struct gpio_dt_spec int_gpio;
	struct gpio_dt_spec rst_gpio;
	const uint16_t *input_codes;
	uint8_t input_codes_count;
	const uint16_t *proximity_codes;
	uint8_t proximity_codes_count;
};

struct cy8cmbr3xxx_data {
	const struct device *dev;
	struct k_work work;
	struct gpio_callback int_gpio_cb;
	uint16_t prev_button_state;
	uint8_t prev_proximity_state;
};

static int cy8cmbr3xxx_i2c_read(const struct device *dev, uint8_t address, void *buf, size_t len)
{
	const struct cy8cmbr3xxx_config *config = dev->config;
	int ret;

	for (int i = 0; i < CY8CMBR3XXX_I2C_RETRIES; i++) {
		ret = i2c_write_read_dt(&config->i2c, &address, sizeof(address), buf, len);
		if (ret == 0) {
			break;
		}
	}

	return ret;
}

static int cy8cmbr3xxx_i2c_write(const struct device *dev, uint8_t address, const void *buf,
				 size_t len)
{
	const struct cy8cmbr3xxx_config *config = dev->config;
	int ret;

	for (int i = 0; i < CY8CMBR3XXX_I2C_RETRIES; i++) {
		ret = i2c_burst_write_dt(&config->i2c, address, buf, len);
		if (ret == 0) {
			break;
		}
	}

	return ret;
}

static int cy8cmbr3xxx_wait_for_command_completion(const struct device *dev, k_timeout_t timeout)
{
	uint8_t current_command;
	int ret;
	k_timepoint_t end = sys_timepoint_calc(timeout);

	do {
		/* Wait for the completion of the command. After a reset command, it can
		 * happen that the device NACKs for some time.
		 */
		ret = cy8cmbr3xxx_i2c_read(dev, CY8CMBR3XXX_CTRL_CMD, &current_command,
					   sizeof(uint8_t));

		/* As soon as current_command is 0x00, the command is completed */
		if (ret == 0 && current_command == 0x00) {
			return 0;
		}

		k_msleep(1);
	} while (!sys_timepoint_expired(end));

	LOG_ERR("Wait for command completion timed out");

	return -ETIMEDOUT;
}

int cy8cmbr3xxx_configure(const struct device *dev, const struct cy8cmbr3xxx_config_data *config)
{
	int ret;
	uint8_t read_config[CY8CMBR3XXX_EZ_CLICK_CONFIG_SIZE];
	uint8_t command;

	if (config == NULL) {
		return -EINVAL;
	}

	/* Read the complete configuration */
	ret = cy8cmbr3xxx_i2c_read(dev, CY8CMBR3XXX_SENSOR_EN, read_config, sizeof(read_config));
	if (ret < 0) {
		LOG_ERR("Failed to read i2c (%d)", ret);
		return ret;
	}

	if (memcmp(read_config, config->data, sizeof(config->data)) == 0) {
		return 0;
	}

	/* Write the complete configuration of 128 bytes to the CY8CMBR3XXX controller */
	ret = cy8cmbr3xxx_i2c_write(dev, CY8CMBR3XXX_SENSOR_EN, config->data, sizeof(config->data));
	if (ret < 0) {
		LOG_ERR("Failed to write i2c (%d)", ret);
		return ret;
	}

	/* The device calculates a CRC checksum over the configuration data in this
	 * register map and compares the result with the content of CONFIG_CRC. If the
	 * two values match, the device saves the configuration and the CRC checksum to
	 * nonvolatile memory.
	 */
	command = CY8CMBR3XXX_CTRL_CMD_CALC_CRC;
	ret = cy8cmbr3xxx_i2c_write(dev, CY8CMBR3XXX_CTRL_CMD, &command, 1);
	if (ret < 0) {
		LOG_ERR("Failed to write i2c (%d)", ret);
		return ret;
	}

	/* 600ms seems to be sufficient */
	ret = cy8cmbr3xxx_wait_for_command_completion(dev, K_MSEC(600));
	if (ret < 0) {
		LOG_ERR("Failed to wait for command completion (%d)", ret);
		return ret;
	}

	/* The device resets itself */
	command = CY8CMBR3XXX_CTRL_CMD_RESET;
	ret = cy8cmbr3xxx_i2c_write(dev, CY8CMBR3XXX_CTRL_CMD, &command, 1);
	if (ret < 0) {
		LOG_ERR("Failed to write i2c (%d)", ret);
		return ret;
	}

	ret = cy8cmbr3xxx_wait_for_command_completion(dev, K_MSEC(50));
	if (ret < 0) {
		LOG_ERR("Failed to wait for command completion (%d)", ret);
		return ret;
	}

	return 0;
}

static int cy8cmbr3xxx_process(const struct device *dev)
{
	const struct cy8cmbr3xxx_config *config = dev->config;
	struct cy8cmbr3xxx_data *data = dev->data;
	int ret;
	uint16_t button_state, single_button_state;
	uint8_t proximity_state, single_proximity_state;

	/* Request button status */
	ret = cy8cmbr3xxx_i2c_read(dev, CY8CMBR3XXX_BUTTON_STAT, &button_state, sizeof(uint16_t));
	if (ret < 0) {
		LOG_ERR("Failed to read button status (%d)", ret);
		return ret;
	}

	for (uint8_t i = 0; i < config->input_codes_count; i++) {
		single_button_state = button_state & BIT(i);
		if (single_button_state != (data->prev_button_state & BIT(i))) {
			input_report_key(dev, config->input_codes[i], single_button_state, true,
					 K_FOREVER);
		}
	}
	data->prev_button_state = button_state;

	/* Request proximity status */
	if (config->proximity_codes_count > 0) {
		ret = cy8cmbr3xxx_i2c_read(dev, CY8CMBR3XXX_PROX_STAT, &proximity_state,
					   sizeof(uint8_t));
		if (ret < 0) {
			LOG_ERR("Failed to read proximity status (%d)", ret);
			return ret;
		}

		for (uint8_t i = 0; i < config->proximity_codes_count; i++) {
			single_proximity_state = proximity_state & BIT(i);
			if (single_proximity_state != (data->prev_proximity_state & BIT(i))) {
				input_report_key(dev, config->proximity_codes[i],
						 single_proximity_state, true, K_FOREVER);
			}
		}
		data->prev_proximity_state = proximity_state;
	}

	return 0;
}

static void cy8cmbr3xxx_work_handler(struct k_work *work)
{
	struct cy8cmbr3xxx_data *data = CONTAINER_OF(work, struct cy8cmbr3xxx_data, work);

	cy8cmbr3xxx_process(data->dev);
}

static void cy8cmbr3xxx_isr_handler(const struct device *dev, struct gpio_callback *cb,
				    uint32_t pins)
{
	struct cy8cmbr3xxx_data *data = CONTAINER_OF(cb, struct cy8cmbr3xxx_data, int_gpio_cb);

	k_work_submit(&data->work);
}

static void cy8cmbr3xxx_reset(const struct device *dev)
{
	const struct cy8cmbr3xxx_config *config = dev->config;
	int ret;

	if (!gpio_is_ready_dt(&config->rst_gpio)) {
		LOG_ERR("GPIO controller device not ready");
		return;
	}

	ret = gpio_pin_configure_dt(&config->rst_gpio, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		LOG_ERR("Could not configure reset GPIO pin (%d)", ret);
		return;
	}

	k_usleep(5);

	ret = gpio_pin_set_dt(&config->rst_gpio, 0);
	if (ret < 0) {
		LOG_ERR("Could not set reset GPIO pin (%d)", ret);
		return;
	}
}

static int cy8cmbr3xxx_init(const struct device *dev)
{
	const struct cy8cmbr3xxx_config *config = dev->config;
	struct cy8cmbr3xxx_data *data = dev->data;
	int ret;

	data->dev = dev;

	k_work_init(&data->work, cy8cmbr3xxx_work_handler);

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C controller device not ready");
		return -ENODEV;
	}

	cy8cmbr3xxx_reset(dev);

	if (!gpio_is_ready_dt(&config->int_gpio)) {
		LOG_ERR("GPIO controller device not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Could not configure interrupt GPIO pin (%d)", ret);
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		LOG_ERR("Could not configure GPIO interrupt (%d)", ret);
		return ret;
	}

	gpio_init_callback(&data->int_gpio_cb, cy8cmbr3xxx_isr_handler, BIT(config->int_gpio.pin));

	ret = gpio_add_callback(config->int_gpio.port, &data->int_gpio_cb);
	if (ret < 0) {
		LOG_ERR("Could not set gpio callback (%d)", ret);
		return ret;
	}

	return 0;
}

#define CY8CMBR3XXX_INIT(inst)                                                                     \
	static const uint16_t cy8cmbr3xxx_input_codes_##inst[] = DT_INST_PROP(inst, input_codes);  \
	static const uint16_t cy8cmbr3xxx_proximity_codes_##inst[] =                               \
		DT_INST_PROP_OR(inst, proximity_codes, {});                                        \
	static const struct cy8cmbr3xxx_config cy8cmbr3xxx_config_##inst = {                       \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.int_gpio = GPIO_DT_SPEC_INST_GET(inst, int_gpios),                                \
		.rst_gpio = GPIO_DT_SPEC_INST_GET(inst, rst_gpios),                                \
		.input_codes = cy8cmbr3xxx_input_codes_##inst,                                     \
		.input_codes_count = DT_INST_PROP_LEN(inst, input_codes),                          \
		.proximity_codes = cy8cmbr3xxx_proximity_codes_##inst,                             \
		.proximity_codes_count = DT_INST_PROP_LEN_OR(inst, proximity_codes, 0),            \
	};                                                                                         \
	static struct cy8cmbr3xxx_data cy8cmbr3xxx_data_##inst;                                    \
	DEVICE_DT_INST_DEFINE(inst, cy8cmbr3xxx_init, NULL, &cy8cmbr3xxx_data_##inst,              \
			      &cy8cmbr3xxx_config_##inst, POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY, \
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(CY8CMBR3XXX_INIT)
