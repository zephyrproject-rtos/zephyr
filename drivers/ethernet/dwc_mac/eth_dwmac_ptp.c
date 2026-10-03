/*
 * Driver for Synopsys DesignWare MAC PTP clock
 *
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT snps_dwmac_ptp_clock

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/ptp_clock.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#ifdef CONFIG_PINCTRL
#include <zephyr/drivers/pinctrl.h>
#endif

#include "eth_dwmac_priv.h"

LOG_MODULE_REGISTER(dwmac_ptp_clock, CONFIG_ETHERNET_LOG_LEVEL);

#define DWMAC_PTP_PINCTRL_ENABLED DT_ANY_INST_HAS_PROP_STATUS_OKAY(pinctrl_0)

struct dwmac_ptp_config {
	const struct device *eth_dev;
#if DWMAC_PTP_PINCTRL_ENABLED
	const struct pinctrl_dev_config *pincfg;
#endif
};

struct dwmac_ptp_data {
	uint32_t default_addend;
};

static void dwmac_ptp_wait_for_clear(mm_reg_t base, uint32_t reg, uint32_t mask)
{
	while (sys_read32(base + reg) & mask) {
		k_yield();
	}
}

static int dwmac_ptp_set(const struct device *dev, struct net_ptp_time *tm)
{
	const struct dwmac_ptp_config *cfg = dev->config;
	const struct device *eth_dev = cfg->eth_dev;
	struct dwmac_priv *p = eth_dev->data;
	mm_reg_t base = DEVICE_MMIO_GET(eth_dev);

	K_SPINLOCK(&p->spinlock) {
		sys_write32(tm->second, base + DWMAC_PTP_SEC_UPDATE_REG);
		sys_write32(DWMAC_PTP_NS_TO_SUBSEC(tm->nanosecond),
			    base + DWMAC_PTP_NSEC_UPDATE_REG);
		sys_write32(sys_read32(base + DWMAC_PTP_CTRL_REG) | DWMAC_PTP_CTRL_TIME_INIT,
			    base + DWMAC_PTP_CTRL_REG);
		while (sys_read32(base + DWMAC_PTP_CTRL_REG) & DWMAC_PTP_CTRL_TIME_INIT) {
			/* spin lock */
		}
	}

	return 0;
}

static int dwmac_ptp_get(const struct device *dev, struct net_ptp_time *tm)
{
	const struct dwmac_ptp_config *cfg = dev->config;
	const struct device *eth_dev = cfg->eth_dev;
	struct dwmac_priv *p = eth_dev->data;
	mm_reg_t base = DEVICE_MMIO_GET(eth_dev);
	uint32_t second_2;

	K_SPINLOCK(&p->spinlock) {
		tm->second = sys_read32(base + DWMAC_PTP_SEC_REG);
		tm->nanosecond = DWMAC_PTP_SUBSEC_TO_NS(sys_read32(base + DWMAC_PTP_NSEC_REG));
		second_2 = sys_read32(base + DWMAC_PTP_SEC_REG);
	}

	if (tm->second != second_2 && tm->nanosecond < NSEC_PER_SEC / 2) {
		/* Second rollover happened between the two reads. */
		tm->second = second_2;
	}

	return 0;
}

static int dwmac_ptp_adjust(const struct device *dev, int increment)
{
	const struct dwmac_ptp_config *cfg = dev->config;
	const struct device *eth_dev = cfg->eth_dev;
	struct dwmac_priv *p = eth_dev->data;
	mm_reg_t base = DEVICE_MMIO_GET(eth_dev);
	uint32_t subsec;

	if ((increment <= (int32_t)(-NSEC_PER_SEC)) || (increment >= (int32_t)NSEC_PER_SEC)) {
		return -EINVAL;
	}

	K_SPINLOCK(&p->spinlock) {
		sys_write32(0, base + DWMAC_PTP_SEC_UPDATE_REG);
		if (increment >= 0) {
			subsec = DWMAC_PTP_NS_TO_SUBSEC((uint32_t)increment);
			sys_write32(subsec, base + DWMAC_PTP_NSEC_UPDATE_REG);
		} else {
			subsec = DWMAC_PTP_NS_TO_SUBSEC((uint32_t)-increment);
#if defined(CONFIG_ETH_DWC_ETHER_QOS_CORE)
			/* the QoS core takes the subtrahend in complement form */
			subsec = DWMAC_PTP_SUBSEC_PER_SEC - subsec;
#endif
			sys_write32(DWMAC_PTP_NSEC_UPDATE_ADDSUB | subsec,
				    base + DWMAC_PTP_NSEC_UPDATE_REG);
		}
		sys_write32(sys_read32(base + DWMAC_PTP_CTRL_REG) | DWMAC_PTP_CTRL_TIME_UPDATE,
			    base + DWMAC_PTP_CTRL_REG);
		while (sys_read32(base + DWMAC_PTP_CTRL_REG) & DWMAC_PTP_CTRL_TIME_UPDATE) {
			/* spin lock */
		}
	}

	return 0;
}

