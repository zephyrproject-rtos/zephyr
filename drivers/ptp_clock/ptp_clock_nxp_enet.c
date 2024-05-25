/*
 * Copyright 2023 NXP
 *
 * Based on a commit to drivers/ethernet/eth_mcux.c which was:
 * Copyright (c) 2018 Intel Coporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_enet_ptp_clock

#include <zephyr/drivers/ptp_clock.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/ethernet/eth_nxp_enet.h>

#include <fsl_enet.h>

struct ptp_clock_nxp_enet_config {
	ENET_Type *base;
	const struct pinctrl_dev_config *pincfg;
	const struct device *port;
	const struct device *clock_dev;
	struct device *clock_subsys;
	void (*irq_config_func)(void);
};

struct ptp_clock_nxp_enet_data {
	double clock_ratio;
	enet_handle_t enet_handle;
	struct k_mutex ptp_mutex;
};

static int ptp_clock_nxp_enet_set(const struct device *dev,
				struct net_ptp_time *tm)
{
	const struct ptp_clock_nxp_enet_config *config = dev->config;
	struct ptp_clock_nxp_enet_data *data = dev->data;
	enet_ptp_time_t enet_time;

	enet_time.second = tm->second;
	enet_time.nanosecond = tm->nanosecond;

	ENET_Ptp1588SetTimer(config->base, &data->enet_handle, &enet_time);

	return 0;
}

static int ptp_clock_nxp_enet_get(const struct device *dev,
				struct net_ptp_time *tm)
{
	const struct ptp_clock_nxp_enet_config *config = dev->config;
	struct ptp_clock_nxp_enet_data *data = dev->data;
	enet_ptp_time_t enet_time;

	ENET_Ptp1588GetTimer(config->base, &data->enet_handle, &enet_time);

	tm->second = enet_time.second;
	tm->nanosecond = enet_time.nanosecond;

	return 0;
}

static int ptp_clock_nxp_enet_adjust(const struct device *dev,
					int increment)
{
	const struct ptp_clock_nxp_enet_config *config = dev->config;
	int ret = 0;
	int key;

	if ((increment <= (int32_t)(-NSEC_PER_SEC)) ||
			(increment >= (int32_t)NSEC_PER_SEC)) {
		ret = -EINVAL;
	} else {
		key = irq_lock();
		if (config->base->ATPER != NSEC_PER_SEC) {
			ret = -EBUSY;
		} else {
			/* Seconds counter is handled by software. Change the
			 * period of one software second to adjust the clock.
			 */
			config->base->ATPER = NSEC_PER_SEC - increment;
			ret = 0;
		}
		irq_unlock(key);
	}

	return ret;

}

static int ptp_clock_nxp_enet_rate_adjust(const struct device *dev,
					double ratio)
{
	const struct ptp_clock_nxp_enet_config *config = dev->config;
	struct ptp_clock_nxp_enet_data *data = dev->data;
	int corr;
	int32_t mul;
	double val;
	uint32_t enet_ref_pll_rate;

	(void) clock_control_get_rate(config->clock_dev, config->clock_subsys,
				&enet_ref_pll_rate);
	int hw_inc = NSEC_PER_SEC / enet_ref_pll_rate;

	/* No change needed. */
	if ((ratio > 1.0 && ratio - 1.0 < 0.00000001) ||
	   (ratio < 1.0 && 1.0 - ratio < 0.00000001)) {
		return 0;
	}

	ratio *= data->clock_ratio;

	/* Limit possible ratio. */
	if ((ratio > 1.0 + 1.0/(2 * hw_inc)) ||
			(ratio < 1.0 - 1.0/(2 * hw_inc))) {
		return -EINVAL;
	}

	/* Save new ratio. */
	data->clock_ratio = ratio;

	if (ratio < 1.0) {
		corr = hw_inc - 1;
		val = 1.0 / (hw_inc * (1.0 - ratio));
	} else if (ratio > 1.0) {
		corr = hw_inc + 1;
		val = 1.0 / (hw_inc * (ratio - 1.0));
	} else {
		val = 0;
		corr = hw_inc;
	}

	if (val >= INT32_MAX) {
		/* Value is too high.
		 * It is not possible to adjust the rate of the clock.
		 */
		mul = 0;
	} else {
		mul = val;
	}

	k_mutex_lock(&data->ptp_mutex, K_FOREVER);

	ENET_Ptp1588AdjustTimer(config->base, corr, mul);

	k_mutex_unlock(&data->ptp_mutex);

	return 0;
}

