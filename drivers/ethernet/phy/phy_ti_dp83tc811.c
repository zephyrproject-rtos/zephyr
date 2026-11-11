/*
 * Copyright (c) 2026 ZAL Zentrum für Angewandte Luftfahrtforschung GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * TI DP83TC811 100BASE-T1 Ethernet PHY.
 *
 * The link is always 100 Mbit/s full duplex (100BASE-T1, no auto-negotiation).
 * The MAC interface (MII, RMII, RGMII) and the master/slave role are selected
 * by strap pins only.
 */

#define DT_DRV_COMPAT ti_dp83tc811

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/mdio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/mii.h>
#include <zephyr/net/phy.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(phy_ti_dp83tc811, CONFIG_PHY_LOG_LEVEL);

/* PHY identifier (PHYIDR1/PHYIDR2); the lowest 4 bits are the revision and
 * are ignored when matching
 */
#define DP83TC811_PHY_ID      0x2000A253U
#define DP83TC811_PHY_ID_MASK 0xFFFFFFF0U

/* Extended register access through REGCR/ADDAR (device address 0x1F) */
#define DP83TC811_REGCR            0x0DU
#define DP83TC811_ADDAR            0x0EU
#define DP83TC811_REGCR_DEVAD      0x001FU
#define DP83TC811_REGCR_DATA_NOINC 0x4000U

/* Power mode register (extended): force the PHY to the normal state */
#define DP83TC811_PWRM        0x018CU
#define DP83TC811_PWRM_NORMAL BIT(0)

/*
 * Interrupt registers: bits 15..8 are the status flags (cleared on read),
 * bits 7..0 the matching enables.
 */
#define DP83TC811_INT_STAT1          0x12U
#define DP83TC811_INT_STAT1_LINK_EN  BIT(5)
#define DP83TC811_INT_STAT2          0x13U
#define DP83TC811_INT_STAT3          0x18U

struct dp83tc811_config {
	uint8_t addr;
	const struct device *mdio_dev;
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
	struct gpio_dt_spec reset_gpio;
	uint32_t reset_assert_us;
	uint32_t reset_deassert_us;
#endif
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
	struct gpio_dt_spec int_gpio;
#endif
};

struct dp83tc811_data {
	const struct device *dev;
	struct phy_link_state state;
	phy_callback_t cb;
	void *cb_data;
	struct k_mutex mutex;
	struct k_work_delayable monitor_work;
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
	struct gpio_callback gpio_cb;
#endif
};

static int dp83tc811_read(const struct device *dev, uint16_t reg, uint32_t *value)
{
	const struct dp83tc811_config *cfg = dev->config;
	uint16_t v;
	int ret;

	ret = mdio_read(cfg->mdio_dev, cfg->addr, reg, &v);
	if (ret == 0) {
		*value = v;
	}
	return ret;
}

static int dp83tc811_write(const struct device *dev, uint16_t reg, uint32_t value)
{
	const struct dp83tc811_config *cfg = dev->config;

	return mdio_write(cfg->mdio_dev, cfg->addr, reg, (uint16_t)value);
}

/* Write a register beyond 0x1F (datasheet: Extended Register Space Access) */
static int dp83tc811_write_ext(const struct device *dev, uint16_t reg, uint16_t value)
{
	int ret;

	ret = dp83tc811_write(dev, DP83TC811_REGCR, DP83TC811_REGCR_DEVAD);
	if (ret == 0) {
		ret = dp83tc811_write(dev, DP83TC811_ADDAR, reg);
	}
	if (ret == 0) {
		ret = dp83tc811_write(dev, DP83TC811_REGCR,
				      DP83TC811_REGCR_DATA_NOINC | DP83TC811_REGCR_DEVAD);
	}
	if (ret == 0) {
		ret = dp83tc811_write(dev, DP83TC811_ADDAR, value);
	}
	return ret;
}

static bool dp83tc811_has_int_gpio(const struct device *dev)
{
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
	const struct dp83tc811_config *cfg = dev->config;

	return cfg->int_gpio.port != NULL;
#else
	ARG_UNUSED(dev);
	return false;
#endif
}

/* Read all interrupt status registers, which clears them */
static void dp83tc811_ack_interrupts(const struct device *dev)
{
	uint32_t v;

	(void)dp83tc811_read(dev, DP83TC811_INT_STAT1, &v);
	(void)dp83tc811_read(dev, DP83TC811_INT_STAT2, &v);
	(void)dp83tc811_read(dev, DP83TC811_INT_STAT3, &v);
}

static int dp83tc811_get_link(const struct device *dev, struct phy_link_state *state)
{
	struct dp83tc811_data *data = dev->data;
	uint32_t bmsr;
	int ret;

	k_mutex_lock(&data->mutex, K_FOREVER);
	/* The link status bit latches low: read twice for the current state */
	ret = dp83tc811_read(dev, MII_BMSR, &bmsr);
	if (ret == 0) {
		ret = dp83tc811_read(dev, MII_BMSR, &bmsr);
	}
	k_mutex_unlock(&data->mutex);

	if (ret != 0) {
		return ret;
	}

	state->is_up = (bmsr & MII_BMSR_LINK_STATUS) != 0U;
	state->speed = LINK_FULL_100BASE;
	return 0;
}

