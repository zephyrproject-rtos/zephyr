/*
 * Copyright 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/shell/shell.h>
#include <zephyr/drivers/usb_c/usbc_ppc.h>

#include "nxp_nx20p3483_priv.h"

#define DT_DRV_COMPAT nxp_nx20p3483
LOG_MODULE_REGISTER(nxp_nx20p3483, CONFIG_USBC_PPC_LOG_LEVEL);

#ifdef CONFIG_USBC_PPC_NX20P3483_DUMP_FULL_REG_NAMES
static const char *const nx20p3483_reg_names[] = {
	"Device ID           ", "Device Status       ", "Switch Control      ",
	"Switch Status       ", "Interrupt 1         ", "Interrupt 2         ",
	"Interrupt 1 Mask    ", "Interrupt 2 Mask    ", "OVLO Threshold      ",
	"HV SRC OCP Threshold", "5V SRC OCP Threshold", "Device Control      ",
};
#endif

/* Driver structures */

struct nx20p3483_cfg {
	/** Device address on I2C bus */
	const struct i2c_dt_spec bus;
	/** GPIO used as interrupt request */
	const struct gpio_dt_spec irq_gpio;

	/** Overvoltage protection threshold for sink role */
	int snk_ovp_thresh;
	/** Boolean value whether to use high-voltage source if true or 5V source if false */
	bool src_use_hv;
	/** Overcurrent protection threshold for 5V source role */
	int src_5v_ocp_thresh;
	/** Overcurrent protection threshold for HV source role */
	int src_hv_ocp_thresh;
};

struct nx20p3483_data {
	/** Device structure to get from data structure */
	const struct device *dev;
	/** Interrupt request callback object */
	struct gpio_callback irq_cb;
	/** Workqueue object for handling interrupts */
	struct k_work irq_work;

	/** Callback used to notify about PPC events, like overcurrent or short */
	usbc_ppc_event_cb_t event_cb;
	/** Data sent as parameter to the callback */
	void *event_cb_data;
};

/* Helper functions */

static int read_reg(const struct device *dev, uint8_t reg, uint8_t *value)
{
	const struct nx20p3483_cfg *cfg = dev->config;
	int ret;

	ret = i2c_reg_read_byte(cfg->bus.bus, cfg->bus.addr, reg, value);
	if (ret != 0) {
		LOG_ERR("Error reading reg %02x: %d", reg, ret);
		return ret;
	}

	return 0;
}

static int write_reg(const struct device *dev, uint8_t reg, uint8_t value)
{
	const struct nx20p3483_cfg *cfg = dev->config;
	int ret;

	ret = i2c_reg_write_byte(cfg->bus.bus, cfg->bus.addr, reg, value);
	if (ret != 0) {
		LOG_ERR("Error writing reg %02x: %d", reg, ret);
		return ret;
	}

	return 0;
}

static int nx20p3483_set_snk_ovp_limit(const struct device *dev, uint8_t u_thresh)
{
	int ret;

	if (u_thresh < NX20P3483_I_THRESHOLD_0_400 || u_thresh > NX20P3483_I_THRESHOLD_3_400) {
		return -EINVAL;
	}

	ret = write_reg(dev, NX20P3483_REG_OVLO_THRESHOLD, u_thresh);
	if (ret != 0) {
		LOG_ERR("Couldn't set SNK OVP: %d", ret);
		return ret;
	}

	LOG_DBG("Set SNK OVP: %d", u_thresh);
	return 0;
}

/* API functions */

int nx20p3483_is_dead_battery_mode(const struct device *dev)
{
	uint8_t sts_reg;
	int ret;

	ret = read_reg(dev, NX20P3483_REG_DEVICE_STATUS, &sts_reg);
	if (ret != 0) {
		return ret;
	}

	return ((sts_reg & NX20P3483_REG_DEVICE_STATUS_MODE_MASK) == NX20P3483_MODE_DEAD_BATTERY);
}

int nx20p3483_exit_dead_battery_mode(const struct device *dev)
{
	uint8_t ctrl_reg;
	int ret;

	ret = read_reg(dev, NX20P3483_REG_DEVICE_CTRL, &ctrl_reg);
	if (ret != 0) {
		return ret;
	}

	ctrl_reg |= NX20P3483_REG_DEVICE_CTRL_DB_EXIT;
	ret = write_reg(dev, NX20P3483_REG_DEVICE_CTRL, ctrl_reg);
	if (ret != 0) {
		return ret;
	}

	return 0;
}