void nxp_enet_ptp_clock_callback(const struct device *dev,
			enum nxp_enet_callback_reason event,
			void *cb_data)
{
	const struct ptp_clock_nxp_enet_config *config = dev->config;
	struct ptp_clock_nxp_enet_data *data = dev->data;

	if (event == NXP_ENET_MODULE_RESET) {
		enet_ptp_config_t ptp_config;
		uint32_t enet_ref_pll_rate;
		uint8_t ptp_multicast[6] = { 0x01, 0x1B, 0x19, 0x00, 0x00, 0x00 };
		uint8_t ptp_peer_multicast[6] = { 0x01, 0x80, 0xC2, 0x00, 0x00, 0x0E };

		(void) clock_control_get_rate(config->clock_dev, config->clock_subsys,
					&enet_ref_pll_rate);

		ENET_AddMulticastGroup(config->base, ptp_multicast);
		ENET_AddMulticastGroup(config->base, ptp_peer_multicast);

		/* only for ERRATA_2579 */
		ptp_config.channel = kENET_PtpTimerChannel3;
		ptp_config.ptp1588ClockSrc_Hz = enet_ref_pll_rate;
		data->clock_ratio = 1.0;

		ENET_Ptp1588SetChannelMode(config->base, kENET_PtpTimerChannel3,
				kENET_PtpChannelPulseHighonCompare, true);
		ENET_Ptp1588Configure(config->base, &data->enet_handle,
				      &ptp_config);
	}

	if (cb_data != NULL) {
		/* Share the mutex with mac driver */
		*(uintptr_t *)cb_data = (uintptr_t)&data->ptp_mutex;
	}
}

static int ptp_clock_nxp_enet_init(const struct device *port)
{
	const struct ptp_clock_nxp_enet_config *config = port->config;
	struct ptp_clock_nxp_enet_data *data = port->data;
	int ret;

	ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret) {
		return ret;
	}

	k_mutex_init(&data->ptp_mutex);

	config->irq_config_func();

	return 0;
}

static void ptp_clock_nxp_enet_isr(const struct device *dev)
{
	const struct ptp_clock_nxp_enet_config *config = dev->config;
	struct ptp_clock_nxp_enet_data *data = dev->data;
	enet_ptp_timer_channel_t channel;

	unsigned int irq_lock_key = irq_lock();

	/* clear channel */
	for (channel = kENET_PtpTimerChannel1; channel <= kENET_PtpTimerChannel4; channel++) {
		if (ENET_Ptp1588GetChannelStatus(config->base, channel)) {
			ENET_Ptp1588ClearChannelStatus(config->base, channel);
		}
	}

	ENET_TimeStampIRQHandler(config->base, &data->enet_handle);

	irq_unlock(irq_lock_key);
}

static const struct ptp_clock_driver_api ptp_clock_nxp_enet_api = {
	.set = ptp_clock_nxp_enet_set,
	.get = ptp_clock_nxp_enet_get,
	.adjust = ptp_clock_nxp_enet_adjust,
	.rate_adjust = ptp_clock_nxp_enet_rate_adjust,
};

#define PTP_CLOCK_NXP_ENET_INIT(n)						\
	static void nxp_enet_ptp_clock_##n##_irq_config_func(void)		\
	{									\
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, 0, irq),			\
				DT_INST_IRQ_BY_IDX(n, 0, priority),		\
				ptp_clock_nxp_enet_isr,				\
				DEVICE_DT_INST_GET(n),				\
				0);						\
		irq_enable(DT_INST_IRQ_BY_IDX(n, 0, irq));			\
	}									\
										\
	PINCTRL_DT_INST_DEFINE(n);						\
										\
	static const struct ptp_clock_nxp_enet_config				\
		ptp_clock_nxp_enet_##n##_config = {				\
			.base = (ENET_Type *) DT_REG_ADDR(DT_INST_PARENT(n)),	\
			.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),		\
			.port = DEVICE_DT_INST_GET(n),				\
			.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),	\
			.clock_subsys = (void *)				\
					DT_INST_CLOCKS_CELL_BY_IDX(n, 0, name),	\
			.irq_config_func =					\
				nxp_enet_ptp_clock_##n##_irq_config_func,	\
		};								\
										\
	static struct ptp_clock_nxp_enet_data ptp_clock_nxp_enet_##n##_data;	\
										\
	DEVICE_DT_INST_DEFINE(n, &ptp_clock_nxp_enet_init, NULL,		\
				&ptp_clock_nxp_enet_##n##_data,			\
				&ptp_clock_nxp_enet_##n##_config,		\
				POST_KERNEL, CONFIG_PTP_CLOCK_INIT_PRIORITY,	\
				&ptp_clock_nxp_enet_api);

DT_INST_FOREACH_STATUS_OKAY(PTP_CLOCK_NXP_ENET_INIT)