static int dp83tc811_cfg_link(const struct device *dev, enum phy_link_speed adv_speeds,
			      enum phy_cfg_link_flag flags)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(flags);

	/* 100BASE-T1 only supports 100 Mbit/s full duplex; nothing to configure */
	return ((adv_speeds & LINK_FULL_100BASE) != 0) ? 0 : -ENOTSUP;
}

static int dp83tc811_link_cb_set(const struct device *dev, phy_callback_t cb, void *user_data)
{
	struct dp83tc811_data *data = dev->data;
	struct phy_link_state state = {0};
	bool valid;

	k_mutex_lock(&data->mutex, K_FOREVER);
	valid = (dp83tc811_get_link(dev, &state) == 0);
	data->cb = cb;
	data->cb_data = user_data;
	if (valid) {
		data->state = state;
	}
	k_mutex_unlock(&data->mutex);

	if (valid && cb != NULL) {
		cb(dev, &state, user_data);
	}
	return 0;
}

/* True while the PHY still asserts INT (the pin is level-triggered) */
static bool dp83tc811_int_pending(const struct device *dev)
{
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
	const struct dp83tc811_config *cfg = dev->config;

	return cfg->int_gpio.port != NULL && gpio_pin_get_dt(&cfg->int_gpio) > 0;
#else
	ARG_UNUSED(dev);
	return false;
#endif
}

static void dp83tc811_monitor_work(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct dp83tc811_data *data = CONTAINER_OF(dwork, struct dp83tc811_data, monitor_work);
	const struct device *dev = data->dev;
	struct phy_link_state state = {0};
	phy_callback_t cb = NULL;
	void *cb_data = NULL;
	bool changed = false;
	int ret;

	if (dp83tc811_has_int_gpio(dev)) {
		dp83tc811_ack_interrupts(dev);
	}

	k_mutex_lock(&data->mutex, K_FOREVER);
	ret = dp83tc811_get_link(dev, &state);
	if (ret == 0 && (state.is_up != data->state.is_up || state.speed != data->state.speed)) {
		data->state = state;
		cb = data->cb;
		cb_data = data->cb_data;
		changed = true;
	}
	k_mutex_unlock(&data->mutex);

	if (changed) {
		LOG_INF("PHY %u link %s", ((const struct dp83tc811_config *)dev->config)->addr,
			state.is_up ? "up (100 Mbit/s full duplex)" : "down");
		if (cb != NULL) {
			cb(dev, &state, cb_data);
		}
	}

	if (!dp83tc811_has_int_gpio(dev) || ret != 0) {
		/* Poll without an interrupt GPIO, or retry after a failed MDIO read */
		k_work_reschedule(&data->monitor_work, K_MSEC(CONFIG_PHY_MONITOR_PERIOD));
	} else if (dp83tc811_int_pending(dev)) {
		/*
		 * An event arrived while the status registers were being read: INT
		 * never deasserted, so no new edge will be generated. Run again,
		 * delayed so that a stuck INT line cannot monopolize the work queue.
		 */
		k_work_reschedule(&data->monitor_work, K_MSEC(1));
	}
}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
static void dp83tc811_int_handler(const struct device *port, struct gpio_callback *cb,
				  gpio_port_pins_t pins)
{
	struct dp83tc811_data *data = CONTAINER_OF(cb, struct dp83tc811_data, gpio_cb);

	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	k_work_reschedule(&data->monitor_work, K_NO_WAIT);
}

static int dp83tc811_int_init(const struct device *dev)
{
	const struct dp83tc811_config *cfg = dev->config;
	struct dp83tc811_data *data = dev->data;
	uint32_t v;
	int ret;

	/* Enable the link status change interrupt in the PHY */
	ret = dp83tc811_read(dev, DP83TC811_INT_STAT1, &v);
	if (ret == 0) {
		ret = dp83tc811_write(dev, DP83TC811_INT_STAT1,
				      (v & 0xFFU) | DP83TC811_INT_STAT1_LINK_EN);
	}
	if (ret != 0) {
		return ret;
	}
	dp83tc811_ack_interrupts(dev);

	if (!gpio_is_ready_dt(&cfg->int_gpio)) {
		return -ENODEV;
	}
	ret = gpio_pin_configure_dt(&cfg->int_gpio, GPIO_INPUT);
	if (ret != 0) {
		return ret;
	}
	gpio_init_callback(&data->gpio_cb, dp83tc811_int_handler, BIT(cfg->int_gpio.pin));
	ret = gpio_add_callback_dt(&cfg->int_gpio, &data->gpio_cb);
	if (ret != 0) {
		return ret;
	}
	return gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
}
#endif

