/*
 * Copyright (c) 2024 Jianxiong Gu <jianxiong.gu@outlook.com>
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
#include "rt1715.h"

#define DT_DRV_COMPAT richtek_rt1715
LOG_MODULE_REGISTER(rt1715, CONFIG_USBC_LOG_LEVEL);

/** Data structure for device instances */
struct rt1715_data {
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
	/** One-slot Rx FIFO */
	struct pd_msg rx_msg;

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

	/* Flag to receive or ignore SOP Prime messages */
	bool rx_sop_prime_enable;
};

/** Configuration structure for device instances */
struct rt1715_cfg {
	/** I2C bus and address used for communication */
	const struct i2c_dt_spec bus;
	/** GPIO specification for alert pin */
	const struct gpio_dt_spec alert_gpio;
	/** GPIO specification for VCONN power control pin */
	const struct gpio_dt_spec vconn_ctrl_gpio;
	/** GPIO specification for VCONN discharge control pin */
	const struct gpio_dt_spec vconn_disc_gpio;
	/** Maximum number of packet retransmissions done by TCPC */
	const uint8_t transmit_retries;
};

static int tcpci_init_alert_mask(const struct device *dev)
{
	const struct rt1715_cfg *cfg = dev->config;

	uint16_t mask = TCPC_REG_ALERT_TX_COMPLETE | TCPC_REG_ALERT_RX_STATUS |
			TCPC_REG_ALERT_RX_HARD_RST | TCPC_REG_ALERT_CC_STATUS |
			TCPC_REG_ALERT_POWER_STATUS | TCPC_REG_ALERT_FAULT |
			TCPC_REG_ALERT_RX_BUF_OVF;

	return tcpci_tcpm_mask_status_register(&cfg->bus, TCPC_ALERT_STATUS, mask);
}

static int rt1715_tcpc_init(const struct device *dev)
{
	struct rt1715_data *data = dev->data;

	if (!data->initialized) {
		if (data->init_retries > CONFIG_USBC_TCPC_RT1715_INIT_RETRIES) {
			LOG_ERR("TCPC was not initialized correctly");
			return -EIO;
		}

		return -EAGAIN;
	}

	data->rx_sop_prime_enable = false;
	data->msg_pending = false;
	memset(&data->rx_msg, 0x00, sizeof(data->rx_msg));

	LOG_INF("RT1715 TCPC initialized");
	return 0;
}

