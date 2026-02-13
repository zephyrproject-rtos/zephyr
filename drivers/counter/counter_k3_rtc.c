/*
 * Copyright (c) 2026 Texas Instruments Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_k3_rtc_counter

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(counter_ti_k3_rtc, CONFIG_COUNTER_LOG_LEVEL);

#define RTC_MAX_TOP_VALUE		UINT32_MAX
#define RTC_MAX_TOP_VALUE_64		BIT_MASK(48)

#define K3_RTC_KICK0_UNLOCK		0x83e70b13
#define K3_RTC_KICK1_UNLOCK		0x95a4f1e0

#define RTC_CNT_FMODE_MASK		BIT(25)
#define RTC_UNLOCK_MASK			BIT(23)
#define RTC_O32K_OSC_MASK		BIT(21)
#define RTC_SYNCPEND_STATUS_MASK	BIT_MASK(2)

#define RTC_IRQ_ALARM1_MASK		BIT(0)
#define RTC_IRQ_ALARM2_MASK		BIT(1)
#define RTC_IRQ_STATUS_MASK		BIT_MASK(2)

#define DEV_CFG(dev)  ((const struct k3_rtc_counter_cfg *)(dev)->config)
#define DEV_DATA(dev) ((struct k3_rtc_counter_data *)(dev)->data)
#define DEV_REGS(dev) ((k3_rtc_regs_t *)DEVICE_MMIO_NAMED_GET(dev, reg_base))

typedef struct {
	volatile uint32_t MOD_VER;		/* Module ID and Version - 0h */
	volatile uint32_t SUB_S_CNT;		/* SubSecondCount - 4h*/
	volatile uint32_t S_CNT_LSW;		/* SecondCount_31_0 - 8h */
	volatile uint32_t S_CNT_MSW;		/* SecondCount_47_32 - Ch */
	volatile uint32_t COMP;			/* Compensation - 10h */
	uint8_t Resv_24[4];
	volatile uint32_t OFF_ON_S_CNT_LSW;	/* OffOnSCnt_31_0 - 18h */
	volatile uint32_t OFF_ON_S_CNT_MSW;	/* OffOnSCnt_47_32 - 1Ch */
	volatile uint32_t ON_OFF_S_CNT_LSW;	/* OnOffSCnt_31_0 - 20h */
	volatile uint32_t ON_OFF_S_CNT_MSW;	/* OnOffSCnt_47_32 - 24h */
	volatile uint32_t DEBOUNCE;		/* Debounce - 28h */
	volatile uint32_t ANALOG;		/* AnalogCfg - 2Ch */
	volatile uint32_t SCRATCH[8];		/* Scratch Storage X - 30h */
	volatile uint32_t GENRAL_CTL;		/* GeneralControl - 50h */
	volatile uint32_t IRQSTATUS_RAW_SYS;	/* Interrupt Raw Status Register - 54h */
	volatile uint32_t IRQSTATUS_SYS;	/* Interrupt Status Register - 58h */
	volatile uint32_t IRQENABLE_SET_SYS;	/* Interrupt Enable Set Register - 5Ch */
	volatile uint32_t IRQENABLE_CLR_SYS;	/* Interrupt Enable Clear Register - 60h */
	uint8_t Resv_104[4];			/* SyncPending - 68h */
	volatile uint32_t SYNCPEND;
	uint8_t Resv_112[4];
	volatile uint32_t KICK0;		/* Kick0 - 70h */
	volatile uint32_t KICK1;		/* Kick1 - 74h */
	uint8_t Resv_128[8];
} k3_rtc_regs_t;

struct k3_rtc_counter_cfg {
	struct counter_config_info counter_info;

	DEVICE_MMIO_NAMED_ROM(reg_base);

	void (*irq_config_func)(void);
	uint32_t osc_freq;
};

struct k3_rtc_counter_data {
	DEVICE_MMIO_NAMED_RAM(reg_base);
#ifdef CONFIG_COUNTER_64BITS_TICKS
	struct counter_alarm_cfg_64 alarm_cfg[2];
#else
	struct counter_alarm_cfg alarm_cfg[2];
#endif /* CONFIG_COUNTER_64BITS_TICKS */
	uint32_t sync_timeout_us;
};

