/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_netc_ptp_clock

#include <zephyr/drivers/ptp_clock.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>

#include <fsl_netc_timer.h>

struct ptp_clock_nxp_netc_config {
	const struct device *dev;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
};

struct ptp_clock_nxp_netc_data {
	NETC_ENETC_Type *base;
	netc_timer_handle_t handle;
	struct k_mutex ptp_mutex;
	netc_timer_config_t ptp_config;
};

static int ptp_clock_nxp_netc_set(const struct device *dev,
				struct net_ptp_time *tm)
{
	struct ptp_clock_nxp_netc_data *data = dev->data;
	uint64_t nanosecond;
	int key;

	nanosecond = tm->second * NSEC_PER_SEC + tm->nanosecond;

	key = irq_lock();
	data->handle.hw.base->TMR_CNT_L = (uint32_t)nanosecond;
	data->handle.hw.base->TMR_CNT_H = (uint32_t)(nanosecond >> 32U);
	data->handle.hw.base->TMROFF_L = 0U;
	data->handle.hw.base->TMROFF_H = 0U;
	irq_unlock(key);

	return 0;
}

static int ptp_clock_nxp_netc_get(const struct device *dev,
				struct net_ptp_time *tm)
{
	struct ptp_clock_nxp_netc_data *data = dev->data;
	uint64_t nanosecond;

	NETC_TimerGetCurrentTime(&data->handle, &nanosecond);

	tm->second = nanosecond / NSEC_PER_SEC;
	tm->nanosecond = nanosecond % NSEC_PER_SEC;

	return 0;
}

static int ptp_clock_nxp_netc_adjust(const struct device *dev,
					int increment)
{
	struct ptp_clock_nxp_netc_data *data = dev->data;
	int key;

	key = irq_lock();
	NETC_TimerAddOffset(&data->handle, (int64_t)increment * NSEC_PER_SEC);
	irq_unlock(key);

	return 0;

}

static int ptp_clock_nxp_netc_rate_adjust(const struct device *dev,
					double ratio)
{
	struct ptp_clock_nxp_netc_data *data = dev->data;
	netc_timer_config_t *ptp_config = &data->ptp_config;

	ptp_config->defaultPpb = (ratio - 1.0) * 1000000000LL;

	k_mutex_lock(&data->ptp_mutex, K_FOREVER);

	NETC_TimerAdjustFreq(&data->handle, ptp_config->defaultPpb);

	k_mutex_unlock(&data->ptp_mutex);

	return 0;
}

static int ptp_clock_nxp_netc_init(const struct device *dev)
{
	const struct ptp_clock_nxp_netc_config *config = dev->config;
	struct ptp_clock_nxp_netc_data *data = dev->data;
	netc_timer_config_t *ptp_config = &data->ptp_config;
	uint32_t netc_ref_pll_rate;
	int ret = 0;

	ret = clock_control_get_rate(config->clock_dev, config->clock_subsys, &netc_ref_pll_rate);
	if (ret) {
		return ret;
	}
	ptp_config->refClkHz = netc_ref_pll_rate/2;
	ptp_config->entryNum = 0U;
	ptp_config->defaultPpb = 0U;
	ptp_config->clockSelect = kNETC_TimerSystemClk;

	k_mutex_init(&data->ptp_mutex);

	ret = NETC_TimerInit(&data->handle, ptp_config);
	if (ret != kStatus_Success) {
		return ret;
	}

	NETC_TimerEnable(&data->handle, true);

	return ret;
}


static DEVICE_API(ptp_clock, ptp_clock_nxp_netc_api) = {
	.set = ptp_clock_nxp_netc_set,
	.get = ptp_clock_nxp_netc_get,
	.adjust = ptp_clock_nxp_netc_adjust,
	.rate_adjust = ptp_clock_nxp_netc_rate_adjust,
};

#define PTP_CLOCK_NXP_NETC_INIT(n)						\
	static const struct ptp_clock_nxp_netc_config				\
		ptp_clock_nxp_netc_##n##_config = {				\
			.dev = DEVICE_DT_INST_GET(n),				\
			.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),	\
			.clock_subsys = (clock_control_subsys_t)		\
					DT_INST_CLOCKS_CELL(n, name),           \
		};								\
										\
	static struct ptp_clock_nxp_netc_data ptp_clock_nxp_netc_##n##_data;	\
										\
	DEVICE_DT_INST_DEFINE(n, &ptp_clock_nxp_netc_init, NULL,		\
				&ptp_clock_nxp_netc_##n##_data,			\
				&ptp_clock_nxp_netc_##n##_config,		\
				POST_KERNEL, CONFIG_PTP_CLOCK_INIT_PRIORITY,	\
				&ptp_clock_nxp_netc_api);

/* Only one instance supported right now */
DT_INST_FOREACH_STATUS_OKAY(PTP_CLOCK_NXP_NETC_INIT)
