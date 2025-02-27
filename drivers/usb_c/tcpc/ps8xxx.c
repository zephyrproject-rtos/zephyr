/*
 * Copyright 2024 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/usb_c/tcpci_priv.h>
#include <zephyr/usb_c/usbc.h>
#include <zephyr/usb_c/tcpci.h>
#include <zephyr/shell/shell.h>
#include "ps8xxx_priv.h"

#define DT_DRV_COMPAT parade_ps8xxx
LOG_MODULE_REGISTER(ps8xxx, CONFIG_USBC_LOG_LEVEL);

/** Data structure for device instances */
struct ps8xxx_data {
	/** Device structure used to retrieve it in k_work functions */
	const struct device *const dev;
	/** Delayable work item for chip initialization */
	struct k_work_delayable init_dwork;
	/** Initialization retries counter */
	int init_retries;
	/** Boolean value if chip was successfully initialized */
	bool initialized;

	/** Callback for alert GPIO */
	struct gpio_callback alert_cb;
	/** Work item for alert handling out of interrupt context */
	struct k_work alert_work;
	/** Boolean value if there is a message pending */
	bool msg_pending;

	/** Alert handler set by USB-C stack */
	tcpc_alert_handler_cb_t alert_handler;
	/** Alert handler data set by USB-C stack */
	void *alert_handler_data;

	/** VCONN discharge callback set by USB-C stack */
	tcpc_vconn_discharge_cb_t vconn_discharge_cb;
	/** VCONN discharge callback data set by USB-C stack */
	tcpc_vconn_control_cb_t vconn_cb;
	/** Polarity of CC lines for PD and VCONN */
	enum tc_cc_polarity cc_polarity;

	/** Boolean value if there was a change on the CC lines since last check */
	bool cc_changed;
	/** State of CC1 line */
	enum tc_cc_voltage_state cc1;
	/** State of CC2 line */
	enum tc_cc_voltage_state cc2;
};

/** Configuration structure for device instances */
struct ps8xxx_cfg {
	/** I2C bus and address used for communication */
	const struct i2c_dt_spec bus;
	/** GPIO specification for alert pin */
	const struct gpio_dt_spec alert_gpio;
	/** Maximum number of packet retransmissions done by TCPC */
	const uint8_t transmit_retries;
};

static int tcpci_init_alert_mask(const struct device *dev)
{
	const struct ps8xxx_cfg *cfg = dev->config;

	uint16_t mask = TCPC_REG_ALERT_TX_COMPLETE | TCPC_REG_ALERT_RX_STATUS |
			TCPC_REG_ALERT_RX_HARD_RST | TCPC_REG_ALERT_CC_STATUS |
			TCPC_REG_ALERT_FAULT | TCPC_REG_ALERT_POWER_STATUS;

	return tcpci_tcpm_mask_status_register(&cfg->bus, TCPC_ALERT_STATUS, mask);
}

static int ps8xxx_tcpc_init(const struct device *dev)
{
	struct ps8xxx_data *data = dev->data;

	if (!data->initialized) {
		if (data->init_retries > CONFIG_USBC_TCPC_PS8XXX_INIT_RETRIES) {
			LOG_ERR("TCPC was not initialized correctly");
			return -EIO;
		}

		return -EAGAIN;
	}

	LOG_INF("PS8xxx TCPC initialized");
	return 0;
}

int ps8xxx_tcpc_get_cc(const struct device *dev, enum tc_cc_voltage_state *cc1,
		       enum tc_cc_voltage_state *cc2)
{
	const struct ps8xxx_cfg *cfg = dev->config;
	struct ps8xxx_data *data = dev->data;
	int ret;

	if (!data->initialized) {
		return -EIO;
	}

	if (IS_ENABLED(CONFIG_USBC_CSM_SINK_ONLY) && !data->cc_changed) {
		*cc1 = data->cc1;
		*cc2 = data->cc2;

		return 0;
	}

	data->cc_changed = false;

	ret = tcpci_tcpm_get_cc(&cfg->bus, cc1, cc2);

	if (IS_ENABLED(CONFIG_USBC_CSM_SINK_ONLY) || *cc1 != data->cc1 || *cc2 != data->cc2) {
		LOG_DBG("CC changed values: %d->%d, %d->%d", data->cc1, *cc1, data->cc2, *cc2);
		data->cc1 = *cc1;
		data->cc2 = *cc2;
	}