static int nx20p3483_is_vbus_source(const struct device *dev)
{
	uint8_t sts_reg;
	int ret;

	ret = read_reg(dev, NX20P3483_REG_SWITCH_STATUS, &sts_reg);
	if (ret != 0) {
		return ret;
	}

	return !!(sts_reg &
		  (NX20P3483_REG_SWITCH_STATUS_5VSRC | NX20P3483_REG_SWITCH_STATUS_HVSRC));
}

static int nx20p3483_is_vbus_sink(const struct device *dev)
{
	uint8_t sts_reg;
	int ret;

	ret = read_reg(dev, NX20P3483_REG_SWITCH_STATUS, &sts_reg);
	if (ret != 0) {
		return ret;
	}

	return !!(sts_reg & NX20P3483_REG_SWITCH_STATUS_HVSNK);
}

static int nx20p3483_set_vbus_sink(const struct device *dev, bool enable)
{
	const struct nx20p3483_cfg *cfg = dev->config;

	/*
	 * The nx20p3483 is enabled by external GPIO signal, however enabling it sets the
	 * overvoltage threshold to the highest possible value. Due to that, the threshold has
	 * to be set here again. Must be called after enabling the path by the external signal.
	 */
	return nx20p3483_set_snk_ovp_limit(dev, cfg->snk_ovp_thresh);
}

static int nx20p3483_set_vbus_discharge(const struct device *dev, bool enable)
{
	uint8_t ctrl_reg;
	int ret;

	ret = read_reg(dev, NX20P3483_REG_DEVICE_CTRL, &ctrl_reg);
	if (ret != 0) {
		return ret;
	}

	if (enable) {
		ctrl_reg |= NX20P3483_REG_DEVICE_CTRL_VBUSDIS_EN;
	} else {
		ctrl_reg &= ~NX20P3483_REG_DEVICE_CTRL_VBUSDIS_EN;
	}

	ret = write_reg(dev, NX20P3483_REG_DEVICE_CTRL, ctrl_reg);

	return ret;
}

static int nx20p3483_set_event_handler(const struct device *dev, usbc_ppc_event_cb_t handler,
				       void *handler_data)
{
	struct nx20p3483_data *data = dev->data;

	data->event_cb = handler;
	data->event_cb_data = handler_data;

	return 0;
}

static int nx20p3483_dump_regs(const struct device *dev)
{
	const struct nx20p3483_cfg *cfg = dev->config;
	uint8_t val;

	LOG_INF("NX20P alert: %d", gpio_pin_get(cfg->irq_gpio.port, cfg->irq_gpio.pin));
	LOG_INF("PPC %s:%s registers:", cfg->bus.bus->name, dev->name);
	for (int a = 0; a <= NX20P3483_REG_DEVICE_CTRL; a++) {
		i2c_reg_read_byte(cfg->bus.bus, cfg->bus.addr, a, &val);

#ifdef CONFIG_USBC_PPC_NX20P3483_DUMP_FULL_REG_NAMES
		LOG_INF("- [%s] = 0x%02x", nx20p3483_reg_names[a], val);
#else
		LOG_INF("- [%02x] = 0x%02x", a, val);
#endif
	}

	return 0;
}

static DEVICE_API(usbc_ppc, nx20p3483_driver_api) = {
	.is_dead_battery_mode = nx20p3483_is_dead_battery_mode,
	.exit_dead_battery_mode = nx20p3483_exit_dead_battery_mode,
	.is_vbus_source = nx20p3483_is_vbus_source,
	.is_vbus_sink = nx20p3483_is_vbus_sink,
	.set_snk_ctrl = nx20p3483_set_vbus_sink,
	.set_vbus_discharge = nx20p3483_set_vbus_discharge,
	.set_event_handler = nx20p3483_set_event_handler,
	.dump_regs = nx20p3483_dump_regs,
};

static int nx20p3483_set_src_ovc_limit(const struct device *dev, uint8_t i_thresh_5v,
				       uint8_t i_thresh_hv)
{
	int ret;

	if (i_thresh_5v < NX20P3483_I_THRESHOLD_0_400 ||
	    i_thresh_5v > NX20P3483_I_THRESHOLD_3_400) {
		LOG_ERR("Invalid SRC 5V ovc threshold: %d", i_thresh_5v);
		return -EINVAL;
	}

	if (i_thresh_hv < NX20P3483_I_THRESHOLD_0_400 ||
	    i_thresh_hv > NX20P3483_I_THRESHOLD_3_400) {
		LOG_ERR("Invalid SRC HV ovc threshold: %d", i_thresh_hv);
		return -EINVAL;
	}

	ret = write_reg(dev, NX20P3483_REG_5V_SRC_OCP_THRESHOLD, i_thresh_5v);
	if (ret != 0) {
		return ret;
	}

	ret = write_reg(dev, NX20P3483_REG_HV_SRC_OCP_THRESHOLD, i_thresh_hv);
	if (ret != 0) {
		return ret;
	}

	LOG_DBG("Set SRC OVC 5V: %d, HV: %d", i_thresh_5v, i_thresh_hv);
	return 0;
}

