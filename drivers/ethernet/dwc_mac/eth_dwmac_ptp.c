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

#include "eth_dwmac_priv.h"

LOG_MODULE_REGISTER(dwmac_ptp_clock, CONFIG_ETHERNET_LOG_LEVEL);

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
	const struct device *eth_dev = dev->config;
	struct dwmac_priv *p = eth_dev->data;
	mm_reg_t base = DEVICE_MMIO_GET(eth_dev);

	K_SPINLOCK(&p->spinlock) {
		sys_write32(tm->second, base + DWMAC_PTP_SEC_UPDATE_REG);
		sys_write32(tm->nanosecond, base + DWMAC_PTP_NSEC_UPDATE_REG);
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
	const struct device *eth_dev = dev->config;
	struct dwmac_priv *p = eth_dev->data;
	mm_reg_t base = DEVICE_MMIO_GET(eth_dev);
	uint32_t second_2;

	K_SPINLOCK(&p->spinlock) {
		tm->second = sys_read32(base + DWMAC_PTP_SEC_REG);
		tm->nanosecond = sys_read32(base + DWMAC_PTP_NSEC_REG);
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
	const struct device *eth_dev = dev->config;
	struct dwmac_priv *p = eth_dev->data;
	mm_reg_t base = DEVICE_MMIO_GET(eth_dev);

	if ((increment <= (int32_t)(-NSEC_PER_SEC)) || (increment >= (int32_t)NSEC_PER_SEC)) {
		return -EINVAL;
	}

	K_SPINLOCK(&p->spinlock) {
		sys_write32(0, base + DWMAC_PTP_SEC_UPDATE_REG);
		if (increment >= 0) {
			sys_write32((uint32_t)increment, base + DWMAC_PTP_NSEC_UPDATE_REG);
		} else {
#if defined(CONFIG_ETH_DWC_ETHER_QOS_CORE)
			sys_write32(DWMAC_PTP_NSEC_UPDATE_ADDSUB | (NSEC_PER_SEC + increment),
				    base + DWMAC_PTP_NSEC_UPDATE_REG);
#else
			sys_write32(DWMAC_PTP_NSEC_UPDATE_ADDSUB | (-increment),
				    base + DWMAC_PTP_NSEC_UPDATE_REG);
#endif
		}
		sys_write32(sys_read32(base + DWMAC_PTP_CTRL_REG) | DWMAC_PTP_CTRL_TIME_UPDATE,
			    base + DWMAC_PTP_CTRL_REG);
		while (sys_read32(base + DWMAC_PTP_CTRL_REG) & DWMAC_PTP_CTRL_TIME_UPDATE) {
			/* spin lock */
		}
	}

	return 0;
}

static int dwmac_ptp_rate_adjust(const struct device *dev, double ratio)
{
	const struct device *eth_dev = dev->config;
	struct dwmac_priv *p = eth_dev->data;
	const struct dwmac_ptp_data *data = dev->data;
	mm_reg_t base = DEVICE_MMIO_GET(eth_dev);
	uint32_t addend_val;

	if (ratio <= 0.0 || ratio > 2.0) {
		return -EINVAL;
	}

	addend_val = (uint32_t)((double)data->default_addend * ratio);

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
	const struct device *eth_dev = dev->config;
	const struct dwmac_config *eth_cfg = eth_dev->config;
	struct dwmac_ptp_data *data = dev->data;
	mm_reg_t base = DEVICE_MMIO_GET(eth_dev);
	uint32_t ptp_clk_rate;
	uint32_t ss_incr_ns;
	uint32_t addend_val;
	uint64_t temp;
	int ret;

	ret = clock_control_get_rate(eth_cfg->clock, eth_cfg->ptp_clk, &ptp_clk_rate);
	if (ret < 0) {
		return -EIO;
	}

	ss_incr_ns = 2000000000ULL / ptp_clk_rate;

	sys_write32(ss_incr_ns << DWMAC_PTP_SSINC_SHIFT, base + DWMAC_PTP_SSINC_REG);

	sys_write32(sys_read32(base + DWMAC_PTP_CTRL_REG) | DWMAC_PTP_CTRL_ENABLE,
		    base + DWMAC_PTP_CTRL_REG);

	temp = 1000000000ULL / ss_incr_ns;

	temp = (uint64_t)(temp << 32);

	addend_val = temp / ptp_clk_rate;

	data->default_addend = addend_val;

	sys_write32(addend_val, base + DWMAC_PTP_ADDEND_REG);
	sys_write32(sys_read32(base + DWMAC_PTP_CTRL_REG) | DWMAC_PTP_CTRL_ADDEND_UPDATE,
		    base + DWMAC_PTP_CTRL_REG);
	dwmac_ptp_wait_for_clear(base, DWMAC_PTP_CTRL_REG, DWMAC_PTP_CTRL_ADDEND_UPDATE);

	sys_write32(sys_read32(base + DWMAC_PTP_CTRL_REG) | DWMAC_PTP_CTRL_FINE_UPDATE,
		    base + DWMAC_PTP_CTRL_REG);
	sys_write32(sys_read32(base + DWMAC_PTP_CTRL_REG) | DWMAC_PTP_CTRL_ROLLOVER,
		    base + DWMAC_PTP_CTRL_REG);

	sys_write32(0, base + DWMAC_PTP_SEC_UPDATE_REG);
	sys_write32(0, base + DWMAC_PTP_NSEC_UPDATE_REG);
	sys_write32(sys_read32(base + DWMAC_PTP_CTRL_REG) | DWMAC_PTP_CTRL_TIME_INIT,
		    base + DWMAC_PTP_CTRL_REG);
	dwmac_ptp_wait_for_clear(base, DWMAC_PTP_CTRL_REG, DWMAC_PTP_CTRL_TIME_INIT);

	uint32_t ctrl = sys_read32(base + DWMAC_PTP_CTRL_REG);

	if (IS_ENABLED(CONFIG_PTP)) {
		/* Use PTPv2 */
		ctrl |= BIT(10);
		/* enable timestamping for L2 PTP packets */
		if (IS_ENABLED(CONFIG_PTP_IEEE_802_3_PROTOCOL)) {
			ctrl |= BIT(11);
		}
		/* enable timestamping for IPv4 PTP packets */
		if (IS_ENABLED(CONFIG_PTP_UDP_IPV4_PROTOCOL)) {
			ctrl |= BIT(13);
		}
		/* enable timestamping for IPv6 PTP packets */
		if (IS_ENABLED(CONFIG_PTP_UDP_IPV6_PROTOCOL)) {
			ctrl |= BIT(12);
		}
	} else {
		/* Enable timestamping for all received packets */
		ctrl |= DWMAC_PTP_CTRL_ALL_RX;
	}

	sys_write32(ctrl, base + DWMAC_PTP_CTRL_REG);

	return 0;
}

static DEVICE_API(ptp_clock, dwmac_ptp_api) = {
	.set = dwmac_ptp_set,
	.get = dwmac_ptp_get,
	.adjust = dwmac_ptp_adjust,
	.rate_adjust = dwmac_ptp_rate_adjust,
};

const struct device *dwmac_get_ptp_clock(const struct device *dev, struct net_if *iface __unused)
{
	const struct dwmac_config *config = dev->config;

	return config->ptp_clock;
}

#define PTP_CLOCK_DWMAC_INIT(n)                                                                    \
	static struct dwmac_ptp_data dwmac_ptp_data_##n;                                           \
	DEVICE_DT_INST_DEFINE(n, dwmac_ptp_init, NULL, &dwmac_ptp_data_##n,                        \
			      DEVICE_DT_GET(DT_INST_PARENT(n)), POST_KERNEL,                       \
			      CONFIG_PTP_CLOCK_INIT_PRIORITY, &dwmac_ptp_api);

DT_INST_FOREACH_STATUS_OKAY(PTP_CLOCK_DWMAC_INIT)