	return ret;
}

int ps8xxx_tcpc_select_rp_value(const struct device *dev, enum tc_rp_value rp)
{
	const struct ps8xxx_cfg *cfg = dev->config;
	struct ps8xxx_data *data = dev->data;

	data->cc_changed = true;

	return tcpci_tcpm_select_rp_value(&cfg->bus, rp);
}

int ps8xxx_tcpc_get_rp_value(const struct device *dev, enum tc_rp_value *rp)
{
	const struct ps8xxx_cfg *cfg = dev->config;

	return tcpci_tcpm_get_rp_value(&cfg->bus, rp);
}

int ps8xxx_tcpc_set_cc(const struct device *dev, enum tc_cc_pull pull)
{
	const struct ps8xxx_cfg *cfg = dev->config;
	struct ps8xxx_data *data = dev->data;

	if (!data->initialized) {
		return -EIO;
	}

	data->cc_changed = true;

	return tcpci_tcpm_set_cc(&cfg->bus, pull);
}

void ps8xxx_tcpc_set_vconn_discharge_cb(const struct device *dev, tcpc_vconn_discharge_cb_t cb)
{
	struct ps8xxx_data *data = dev->data;

	data->vconn_discharge_cb = cb;
}

void ps8xxx_tcpc_set_vconn_cb(const struct device *dev, tcpc_vconn_control_cb_t vconn_cb)
{
	struct ps8xxx_data *data = dev->data;

	data->vconn_cb = vconn_cb;
}

int ps8xxx_tcpc_vconn_discharge(const struct device *dev, bool enable)
{
	const struct ps8xxx_cfg *cfg = dev->config;

	return tcpci_update_reg8(&cfg->bus, TCPC_REG_POWER_CTRL,
				 TCPC_REG_POWER_CTRL_AUTO_DISCHARGE_DISCONNECT,
				 (enable) ? TCPC_REG_POWER_CTRL_AUTO_DISCHARGE_DISCONNECT : 0);
}

int ps8xxx_tcpc_set_vconn(const struct device *dev, bool enable)
{
	const struct ps8xxx_cfg *cfg = dev->config;
	struct ps8xxx_data *data = dev->data;
	int ret;

	if (!data->initialized) {
		return -EIO;
	}

	data->cc_changed = true;
	ret = tcpci_tcpm_set_vconn(&cfg->bus, enable);

	if (ret != 0) {
		return ret;
	}

	if (data->vconn_cb != NULL) {
		ret = data->vconn_cb(dev, data->cc_polarity, enable);
	}

	return ret;
}

int ps8xxx_tcpc_set_roles(const struct device *dev, enum tc_power_role power_role,
			  enum tc_data_role data_role)
{
	const struct ps8xxx_cfg *cfg = dev->config;

	return tcpci_tcpm_set_roles(&cfg->bus, PD_REV30, power_role, data_role);
}

int ps8xxx_tcpc_get_rx_pending_msg(const struct device *dev, struct pd_msg *msg)
{
	const struct ps8xxx_cfg *cfg = dev->config;
	struct ps8xxx_data *data = dev->data;
	struct i2c_msg buf[5];
	uint8_t msg_len = 0;
	uint8_t unused;
	int buf_count;
	int reg = TCPC_REG_RX_BUFFER;
	int ret;

	if (!data->msg_pending) {
		return -ENODATA;
	}

	if (msg == NULL) {
		return 0;
	}

	data->msg_pending = false;

	buf[0].buf = (uint8_t *)&reg;
	buf[0].len = 1;
	buf[0].flags = I2C_MSG_WRITE;

	buf[1].buf = &msg_len;
	buf[1].len = 1;
	buf[1].flags = I2C_MSG_RESTART | I2C_MSG_READ | I2C_MSG_STOP;
	ret = i2c_transfer(cfg->bus.bus, buf, 2, cfg->bus.addr);

	if (msg_len <= 1) {
		return 0;
	}

	buf[1].buf = &unused;
	buf[1].len = 1;
	buf[1].flags = I2C_MSG_RESTART | I2C_MSG_READ;

	buf[2].buf = (uint8_t *)&msg->type;
	buf[2].len = 1;
	buf[2].flags = I2C_MSG_RESTART | I2C_MSG_READ;

	msg->header.raw_value = 0;
	buf[3].buf = (uint8_t *)&msg->header.raw_value;
	buf[3].len = 2;
	buf[3].flags = I2C_MSG_RESTART | I2C_MSG_READ;

	if (msg_len > 3) {
		buf[4].buf = msg->data;
		buf[4].len = msg_len - 3;
		buf[4].flags = I2C_MSG_RESTART | I2C_MSG_READ | I2C_MSG_STOP;
		buf_count = 5;
	} else if (msg_len == 3) {
		buf[3].flags |= I2C_MSG_STOP;
		buf_count = 4;
	} else {
		buf[2].flags |= I2C_MSG_STOP;
		buf_count = 3;
	}

	ret = i2c_transfer(cfg->bus.bus, buf, buf_count, cfg->bus.addr);
	if (ret != 0) {
		LOG_ERR("I2C transfer error: %d", ret);
	} else {
		msg->len = (msg_len > 3) ? msg_len - 3 : 0;
		ret = sizeof(msg->header.raw_value) + msg->len;
	}

	tcpci_write_reg16(&cfg->bus, TCPC_REG_ALERT, TCPC_REG_ALERT_RX_STATUS);
	return ret;
}