static void nx20p3483_send_event(const struct device *dev, enum usbc_ppc_event ev)
{
	struct nx20p3483_data *data = dev->data;

	if (data->event_cb != NULL) {
		data->event_cb(dev, data->event_cb_data, ev);
	}
}

static void nx20p3483_irq_handler(const struct device *port, struct gpio_callback *cb,
				  gpio_port_pins_t pins)
{
	struct nx20p3483_data *data = CONTAINER_OF(cb, struct nx20p3483_data, irq_cb);

	k_work_submit(&data->irq_work);
}

static void nx20p3483_irq_worker(struct k_work *work)
{
	struct nx20p3483_data *data = CONTAINER_OF(work, struct nx20p3483_data, irq_work);
	const struct device *dev = data->dev;
	uint8_t irq1, irq2;
	int ret;

	ret = read_reg(dev, NX20P3483_REG_INT1, &irq1);
	if (ret != 0) {
		LOG_ERR("Couldn't read irq1");
		return;
	}

	ret = read_reg(dev, NX20P3483_REG_INT2, &irq2);
	if (ret != 0) {
		LOG_ERR("Couldn't read irq2");
		return;
	}

	if (data->event_cb == NULL) {
		LOG_DBG("No callback set: %02x %02x", irq1, irq1);
	}

	/* Generic alerts */
	if (irq1 & NX20P3483_REG_INT1_DBEXIT_ERR) {
		LOG_INF("PPC dead battery exit failed");
		nx20p3483_send_event(dev, USBC_PPC_EVENT_DEAD_BATTERY_ERROR);
	}

	if (irq1 & NX20P3483_REG_INT1_OTP) {
		LOG_INF("PPC over temperature");
		nx20p3483_send_event(dev, USBC_PPC_EVENT_OVER_TEMPERATURE);
	}

	if (irq1 & NX20P3483_REG_INT2_EN_ERR) {
		LOG_INF("PPC source and sink enabled");
		nx20p3483_send_event(dev, USBC_PPC_EVENT_BOTH_SNKSRC_ENABLED);
	}

	/* Source */
	if (irq1 & NX20P3483_REG_INT1_OV_5VSRC || irq2 & NX20P3483_REG_INT2_OV_HVSRC) {
		LOG_INF("PPC source overvoltage");
		nx20p3483_send_event(dev, USBC_PPC_EVENT_SRC_OVERVOLTAGE);
	}

	if (irq1 & NX20P3483_REG_INT1_RCP_5VSRC || irq2 & NX20P3483_REG_INT2_RCP_HVSRC) {
		LOG_INF("PPC source reverse current");
		nx20p3483_send_event(dev, USBC_PPC_EVENT_SRC_REVERSE_CURRENT);
	}

	if (irq1 & NX20P3483_REG_INT1_OC_5VSRC || irq2 & NX20P3483_REG_INT2_OC_HVSRC) {
		LOG_INF("PPC source overcurrent");
		nx20p3483_send_event(dev, USBC_PPC_EVENT_SRC_OVERCURRENT);
	}

	if (irq1 & NX20P3483_REG_INT1_SC_5VSRC || irq2 & NX20P3483_REG_INT2_SC_HVSRC) {
		LOG_INF("PPC source short");
		nx20p3483_send_event(dev, USBC_PPC_EVENT_SRC_SHORT);
	}

	/* Sink */
	if (irq2 & NX20P3483_REG_INT2_RCP_HVSNK) {
		LOG_INF("PPC sink reverse current");
		nx20p3483_send_event(dev, USBC_PPC_EVENT_SNK_REVERSE_CURRENT);
	}

	if (irq2 & NX20P3483_REG_INT2_SC_HVSNK) {
		LOG_INF("PPC sink short");
		nx20p3483_send_event(dev, USBC_PPC_EVENT_SNK_SHORT);
	}

	if (irq2 & NX20P3483_REG_INT2_OV_HVSNK) {
		LOG_INF("PPC sink overvoltage");
		nx20p3483_send_event(dev, USBC_PPC_EVENT_SNK_OVERVOLTAGE);
	}
}

