/*
 * Copyright (c) 2023, ithinx GmbH
 * Copyright (c) 2023, Tonies GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/drivers/usb_c/usbc_tc.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#define DT_DRV_COMPAT ti_tps25750

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ucpd_tps25750, CONFIG_USBC_LOG_LEVEL);

#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <soc.h>
#include <stddef.h>
#include <zephyr/math/ilog2.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>

#include "ucpd_tps25750_priv.h"

#define TEST_BIT_ARR_ELEM(bit) (bit / 8)
#define TEST_BIT(buf, bit)     ((buf[TEST_BIT_ARR_ELEM(bit)] & (1 << (bit % 8))) != 0)
#define CLEAR_BIT(buf, bit)    (buf[TEST_BIT_ARR_ELEM(bit)] &= ~(1 << (bit % 8)))
#define SET_BIT(buf, bit)      (buf[TEST_BIT_ARR_ELEM(bit)] |= (1 << (bit % 8)))

#define MAX_I2C_TRANSFER_SIZE 64
#define CMD_WAIT_TIME_MS      1

/**
 * @brief Read from the TPS25750 I2C interface
 *
 * @note Skips the first "Byte Count" byte when returning data to the user
 *
 * @param dev Device pointer
 * @param reg Register to read from
 * @param data Buffer to put the data to
 * @param len Length to read
 * @return 0 on success
 * @return -EINVAL in case the requested len is too big
 * @return other error code returned by i2c_burst_read_dt
 */
static int tps25750_i2c_read(const struct device *dev, uint8_t reg, uint8_t *data, size_t len)
{
	if (len > MAX_I2C_TRANSFER_SIZE) {
		return -EINVAL;
	}

	const struct tcpc_config *const cfg = dev->config;
	uint8_t read_data[MAX_I2C_TRANSFER_SIZE + 1];

	const int ret = i2c_burst_read_dt(&cfg->i2c, reg, read_data, len + 1);

	if (ret != 0) {
		return ret;
	}

	/* Skip the byte count given in the 0th byte when returning data to the user */
	memcpy(data, &read_data[1], len);

	return 0;
}

/**
 * @brief Read from the TPS25750 I2C interface
 *
 * @note Automatically adds the "Byte Count" byte to the I2C operation
 *
 * @param dev Device pointer
 * @param reg Register to write to
 * @param data Data to write
 * @param len Length to write
 * @return other error code returned by i2c_write_dt
 */
static int tps25750_i2c_write(const struct device *dev, uint8_t reg, uint8_t const *data,
			      size_t len)
{
	if (len > MAX_I2C_TRANSFER_SIZE) {
		return -EINVAL;
	}

	const struct tcpc_config *const cfg = dev->config;
	uint8_t send_data[MAX_I2C_TRANSFER_SIZE + 2];

	send_data[0] = reg;
	send_data[1] = len;
	memcpy(&send_data[2], data, len);

	return i2c_write_dt(&cfg->i2c, send_data, len + 2);
}

/**
 * @brief Issue a so called 4 character command (4CC) to the chip
 *
 * With 4CC commands different tasks can be started. E.g. changing
 * between sink/source mode.
 *
 * @param dev Device
 * @param cmd The command to isse
 * @return int
 * @return other error code returned by tps25750_i2c_write
 */