static int dwmac_ptp_adjust_rate(const struct device *dev, int64_t scaled_ppm)
{
	const struct dwmac_ptp_config *cfg = dev->config;
	const struct device *eth_dev = cfg->eth_dev;
	struct dwmac_priv *p = eth_dev->data;
	const struct dwmac_ptp_data *data = dev->data;
	mm_reg_t base = DEVICE_MMIO_GET(eth_dev);
	uint32_t addend_val;

	if (ptp_clock_adjust_by_scaled_ppm(data->default_addend, scaled_ppm, &addend_val) != 0) {
		return -EINVAL;
	}

	K_SPINLOCK(&p->spinlock) {
		sys_write32(addend_val, base + DWMAC_PTP_ADDEND_REG);
		sys_write32(sys_read32(base + DWMAC_PTP_CTRL_REG) | DWMAC_PTP_CTRL_ADDEND_UPDATE,
			    base + DWMAC_PTP_CTRL_REG);
		while (sys_read32(base + DWMAC_PTP_CTRL_REG) & DWMAC_PTP_CTRL_ADDEND_UPDATE) {
			/* spin lock */
		}
	}

	return 0;
}

static int dwmac_ptp_init(const struct device *dev)
{
	const struct dwmac_ptp_config *cfg = dev->config;
	const struct device *eth_dev = cfg->eth_dev;
	const struct dwmac_config *eth_cfg = eth_dev->config;
	struct dwmac_ptp_data *data = dev->data;
	mm_reg_t base = DEVICE_MMIO_GET(eth_dev);
	uint32_t ptp_clk_rate;
	uint32_t ss_incr;
	uint32_t addend_val;
	uint64_t temp;
	int ret;

#if DWMAC_PTP_PINCTRL_ENABLED
	if (cfg->pincfg != NULL) {
		ret = pinctrl_apply_state(cfg->pincfg, PINCTRL_STATE_DEFAULT);
		if (ret < 0) {
			return ret;
		}
	}
#endif /* DWMAC_PTP_PINCTRL_ENABLED */

	ret = clock_control_get_rate(eth_cfg->clock, eth_cfg->ptp_clk, &ptp_clk_rate);
	if (ret < 0) {
		return -EIO;
	}

	/*
	 * Increment the sub-second counter by more than two reference clock
	 * periods per accumulator overflow, so that the default addend stays below
	 * half scale and any rate ratio up to 2.0 fits the addend register.
	 */
	ss_incr = (2ULL * DWMAC_PTP_SUBSEC_PER_SEC) / ptp_clk_rate + 1U;
	if (ss_incr > UINT8_MAX) {
		LOG_ERR("PTP reference clock of %u Hz is too slow", ptp_clk_rate);
		return -EINVAL;
	}

	sys_write32(ss_incr << DWMAC_PTP_SSINC_SHIFT, base + DWMAC_PTP_SSINC_REG);

	sys_write32(sys_read32(base + DWMAC_PTP_CTRL_REG) | DWMAC_PTP_CTRL_ENABLE,
		    base + DWMAC_PTP_CTRL_REG);

	/* accumulator overflows per second, then the addend producing that rate */
	temp = DWMAC_PTP_SUBSEC_PER_SEC / ss_incr;

	temp = (uint64_t)(temp << 32);

	addend_val = temp / ptp_clk_rate;

	data->default_addend = addend_val;

	sys_write32(addend_val, base + DWMAC_PTP_ADDEND_REG);
	sys_write32(sys_read32(base + DWMAC_PTP_CTRL_REG) | DWMAC_PTP_CTRL_ADDEND_UPDATE,
		    base + DWMAC_PTP_CTRL_REG);
	dwmac_ptp_wait_for_clear(base, DWMAC_PTP_CTRL_REG, DWMAC_PTP_CTRL_ADDEND_UPDATE);

	sys_write32(sys_read32(base + DWMAC_PTP_CTRL_REG) | DWMAC_PTP_CTRL_FINE_UPDATE,
		    base + DWMAC_PTP_CTRL_REG);
	if (IS_ENABLED(CONFIG_PTP_CLOCK_DWC_MAC_DIGITAL_ROLLOVER)) {
		sys_write32(sys_read32(base + DWMAC_PTP_CTRL_REG) | DWMAC_PTP_CTRL_ROLLOVER,
			    base + DWMAC_PTP_CTRL_REG);
	}

	sys_write32(0, base + DWMAC_PTP_SEC_UPDATE_REG);
	sys_write32(0, base + DWMAC_PTP_NSEC_UPDATE_REG);
	sys_write32(sys_read32(base + DWMAC_PTP_CTRL_REG) | DWMAC_PTP_CTRL_TIME_INIT,
		    base + DWMAC_PTP_CTRL_REG);
	dwmac_ptp_wait_for_clear(base, DWMAC_PTP_CTRL_REG, DWMAC_PTP_CTRL_TIME_INIT);

	uint32_t ctrl = sys_read32(base + DWMAC_PTP_CTRL_REG);

	if (IS_ENABLED(CONFIG_PTP_CLOCK_DWC_MAC_RX_TIMESTAMP_ALL)) {
		ctrl |= DWMAC_PTP_CTRL_ALL_RX;
	} else {
		/* all event messages, for both E2E and P2P delay mechanisms */
		uint32_t snaptypsel = DWMAC_PTP_CTRL_SNAPTYPSEL_EVENT;

#ifdef CONFIG_ETH_DWC_ETHER_1000_CORE
		/* older cores select a clock node type here instead */
		if (FIELD_GET(DWMAC_MACVERR_SNPSVER, sys_read32(base + DWMAC_MACVERR)) <
		    DWMAC_CORE_3_70) {
			snaptypsel = DWMAC_PTP_CTRL_SNAPTYPSEL_P2P_TC;
		}
#endif
		ctrl |= FIELD_PREP(DWMAC_PTP_CTRL_SNAPTYPSEL, snaptypsel);
	}
	if (IS_ENABLED(CONFIG_PTP_CLOCK_DWC_MAC_RX_TIMESTAMP_PTPV2)) {
		ctrl |= DWMAC_PTP_CTRL_PTPV2;
	}
	if (IS_ENABLED(CONFIG_PTP_CLOCK_DWC_MAC_RX_TIMESTAMP_L2)) {
		ctrl |= DWMAC_PTP_CTRL_L2;
	}
	if (IS_ENABLED(CONFIG_PTP_CLOCK_DWC_MAC_RX_TIMESTAMP_IPV4)) {
		ctrl |= DWMAC_PTP_CTRL_IPV4;
	}
	if (IS_ENABLED(CONFIG_PTP_CLOCK_DWC_MAC_RX_TIMESTAMP_IPV6)) {
		ctrl |= DWMAC_PTP_CTRL_IPV6;
	}

	sys_write32(ctrl, base + DWMAC_PTP_CTRL_REG);

	return 0;
}