static int k3_rtc_fence(const struct device *dev)
{
	k3_rtc_regs_t *rtc_regs = DEV_REGS(dev);
	struct k3_rtc_counter_data *data = DEV_DATA(dev);

	WAIT_FOR((rtc_regs->SYNCPEND & RTC_SYNCPEND_STATUS_MASK) != 0,
		data->sync_timeout_us,
		k_yield());

	return 0;
}

static int k3_rtc_unlock_status(k3_rtc_regs_t *rtc_regs)
{
	return !!(rtc_regs->GENRAL_CTL & RTC_UNLOCK_MASK);
}

static int k3_rtc_unlock(const struct device *dev)
{
	k3_rtc_regs_t *rtc_regs = DEV_REGS(dev);
	int ret;

	if (k3_rtc_unlock_status(rtc_regs) == 0) {
		rtc_regs->KICK0 = K3_RTC_KICK0_UNLOCK;
		rtc_regs->KICK1 = K3_RTC_KICK1_UNLOCK;

		ret = k3_rtc_fence(dev);
		if (ret != 0) {
			LOG_ERR("Failed to sync after unlock");
			return ret;
		}

		if (k3_rtc_unlock_status(rtc_regs) == 0) {
			LOG_ERR("RTC unlock failed!");
			return -EIO;
		}
	}

	return 0;
}

static int k3_counter_start(const struct device *dev)
{
	ARG_UNUSED(dev);
	/* RTC counter runs after power-on reset */
	return 0;
}

static int k3_counter_stop(const struct device *dev)
{
	ARG_UNUSED(dev);
	/* RTC counter can only be reset, not stopped */
	return 0;
}

static int k3_counter_get_value(const struct device *dev, uint32_t *ticks)
{
	k3_rtc_regs_t *rtc_regs = DEV_REGS(dev);

	if (ticks == NULL) {
		return -EINVAL;
	}

	*ticks = rtc_regs->S_CNT_LSW;

	return 0;
}

static int k3_counter_reset(const struct device *dev)
{
	k3_rtc_regs_t *rtc_regs = DEV_REGS(dev);
	int ret;

	rtc_regs->S_CNT_LSW = 0;
	rtc_regs->S_CNT_MSW = 0;

	ret = k3_rtc_fence(dev);
	if (ret != 0) {
		LOG_ERR("Failed to sync after reset");
		return ret;
	}

	return ret;
}

static int k3_counter_set_value(const struct device *dev, uint32_t ticks)
{
	k3_rtc_regs_t *rtc_regs = DEV_REGS(dev);
	int ret;

	if (ticks == 0) {
		return -EINVAL;
	}

	rtc_regs->S_CNT_LSW = ticks;
	rtc_regs->S_CNT_MSW = 0;

	ret = k3_rtc_fence(dev);
	if (ret != 0) {
		LOG_ERR("Failed to sync after set_value");
		return ret;
	}

	return ret;
}

static int k3_counter_set_alarm(const struct device *dev, uint8_t chan_id,
				const struct counter_alarm_cfg *alarm_cfg)
{
	k3_rtc_regs_t *rtc_regs = DEV_REGS(dev);
	struct k3_rtc_counter_data *data = DEV_DATA(dev);
	int ret, key;

	if (chan_id > 1) {
		LOG_ERR("Invalid alarm channel");
		return -EINVAL;
	}

	if (alarm_cfg == NULL) {
		LOG_ERR("NULL alarm config");
		return -EINVAL;
	}

	if ((alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) != 0) {
		data->alarm_cfg[chan_id].ticks = alarm_cfg->ticks;
	} else {
		uint32_t now;

		ret = k3_counter_get_value(dev, &now);
		if (ret != 0) {
			return ret;
		}
		data->alarm_cfg[chan_id].ticks = now + alarm_cfg->ticks;
	}

	key = irq_lock();

	if (chan_id == 0) {
		rtc_regs->ON_OFF_S_CNT_LSW = (uint32_t)data->alarm_cfg[chan_id].ticks;
		rtc_regs->ON_OFF_S_CNT_MSW = 0;
		rtc_regs->IRQSTATUS_SYS = RTC_IRQ_ALARM1_MASK;
		rtc_regs->IRQENABLE_SET_SYS = RTC_IRQ_ALARM1_MASK;
	} else {
		rtc_regs->OFF_ON_S_CNT_LSW = (uint32_t)data->alarm_cfg[chan_id].ticks;
		rtc_regs->OFF_ON_S_CNT_MSW = 0;
		rtc_regs->IRQSTATUS_SYS = RTC_IRQ_ALARM2_MASK;
		rtc_regs->IRQENABLE_SET_SYS = RTC_IRQ_ALARM2_MASK;
	}
	ret = k3_rtc_fence(dev);
	if (ret != 0) {
		LOG_ERR("Failed to sync after alarm set: %d", ret);
		irq_unlock(key);
		return ret;
	}

	data->alarm_cfg[chan_id].callback = alarm_cfg->callback;
	data->alarm_cfg[chan_id].user_data = alarm_cfg->user_data;

	irq_unlock(key);

	return ret;
}