static int tps25750_issue_4cc(const struct device *dev, char *cmd)
{
	int ret = tps25750_i2c_write(dev, TPS_CMD1_REG, (uint8_t *)cmd, 4);

	if (ret != 0) {
		LOG_ERR("Issuing 4cc %.4s failed\r", cmd);
		return ret;
	}

	const uint8_t max_retries = 25;

	for (uint8_t i = 0; i < max_retries; i++) {
		uint8_t read_cmd[TPS_CMD1_REG_SIZE];

		ret = tps25750_i2c_read(dev, TPS_CMD1_REG, read_cmd, TPS_CMD1_REG_SIZE);

		if (ret != 0) {
			LOG_ERR("I2C error");
			return ret;
		}

		if (strncmp((char *)read_cmd, "!CMD", 4) == 0) {
			LOG_ERR("Command %.4s rejected", cmd);
			return -EIO; /* TODO: better err codes */
		}

		const uint8_t compare_data[] = {0, 0, 0, 0};

		if (memcmp(read_cmd, compare_data, 4) != 0) {
			k_msleep(CMD_WAIT_TIME_MS);
			continue;
		}

		uint8_t task_ret_code;

		ret = tps25750_i2c_read(dev, TPS_DATA_REG, &task_ret_code, 1);
		if (ret != 0) {
			LOG_ERR("I2C error");
			return ret;
		}
		if (task_ret_code != 0) {
			LOG_ERR("Task %.4s execution failed. Task result = %u", cmd, task_ret_code);
			return -EIO; /* TODO: better err codes */
		}

		return 0;
	}

	return -EIO; /* TODO: better err codes */
}

/**
 * @brief Get the chip's operating mode
 *
 * Writes 'APP ', 'BOOT' or 'PTCH' into the user-provided buffer mode.
 *
 * @note: No string-terminator is added!
 *
 * @param dev Device
 * @param mode A user allocated buffer of at least 4
 * @return int value of tps25750_i2c_read()
 */
static int tps25750_get_mode(const struct device *dev, uint8_t *mode)
{
	return tps25750_i2c_read(dev, TPS_MODE_REG, mode, 4);
}

static bool ucpd_get_snk_ctrl(const struct device *dev)
{
	uint8_t buf[TPS_STATUS_REG_SIZE];
	const int ret = tps25750_i2c_read(dev, TPS_STATUS_REG, buf, TPS_STATUS_REG_SIZE);

	if (ret != 0) {
		LOG_ERR("Failed to read from i2c");
	}
	return (buf[0] & 0x20) == 0;
}

static bool ucpd_get_src_ctrl(const struct device *dev)
{
	return !ucpd_get_snk_ctrl(dev);
}

static int ucpd_get_cc(const struct device *dev, enum tc_cc_voltage_state *cc1,
		       enum tc_cc_voltage_state *cc2)
{
	uint8_t buf[TPS_TYPEC_STATE_REG_SIZE];

	const int ret = tps25750_i2c_read(dev, TPS_TYPEC_STATE_REG, buf, TPS_TYPEC_STATE_REG_SIZE);

	if (ret != 0) {
		LOG_ERR("cc read failed");
		return ret;
	}

	*cc1 = buf[1];
	*cc2 = buf[2];

	return 0;
}

static int ucpd_set_cc(const struct device *dev, enum tc_cc_pull cc_pull)
{
	/* TODO: this shall be implemented */
	LOG_ERR("called unimplemented %s(cc_pull=%u)", __func__, cc_pull);
	return 0;
}

static int ucpd_set_roles(const struct device *dev, enum tc_power_role power_role,
			  enum tc_data_role data_role)
{
	int power_role_ret;
	int data_role_ret;

	switch (power_role) {
	case TC_ROLE_SINK:
		if (!ucpd_get_snk_ctrl(dev)) {
			power_role_ret = tps25750_issue_4cc(dev, "SWSk");
		}
	case TC_ROLE_SOURCE:
		if (!ucpd_get_src_ctrl(dev)) {
			power_role_ret = tps25750_issue_4cc(dev, "SWSr");
		}
	default:
		return -ENOSYS;
	}

	if (power_role_ret) {
		/* TODO: return here? What about data direction setting below? */
		return power_role_ret;
	}

	uint8_t buf[TPS_STATUS_REG_SIZE];

	const int ret = tps25750_i2c_read(dev, TPS_STATUS_REG, buf, TPS_STATUS_REG_SIZE);

	if (ret != 0) {
		/* TODO: return here? What about data direction setting below? */
		return -EIO;
	}

