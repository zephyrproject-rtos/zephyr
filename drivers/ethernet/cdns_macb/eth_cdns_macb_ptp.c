/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * PTP clock of the Cadence GEM: the 1588 time stamp unit (TSU), a child
 * device of the MAC. The MAC driver enables the descriptor timestamps and
 * turns them into packet timestamps; this device runs the clock itself.
 */

#define DT_DRV_COMPAT cdns_macb_ptp_clock

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(cdns_macb_ptp, CONFIG_ETHERNET_LOG_LEVEL);

#include <zephyr/device.h>
#include <zephyr/drivers/ptp_clock.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include "eth_cdns_macb_priv.h"

/*
 * The TSU advances by an increment of 8 integer and 24 fractional nanosecond
 * bits per clock, kept here as one 32-bit word.
 */
#define TSU_INCR_SUBNS_BITS  24
#define TSU_INCR_NS_MAX      FIELD_GET(GEM_TI_NSINCR, GEM_TI_NSINCR)
#define TSU_SEC_MAX          BIT64(48)

struct cdns_macb_ptp_config {
	const struct device *mac_dev;
	uint32_t tsu_clock_frequency;
};

struct cdns_macb_ptp_data {
	/* The increment for the nominal TSU clock rate */
	uint32_t incr_word;
};

static void cdns_macb_ptp_set_incr(mm_reg_t base, uint32_t incr_word)
{
	uint32_t sub_ns = incr_word & BIT_MASK(TSU_INCR_SUBNS_BITS);
	uint32_t ns = incr_word >> TSU_INCR_SUBNS_BITS;

	/* The sub-ns register takes effect with the write of the ns register */
	sys_write32(FIELD_PREP(GEM_TISUBN_SUBNSINCRL, sub_ns & 0xffU) |
			    FIELD_PREP(GEM_TISUBN_SUBNSINCRH, sub_ns >> 8),
		    base + GEM_TISUBN);
	sys_write32(FIELD_PREP(GEM_TI_NSINCR, ns), base + GEM_TI);
}

/* Write the TSU time; the caller holds the driver lock */
static void cdns_macb_ptp_tsu_write(mm_reg_t base, const struct net_ptp_time *tm)
{
	/* The seconds registers do not latch: clear the nanoseconds first */
	sys_write32(0U, base + GEM_TN);
	sys_write32(FIELD_PREP(GEM_TSH_SEC, tm->second >> 32), base + GEM_TSH);
	sys_write32((uint32_t)tm->second, base + GEM_TSL);
	sys_write32(tm->nanosecond, base + GEM_TN);
}

static int cdns_macb_ptp_set(const struct device *dev, struct net_ptp_time *tm)
{
	const struct cdns_macb_ptp_config *cfg = dev->config;
	struct cdns_macb_priv *p = cfg->mac_dev->data;
	mm_reg_t base = cdns_macb_reg_base(cfg->mac_dev);

	if ((tm->second >= TSU_SEC_MAX) || (tm->nanosecond >= NSEC_PER_SEC)) {
		return -EINVAL;
	}

	K_SPINLOCK(&p->lock) {
		cdns_macb_ptp_tsu_write(base, tm);
	}

	return 0;
}

static int cdns_macb_ptp_get(const struct device *dev, struct net_ptp_time *tm)
{
	const struct cdns_macb_ptp_config *cfg = dev->config;
	struct cdns_macb_priv *p = cfg->mac_dev->data;
	mm_reg_t base = cdns_macb_reg_base(cfg->mac_dev);

	K_SPINLOCK(&p->lock) {
		cdns_macb_tsu_read(base, tm);
	}

	return 0;
}