static int k3_counter_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	k3_rtc_regs_t *rtc_regs = DEV_REGS(dev);
	struct k3_rtc_counter_data *data = DEV_DATA(dev);
	int ret, key;

	if (chan_id > 1) {
		LOG_ERR("Invalid alarm channel");
		return -EINVAL;
	}

	key = irq_lock();
	if (chan_id == 0) {
		rtc_regs->IRQENABLE_CLR_SYS = RTC_IRQ_ALARM1_MASK;
		rtc_regs->IRQSTATUS_SYS = RTC_IRQ_ALARM1_MASK;
	} else {
		rtc_regs->IRQENABLE_CLR_SYS = RTC_IRQ_ALARM2_MASK;
		rtc_regs->IRQSTATUS_SYS = RTC_IRQ_ALARM2_MASK;
	}
	ret = k3_rtc_fence(dev);
	if (ret != 0) {
		LOG_ERR("Failed to sync after alarm cancel: %d", ret);
		irq_unlock(key);
		return ret;
	}

	data->alarm_cfg[chan_id].callback = NULL;
	data->alarm_cfg[chan_id].user_data = NULL;
	data->alarm_cfg[chan_id].flags = 0;

	irq_unlock(key);

	return ret;
}

static uint32_t k3_counter_get_pending_int(const struct device *dev)
{
	k3_rtc_regs_t *rtc_regs = DEV_REGS(dev);

	return rtc_regs->IRQSTATUS_SYS & RTC_IRQ_STATUS_MASK;
}

static uint32_t k3_counter_get_top_value(const struct device *dev)
{
	ARG_UNUSED(dev);
	return RTC_MAX_TOP_VALUE;
}

static uint32_t k3_counter_get_frequency(const struct device *dev)
{
	const struct k3_rtc_counter_cfg *config = DEV_CFG(dev);

	return config->counter_info.freq;
}

#ifdef CONFIG_COUNTER_64BITS_TICKS
static int k3_counter_get_value_64(const struct device *dev, uint64_t *ticks)
{
	k3_rtc_regs_t *rtc_regs = DEV_REGS(dev);

	if (ticks == NULL) {
		return -EINVAL;
	}

	uint32_t seconds_low = rtc_regs->S_CNT_LSW;
	uint32_t seconds_high = rtc_regs->S_CNT_MSW;

	*ticks = ((uint64_t)seconds_high << 32) | (uint64_t)seconds_low;

	return 0;
}

static int k3_counter_set_value_64(const struct device *dev, uint64_t ticks)
{
	k3_rtc_regs_t *rtc_regs = DEV_REGS(dev);
	int ret;

	if (ticks == 0) {
		return -EINVAL;
	}

	rtc_regs->S_CNT_LSW = ticks;
	rtc_regs->S_CNT_MSW = (uint32_t)(ticks >> 32);

	ret = k3_rtc_fence(dev);
	if (ret != 0) {
		LOG_ERR("Failed to sync after set_value");
		return ret;
	}

	return ret;
}