	bool is_ufp = buf[0] & 0x40;

	switch (data_role) {
	case TC_ROLE_UFP:
		if (!is_ufp) {
			data_role_ret = tps25750_issue_4cc(dev, "SWUF");
		}
	case TC_ROLE_DFP:
		if (is_ufp) {
			data_role_ret = tps25750_issue_4cc(dev, "SWDF");
		}
	case TC_ROLE_DISCONNECTED:
		LOG_ERR("## calling unimpl. TC_ROLE_DISCONNECTED"); /* TODO: what to do here? */
	default:
		return -ENOSYS;
	}

	return data_role_ret;
}

/**
 * @brief Wrapper function for calling alert handler
 */
static void ucpd_notify_handler(struct alert_info *info, enum tcpc_alert alert)
{
	if (info->handler) {
		info->handler(info->dev, info->data, alert);
	}
}

/**
 * @brief Alert handler
 */
static void ucpd_alert_handler(struct k_work *item)
{
	struct alert_info *info = CONTAINER_OF(item, struct alert_info, work);
	const struct device *dev = info->dev;
	uint8_t buf[TPS_INT_EVENT1_REG_SIZE] = {0};
	uint8_t clear_mask[TPS_INT_EVENT1_REG_SIZE] = {0};

	int ret = tps25750_i2c_read(dev, TPS_INT_EVENT1_REG, buf, TPS_INT_EVENT1_REG_SIZE);

	if (ret != 0) {
		LOG_ERR("Err reading from TPS25750");
		return;
	}

	if (TEST_BIT(buf, EVENT_BIT_PLUG_INSERT_OR_REMOVAL)) {
		LOG_DBG("  EVENT_BIT_PLUG_INSERT_OR_REMOVAL");
		ucpd_notify_handler(info, TCPC_ALERT_CC_STATUS);
		SET_BIT(clear_mask, EVENT_BIT_PLUG_INSERT_OR_REMOVAL);
	}
	if (TEST_BIT(buf, EVENT_BIT_PD_HARD_RESET)) {
		LOG_DBG("  EVENT_BIT_PD_HARD_RESET");
		ucpd_notify_handler(info, TCPC_ALERT_HARD_RESET_RECEIVED);
		SET_BIT(clear_mask, EVENT_BIT_PD_HARD_RESET);
	}
	if (TEST_BIT(buf, EVENT_BIT_POWER_STATUS_UPDATE)) {
		LOG_DBG("  EVENT_BIT_POWER_STATUS_UPDATE");
		ucpd_notify_handler(info, TCPC_ALERT_POWER_STATUS);
	}
	if (TEST_BIT(buf, EVENT_BIT_ERROR_POWER_EVENT_OCCURRED)) {
		LOG_DBG("  EVENT_BIT_ERROR_POWER_EVENT_OCCURRED");
		/* ucpd_notify_handler(info, TCPC_ALERT_VBUS_ALARM_HI); */
		/* ucpd_notify_handler(info, TCPC_ALERT_VBUS_ALARM_LO); */
		/* SET_BIT(clear_mask, EVENT_BIT_ERROR_POWER_EVENT_OCCURRED); */
	}
	if (TEST_BIT(buf, EVENT_BIT_SOURCE_CAP_MSG_RCVD)) {
		LOG_DBG("  EVENT_BIT_SOURCE_CAP_MSG_RCVD");
	}

	/* TCPC_ALERT_FAULT_STATUS */
	/* TCPC_ALERT_RX_BUFFER_OVERFLOW */
	/* TCPC_ALERT_VBUS_SNK_DISCONNECT */
	/* TCPC_ALERT_EXTENDED_STATUS */
	/* TCPC_ALERT_EXTENDED */
	/* TCPC_ALERT_VENDOR_DEFINED */
	/* TCPC_ALERT_MSG_STATUS */
	/* TCPC_ALERT_BEGINNING_MSG_STATUS */
	/* TCPC_ALERT_TRANSMIT_MSG_FAILED */
	/* TCPC_ALERT_TRANSMIT_MSG_DISCARDED */
	/* TCPC_ALERT_TRANSMIT_MSG_SUCCESS */

	memset(clear_mask, 0xFF, TPS_INT_CLEAR1_REG_SIZE);

	/* clear interrupts */
	ret = tps25750_i2c_write(dev, TPS_INT_CLEAR1_REG, clear_mask, TPS_INT_CLEAR1_REG_SIZE);
	if (ret != 0) {
		LOG_ERR("Err clearing interrupt bits on TPS25750");
		return;
	}
}