static int cdns_macb_ptp_adjust(const struct device *dev, int increment)
{
	const struct cdns_macb_ptp_config *cfg = dev->config;
	struct cdns_macb_priv *p = cfg->mac_dev->data;
	mm_reg_t base = cdns_macb_reg_base(cfg->mac_dev);
	uint32_t magnitude = (increment < 0) ? (uint32_t)-increment : (uint32_t)increment;

	/* The adjust register takes up to a second, the TSU applies it on the fly */
	if (magnitude > FIELD_GET(GEM_TA_ADJ, GEM_TA_ADJ)) {
		return -EINVAL;
	}

	K_SPINLOCK(&p->lock) {
		sys_write32(FIELD_PREP(GEM_TA_ADJ, magnitude) | ((increment < 0) ? GEM_TA_SUB : 0U),
			    base + GEM_TA);
	}

	return 0;
}

static int cdns_macb_ptp_rate_adjust(const struct device *dev, double ratio)
{
	const struct cdns_macb_ptp_config *cfg = dev->config;
	struct cdns_macb_ptp_data *data = dev->data;
	struct cdns_macb_priv *p = cfg->mac_dev->data;
	mm_reg_t base = cdns_macb_reg_base(cfg->mac_dev);
	double incr;

	if ((ratio <= 0.0) || (ratio > 2.0)) {
		return -EINVAL;
	}

	incr = (double)data->incr_word * ratio;
	if (incr >= (double)((TSU_INCR_NS_MAX + 1U) << TSU_INCR_SUBNS_BITS)) {
		return -EINVAL;
	}

	K_SPINLOCK(&p->lock) {
		cdns_macb_ptp_set_incr(base, (uint32_t)incr);
	}

	return 0;
}

static int cdns_macb_ptp_init(const struct device *dev)
{
	const struct cdns_macb_ptp_config *cfg = dev->config;
	struct cdns_macb_ptp_data *data = dev->data;
	struct cdns_macb_priv *p = cfg->mac_dev->data;
	const struct net_ptp_time epoch = {.second = 0U, .nanosecond = 0U};
	mm_reg_t base;
	uint64_t incr;

	/* The MAC maps the registers */
	if (!device_is_ready(cfg->mac_dev)) {
		LOG_ERR("%s: parent MAC device not ready", dev->name);
		return -ENODEV;
	}
	base = cdns_macb_reg_base(cfg->mac_dev);

	/* Nanoseconds per TSU clock in 8.24 fixed point */
	incr = ((uint64_t)NSEC_PER_SEC << TSU_INCR_SUBNS_BITS) / cfg->tsu_clock_frequency;
	if (incr >= ((uint64_t)(TSU_INCR_NS_MAX + 1U) << TSU_INCR_SUBNS_BITS)) {
		LOG_ERR("%s: TSU clock of %u Hz is too slow", dev->name, cfg->tsu_clock_frequency);
		return -EINVAL;
	}
	data->incr_word = (uint32_t)incr;

	K_SPINLOCK(&p->lock) {
		cdns_macb_ptp_tsu_write(base, &epoch);
		cdns_macb_ptp_set_incr(base, data->incr_word);
		sys_write32(0U, base + GEM_TA);
	}

	return 0;
}

static DEVICE_API(ptp_clock, cdns_macb_ptp_api) = {
	.set = cdns_macb_ptp_set,
	.get = cdns_macb_ptp_get,
	.adjust = cdns_macb_ptp_adjust,
	.rate_adjust = cdns_macb_ptp_rate_adjust,
};

#define CDNS_MACB_PTP_DEVICE(n)                                                                    \
	static const struct cdns_macb_ptp_config cdns_macb_ptp_config_##n = {                      \
		.mac_dev = DEVICE_DT_GET(DT_INST_PARENT(n)),                                       \
		.tsu_clock_frequency = DT_INST_PROP(n, clock_frequency),                           \
	};                                                                                         \
                                                                                                   \
	static struct cdns_macb_ptp_data cdns_macb_ptp_data_##n;                                   \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, cdns_macb_ptp_init, NULL, &cdns_macb_ptp_data_##n,                \
			      &cdns_macb_ptp_config_##n, POST_KERNEL,                              \
			      CONFIG_PTP_CLOCK_INIT_PRIORITY, &cdns_macb_ptp_api);

DT_INST_FOREACH_STATUS_OKAY(CDNS_MACB_PTP_DEVICE)