static int rt1715_tcpc_get_cc(const struct device *dev, enum tc_cc_voltage_state *cc1,
			      enum tc_cc_voltage_state *cc2)
{
	const struct rt1715_cfg *cfg = dev->config;
	struct rt1715_data *data = dev->data;
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

static int rt1715_tcpc_select_rp_value(const struct device *dev, enum tc_rp_value rp)
{
	const struct rt1715_cfg *cfg = dev->config;
	struct rt1715_data *data = dev->data;

	data->cc_changed = true;

	return tcpci_tcpm_select_rp_value(&cfg->bus, rp);
}

static int rt1715_tcpc_get_rp_value(const struct device *dev, enum tc_rp_value *rp)
{
	const struct rt1715_cfg *cfg = dev->config;

	return tcpci_tcpm_get_rp_value(&cfg->bus, rp);
}

static int rt1715_tcpc_set_cc(const struct device *dev, enum tc_cc_pull pull)
{
	const struct rt1715_cfg *cfg = dev->config;
	struct rt1715_data *data = dev->data;

	if (!data->initialized) {
		return -EIO;
	}

	data->cc_changed = true;

	return tcpci_tcpm_set_cc(&cfg->bus, pull);
}

static void rt1715_tcpc_set_vconn_discharge_cb(const struct device *dev,
					       tcpc_vconn_discharge_cb_t cb)
{
	struct rt1715_data *data = dev->data;

	data->vconn_discharge_cb = cb;
}

static void rt1715_tcpc_set_vconn_cb(const struct device *dev, tcpc_vconn_control_cb_t vconn_cb)
{
	struct rt1715_data *data = dev->data;

	data->vconn_cb = vconn_cb;
}

static int rt1715_tcpc_vconn_discharge(const struct device *dev, bool enable)
{
	const struct rt1715_cfg *cfg = dev->config;

	if (cfg->vconn_disc_gpio.port == NULL) {
		/* RT1715 does not have built-in VCONN discharge path */
		LOG_ERR("VCONN discharge GPIO is not defined");
		return -EIO;
	}

	return gpio_pin_set_dt(&cfg->vconn_disc_gpio, enable);
}

static int rt1715_tcpc_set_vconn(const struct device *dev, bool enable)
{
	const struct rt1715_cfg *cfg = dev->config;
	struct rt1715_data *data = dev->data;
	int ret;

	if (!data->initialized) {
		return -EIO;
	}

	if (enable == true && cfg->vconn_ctrl_gpio.port != NULL) {
		/* Enable external VCONN power supply */
		gpio_pin_set_dt(&cfg->vconn_ctrl_gpio, true);
	}

	data->cc_changed = true;
	ret = tcpci_tcpm_set_vconn(&cfg->bus, enable);

	if (ret != 0) {
		return ret;
	}

	if (enable == false && cfg->vconn_ctrl_gpio.port != NULL) {
		/* Disable external VCONN power supply */
		gpio_pin_set_dt(&cfg->vconn_ctrl_gpio, false);
	}

	if (data->vconn_cb != NULL) {
		ret = data->vconn_cb(dev, data->cc_polarity, enable);
	}

	return ret;
}

static int rt1715_tcpc_set_roles(const struct device *dev, enum tc_power_role power_role,
				 enum tc_data_role data_role)
{
	const struct rt1715_cfg *cfg = dev->config;