int ps8xxx_tcpc_set_rx_enable(const struct device *dev, bool enable)
{
	const struct ps8xxx_cfg *cfg = dev->config;
	int detect_sop_en = enable ? TCPC_REG_RX_DETECT_SOP_HRST_MASK : 0;

	return tcpci_tcpm_set_rx_type(&cfg->bus, detect_sop_en);
}

int ps8xxx_tcpc_set_cc_polarity(const struct device *dev, enum tc_cc_polarity polarity)
{
	const struct ps8xxx_cfg *cfg = dev->config;
	struct ps8xxx_data *data = dev->data;
	int ret;

	if (!data->initialized) {
		return -EIO;
	}

	ret = tcpci_tcpm_set_cc_polarity(&cfg->bus, polarity);

	if (ret != 0) {
		return ret;
	}

	data->cc_changed = true;
	data->cc_polarity = polarity;
	return 0;
}

int ps8xxx_tcpc_transmit_data(const struct device *dev, struct pd_msg *msg)
{
	const struct ps8xxx_cfg *cfg = dev->config;

	return tcpci_tcpm_transmit_data(&cfg->bus, msg, cfg->transmit_retries);
}

int ps8xxx_tcpc_dump_std_reg(const struct device *dev)
{
	const struct ps8xxx_cfg *cfg = dev->config;

	LOG_INF("TCPC %s:%s registers:", cfg->bus.bus->name, dev->name);

	return tcpci_tcpm_dump_std_reg(&cfg->bus);
}

void ps8xxx_tcpc_alert_handler_cb(const struct device *dev, void *data, enum tcpc_alert alert)
{
}

int ps8xxx_tcpc_get_status_register(const struct device *dev, enum tcpc_status_reg reg,
				    uint32_t *status)
{
	return -ENOSYS;
}

int ps8xxx_tcpc_clear_status_register(const struct device *dev, enum tcpc_status_reg reg,
				      uint32_t mask)
{
	return -ENOSYS;
}

int ps8xxx_tcpc_mask_status_register(const struct device *dev, enum tcpc_status_reg reg,
				     uint32_t mask)
{
	return -ENOSYS;
}

int ps8xxx_tcpc_set_debug_accessory(const struct device *dev, bool enable)
{
	return -ENOSYS;
}

int ps8xxx_tcpc_set_debug_detach(const struct device *dev)
{
	return -ENOSYS;
}

int ps8xxx_tcpc_set_drp_toggle(const struct device *dev, bool enable)
{
	return -ENOSYS;
}

int ps8xxx_tcpc_get_snk_ctrl(const struct device *dev)
{
	return -ENOSYS;
}

int ps8xxx_tcpc_set_snk_ctrl(const struct device *dev, bool enable)
{
	const struct ps8xxx_cfg *cfg = dev->config;
	uint8_t cmd = (enable) ? TCPC_REG_COMMAND_SNK_CTRL_HIGH : TCPC_REG_COMMAND_SNK_CTRL_LOW;

	return tcpci_write_reg8(&cfg->bus, TCPC_REG_COMMAND, cmd);
}