static int dp83tc811_reset(const struct device *dev)
{
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
	const struct dp83tc811_config *cfg = dev->config;
	int ret;

	if (cfg->reset_gpio.port == NULL) {
		return 0;
	}
	if (!gpio_is_ready_dt(&cfg->reset_gpio)) {
		return -ENODEV;
	}
	ret = gpio_pin_configure_dt(&cfg->reset_gpio, GPIO_OUTPUT_ACTIVE);
	if (ret != 0) {
		return ret;
	}
	k_busy_wait(cfg->reset_assert_us);
	ret = gpio_pin_set_dt(&cfg->reset_gpio, 0);
	if (ret != 0) {
		return ret;
	}
	k_usleep(cfg->reset_deassert_us);
	return 0;
#else
	ARG_UNUSED(dev);
	return 0;
#endif
}

/*
 * Wait until the PHY answers on MDIO with the expected identifier. After
 * power-up the PHY needs up to 60 ms before register access is possible
 * (datasheet T5.5), after a reset 2.5 us (T6.2).
 */
static int dp83tc811_wait_ready(const struct device *dev)
{
	const struct dp83tc811_config *cfg = dev->config;
	int64_t end = k_uptime_get() + CONFIG_PHY_TI_DP83TC811_READY_TIMEOUT_MS;
	uint32_t id1;
	uint32_t id2;
	uint32_t id;

	do {
		/* Report 0 for an ID half whose last read failed, not a stale value */
		id1 = 0U;
		id2 = 0U;
		if (dp83tc811_read(dev, MII_PHYID1R, &id1) == 0 &&
		    dp83tc811_read(dev, MII_PHYID2R, &id2) == 0) {
			id = (id1 << 16) | id2;
			if ((id & DP83TC811_PHY_ID_MASK) ==
			    (DP83TC811_PHY_ID & DP83TC811_PHY_ID_MASK)) {
				LOG_DBG("PHY %u: ID 0x%08x after reset", cfg->addr, id);
				return 0;
			}
		}
		k_msleep(1);
	} while (k_uptime_get() < end);

	LOG_ERR("PHY %u: no DP83TC811 (ID 0x%04x%04x), expected 0x%08x", cfg->addr, id1, id2,
		DP83TC811_PHY_ID);
	return -ENODEV;
}

static int dp83tc811_init(const struct device *dev)
{
	const struct dp83tc811_config *cfg = dev->config;
	struct dp83tc811_data *data = dev->data;
	int ret;

	data->dev = dev;
	k_mutex_init(&data->mutex);
	k_work_init_delayable(&data->monitor_work, dp83tc811_monitor_work);

	if (!device_is_ready(cfg->mdio_dev)) {
		return -ENODEV;
	}

	ret = dp83tc811_reset(dev);
	if (ret != 0) {
		return ret;
	}

	ret = dp83tc811_wait_ready(dev);
	if (ret != 0) {
		return ret;
	}

	/*
	 * A PHY strapped as 100BASE-T1 master in managed operation stays in
	 * standby (no link) after power-up and reset until it is commanded to
	 * the normal state. Slaves and autonomous masters are already in the
	 * normal state; the command has no effect on them.
	 */
	ret = dp83tc811_write_ext(dev, DP83TC811_PWRM, DP83TC811_PWRM_NORMAL);
	if (ret != 0) {
		return ret;
	}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
	if (dp83tc811_has_int_gpio(dev)) {
		ret = dp83tc811_int_init(dev);
		if (ret != 0) {
			return ret;
		}
	}
#endif

	/* Initial link state; continues polling if there is no interrupt GPIO */
	k_work_reschedule(&data->monitor_work, K_NO_WAIT);
	return 0;
}

static DEVICE_API(ethphy, dp83tc811_api) = {
	.get_link = dp83tc811_get_link,
	.cfg_link = dp83tc811_cfg_link,
	.link_cb_set = dp83tc811_link_cb_set,
	.read = dp83tc811_read,
	.write = dp83tc811_write,
};

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
#define DP83TC811_RESET_GPIO(n)                                                                    \
	.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(n, reset_gpios, {0}),                               \
	.reset_assert_us = DT_INST_PROP_OR(n, reset_assert_us, 25),                                \
	.reset_deassert_us = DT_INST_PROP_OR(n, reset_deassert_us, 0),
#else
#define DP83TC811_RESET_GPIO(n)
#endif

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
#define DP83TC811_INT_GPIO(n) .int_gpio = GPIO_DT_SPEC_INST_GET_OR(n, int_gpios, {0}),
#else
#define DP83TC811_INT_GPIO(n)
#endif

#define DP83TC811_DEFINE(n)                                                                        \
	static const struct dp83tc811_config dp83tc811_config_##n = {                              \
		.addr = DT_INST_REG_ADDR(n),                                                       \
		.mdio_dev = DEVICE_DT_GET(DT_INST_PARENT(n)),                                      \
		DP83TC811_RESET_GPIO(n) DP83TC811_INT_GPIO(n)};                                    \
	static struct dp83tc811_data dp83tc811_data_##n;                                           \
	DEVICE_DT_INST_DEFINE(n, dp83tc811_init, NULL, &dp83tc811_data_##n,                        \
			      &dp83tc811_config_##n, POST_KERNEL, CONFIG_PHY_INIT_PRIORITY,        \
			      &dp83tc811_api);

DT_INST_FOREACH_STATUS_OKAY(DP83TC811_DEFINE)