static DEVICE_API(ptp_clock, dwmac_ptp_api) = {
	.set = dwmac_ptp_set,
	.get = dwmac_ptp_get,
	.adjust = dwmac_ptp_adjust,
	.adjust_rate = dwmac_ptp_adjust_rate,
};

const struct device *dwmac_get_ptp_clock(const struct device *dev, struct net_if *iface __unused)
{
	const struct dwmac_config *config = dev->config;

	return config->ptp_clock;
}

#if DWMAC_PTP_PINCTRL_ENABLED
#define DWMAC_PTP_PINCTRL_DEFINE(n)                                                                \
	IF_ENABLED(DT_INST_PINCTRL_HAS_NAME(n, default), (PINCTRL_DT_INST_DEFINE(n);))

#define DWMAC_PTP_PINCTRL_CONFIG(n)                                                                \
	IF_ENABLED(DT_INST_PINCTRL_HAS_NAME(n, default),                                           \
		   (.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),))
#else
#define DWMAC_PTP_PINCTRL_DEFINE(n)
#define DWMAC_PTP_PINCTRL_CONFIG(n)
#endif

#define PTP_CLOCK_DWMAC_INIT(n)                                                                    \
	DWMAC_PTP_PINCTRL_DEFINE(n)                                                                \
                                                                                                   \
	static const struct dwmac_ptp_config dwmac_ptp_config_##n = {                              \
		.eth_dev = DEVICE_DT_GET(DT_INST_PARENT(n)),                                       \
		DWMAC_PTP_PINCTRL_CONFIG(n)                                                        \
	};                                                                                         \
                                                                                                   \
	static struct dwmac_ptp_data dwmac_ptp_data_##n;                                           \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, dwmac_ptp_init, NULL, &dwmac_ptp_data_##n,                        \
			      &dwmac_ptp_config_##n, POST_KERNEL,                                  \
			      CONFIG_PTP_CLOCK_INIT_PRIORITY, &dwmac_ptp_api);

DT_INST_FOREACH_STATUS_OKAY(PTP_CLOCK_DWMAC_INIT)