int ps8xxx_tcpc_get_src_ctrl(const struct device *dev)
{
	return -ENOSYS;
}

int ps8xxx_tcpc_set_src_ctrl(const struct device *dev, bool enable)
{
	const struct ps8xxx_cfg *cfg = dev->config;
	uint8_t cmd = (enable) ? TCPC_REG_COMMAND_SRC_CTRL_DEF : TCPC_REG_COMMAND_SRC_CTRL_LOW;

	return tcpci_write_reg8(&cfg->bus, TCPC_REG_COMMAND, cmd);
}

int ps8xxx_tcpc_get_chip_info(const struct device *dev, struct tcpc_chip_info *chip_info)
{
	const struct ps8xxx_cfg *cfg = dev->config;
	int ret = 0;

	if (chip_info == NULL) {
		return -EIO;
	}

	ret = tcpci_tcpm_get_chip_info(&cfg->bus, chip_info);

	/* Vendor specific register for PS8815 model only */
	if (chip_info->product_id == PS8815_PRODUCT_ID) {
		uint8_t fw_ver;

		ret |= tcpci_read_reg8(&cfg->bus, PS8815_REG_FW_VER, &fw_ver);
		chip_info->fw_version_number = fw_ver;
	} else {
		chip_info->fw_version_number = 0;
	}

	chip_info->min_req_fw_version_number = 0;

	return ret;
}

int ps8xxx_tcpc_set_low_power_mode(const struct device *dev, bool enable)
{
	const struct ps8xxx_cfg *cfg = dev->config;

	return tcpci_write_reg8(&cfg->bus, TCPC_REG_COMMAND, TCPC_REG_COMMAND_I2CIDLE);
}

int ps8xxx_tcpc_sop_prime_enable(const struct device *dev, bool enable)
{
	return -ENOSYS;
}

int ps8xxx_tcpc_set_bist_test_mode(const struct device *dev, bool enable)
{
	return -ENOSYS;
}

int ps8xxx_tcpc_set_alert_handler_cb(const struct device *dev, tcpc_alert_handler_cb_t handler,
				     void *handler_data)
{
	struct ps8xxx_data *data = dev->data;

	if (data->alert_handler == handler && data->alert_handler_data == handler_data) {
		return 0;
	}

	data->alert_handler = handler;
	data->alert_handler_data = handler_data;

	return 0;
}

/* Functions not assigned to the driver API but used by device */

static DEVICE_API(tcpc, ps8xxx_driver_api) = {
	.init = ps8xxx_tcpc_init,
	.get_cc = ps8xxx_tcpc_get_cc,
	.select_rp_value = ps8xxx_tcpc_select_rp_value,
	.get_rp_value = ps8xxx_tcpc_get_rp_value,
	.set_cc = ps8xxx_tcpc_set_cc,
	.set_vconn_discharge_cb = ps8xxx_tcpc_set_vconn_discharge_cb,
	.set_vconn_cb = ps8xxx_tcpc_set_vconn_cb,
	.vconn_discharge = ps8xxx_tcpc_vconn_discharge,
	.set_vconn = ps8xxx_tcpc_set_vconn,
	.set_roles = ps8xxx_tcpc_set_roles,
	.get_rx_pending_msg = ps8xxx_tcpc_get_rx_pending_msg,
	.set_rx_enable = ps8xxx_tcpc_set_rx_enable,
	.set_cc_polarity = ps8xxx_tcpc_set_cc_polarity,
	.transmit_data = ps8xxx_tcpc_transmit_data,
	.dump_std_reg = ps8xxx_tcpc_dump_std_reg,
	.alert_handler_cb = ps8xxx_tcpc_alert_handler_cb,
	.get_status_register = ps8xxx_tcpc_get_status_register,
	.clear_status_register = ps8xxx_tcpc_clear_status_register,
	.mask_status_register = ps8xxx_tcpc_mask_status_register,
	.set_debug_accessory = ps8xxx_tcpc_set_debug_accessory,
	.set_debug_detach = ps8xxx_tcpc_set_debug_detach,
	.set_drp_toggle = ps8xxx_tcpc_set_drp_toggle,
	.get_snk_ctrl = ps8xxx_tcpc_get_snk_ctrl,
	.set_snk_ctrl = ps8xxx_tcpc_set_snk_ctrl,
	.get_src_ctrl = ps8xxx_tcpc_get_src_ctrl,
	.set_src_ctrl = ps8xxx_tcpc_set_src_ctrl,
	.get_chip_info = ps8xxx_tcpc_get_chip_info,
	.set_low_power_mode = ps8xxx_tcpc_set_low_power_mode,
	.sop_prime_enable = ps8xxx_tcpc_sop_prime_enable,
	.set_bist_test_mode = ps8xxx_tcpc_set_bist_test_mode,
	.set_alert_handler_cb = ps8xxx_tcpc_set_alert_handler_cb,
};

