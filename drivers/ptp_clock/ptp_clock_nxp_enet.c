/*
 * Copyright 2023-2024 NXP
 *
 * Based on a commit to drivers/ethernet/eth_mcux.c which was:
 * Copyright (c) 2018 Intel Corporation
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
#include <zephyr/logging/log.h>

#include <fsl_enet.h>

#include "ptp_clock_nxp_enet_rate_math.h"

LOG_MODULE_REGISTER(ptp_clock_nxp_enet);

struct ptp_clock_nxp_enet_config {
	const struct pinctrl_dev_config *pincfg;
	const struct device *module_dev;
	const struct device *port;
	const struct device *clock_dev;
	struct device *clock_subsys;
	void (*irq_config_func)(void);
};

struct ptp_clock_nxp_enet_data {
	ENET_Type *base;
	enet_handle_t *enet_handle;
	struct k_mutex ptp_mutex;
	/* The timer is only started when its clock rate is usable */
	bool timer_running;
};

/* ATINC.INC holds the tick in whole nanoseconds, in 7 bits */
static bool ptp_clock_nxp_enet_rate_usable(uint32_t rate)
{
	uint32_t inc;

	if (rate == 0U) {
		return false;
	}

	inc = NSEC_PER_SEC / rate;

	return (inc >= 1U) && (inc <= (ENET_ATINC_INC_MASK >> ENET_ATINC_INC_SHIFT));
}

static int ptp_clock_nxp_enet_set(const struct device *dev,
				struct net_ptp_time *tm)
{
	struct ptp_clock_nxp_enet_data *data = dev->data;
	enet_ptp_time_t enet_time;

	if (!data->timer_running) {
		return -ENODEV;
	}

	enet_time.second = tm->second;
	enet_time.nanosecond = tm->nanosecond;

	ENET_Ptp1588SetTimer(data->base, data->enet_handle, &enet_time);

	return 0;
}

static int ptp_clock_nxp_enet_get(const struct device *dev,
				struct net_ptp_time *tm)
{
	struct ptp_clock_nxp_enet_data *data = dev->data;
	enet_ptp_time_t enet_time;

	if (!data->timer_running) {
		tm->second = 0;
		tm->nanosecond = 0;
		return -ENODEV;
	}

	ENET_Ptp1588GetTimer(data->base, data->enet_handle, &enet_time);

	tm->second = enet_time.second;
	tm->nanosecond = enet_time.nanosecond;

	return 0;
}

static int ptp_clock_nxp_enet_adjust(const struct device *dev,
					int increment)
{
	struct ptp_clock_nxp_enet_data *data = dev->data;
	int ret = 0;
	int key;

	if (!data->timer_running) {
		return -ENODEV;
	}

	if ((increment <= (int32_t)(-NSEC_PER_SEC)) ||
			(increment >= (int32_t)NSEC_PER_SEC)) {
		ret = -EINVAL;
	} else {
		key = irq_lock();
		if (data->base->ATPER != NSEC_PER_SEC) {
			ret = -EBUSY;
		} else {
			/* Seconds counter is handled by software. Change the
			 * period of one software second to adjust the clock.
			 */
			data->base->ATPER = NSEC_PER_SEC - increment;
			ret = 0;
		}
		irq_unlock(key);
	}

	return ret;

}

/*
 * ENET_Ptp1588StartTimer() sets ATINC.INC to the tick truncated to whole nanoseconds. Find the
 * ATINC.INC_CORR and ATCOR values that make the average tick the one of the clock rate times
 * the ratio, which also absorbs the fractional part of the tick.
 */
static int ptp_clock_nxp_enet_correction(uint32_t clock_rate, double ratio, int *inc_corr,
					 uint32_t *cor)
{
	int hw_inc = NSEC_PER_SEC / clock_rate;
	double target_ns = (double)NSEC_PER_SEC / (double)clock_rate * ratio;

	return ptp_clock_nxp_enet_find_correction(
		hw_inc, target_ns, ENET_ATINC_INC_CORR_MASK >> ENET_ATINC_INC_CORR_SHIFT,
		ENET_ATCOR_COR_MASK, inc_corr, cor);
}

