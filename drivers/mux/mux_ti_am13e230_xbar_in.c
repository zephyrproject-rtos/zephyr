/*
 * SPDX-FileCopyrightText: 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_am13e230_xbar_in

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/mux.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/util.h>

#include <zephyr/dt-bindings/mux/ti_am13e230_xbar_in.h>

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(ti_pwren)
#include <zephyr/drivers/syscon.h>
#include <zephyr/kernel.h>

#define AM13E230_XBAR_SYSCTL_PWREN_KEY (0x26U << 24)
#define AM13E230_XBAR_PWREN_SETTLE_US 1U
#endif

LOG_MODULE_REGISTER(mux_ti_am13e230_xbar_in, CONFIG_MUX_LOG_LEVEL);

#define AM13E230_XBAR_IN_NUM_CHANNELS              28
#define AM13E230_XBAR_IN_INPUTSELECT_OFFSET        0U
#define AM13E230_XBAR_IN_CHANNEL_SELECT_OFFSET(ch) ((ch) * 4U)

/*
 * Each GPIO's signal reaches the Input X-BAR through a RAW_INPUTSELECT_Pn
 * mux (TRM Figure 20-1): bit=0 (reset default) selects the signal AFTER
 * the GPIO peripheral module processes it, bit=1 selects the raw pad
 * signal directly. Caller-controlled via the packed state's raw field
 * (AM13E230_XBAR_IN_STATE()) - see ti_am13e230_xbar_in.h.
 */
#define AM13E230_XBAR_IN_RAW_INPUTSELECT_OFFSET      0x410U
#define AM13E230_XBAR_IN_RAW_INPUTSELECT_PORT_STRIDE 0x4U
#define AM13E230_XBAR_IN_GPIO_PER_PORT               32U
#define AM13E230_XBAR_IN_NUM_PORTS                   4U

struct am13e230_xbar_config {
	DEVICE_MMIO_ROM;
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(ti_pwren)
	const struct device *pwren_syscon;
	uint32_t pwren_offset;
	uint8_t pwren_bit;
	bool has_pwren;
#endif
};

struct am13e230_xbar_data {
	DEVICE_MMIO_RAM;
};

static int am13e230_xbar_set(const struct device *dev, const struct mux_control *control,
			     uint32_t state)
{
	uint32_t base = DEVICE_MMIO_GET(dev);
	uint32_t channel = control->cells[0];
	uint32_t pin = AM13E230_XBAR_IN_STATE_PIN(state);
	uint32_t raw = AM13E230_XBAR_IN_STATE_RAW(state);
	uint32_t port = pin / AM13E230_XBAR_IN_GPIO_PER_PORT;
	uint32_t bit = pin % AM13E230_XBAR_IN_GPIO_PER_PORT;
	uint32_t addr;
	uint32_t raw_addr;
	uint32_t raw_val;

	if (channel >= AM13E230_XBAR_IN_NUM_CHANNELS) {
		LOG_ERR("input xbar channel %u out of range", channel);
		return -EINVAL;
	}

	if (port >= AM13E230_XBAR_IN_NUM_PORTS) {
		LOG_ERR("gpio %u has no RAW_INPUTSELECT port", pin);
		return -EINVAL;
	}

	addr = base + AM13E230_XBAR_IN_INPUTSELECT_OFFSET +
	       AM13E230_XBAR_IN_CHANNEL_SELECT_OFFSET(channel);
	sys_write32(pin, addr);

	raw_addr = base + AM13E230_XBAR_IN_RAW_INPUTSELECT_OFFSET +
		   port * AM13E230_XBAR_IN_RAW_INPUTSELECT_PORT_STRIDE;
	raw_val = sys_read32(raw_addr);
	if (raw != 0U) {
		raw_val |= BIT(bit);
	} else {
		raw_val &= ~BIT(bit);
	}
	sys_write32(raw_val, raw_addr);

	LOG_DBG("xbar routed: channel=%u <- pin=%u raw=%u", channel, pin, raw);

	return 0;
}