static void ucpd_isr(const struct device *dev, struct gpio_callback *cb, uint32_t pin)
{
	struct tcpc_data *data = CONTAINER_OF(cb, struct tcpc_data, gpio_cb);
	struct alert_info *info = &data->alert_info;

	k_work_submit(&info->work);
}

static int ucpd_dump_std_reg(const struct device *dev)
{
	const struct tcpc_config *config = dev->config;
	uint8_t buf[32];

	const int ret = tps25750_get_mode(dev, buf);

	if (ret != 0) {
		LOG_ERR("Could not read mode\r");
		return -EIO;
	}
	LOG_INF("Mode: %.4s", buf);

	tps25750_i2c_read(dev, TPS_STATUS_REG, buf, TPS_STATUS_REG_SIZE);
	LOG_INF("STATUS:     0x%02x 0x%02x 0x%02x 0x%02x 0x%02x", buf[0], buf[1], buf[2], buf[3],
		buf[4]);

	tps25750_i2c_read(dev, TPS_INT_EVENT1_REG, buf, TPS_INT_EVENT1_REG_SIZE);
	LOG_INF("INT_EVENT1: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x "
		"0x%02x",
		buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8], buf[9],
		buf[10]);

	tps25750_i2c_read(dev, TPS_INT_MASK1_REG, buf, TPS_INT_MASK1_REG_SIZE);
	LOG_INF("INT_MASK1:  0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x "
		"0x%02x",
		buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8], buf[9],
		buf[10]);

	const int pin_lvl = gpio_pin_get_dt(&config->irq_gpio);

	LOG_INF("IRQ logical level = %d", pin_lvl);

	return 0;
}

static int ucpd_set_alert_handler_cb(const struct device *dev, tcpc_alert_handler_cb_t handler,
				     void *alert_data)
{
	struct tcpc_data *data = dev->data;

	data->alert_info.handler = handler;
	data->alert_info.data = alert_data;

	return 0;
}

static void ucpd_set_vconn_cb(const struct device *dev, tcpc_vconn_control_cb_t vconn_cb)
{
	/* TODO: this shall be implemented */
	LOG_DBG("called unimplemented %s()", __func__);
}

static void ucpd_set_vconn_discharge_cb(const struct device *dev, tcpc_vconn_discharge_cb_t cb)
{
	/* TODO: this shall be implemented */
	LOG_ERR("called unimplemented %s()", __func__);
}

static int ucpd_get_status_register(const struct device *dev, enum tcpc_status_reg reg,
				    int32_t *status)
{
	uint8_t buf[4];
	uint8_t i2c_reg;

	switch (reg) {
	case TCPC_CC_STATUS:
		i2c_reg = TPS_TYPEC_STATE_REG;
		break;
	case TCPC_POWER_STATUS:
		i2c_reg = TPS_POWER_STATUS_REG;
		break;
	case TCPC_FAULT_STATUS:
	case TCPC_EXTENDED_STATUS:
	case TCPC_EXTENDED_ALERT_STATUS:
	case TCPC_VENDOR_DEFINED_STATUS:
	default:
		return -ENOSYS;
	}

	const int ret = tps25750_i2c_read(dev, i2c_reg, buf, 4);

	if (ret != 0) {
		return ret;
	}

	*status = sys_get_le32(buf);

	return 0;
}

