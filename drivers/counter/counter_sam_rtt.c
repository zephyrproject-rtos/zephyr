/*
 * Copyright (c) 2023 Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmel_sam_rtt

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/sys/util.h>

#include <string.h>
#include <soc.h>

typedef void (*rtt_sam_irq_init_fn_ptr)(void);

enum rtt_sam_source {
	RTT_SAM_SOURCE_SCLK = 0,
	RTT_SAM_SOURCE_RTC
};

struct rtt_sam_config {
	const struct counter_config_info info;
	Rtt *regs;
	uint16_t irq_num;
	rtt_sam_irq_init_fn_ptr irq_init_fn_ptr;
	uint32_t prescaler;
	enum rtt_sam_source source;
};

struct rtt_sam_data {
	counter_alarm_callback_t callback;
	void *callback_user_data;
	uint32_t guard_period;
	struct k_spinlock lock;
};

static inline void rtt_sam_set_mode_default(Rtt *regs)
{
	regs->RTT_MR = 0x00008000;
}

static inline void rtt_sam_set_prescaler(Rtt *regs, uint32_t prescaler)
{
	/* Prescaler has a reserved value 0 which sets the prescaler to 0x10000 */
	if (prescaler == 0x10000) {
		prescaler = 0;
	}

	regs->RTT_MR &= ~RTT_MR_RTPRES_Msk;
	regs->RTT_MR |= RTT_MR_RTPRES(prescaler);
}

static inline void rtt_sam_restart_timer(Rtt *regs)
{
	regs->RTT_MR |= RTT_MR_RTTRST;
}

static inline void rtt_sam_set_source(Rtt *regs, enum rtt_sam_source source)
{
	if (source == RTT_SAM_SOURCE_SCLK) {
		regs->RTT_MR &= ~RTT_MR_RTC1HZ;
	} else {
		regs->RTT_MR |= RTT_MR_RTC1HZ;
	}
}

static inline void rtt_sam_enable_timer(Rtt *regs)
{
	regs->RTT_MR &= ~RTT_MR_RTTDIS;
}

static inline void rtt_sam_disable_timer(Rtt *regs)
{
	regs->RTT_MR |= RTT_MR_RTTDIS;
}

static inline bool rtt_sam_alarm_irq_is_enabled(const Rtt *regs)
{
	return (regs->RTT_MR & RTT_MR_ALMIEN) > 0;
}

static inline void rtt_sam_alarm_enable_irq(Rtt *regs)
{
	regs->RTT_MR |= RTT_MR_ALMIEN;
}

static inline void rtt_sam_alarm_disable_irq(Rtt *regs)
{
	regs->RTT_MR &= ~RTT_MR_ALMIEN;
}

static inline uint32_t rtt_sam_alarm_get_status(const Rtt *regs)
{
	return regs->RTT_SR;
}

static inline void rtt_sam_alarm_set_ticks(Rtt *regs, uint32_t ticks)
{
	regs->RTT_AR = ticks;
}

static inline void rtt_sam_set_alarm_ticks_default(Rtt *regs)
{
	rtt_sam_alarm_set_ticks(regs, UINT32_MAX);
}

static inline uint32_t rtt_sam_get_ticks(Rtt *regs)
{
	uint32_t ticks;

	/* Register is updated async so read twice to ensure correct read */
	do {
		ticks = regs->RTT_VR;
	} while (ticks != regs->RTT_VR);

	return ticks;
}

static int rtt_sam_start(const struct device *dev)
{
	struct rtt_sam_data *data = dev->data;
	const struct rtt_sam_config *config = dev->config;
	Rtt *regs = config->regs;

	K_SPINLOCK(&data->lock) {
		rtt_sam_enable_timer(regs);
	}

	return 0;
}

static int rtt_sam_stop(const struct device *dev)
{
	struct rtt_sam_data *data = dev->data;
	const struct rtt_sam_config *config = dev->config;
	Rtt *regs = config->regs;

	K_SPINLOCK(&data->lock) {
		rtt_sam_disable_timer(regs);
	}

	return 0;
}

static int rtt_sam_get_value(const struct device *dev, uint32_t *ticks)
{
	const struct rtt_sam_config *config = dev->config;
	Rtt *regs = config->regs;

	*ticks = rtt_sam_get_ticks(regs);
	return 0;
}

static int rtt_sam_set_alarm(const struct device *dev, uint8_t chan_id,
			     const struct counter_alarm_cfg *alarm_cfg)
{
	struct rtt_sam_data *data = dev->data;
	const struct rtt_sam_config *config = dev->config;
	Rtt *regs = config->regs;
	uint32_t alarm_ticks;
	int ret = -EINVAL;

	if (!alarm_cfg->callback) {
		return ret;
	}

	K_SPINLOCK(&data->lock) {
		if (rtt_sam_alarm_irq_is_enabled(regs)) {
			/* Alarm already set */
			ret = -EBUSY;
			K_SPINLOCK_BREAK;
		}

		if (alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) {
			alarm_ticks = alarm_cfg->ticks;
		} else {
			alarm_ticks = alarm_cfg->ticks + rtt_sam_get_ticks(regs);
		}

		rtt_sam_alarm_set_ticks(regs, alarm_ticks);

		/* Clear alarm pending status */
		(void)rtt_sam_alarm_get_status(regs);

		/* Check if alarm was set too late */
		if (alarm_ticks <= rtt_sam_get_ticks(regs)) {
			ret = -ETIME;

			if (alarm_cfg->flags & COUNTER_ALARM_CFG_EXPIRE_WHEN_LATE) {
				/* Call back immediately */
				alarm_cfg->callback(dev, 0, rtt_sam_get_ticks(regs),
						    alarm_cfg->user_data);
			}

			K_SPINLOCK_BREAK;
		}

		/* Store callback */
		data->callback = alarm_cfg->callback;
		data->callback_user_data = alarm_cfg->user_data;

		/* Enable alarm interrupt */
		rtt_sam_alarm_enable_irq(regs);
		ret = 0;
	}

	return ret;
}