void ps8xxx_alert_cb(const struct device *port, struct gpio_callback *cb, gpio_port_pins_t pins)
{
	struct ps8xxx_data *data = CONTAINER_OF(cb, struct ps8xxx_data, alert_cb);

	k_work_submit(&data->alert_work);
}

void ps8xxx_alert_work_cb(struct k_work *work)
{
	struct ps8xxx_data *data = CONTAINER_OF(work, struct ps8xxx_data, alert_work);
	const struct device *dev = data->dev;
	const struct ps8xxx_cfg *cfg = dev->config;
	uint16_t alert_reg = 0;
	uint16_t clear_flags = 0;

	if (!data->initialized) {
		return;
	}

	tcpci_tcpm_get_status_register(&cfg->bus, TCPC_ALERT_STATUS, &alert_reg);

	while (alert_reg != 0) {
		enum tcpc_alert alert_type = tcpci_alert_reg_to_enum(alert_reg);

		if (alert_type == TCPC_ALERT_HARD_RESET_RECEIVED) {
			LOG_DBG("PS8xxx hard rst received");
			tcpci_init_alert_mask(dev);
			data->cc_changed = true;
		} else if (alert_type == TCPC_ALERT_FAULT_STATUS) {
			uint8_t fault;

			tcpci_tcpm_get_status_register(&cfg->bus, TCPC_FAULT_STATUS,
						       (uint16_t *)&fault);
			tcpci_tcpm_clear_status_register(&cfg->bus, TCPC_FAULT_STATUS,
							 (uint16_t)fault);

			LOG_DBG("PS8xxx fault: %02x", fault);
		} else if (alert_type == TCPC_ALERT_EXTENDED_STATUS) {
			uint8_t ext_status;

			tcpci_tcpm_get_status_register(&cfg->bus, TCPC_EXTENDED_STATUS,
						       (uint16_t *)&ext_status);
			tcpci_tcpm_clear_status_register(&cfg->bus, TCPC_EXTENDED_STATUS,
							 (uint16_t)ext_status);

			data->cc_changed = true;
			LOG_DBG("PS8xxx ext status: %02x", ext_status);
		} else if (alert_type == TCPC_ALERT_POWER_STATUS) {
			uint8_t pwr_status;

			tcpci_tcpm_get_status_register(&cfg->bus, TCPC_POWER_STATUS,
						       (uint16_t *)&pwr_status);

			LOG_DBG("PS8xxx power status: %02x", pwr_status);
		} else if (alert_type == TCPC_ALERT_EXTENDED) {
			uint8_t alert_status;

			tcpci_tcpm_get_status_register(&cfg->bus, TCPC_EXTENDED_ALERT_STATUS,
						       (uint16_t *)&alert_status);
			tcpci_tcpm_clear_status_register(&cfg->bus, TCPC_EXTENDED_ALERT_STATUS,
							 (uint16_t)alert_status);

			LOG_DBG("PS8xxx ext alert: %02x", alert_status);
		} else if (alert_type == TCPC_ALERT_MSG_STATUS) {
			data->msg_pending = true;
		} else if (alert_type == TCPC_ALERT_CC_STATUS) {
			data->cc_changed = true;
		}

		if (data->alert_handler != NULL) {
			data->alert_handler(dev, data->alert_handler_data, alert_type);
		}

		clear_flags |= BIT(alert_type);
		alert_reg &= ~BIT(alert_type);
	}

	tcpci_tcpm_clear_status_register(&cfg->bus, TCPC_ALERT_STATUS, clear_flags);
	tcpci_tcpm_get_status_register(&cfg->bus, TCPC_ALERT_STATUS, &alert_reg);

	if (alert_reg != 0) {
		k_work_submit(work);
	}
}

