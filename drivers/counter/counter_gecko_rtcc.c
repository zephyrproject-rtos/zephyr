/*
 * Copyright (c) 2019, Piotr Mienkowski
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT silabs_gecko_rtcc

#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <kernel.h>
#include <device.h>
#include <soc.h>
#include <em_cmu.h>
#include <em_rtcc.h>
#include <drivers/counter.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(counter_gecko, CONFIG_COUNTER_LOG_LEVEL);

#define RTCC_MAX_VALUE       (_RTCC_CNT_MASK)
#define RTCC_ALARM_NUM        2

struct counter_gecko_config {
	struct counter_config_info info;
	void (*irq_config)(void);
	uint32_t prescaler;
};

struct counter_gecko_alarm_data {
	counter_alarm_callback_t callback;
	void *user_data;
};

struct counter_gecko_data {
	struct counter_gecko_alarm_data alarm[RTCC_ALARM_NUM];
	counter_top_callback_t top_callback;
	void *top_user_data;
};

#define DEV_NAME(dev) ((dev)->name)
#define DEV_CFG(dev) \
	((const struct counter_gecko_config * const)(dev)->config)
#define DEV_DATA(dev) \
	((struct counter_gecko_data *const)(dev)->data)

#ifdef CONFIG_SOC_GECKO_HAS_ERRATA_RTCC_E201
#define ERRATA_RTCC_E201_MESSAGE \
	"Errata RTCC_E201: In case RTCC prescaler != 1 the module does not " \
	"reset the counter value on CCV1 compare."
#endif

/* Map channel id to CC channel provided by the RTCC module */
static uint8_t chan_id2cc_idx(uint8_t chan_id)
{
	uint8_t cc_idx;

	switch (chan_id) {
	case 0:
		cc_idx = 2;
		break;
	default:
		cc_idx = 0;
		break;
	}
	return cc_idx;
}

static int counter_gecko_start(const struct device *dev)
{
	ARG_UNUSED(dev);

	RTCC_Enable(true);

	return 0;
}

static int counter_gecko_stop(const struct device *dev)
{
	ARG_UNUSED(dev);

	RTCC_Enable(false);

	return 0;
}

static int counter_gecko_get_value(const struct device *dev, uint32_t *ticks)
{
	ARG_UNUSED(dev);

	*ticks = RTCC_CounterGet();
	return 0;
}

static int counter_gecko_set_top_value(const struct device *dev,
				       const struct counter_top_cfg *cfg)
{
	struct counter_gecko_data *const dev_data = DEV_DATA(dev);
	uint32_t ticks;
	uint32_t flags;
	int err = 0;

#ifdef CONFIG_SOC_GECKO_HAS_ERRATA_RTCC_E201
	const struct counter_gecko_config *const dev_cfg = DEV_CFG(dev);

	if (dev_cfg->prescaler != 1) {
		LOG_ERR(ERRATA_RTCC_E201_MESSAGE);
		return -EINVAL;
	}
#endif

	/* Counter top value can only be changed when all alarms are disabled */
	for (int i = 0; i < RTCC_ALARM_NUM; i++) {
		if (dev_data->alarm[i].callback) {
			return -EBUSY;
		}
	}

	RTCC_IntClear(RTCC_IF_CC1);

	dev_data->top_callback = cfg->callback;
	dev_data->top_user_data = cfg->user_data;
	ticks = cfg->ticks;
	flags = cfg->flags;

	if (!(flags & COUNTER_TOP_CFG_DONT_RESET)) {
		RTCC_CounterSet(0);
	}

	RTCC_ChannelCCVSet(1, ticks);

	LOG_DBG("set top value: %u", ticks);

	if ((flags & COUNTER_TOP_CFG_DONT_RESET) &&
		RTCC_CounterGet() > ticks) {
		err = -ETIME;
		if (flags & COUNTER_TOP_CFG_RESET_WHEN_LATE) {
			RTCC_CounterSet(0);
		}
	}

	/* Enable the compare interrupt */
	RTCC_IntEnable(RTCC_IF_CC1);

	return err;
}

static uint32_t counter_gecko_get_top_value(const struct device *dev)
{
	ARG_UNUSED(dev);

	return RTCC_ChannelCCVGet(1);
}

static uint32_t counter_gecko_get_max_relative_alarm(const struct device *dev)
{
	ARG_UNUSED(dev);

	return RTCC_ChannelCCVGet(1);
}