static uint64_t k3_counter_get_top_value_64(const struct device *dev)
{
	ARG_UNUSED(dev);
	return RTC_MAX_TOP_VALUE_64;
}

static int k3_counter_set_alarm_64(const struct device *dev, uint8_t chan_id,
				   const struct counter_alarm_cfg_64 *alarm_cfg)
{
	k3_rtc_regs_t *rtc_regs = DEV_REGS(dev);
	struct k3_rtc_counter_data *data = DEV_DATA(dev);
	int ret, key;

	if (chan_id > 1) {
		LOG_ERR("Invalid alarm channel");
		return -EINVAL;
	}

	if (alarm_cfg == NULL) {
		LOG_ERR("NULL alarm config");
		return -EINVAL;
	}

	if ((alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) != 0) {
		data->alarm_cfg[chan_id].ticks = alarm_cfg->ticks;
	} else {
		uint64_t now;

		ret = k3_counter_get_value_64(dev, &now);
		if (ret != 0) {
			return ret;
		}
		data->alarm_cfg[chan_id].ticks = now + alarm_cfg->ticks;
	}

	key = irq_lock();

	if (chan_id == 0) {
		rtc_regs->ON_OFF_S_CNT_LSW = (uint32_t)data->alarm_cfg[chan_id].ticks;
		rtc_regs->ON_OFF_S_CNT_MSW = (uint32_t)(data->alarm_cfg[chan_id].ticks >> 32);
		rtc_regs->IRQSTATUS_SYS = RTC_IRQ_ALARM1_MASK;
		rtc_regs->IRQENABLE_SET_SYS = RTC_IRQ_ALARM1_MASK;
	} else {
		rtc_regs->OFF_ON_S_CNT_LSW = (uint32_t)data->alarm_cfg[chan_id].ticks;
		rtc_regs->OFF_ON_S_CNT_MSW = (uint32_t)(data->alarm_cfg[chan_id].ticks >> 32);
		rtc_regs->IRQSTATUS_SYS = RTC_IRQ_ALARM2_MASK;
		rtc_regs->IRQENABLE_SET_SYS = RTC_IRQ_ALARM2_MASK;
	}
	ret = k3_rtc_fence(dev);
	if (ret != 0) {
		LOG_ERR("Failed to sync after alarm set: %d", ret);
		irq_unlock(key);
		return ret;
	}

	data->alarm_cfg[chan_id].callback = alarm_cfg->callback;
	data->alarm_cfg[chan_id].user_data = alarm_cfg->user_data;
	data->alarm_cfg[chan_id].flags = alarm_cfg->flags;

	irq_unlock(key);

	return ret;
}
#endif /* CONFIG_COUNTER_64BITS_TICKS */

static void k3_rtc_isr(const struct device *dev)
{
	k3_rtc_regs_t *rtc_regs = DEV_REGS(dev);
	struct k3_rtc_counter_data *data = DEV_DATA(dev);
	uint32_t status;
#ifdef CONFIG_COUNTER_64BITS_TICKS
	uint64_t now;
#else
	uint32_t now;
#endif /* CONFIG_COUNTER_64BITS_TICKS */

	status = rtc_regs->IRQSTATUS_SYS;
	if (status == 0) {
		return;
	}

	rtc_regs->IRQSTATUS_SYS = status;

	if ((status & RTC_IRQ_ALARM1_MASK) != 0 && data->alarm_cfg[0].callback != NULL) {
#ifdef CONFIG_COUNTER_64BITS_TICKS
		k3_counter_get_value_64(dev, &now);
#else
		k3_counter_get_value(dev, &now);
#endif /* CONFIG_COUNTER_64BITS_TICKS */
		data->alarm_cfg[0].callback(dev, 0, now, data->alarm_cfg[0].user_data);
	}

	if ((status & RTC_IRQ_ALARM2_MASK) != 0 && data->alarm_cfg[1].callback != NULL) {
#ifdef CONFIG_COUNTER_64BITS_TICKS
		k3_counter_get_value_64(dev, &now);
#else
		k3_counter_get_value(dev, &now);
#endif /* CONFIG_COUNTER_64BITS_TICKS */
		data->alarm_cfg[1].callback(dev, 1, now, data->alarm_cfg[1].user_data);
	}
}