void ps8xxx_init_work_cb(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct ps8xxx_data *data = CONTAINER_OF(dwork, struct ps8xxx_data, init_dwork);

	const struct ps8xxx_cfg *cfg = data->dev->config;
	uint8_t power_reg = 0;
	struct tcpc_chip_info chip_info;
	int ret;

	LOG_INF("Initializing PS8xxx chip: %s", data->dev->name);
	ret = tcpci_tcpm_get_status_register(&cfg->bus, TCPC_POWER_STATUS, (uint16_t *)&power_reg);
	if (ret != 0 || (power_reg & TCPC_REG_POWER_STATUS_UNINIT)) {
		data->init_retries++;

		if (data->init_retries > CONFIG_USBC_TCPC_PS8XXX_INIT_RETRIES) {
			LOG_ERR("Chip didn't respond");
			return;
		}

		LOG_DBG("Postpone chip initialization %d", data->init_retries);
		k_work_schedule(&data->init_dwork, K_MSEC(CONFIG_USBC_TCPC_PS8XXX_INIT_DELAY));

		return;
	}

	ps8xxx_tcpc_get_chip_info(data->dev, &chip_info);
	LOG_INF("Initialized chip is: %04x:%04x:%04x", chip_info.vendor_id, chip_info.product_id,
		chip_info.device_id);

	/* Initialize alert interrupt */
	gpio_pin_configure_dt(&cfg->alert_gpio, GPIO_INPUT);

	gpio_init_callback(&data->alert_cb, ps8xxx_alert_cb, BIT(cfg->alert_gpio.pin));
	gpio_add_callback(cfg->alert_gpio.port, &data->alert_cb);
	gpio_pin_interrupt_configure_dt(&cfg->alert_gpio, GPIO_INT_EDGE_TO_ACTIVE);

	tcpci_init_alert_mask(data->dev);
	data->initialized = true;

	/* Disable the vconn and open CC lines to reinitialize the communication with partner */
	ps8xxx_tcpc_set_vconn(data->dev, false);
	ps8xxx_tcpc_set_cc(data->dev, TC_CC_OPEN);

	/* Check and clear any alert set after initialization */
	k_work_submit(&data->alert_work);
}

static int ps8xxx_dev_init(const struct device *dev)
{
	const struct ps8xxx_cfg *cfg = dev->config;
	struct ps8xxx_data *data = dev->data;

	if (!device_is_ready(cfg->bus.bus)) {
		return -EIO;
	}

	k_work_init_delayable(&data->init_dwork, ps8xxx_init_work_cb);
	k_work_schedule(&data->init_dwork, K_MSEC(CONFIG_USBC_TCPC_PS8XXX_INIT_DELAY));

	k_work_init(&data->alert_work, ps8xxx_alert_work_cb);

	return 0;
}

#define PS8XXX_DRIVER_DATA_INIT(node)                                                              \
	{                                                                                          \
		.dev = DEVICE_DT_GET(node),                                                        \
		.init_retries = 0,                                                                 \
		.cc_changed = true,                                                                \
	}

#define PS8XXX_DRIVER_CFG_INIT(node)                                                               \
	{                                                                                          \
		.bus = I2C_DT_SPEC_GET(node),                                                      \
		.alert_gpio = GPIO_DT_SPEC_GET(node, irq_gpios),                                   \
		.transmit_retries = DT_PROP(node, transmit_retries),                               \
	}

#define PS8XXX_DRIVER_INIT(inst)                                                                   \
	static struct ps8xxx_data drv_data_ps8xxx##inst =                                          \
		PS8XXX_DRIVER_DATA_INIT(DT_DRV_INST(inst));                                        \
	static struct ps8xxx_cfg drv_cfg_ps8xxx##inst = PS8XXX_DRIVER_CFG_INIT(DT_DRV_INST(inst)); \
	DEVICE_DT_INST_DEFINE(inst, &ps8xxx_dev_init, NULL, &drv_data_ps8xxx##inst,                \
			      &drv_cfg_ps8xxx##inst, POST_KERNEL, CONFIG_USBC_TCPC_INIT_PRIORITY,  \
			      &ps8xxx_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PS8XXX_DRIVER_INIT)