static int counter_gecko_set_alarm(const struct device *dev, uint8_t chan_id,
				   const struct counter_alarm_cfg *alarm_cfg)
{
	uint32_t count = RTCC_CounterGet();
	struct counter_gecko_data *const dev_data = DEV_DATA(dev);
	uint32_t top_value = counter_gecko_get_top_value(dev);
	uint32_t ccv;

	if ((top_value != 0) && (alarm_cfg->ticks > top_value)) {
		return -EINVAL;
	}
	if (dev_data->alarm[chan_id].callback != NULL) {
		return -EBUSY;
	}

	if ((alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) != 0) {
		ccv = alarm_cfg->ticks;
	} else {
		if (top_value == 0) {
			ccv = count + alarm_cfg->ticks;
		} else {
			uint64_t ccv64 = count + alarm_cfg->ticks;

			ccv = (uint32_t)(ccv64 % top_value);
		}
	}

	uint8_t cc_idx = chan_id2cc_idx(chan_id);

	RTCC_IntClear(RTCC_IF_CC0 << cc_idx);

	dev_data->alarm[chan_id].callback = alarm_cfg->callback;
	dev_data->alarm[chan_id].user_data = alarm_cfg->user_data;

	RTCC_ChannelCCVSet(cc_idx, ccv);

	LOG_DBG("set alarm: channel %u, count %u", chan_id, ccv);

	/* Enable the compare interrupt */
	RTCC_IntEnable(RTCC_IF_CC0 << cc_idx);

	return 0;
}

static int counter_gecko_cancel_alarm(const struct device *dev,
				      uint8_t chan_id)
{
	struct counter_gecko_data *const dev_data = DEV_DATA(dev);

	uint8_t cc_idx = chan_id2cc_idx(chan_id);

	/* Disable the compare interrupt */
	RTCC_IntDisable(RTCC_IF_CC0 << cc_idx);
	RTCC_IntClear(RTCC_IF_CC0 << cc_idx);

	dev_data->alarm[chan_id].callback = NULL;
	dev_data->alarm[chan_id].user_data = NULL;

	RTCC_ChannelCCVSet(cc_idx, 0);

	LOG_DBG("cancel alarm: channel %u", chan_id);

	return 0;
}