static int nx20p3483_dev_init(const struct device *dev)
{
	const struct nx20p3483_cfg *cfg = dev->config;
	struct nx20p3483_data *data = dev->data;
	uint8_t reg;
	int ret;

	LOG_INF("Initializing PPC");

	/* Initialize irq */
	ret = gpio_pin_configure(cfg->irq_gpio.port, cfg->irq_gpio.pin, GPIO_INPUT | GPIO_PULL_UP);
	if (ret != 0) {
		return ret;
	}

	ret = gpio_pin_interrupt_configure(cfg->irq_gpio.port, cfg->irq_gpio.pin,
					   GPIO_INT_EDGE_FALLING);
	if (ret != 0) {
		return ret;
	}

	gpio_init_callback(&data->irq_cb, nx20p3483_irq_handler, BIT(cfg->irq_gpio.pin));
	ret = gpio_add_callback(cfg->irq_gpio.port, &data->irq_cb);
	if (ret != 0) {
		return ret;
	}

	/* Initialize work_q */
	k_work_init(&data->irq_work, nx20p3483_irq_worker);
	k_work_submit(&data->irq_work);

	/* If src_use_hv, select the HV src path but do not enable it yet */
	read_reg(dev, NX20P3483_REG_SWITCH_CTRL, &reg);
	if (cfg->src_use_hv) {
		reg |= NX20P3483_REG_SWITCH_CTRL_SRC;
	} else {
		reg &= ~NX20P3483_REG_SWITCH_CTRL_SRC;
	}

	write_reg(dev, NX20P3483_REG_SWITCH_CTRL, reg);

	/* Set limits */
	ret = nx20p3483_set_snk_ovp_limit(dev, cfg->snk_ovp_thresh);
	if (ret != 0) {
		return ret;
	}

	ret = nx20p3483_set_src_ovc_limit(dev, cfg->src_5v_ocp_thresh, cfg->src_hv_ocp_thresh);
	if (ret != 0) {
		return ret;
	}

	return 0;
}

#define NX20P3483_DRIVER_CFG_INIT(node)                                                            \
	{                                                                                          \
		.bus = I2C_DT_SPEC_GET(node), .irq_gpio = GPIO_DT_SPEC_GET(node, irq_gpios),       \
		.snk_ovp_thresh = DT_PROP(node, snk_ovp), .src_use_hv = DT_PROP(node, src_hv),     \
		.src_5v_ocp_thresh = DT_PROP(node, src_5v_ocp),                                    \
		.src_hv_ocp_thresh = DT_PROP(node, src_hv_ocp),                                    \
	}

#define NX20P3483_DRIVER_CFG_ASSERTS(node)                                                         \
	BUILD_ASSERT(DT_PROP(node, snk_ovp) >= NX20P3483_U_THRESHOLD_6_0 &&                        \
			     DT_PROP(node, snk_ovp) <= NX20P3483_U_THRESHOLD_23_0,                 \
		     "Invalid overvoltage threshold");                                             \
	BUILD_ASSERT(DT_PROP(node, src_5v_ocp) >= NX20P3483_I_THRESHOLD_0_400 &&                   \
			     DT_PROP(node, src_5v_ocp) <= NX20P3483_I_THRESHOLD_3_400,             \
		     "Invalid overcurrent threshold");                                             \
	BUILD_ASSERT(DT_PROP(node, src_hv_ocp) >= NX20P3483_I_THRESHOLD_0_400 &&                   \
			     DT_PROP(node, src_hv_ocp) <= NX20P3483_I_THRESHOLD_3_400,             \
		     "Invalid overcurrent threshold");

#define NX20P3483_DRIVER_DATA_INIT(node)                                                           \
	{                                                                                          \
		.dev = DEVICE_DT_GET(node),                                                        \
	}

#define NX20P3483_DRIVER_INIT(inst)                                                                \
	static struct nx20p3483_data drv_data_nx20p3483##inst =                                    \
		NX20P3483_DRIVER_DATA_INIT(DT_DRV_INST(inst));                                     \
	NX20P3483_DRIVER_CFG_ASSERTS(DT_DRV_INST(inst));                                           \
	static struct nx20p3483_cfg drv_cfg_nx20p3483##inst =                                      \
		NX20P3483_DRIVER_CFG_INIT(DT_DRV_INST(inst));                                      \
	DEVICE_DT_INST_DEFINE(inst, &nx20p3483_dev_init, NULL, &drv_data_nx20p3483##inst,          \
			      &drv_cfg_nx20p3483##inst, POST_KERNEL,                               \
			      CONFIG_USBC_PPC_INIT_PRIORITY, &nx20p3483_driver_api);

DT_INST_FOREACH_STATUS_OKAY(NX20P3483_DRIVER_INIT)