static int am13e230_xbar_get_state(const struct device *dev, const struct mux_control *control,
				   uint32_t *state)
{
	uint32_t base = DEVICE_MMIO_GET(dev);
	uint32_t channel = control->cells[0];
	uint32_t addr;
	uint32_t pin;
	uint32_t port;
	uint32_t bit;
	uint32_t raw = 0U;

	if (channel >= AM13E230_XBAR_IN_NUM_CHANNELS) {
		LOG_ERR("input xbar channel %u out of range", channel);
		return -EINVAL;
	}

	addr = base + AM13E230_XBAR_IN_INPUTSELECT_OFFSET +
	       AM13E230_XBAR_IN_CHANNEL_SELECT_OFFSET(channel);
	pin = sys_read32(addr) & 0xFFU;

	port = pin / AM13E230_XBAR_IN_GPIO_PER_PORT;
	bit = pin % AM13E230_XBAR_IN_GPIO_PER_PORT;
	if (port < AM13E230_XBAR_IN_NUM_PORTS) {
		uint32_t raw_addr = base + AM13E230_XBAR_IN_RAW_INPUTSELECT_OFFSET +
				    port * AM13E230_XBAR_IN_RAW_INPUTSELECT_PORT_STRIDE;

		raw = (sys_read32(raw_addr) >> bit) & 0x1U;
	}

	*state = AM13E230_XBAR_IN_STATE(pin, raw);

	LOG_DBG("xbar channel=%u currently routed pin=%u raw=%u", channel, pin, raw);

	return 0;
}

static DEVICE_API(mux_control, am13e230_xbar_api) = {
	.set = am13e230_xbar_set,
	.get_state = am13e230_xbar_get_state,
};

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(ti_pwren)
static int am13e230_xbar_pwren_enable(const struct am13e230_xbar_config *cfg)
{
	uint32_t bit;
	uint32_t mask;
	int ret;

	if (!cfg->has_pwren) {
		return 0;
	}

	if (!device_is_ready(cfg->pwren_syscon)) {
		LOG_ERR("SYSCTL (pwren) device not ready");
		return -ENODEV;
	}

	bit = BIT(cfg->pwren_bit);
	mask = bit | AM13E230_XBAR_SYSCTL_PWREN_KEY;
	ret = syscon_update_bits(cfg->pwren_syscon, cfg->pwren_offset, mask, mask);
	if (ret < 0) {
		LOG_ERR("failed to set SYSCTL PWREN bit %u: %d", cfg->pwren_bit, ret);
		return ret;
	}

	k_busy_wait(AM13E230_XBAR_PWREN_SETTLE_US);

	return 0;
}
#endif

static int am13e230_xbar_init(const struct device *dev)
{
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(ti_pwren)
	const struct am13e230_xbar_config *cfg = dev->config;
	int ret;

	ret = am13e230_xbar_pwren_enable(cfg);
	if (ret < 0) {
		return ret;
	}
#else
	ARG_UNUSED(dev);
#endif

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);
	return 0;
}

#define AM13E230_XBAR_IN_PWREN_INIT(n)                                                             \
	IF_ENABLED(DT_ANY_INST_HAS_PROP_STATUS_OKAY(ti_pwren),                                    \
		   (COND_CODE_1(DT_INST_NODE_HAS_PROP(n, ti_pwren),                               \
				(.has_pwren = true,                                               \
				 .pwren_syscon = DEVICE_DT_GET(DT_INST_PHANDLE(n, ti_pwren)),     \
				 .pwren_offset = DT_INST_PHA(n, ti_pwren, offset),                \
				 .pwren_bit = DT_INST_PHA(n, ti_pwren, bit),), ())))

#define MUX_TI_AM13E230_XBAR_IN_INIT(n)                                                            \
	static const struct am13e230_xbar_config am13e230_xbar_cfg##n = {                          \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),                                              \
		AM13E230_XBAR_IN_PWREN_INIT(n)};                                                   \
	static struct am13e230_xbar_data am13e230_xbar_data_##n;                                   \
	DEVICE_DT_INST_DEFINE(n, am13e230_xbar_init, NULL, &am13e230_xbar_data_##n,                \
			      &am13e230_xbar_cfg##n, POST_KERNEL, CONFIG_MUX_INIT_PRIORITY,        \
			      &am13e230_xbar_api);

DT_INST_FOREACH_STATUS_OKAY(MUX_TI_AM13E230_XBAR_IN_INIT)