static uint32_t counter_gecko_get_pending_int(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static int counter_gecko_init(const struct device *dev)
{
	const struct counter_gecko_config *const dev_cfg = DEV_CFG(dev);

	RTCC_Init_TypeDef rtcc_config = {
		false,                /* Don't start counting */
		false,                /* Disable RTC during debug halt. */
		false,                /* Don't wrap prescaler on CCV0 */
		true,                 /* Counter wrap on CCV1 */
#if defined(_SILICON_LABS_32B_SERIES_2)
		(RTCC_CntPresc_TypeDef)(31UL - __CLZ(dev_cfg->prescaler)),
#else
		(RTCC_CntPresc_TypeDef)CMU_DivToLog2(dev_cfg->prescaler),
#endif
		rtccCntTickPresc,     /* Count according to prescaler value */
#if defined(_RTCC_CTRL_BUMODETSEN_MASK)
		false,                /* Don't store RTCC counter value in
				       * RTCC_CCV2 upon backup mode entry.
				       */
#endif
#if defined(_RTCC_CTRL_OSCFDETEN_MASK)
		false,                /* Don't enable LFXO fail detection */
#endif
#if defined (_RTCC_CTRL_CNTMODE_MASK)
		rtccCntModeNormal,    /* Use RTCC in normal mode */
#endif
#if defined (_RTCC_CTRL_LYEARCORRDIS_MASK)
		false                 /* No leap year correction. */
#endif
	};

	RTCC_CCChConf_TypeDef rtcc_channel_config = {
		rtccCapComChModeCompare,    /* Use compare mode */
		rtccCompMatchOutActionPulse,/* Don't care */
		rtccPRSCh0,                 /* PRS is not used */
		rtccInEdgeNone,             /* Capture input is not used */
		rtccCompBaseCnt,            /* Compare with base CNT register */
#if defined (_RTCC_CC_CTRL_COMPMASK_MASK)
		0,                          /* Compare mask */
#endif
#if defined (_RTCC_CC_CTRL_DAYCC_MASK)
		rtccDayCompareModeMonth,    /* Don't care */
#endif
	};

#if defined(cmuClock_CORELE)
	/* Ensure LE modules are clocked. */
	CMU_ClockEnable(cmuClock_CORELE, true);
#endif

#if defined(CMU_LFECLKEN0_RTCC)
	/* Enable LFECLK in CMU (will also enable oscillator if not enabled). */
	CMU_ClockSelectSet(cmuClock_LFE, cmuSelect_LFXO);
#elif defined(_SILICON_LABS_32B_SERIES_2)
	CMU_ClockSelectSet(cmuClock_RTCC, cmuSelect_LFXO);
#else
	/* Enable LFACLK in CMU (will also enable oscillator if not enabled). */
	CMU_ClockSelectSet(cmuClock_LFA, cmuSelect_LFXO);
#endif

	/* Enable RTCC module clock */
	CMU_ClockEnable(cmuClock_RTCC, true);

	/* Initialize RTCC */
	RTCC_Init(&rtcc_config);

	/* Set up compare channels */
	RTCC_ChannelInit(0, &rtcc_channel_config);
	RTCC_ChannelInit(1, &rtcc_channel_config);
	RTCC_ChannelInit(2, &rtcc_channel_config);

	/* Disable module's internal interrupt sources */
	RTCC_IntDisable(_RTCC_IF_MASK);
	RTCC_IntClear(_RTCC_IF_MASK);

	/* Clear the counter */
	RTCC->CNT = 0;

	/* Configure & enable module interrupts */
	dev_cfg->irq_config();

	LOG_INF("Device %s initialized", DEV_NAME(dev));

	return 0;
}

static const struct counter_driver_api counter_gecko_driver_api = {
	.start = counter_gecko_start,
	.stop = counter_gecko_stop,
	.get_value = counter_gecko_get_value,
	.set_alarm = counter_gecko_set_alarm,
	.cancel_alarm = counter_gecko_cancel_alarm,
	.set_top_value = counter_gecko_set_top_value,
	.get_pending_int = counter_gecko_get_pending_int,
	.get_top_value = counter_gecko_get_top_value,
	.get_max_relative_alarm = counter_gecko_get_max_relative_alarm,
};

/* RTCC0 */

DEVICE_DECLARE(counter_gecko_0);

ISR_DIRECT_DECLARE(counter_gecko_isr_0)
{
	const struct device *dev = DEVICE_GET(counter_gecko_0);
	struct counter_gecko_data *const dev_data = DEV_DATA(dev);
	counter_alarm_callback_t alarm_callback;
	uint32_t count = RTCC_CounterGet();
	uint32_t flags = RTCC_IntGetEnabled();

	RTCC_IntClear(flags);

	if (flags & RTCC_IF_CC1) {
		if (dev_data->top_callback) {
			dev_data->top_callback(dev, dev_data->top_user_data);
		}
	}
	for (int i = 0; i < RTCC_ALARM_NUM; i++) {
		uint8_t cc_idx = chan_id2cc_idx(i);

		if (flags & (RTCC_IF_CC0 << cc_idx)) {
			if (dev_data->alarm[i].callback) {
				alarm_callback = dev_data->alarm[i].callback;
				dev_data->alarm[i].callback = NULL;
				alarm_callback(dev, i, count,
					       dev_data->alarm[i].user_data);
			}
		}
	}

	ISR_DIRECT_PM();

	return 1;
}

BUILD_ASSERT((DT_INST_PROP(0, prescaler) > 0U) &&
	     (DT_INST_PROP(0, prescaler) <= 32768U));

static void counter_gecko_0_irq_config(void)
{
	IRQ_DIRECT_CONNECT(DT_INST_IRQN(0),
			   DT_INST_IRQ(0, priority),
			   counter_gecko_isr_0, 0);
	irq_enable(DT_INST_IRQN(0));
}

static const struct counter_gecko_config counter_gecko_0_config = {
	.info = {
		.max_top_value = RTCC_MAX_VALUE,
		.freq = DT_INST_PROP(0, clock_frequency) /
			DT_INST_PROP(0, prescaler),
		.flags = COUNTER_CONFIG_INFO_COUNT_UP,
		.channels = RTCC_ALARM_NUM,
	},
	.irq_config = counter_gecko_0_irq_config,
	.prescaler = DT_INST_PROP(0, prescaler),
};

static struct counter_gecko_data counter_gecko_0_data;

DEVICE_AND_API_INIT(counter_gecko_0, DT_INST_LABEL(0),
	counter_gecko_init, &counter_gecko_0_data, &counter_gecko_0_config,
	PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
	&counter_gecko_driver_api);