static int ptp_clock_nxp_enet_rate_adjust(const struct device *dev,
					double ratio)
{
	const struct ptp_clock_nxp_enet_config *config = dev->config;
	struct ptp_clock_nxp_enet_data *data = dev->data;
	uint32_t enet_ref_pll_rate;
	uint32_t cor;
	int inc_corr;
	int ret;

	if (!data->timer_running) {
		return -ENODEV;
	}

	/* Do not let a servo that went out of control reach the timer */
	if ((ratio > 1.0 + CONFIG_PTP_CLOCK_NXP_ENET_MAX_RATIO_PPM * 1.0e-6) ||
	    (ratio < 1.0 - CONFIG_PTP_CLOCK_NXP_ENET_MAX_RATIO_PPM * 1.0e-6)) {
		return -EINVAL;
	}

	(void)clock_control_get_rate(config->clock_dev, config->clock_subsys, &enet_ref_pll_rate);

	/* There is no early return for a ratio of 1.0, the fractional tick still needs a value */
	ret = ptp_clock_nxp_enet_correction(enet_ref_pll_rate, ratio, &inc_corr, &cor);
	if (ret != 0) {
		return ret;
	}

	k_mutex_lock(&data->ptp_mutex, K_FOREVER);

	ENET_Ptp1588AdjustTimer(data->base, inc_corr, cor);

	k_mutex_unlock(&data->ptp_mutex);

	return 0;
}

void nxp_enet_ptp_clock_callback(const struct device *dev,
			enum nxp_enet_callback_reason event,
			void *cb_data)
{
	const struct ptp_clock_nxp_enet_config *config = dev->config;
	struct ptp_clock_nxp_enet_data *data = dev->data;
	struct nxp_enet_ptp_data *ptp_data;

	__ASSERT(cb_data != NULL, "ptp data is NULL");

	ptp_data = (struct nxp_enet_ptp_data *)cb_data;

	if (event == NXP_ENET_MODULE_RESET) {
		enet_ptp_config_t ptp_config;
		uint32_t enet_ref_pll_rate;
		uint32_t cor;
		int inc_corr;
		uint8_t ptp_multicast[6] = { 0x01, 0x1B, 0x19, 0x00, 0x00, 0x00 };
		uint8_t ptp_peer_multicast[6] = { 0x01, 0x80, 0xC2, 0x00, 0x00, 0x0E };

		if (clock_control_get_rate(config->clock_dev, config->clock_subsys,
					   &enet_ref_pll_rate) < 0) {
			enet_ref_pll_rate = 0U;
		}

		ENET_AddMulticastGroup(data->base, ptp_multicast);
		ENET_AddMulticastGroup(data->base, ptp_peer_multicast);

		/* only for ERRATA_2579 */
		ptp_config.channel = kENET_PtpTimerChannel3;
		ptp_config.ptp1588ClockSrc_Hz = enet_ref_pll_rate;

		/* Share the mutex with mac driver */
		ptp_data->ptp_mutex = &data->ptp_mutex;
		/* Get enet handle from mac driver */
		data->enet_handle = ptp_data->enet;

		if (!ptp_clock_nxp_enet_rate_usable(enet_ref_pll_rate)) {
			LOG_ERR("1588 timer clock of %u Hz is not usable, timer not started",
				enet_ref_pll_rate);
			return;
		}

		ENET_Ptp1588SetChannelMode(data->base, kENET_PtpTimerChannel3,
				kENET_PtpChannelPulseHighonCompare, true);
		ENET_Ptp1588StartTimer(data->base, ptp_config.ptp1588ClockSrc_Hz);

		/* Make up for the fractional part of the tick that the start above cut off */
		if (ptp_clock_nxp_enet_correction(enet_ref_pll_rate, 1.0, &inc_corr, &cor) == 0) {
			ENET_Ptp1588AdjustTimer(data->base, inc_corr, cor);
		}

		data->timer_running = true;
		ENET_EnableInterrupts(data->base, ENET_TS_INTERRUPT);
	}
}

static int ptp_clock_nxp_enet_init(const struct device *port)
{
	const struct ptp_clock_nxp_enet_config *config = port->config;
	struct ptp_clock_nxp_enet_data *data = port->data;
	int ret;

	data->base = (ENET_Type *)DEVICE_MMIO_GET(config->module_dev);

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
	struct ptp_clock_nxp_enet_data *data = dev->data;
	enet_ptp_timer_channel_t channel;

	unsigned int irq_lock_key = irq_lock();

	/* clear channel */
	for (channel = kENET_PtpTimerChannel1; channel <= kENET_PtpTimerChannel4; channel++) {
		if (ENET_Ptp1588GetChannelStatus(data->base, channel)) {
			ENET_Ptp1588ClearChannelStatus(data->base, channel);
		}
	}

	ENET_TimeStampIRQHandler(data->base, data->enet_handle);

	irq_unlock(irq_lock_key);
}

static DEVICE_API(ptp_clock, ptp_clock_nxp_enet_api) = {
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
			.module_dev = DEVICE_DT_GET(DT_INST_PARENT(n)),		\
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