static int k3_counter_init(const struct device *dev)
{
	DEVICE_MMIO_NAMED_MAP(dev, reg_base, K_MEM_CACHE_NONE);
	const struct k3_rtc_counter_cfg *config = DEV_CFG(dev);
	struct k3_rtc_counter_data *data = DEV_DATA(dev);
	k3_rtc_regs_t *rtc_regs = DEV_REGS(dev);
	int ret;

	ret = k3_rtc_unlock(dev);
	if (ret != 0) {
		LOG_ERR("Failed to unlock RTC");
		return ret;
	}

	/* 4 clock cycles for sync */
	data->sync_timeout_us = (USEC_PER_SEC / config->osc_freq) * 4;

	/* Enable 32k oscillator dependency */
	rtc_regs->GENRAL_CTL |= RTC_O32K_OSC_MASK;

	/* Enable freeze mode for atomic 48-bit reads */
	rtc_regs->GENRAL_CTL |= RTC_CNT_FMODE_MASK;

	/* Clear and disable all interrupts */
	rtc_regs->IRQSTATUS_SYS = RTC_IRQ_STATUS_MASK;
	rtc_regs->IRQENABLE_CLR_SYS = RTC_IRQ_STATUS_MASK;

	ret = k3_rtc_fence(dev);
	if (ret != 0) {
		LOG_ERR("Failed sync in init");
		return ret;
	}

	config->irq_config_func();

	return 0;
}

static const struct counter_driver_api k3_counter_api = {
	.start = k3_counter_start,
	.stop = k3_counter_stop,
	.get_value = k3_counter_get_value,
	.reset = k3_counter_reset,
	.set_value = k3_counter_set_value,
	.set_alarm = k3_counter_set_alarm,
	.cancel_alarm = k3_counter_cancel_alarm,
	.get_pending_int = k3_counter_get_pending_int,
	.get_top_value = k3_counter_get_top_value,
	.get_freq = k3_counter_get_frequency,
#ifdef CONFIG_COUNTER_64BITS_TICKS
	.get_value_64 = k3_counter_get_value_64,
	.set_value_64 = k3_counter_set_value_64,
	.set_alarm_64 = k3_counter_set_alarm_64,
	.get_top_value_64 = k3_counter_get_top_value_64,
#endif /* CONFIG_COUNTER_64BITS_TICKS */
};

#define TI_K3_COUNTER_INIT(i)								\
	static void ti_k3_counter_irq_config_##i(void)					\
	{										\
		IRQ_CONNECT(DT_INST_IRQN(i),						\
			DT_INST_IRQ(i, priority),					\
			k3_rtc_isr,							\
			DEVICE_DT_INST_GET(i), 0);					\
		irq_enable(DT_INST_IRQN(i));						\
	}										\
	static struct k3_rtc_counter_data k3_counter_data_##i = {};			\
	static struct k3_rtc_counter_cfg k3_counter_cfg_##i = {				\
		.counter_info =								\
			{								\
				.freq = 1,						\
				COND_CODE_1(CONFIG_COUNTER_64BITS_TICKS,		\
					(.max_top_value_64 = RTC_MAX_TOP_VALUE_64,),	\
					(.max_top_value = RTC_MAX_TOP_VALUE,))		\
				.flags = COUNTER_CONFIG_INFO_COUNT_UP,			\
				.channels = 2,						\
			},								\
		DEVICE_MMIO_NAMED_ROM_INIT(reg_base, DT_DRV_INST(i)),			\
		.irq_config_func = ti_k3_counter_irq_config_##i,			\
		.osc_freq = DT_INST_PROP(i, clock_frequency)				\
	};										\
	DEVICE_DT_INST_DEFINE(i,							\
		k3_counter_init,							\
		NULL,									\
		&k3_counter_data_##i,							\
		&k3_counter_cfg_##i,							\
		POST_KERNEL,								\
		CONFIG_COUNTER_INIT_PRIORITY,						\
		&k3_counter_api								\
	);

DT_INST_FOREACH_STATUS_OKAY(TI_K3_COUNTER_INIT)