static int ucpd_cc_set_polarity(const struct device *dev, enum tc_cc_polarity polarity)
{
	/* TODO: this is unimplemented since the chip doesn't support it */
	LOG_DBG("called unimplemented %s()", __func__);
	return -ENOSYS;
}

static int ucpd_init(const struct device *dev)
{
	struct tcpc_data *data = dev->data;
	struct alert_info *info = &data->alert_info;
	const struct tcpc_config *config = dev->config;

	info->dev = dev;

	uint8_t buf[TPS_INT_MASK1_REG_SIZE];

	/* TODO: add ability to define capabilities in dev-tree and send them to the chip */

	/* set interrupt mask */
	tps25750_i2c_write(dev, TPS_INT_MASK1_REG, buf, TPS_INT_MASK1_REG_SIZE);

	memset(buf, 0xFF, TPS_INT_MASK1_REG_SIZE);
	tps25750_i2c_write(dev, TPS_INT_CLEAR1_REG, buf, TPS_INT_CLEAR1_REG_SIZE);

	/* Initialize the work handler */
	k_work_init(&info->work, ucpd_alert_handler);

	/* Configure the GPIO to trigger an ISR */
	int ret;

	if (!gpio_is_ready_dt(&config->irq_gpio)) {
		LOG_ERR("Error: button device %s is not ready", config->irq_gpio.port->name);
		return 0;
	}

	ret = gpio_pin_configure_dt(&config->irq_gpio, GPIO_INPUT);
	if (ret != 0) {
		LOG_ERR("Error %d: failed to configure %s pin %d", ret, config->irq_gpio.port->name,
			config->irq_gpio.pin);
		return 0;
	}

	gpio_init_callback(&data->gpio_cb, ucpd_isr, (1 << config->irq_gpio.pin));
	ret = gpio_add_callback(config->irq_gpio.port, &data->gpio_cb);
	if (ret != 0) {
		LOG_ERR("Failed to add gpio callback (ret=%d)", ret);
		return -EIO;
	}

	LOG_DBG("Configuring %s interrupt", config->irq_gpio.port->name);
	ret = gpio_pin_interrupt_configure_dt(&config->irq_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		LOG_ERR("Failed to configure interrupt on pin %d (ret=%d)", config->irq_gpio.pin,
			ret);
		return -EIO;
	}

	/* Set GPIO data to dev data */

	return 0;
}

static const struct tcpc_driver_api driver_api = {
	.init = ucpd_init,
	.set_alert_handler_cb = ucpd_set_alert_handler_cb,
	.get_cc = ucpd_get_cc,
	.set_cc = ucpd_set_cc,
	.set_roles = ucpd_set_roles,
	.set_vconn_cb = ucpd_set_vconn_cb,
	.set_vconn_discharge_cb = ucpd_set_vconn_discharge_cb,
	.set_cc_polarity = ucpd_cc_set_polarity,
	.dump_std_reg = ucpd_dump_std_reg,
	.get_status_register = ucpd_get_status_register,
	.get_snk_ctrl = ucpd_get_snk_ctrl,
	.get_src_ctrl = ucpd_get_src_ctrl,
};

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) > 0,
	     "No compatible TPS25750 TCPC instance found");

#define TCPC_DRIVER_INIT(inst)                                                                     \
	static struct tcpc_data drv_data_##inst;                                                   \
	static const struct tcpc_config drv_config_##inst = {                                      \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.irq_gpio = GPIO_DT_SPEC_INST_GET(inst, irq_gpios),                                \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, &ucpd_init, NULL, &drv_data_##inst, &drv_config_##inst,        \
			      POST_KERNEL, CONFIG_USBC_INIT_PRIORITY, &driver_api);

DT_INST_FOREACH_STATUS_OKAY(TCPC_DRIVER_INIT)