static int rtt_sam_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	struct rtt_sam_data *data = dev->data;
	const struct rtt_sam_config *config = dev->config;
	Rtt *regs = config->regs;
	int ret = -ENOTSUP;

	K_SPINLOCK(&data->lock) {
		if (rtt_sam_alarm_irq_is_enabled(regs)) {
			rtt_sam_alarm_disable_irq(regs);
			ret = 0;
		}
	}

	return ret;
}

static int rtt_sam_set_top_value(const struct device *dev,
				 const struct counter_top_cfg *cfg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cfg);
	return -ENOTSUP;
}

static uint32_t rtt_sam_get_pending_int(const struct device *dev)
{
	const struct rtt_sam_config *config = dev->config;
	Rtt *regs = config->regs;

	return (regs->RTT_SR & RTT_SR_ALMS) ? 1 : 0;
}

static uint32_t rtt_sam_get_top_value(const struct device *dev)
{
	return UINT32_MAX;
}

static uint32_t rtt_sam_get_freq(const struct device *dev)
{
	const struct rtt_sam_config *config = dev->config;

	if (config->source == RTT_SAM_SOURCE_RTC) {
		return 1;
	}

	return 32768U / config->prescaler;
}

static struct counter_driver_api rtt_sam_driver_api = {
	.start = rtt_sam_start,
	.stop = rtt_sam_stop,
	.get_value = rtt_sam_get_value,
	.set_alarm = rtt_sam_set_alarm,
	.cancel_alarm = rtt_sam_cancel_alarm,
	.set_top_value = rtt_sam_set_top_value,
	.get_pending_int = rtt_sam_get_pending_int,
	.get_top_value = rtt_sam_get_top_value,
	.get_freq = rtt_sam_get_freq,
};

static void rtt_sam_irq_handler(const struct device *dev)
{
	struct rtt_sam_data *data = dev->data;
	const struct rtt_sam_config *config = dev->config;
	Rtt *regs = config->regs;

	rtt_sam_alarm_disable_irq(regs);

	if (data->callback) {
		data->callback(dev, 0, rtt_sam_get_ticks(regs), data->callback_user_data);
	}
}

static int rtt_sam_init(const struct device *dev)
{
	const struct rtt_sam_config *config = dev->config;
	Rtt *regs = config->regs;
	enum rtt_sam_source source = config->source;
	uint32_t prescaler = config->prescaler;

	soc_sysc_disable_write_protection();
	rtt_sam_set_alarm_ticks_default(regs);
	rtt_sam_set_mode_default(regs);
	rtt_sam_set_source(regs, source);
	rtt_sam_set_prescaler(regs, prescaler);
	rtt_sam_restart_timer(regs);

	config->irq_init_fn_ptr();
	irq_enable(config->irq_num);
	return 0;
}

#define SAM_RTT_DEVICE(inst)						\
	BUILD_ASSERT(							\
		DT_INST_PROP_OR(inst, prescaler, 3) > 2,		\
		"Prescaler must be higher than 2"			\
	);								\
									\
	BUILD_ASSERT(							\
		DT_INST_PROP_OR(inst, prescaler, 3) < 65537,		\
		"Prescaler must be lower than 65537"			\
	);								\
									\
	static void rtt_sam_irq_init_##inst(void)			\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(inst),				\
			    DT_INST_IRQ(inst, priority),		\
			    rtt_sam_irq_handler,			\
			    DEVICE_DT_INST_GET(inst), 0);		\
	}								\
									\
	static const struct rtt_sam_config rtt_sam_config_##inst = {	\
		.info = {						\
			.max_top_value = UINT32_MAX,			\
			.flags = COUNTER_CONFIG_INFO_COUNT_UP,		\
			.channels = 1,					\
		},							\
		.regs = (Rtt *)DT_INST_REG_ADDR(inst),			\
		.irq_num = DT_INST_IRQN(inst),				\
		.irq_init_fn_ptr = rtt_sam_irq_init_##inst,		\
		.prescaler = DT_INST_PROP_OR(inst, prescaler, 3),	\
		.source = DT_INST_PROP(inst, source),			\
	};								\
									\
	static struct rtt_sam_data rtt_sam_data_##inst;			\
									\
	DEVICE_DT_INST_DEFINE(inst, rtt_sam_init, NULL,			\
			      &rtt_sam_data_##inst,			\
			      &rtt_sam_config_##inst, POST_KERNEL,	\
			      CONFIG_COUNTER_INIT_PRIORITY,		\
			      &rtt_sam_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SAM_RTT_DEVICE);