	return tcpci_tcpm_set_roles(&cfg->bus, PD_REV30, power_role, data_role);
}

static int rt1715_tcpc_get_rx_pending_msg(const struct device *dev, struct pd_msg *msg)
{
	struct rt1715_data *data = dev->data;

	/* Rx message pending? */
	if (!data->msg_pending) {
		return -ENODATA;
	}

	/* Query status only? */
	if (msg == NULL) {
		return 0;
	}

	/* Dequeue Rx FIFO */
	*msg = data->rx_msg;
	data->msg_pending = false;

	/* Indicate Rx message returned */
	return 1;
}

static int rt1715_tcpc_rx_fifo_enqueue(const struct device *dev)
{
	const struct rt1715_cfg *const cfg = dev->config;
	struct rt1715_data *data = dev->data;
	uint8_t buf[4];
	uint8_t rxbcnt;
	uint8_t rxftype;
	uint16_t rxhead;
	uint8_t rx_data_size;
	struct pd_msg *msg = &data->rx_msg;
	int ret = 0;

	ret = i2c_burst_read_dt(&cfg->bus, TCPC_REG_RX_BUFFER, buf, sizeof(buf));
	if (ret != 0) {
		return ret;
	}

	rxbcnt = buf[0];
	rxftype = buf[1];
	rxhead = (buf[3] << 8) | buf[2];

	/* rxbcnt = 1 (frame type) + 2 (Message Header) + Rx data byte count */
	if (rxbcnt < 3) {
		LOG_ERR("Invalid RXBCNT: %d", rxbcnt);
		return -EIO;
	}
	rx_data_size = rxbcnt - 3;

	/* Not support Unchunked Extended Message exceeding PD_CONVERT_PD_HEADER_COUNT_TO_BYTES */
	if (rx_data_size > (PD_MAX_EXTENDED_MSG_LEGACY_LEN + 2)) {
		LOG_ERR("Not support Unchunked Extended Message exceeding "
			"PD_CONVERT_PD_HEADER_COUNT_TO_BYTES: %d",
			rx_data_size);
		return -EIO;
	}

	/* Rx frame type */
	msg->type = rxftype;

	/* Rx header */
	msg->header.raw_value = (uint16_t)rxhead;

	/* Rx data size */
	msg->len = rx_data_size;

	/* Rx data */
	if (rx_data_size > 0) {
		ret = i2c_burst_read_dt(&cfg->bus, TCPC_REG_RX_BUFFER + 4, msg->data, rx_data_size);
		if (ret) {
			LOG_ERR("Failed to read Rx data: %d", ret);
		}
	}

	return ret;
}

static int rt1715_tcpc_set_rx_enable(const struct device *dev, bool enable)
{
	const struct rt1715_cfg *cfg = dev->config;
	struct rt1715_data *data = dev->data;

	if (!enable) {
		return tcpci_tcpm_set_rx_type(&cfg->bus, 0);
	}

	if (data->rx_sop_prime_enable) {
		return tcpci_tcpm_set_rx_type(&cfg->bus,
					      TCPC_REG_RX_DETECT_SOP_SOPP_SOPPP_HRST_MASK);
	} else {
		return tcpci_tcpm_set_rx_type(&cfg->bus, TCPC_REG_RX_DETECT_SOP_HRST_MASK);
	}
}

static int rt1715_tcpc_set_cc_polarity(const struct device *dev, enum tc_cc_polarity polarity)
{
	const struct rt1715_cfg *cfg = dev->config;
	struct rt1715_data *data = dev->data;
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

static int rt1715_tcpc_transmit_data(const struct device *dev, struct pd_msg *msg)
{
	const struct rt1715_cfg *cfg = dev->config;

	return tcpci_tcpm_transmit_data(&cfg->bus, msg, cfg->transmit_retries);
}

static int rt1715_tcpc_dump_std_reg(const struct device *dev)
{
	const struct rt1715_cfg *cfg = dev->config;

	LOG_INF("TCPC %s:%s registers:", cfg->bus.bus->name, dev->name);

	return tcpci_tcpm_dump_std_reg(&cfg->bus);
}

void rt1715_tcpc_alert_handler_cb(const struct device *dev, void *data, enum tcpc_alert alert)
{
}

static int rt1715_tcpc_get_status_register(const struct device *dev, enum tcpc_status_reg reg,
					   uint32_t *status)
{
	const struct rt1715_cfg *cfg = dev->config;

	if (reg == TCPC_VENDOR_DEFINED_STATUS) {
		return tcpci_read_reg8(&cfg->bus, RT1715_REG_RT_INT, (uint8_t *)status);
	}

	return tcpci_tcpm_get_status_register(&cfg->bus, reg, (uint16_t *)status);
}

static int rt1715_tcpc_clear_status_register(const struct device *dev, enum tcpc_status_reg reg,
					     uint32_t mask)
{
	const struct rt1715_cfg *cfg = dev->config;

	if (reg == TCPC_VENDOR_DEFINED_STATUS) {
		return tcpci_write_reg8(&cfg->bus, RT1715_REG_RT_INT, (uint8_t)mask);
	}

	return tcpci_tcpm_clear_status_register(&cfg->bus, reg, (uint16_t)mask);
}

static int rt1715_tcpc_mask_status_register(const struct device *dev, enum tcpc_status_reg reg,
					    uint32_t mask)
{
	const struct rt1715_cfg *cfg = dev->config;

	if (reg == TCPC_VENDOR_DEFINED_STATUS) {
		return tcpci_write_reg8(&cfg->bus, RT1715_REG_RT_INT_MASK, (uint8_t)mask);
	}

	return tcpci_tcpm_mask_status_register(&cfg->bus, reg, (uint16_t)mask);
}

static int rt1715_tcpc_set_drp_toggle(const struct device *dev, bool enable)
{
	const struct rt1715_cfg *cfg = dev->config;

	return tcpci_tcpm_set_drp_toggle(&cfg->bus, enable);
}

static int rt1715_tcpc_get_chip_info(const struct device *dev, struct tcpc_chip_info *chip_info)
{
	const struct rt1715_cfg *cfg = dev->config;

	if (chip_info == NULL) {
		return -EIO;
	}

	chip_info->fw_version_number = 0;
	chip_info->min_req_fw_version_number = 0;

	return tcpci_tcpm_get_chip_info(&cfg->bus, chip_info);
}

static int rt1715_tcpc_set_low_power_mode(const struct device *dev, bool enable)
{
	const struct rt1715_cfg *cfg = dev->config;
	int ret;

	ret = tcpci_write_reg8(&cfg->bus, RT1715_REG_SYS_WAKEUP, RT1715_REG_SYS_WAKEUP_EN);
	if (ret < 0) {
		return ret;
	}

	return tcpci_update_reg8(&cfg->bus, RT1715_REG_SYS_CTRL_1,
				 RT1715_REG_SYS_CTRL_1_BMCIO_LP_EN,
				 enable ? RT1715_REG_SYS_CTRL_1_BMCIO_LP_EN : 0);
}

static int rt1715_tcpc_sop_prime_enable(const struct device *dev, bool enable)
{
	struct rt1715_data *data = dev->data;

	data->rx_sop_prime_enable = enable;

	return 0;
}

static int rt1715_tcpc_set_bist_test_mode(const struct device *dev, bool enable)
{
	const struct rt1715_cfg *cfg = dev->config;

	return tcpci_tcpm_set_bist_test_mode(&cfg->bus, enable);
}

static int rt1715_tcpc_set_alert_handler_cb(const struct device *dev,
					    tcpc_alert_handler_cb_t handler, void *handler_data)
{
	struct rt1715_data *data = dev->data;

	if (data->alert_handler == handler && data->alert_handler_data == handler_data) {
		return 0;
	}

	data->alert_handler = handler;
	data->alert_handler_data = handler_data;

	return 0;
}

static DEVICE_API(tcpc, rt1715_driver_api) = {
	.init = rt1715_tcpc_init,
	.get_cc = rt1715_tcpc_get_cc,
	.select_rp_value = rt1715_tcpc_select_rp_value,
	.get_rp_value = rt1715_tcpc_get_rp_value,
	.set_cc = rt1715_tcpc_set_cc,
	.set_vconn_discharge_cb = rt1715_tcpc_set_vconn_discharge_cb,
	.set_vconn_cb = rt1715_tcpc_set_vconn_cb,
	.vconn_discharge = rt1715_tcpc_vconn_discharge,
	.set_vconn = rt1715_tcpc_set_vconn,
	.set_roles = rt1715_tcpc_set_roles,
	.get_rx_pending_msg = rt1715_tcpc_get_rx_pending_msg,
	.set_rx_enable = rt1715_tcpc_set_rx_enable,
	.set_cc_polarity = rt1715_tcpc_set_cc_polarity,
	.transmit_data = rt1715_tcpc_transmit_data,
	.dump_std_reg = rt1715_tcpc_dump_std_reg,
	.alert_handler_cb = rt1715_tcpc_alert_handler_cb,
	.get_status_register = rt1715_tcpc_get_status_register,
	.clear_status_register = rt1715_tcpc_clear_status_register,
	.mask_status_register = rt1715_tcpc_mask_status_register,
	.set_drp_toggle = rt1715_tcpc_set_drp_toggle,
	.get_chip_info = rt1715_tcpc_get_chip_info,
	.set_low_power_mode = rt1715_tcpc_set_low_power_mode,
	.sop_prime_enable = rt1715_tcpc_sop_prime_enable,
	.set_bist_test_mode = rt1715_tcpc_set_bist_test_mode,
	.set_alert_handler_cb = rt1715_tcpc_set_alert_handler_cb,
};

void rt1715_alert_cb(const struct device *port, struct gpio_callback *cb, gpio_port_pins_t pins)
{
	struct rt1715_data *data = CONTAINER_OF(cb, struct rt1715_data, alert_cb);

	k_work_submit(&data->alert_work);
}

void rt1715_alert_work_cb(struct k_work *work)
{
	struct rt1715_data *data = CONTAINER_OF(work, struct rt1715_data, alert_work);
	const struct device *dev = data->dev;
	const struct rt1715_cfg *cfg = dev->config;
	uint16_t alert_reg = 0;
	uint16_t clear_flags = 0;

	if (!data->initialized) {
		return;
	}

	tcpci_tcpm_get_status_register(&cfg->bus, TCPC_ALERT_STATUS, &alert_reg);

	while (alert_reg != 0) {
		enum tcpc_alert alert_type = tcpci_alert_reg_to_enum(alert_reg);

		if (alert_type == TCPC_ALERT_HARD_RESET_RECEIVED) {
			LOG_DBG("hard rst received");
			tcpci_init_alert_mask(dev);
			data->cc_changed = true;
		} else if (alert_type == TCPC_ALERT_FAULT_STATUS) {
			uint8_t fault;

			tcpci_tcpm_get_status_register(&cfg->bus, TCPC_FAULT_STATUS,
						       (uint16_t *)&fault);
			tcpci_tcpm_clear_status_register(&cfg->bus, TCPC_FAULT_STATUS,
							 (uint16_t)fault);

			LOG_DBG("fault: %02x", fault);
		} else if (alert_type == TCPC_ALERT_POWER_STATUS) {
			uint8_t pwr_status;

			tcpci_tcpm_get_status_register(&cfg->bus, TCPC_POWER_STATUS,
						       (uint16_t *)&pwr_status);

			LOG_DBG("power status: %02x", pwr_status);
		} else if (alert_type == TCPC_ALERT_MSG_STATUS) {
			LOG_DBG("MSG pending");

			if (rt1715_tcpc_rx_fifo_enqueue(dev) == 0) {
				data->msg_pending = true;
			}
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

	/* If alert_reg is not 0 or the interrupt signal is still active */
	if ((alert_reg != 0) || gpio_pin_get_dt(&cfg->alert_gpio)) {
		k_work_submit(work);
	}
}

void rt1715_init_work_cb(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct rt1715_data *data = CONTAINER_OF(dwork, struct rt1715_data, init_dwork);

	const struct rt1715_cfg *cfg = data->dev->config;
	uint8_t power_reg, lp_reg = 0;
	struct tcpc_chip_info chip_info;
	int ret;

	LOG_INF("Initializing RT1715 chip: %s", data->dev->name);

	ret = tcpci_tcpm_get_status_register(&cfg->bus, TCPC_POWER_STATUS, (uint16_t *)&power_reg);
	if (ret != 0 || (power_reg & TCPC_REG_POWER_STATUS_UNINIT)) {
		data->init_retries++;

		if (data->init_retries > CONFIG_USBC_TCPC_RT1715_INIT_RETRIES) {
			LOG_ERR("Chip didn't respond");
			return;
		}

		LOG_DBG("Postpone chip initialization %d", data->init_retries);
		k_work_schedule(&data->init_dwork, K_MSEC(CONFIG_USBC_TCPC_RT1715_INIT_DELAY));

		return;
	}

	rt1715_tcpc_get_chip_info(data->dev, &chip_info);
	LOG_INF("Initialized chip is: %04x:%04x:%04x", chip_info.vendor_id, chip_info.product_id,
		chip_info.device_id);

	/* Exit shutdown mode & Enable ext messages */
	lp_reg = RT1715_REG_LP_CTRL_SHUTDOWN_OFF | RT1715_REG_LP_CTRL_ENEXTMSG;
	/* Disable idle mode */
	lp_reg &= ~RT1715_REG_LP_CTRL_AUTOIDLE_EN;
	tcpci_write_reg8(&cfg->bus, RT1715_REG_LP_CTRL, lp_reg);

	/* Initialize alert interrupt */
	gpio_pin_configure_dt(&cfg->alert_gpio, GPIO_INPUT);

	gpio_init_callback(&data->alert_cb, rt1715_alert_cb, BIT(cfg->alert_gpio.pin));
	gpio_add_callback(cfg->alert_gpio.port, &data->alert_cb);
	gpio_pin_interrupt_configure_dt(&cfg->alert_gpio, GPIO_INT_EDGE_TO_ACTIVE);

	tcpci_init_alert_mask(data->dev);
	data->initialized = true;

	/* Disable the vconn and open CC lines to reinitialize the communication with partner */
	rt1715_tcpc_set_vconn(data->dev, false);
	rt1715_tcpc_set_cc(data->dev, TC_CC_OPEN);

	/* Check and clear any alert set after initialization */
	k_work_submit(&data->alert_work);
}

static int rt1715_dev_init(const struct device *dev)
{
	const struct rt1715_cfg *cfg = dev->config;
	struct rt1715_data *data = dev->data;
	int ret;

	if (!device_is_ready(cfg->bus.bus)) {
		return -EIO;
	}

	/* Resets the chip */
	ret = tcpci_write_reg8(&cfg->bus, RT1715_REG_SW_RST, RT1715_REG_SW_RST_EN);
	if (ret != 0) {
		LOG_ERR("Failed to reset chip: %d", ret);
		return ret;
	}

	k_work_init_delayable(&data->init_dwork, rt1715_init_work_cb);
	k_work_schedule(&data->init_dwork, K_MSEC(CONFIG_USBC_TCPC_RT1715_INIT_DELAY));

	k_work_init(&data->alert_work, rt1715_alert_work_cb);

	return 0;
}

#define RT1715_DRIVER_DATA_INIT(node)                                                              \
	{                                                                                          \
		.dev = DEVICE_DT_GET(node),                                                        \
		.init_retries = 0,                                                                 \
		.cc_changed = true,                                                                \
	}

#define VCONN_CTRL_GPIO(node)                                                                      \
	.vconn_ctrl_gpio = COND_CODE_1(DT_INST_NODE_HAS_PROP(node, vconn_ctrl_gpios),              \
				       (GPIO_DT_SPEC_INST_GET(node, vconn_ctrl_gpios)), ({0}))

#define VCONN_DISC_GPIO(node)                                                                      \
	.vconn_disc_gpio = COND_CODE_1(DT_INST_NODE_HAS_PROP(node, vconn_disc_gpios),              \
				       (GPIO_DT_SPEC_INST_GET(node, vconn_disc_gpios)), ({0}))

#define RT1715_DRIVER_CFG_INIT(node)                                                               \
	{                                                                                          \
		.bus = I2C_DT_SPEC_GET(node),                                                      \
		.alert_gpio = GPIO_DT_SPEC_GET(node, irq_gpios),                                   \
		.transmit_retries = DT_PROP(node, transmit_retries),                               \
		VCONN_CTRL_GPIO(node),                                                             \
		VCONN_DISC_GPIO(node),                                                             \
	}

#define RT1715_DRIVER_INIT(inst)                                                                   \
	static struct rt1715_data drv_data_rt1715##inst =                                          \
		RT1715_DRIVER_DATA_INIT(DT_DRV_INST(inst));                                        \
	static struct rt1715_cfg drv_cfg_rt1715##inst = RT1715_DRIVER_CFG_INIT(DT_DRV_INST(inst)); \
	DEVICE_DT_INST_DEFINE(inst, &rt1715_dev_init, NULL, &drv_data_rt1715##inst,                \
			      &drv_cfg_rt1715##inst, POST_KERNEL, CONFIG_USBC_TCPC_INIT_PRIORITY,  \
			      &rt1715_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RT1715_DRIVER_INIT)
