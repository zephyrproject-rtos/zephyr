/*
 * Copyright (c) 2020 Intel Corporation
 * Copyright (c) 2022 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_xec_tach

#include <errno.h>
#include <soc.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/dt-bindings/interrupt-controller/mchp-xec-ecia.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>

LOG_MODULE_REGISTER(tach_xec, CONFIG_SENSOR_LOG_LEVEL);

/* Frequency of the clock the hardware counts in reading mode 1 */
#define TACH_XEC_CLOCK_HZ	100000U
#define TACH_XEC_SEC_PER_MIN	60U

/*
 * A latched count of 0xffff means the programmed number of TACH edges was not
 * observed before the internal counter saturated, i.e. the fan is stopped,
 * jammed, or turning slower than the block can measure.
 */
#define TACH_XEC_COUNT_STOPPED	0xffffU

/*
 * The internal counter is latched either when the programmed number of edges
 * has been seen or when it saturates. Saturation from zero takes
 * 0xffff / 100 kHz = 655.35 ms, so a slightly longer timeout guarantees a
 * reading is available for any fan speed the block supports. Exceeding it
 * means the block is not counting at all.
 */
#define TACH_XEC_LATCH_TIMEOUT_MS	700U

/* Byte offset of the read-only latched counter in the control register */
#define TACH_XEC_COUNTER_REG_OFS	(MCHP_TACH_CONTROL_REG_OFS + 2U)

/* Writable (R/W1C) bits of the status register */
#define TACH_XEC_STS_RW1C	(MCHP_TACH_STS_EXCEED_LIMIT | MCHP_TACH_STS_TOGGLE |	\
				 MCHP_TACH_STS_CNT_RDY)

struct tach_xec_config {
	uintptr_t base;
	/* TACH_EDGES control field, pre-shifted into position */
	uint32_t ctrl_edges;
	/* Width of the measurement window in half TACH periods: 1, 2, 4 or 8 */
	uint32_t window_half_periods;
	/* TACH periods the fan generates per revolution */
	uint32_t pulses_per_round;
	uint8_t pcr_scr;
	/* Interrupt aggregator source this instance is wired to */
	uint8_t girq;
	uint8_t girq_pos;
	const struct pinctrl_dev_config *pcfg;
	void (*irq_config_func)(void);
};

struct tach_xec_data {
	/* Signalled by the ISR once a fresh count has been latched */
	struct k_sem latched;
	/* Control register image, maintained so the counter field is never written back */
	uint32_t control;
	uint16_t count;
	/* Set while a fetch is waiting for a latch */
	bool fetch_pending;
#ifdef CONFIG_TACH_XEC_TRIGGER
	/* Back pointer for the thread and work queue entry points */
	const struct device *dev;
	const struct sensor_trigger *drdy_trigger;
	sensor_trigger_handler_t drdy_handler;
#if defined(CONFIG_TACH_XEC_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_TACH_XEC_THREAD_STACK_SIZE);
	struct k_sem drdy_sem;
	struct k_thread thread;
#elif defined(CONFIG_TACH_XEC_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif
#endif /* CONFIG_TACH_XEC_TRIGGER */
};

static void tach_xec_cnt_rdy_int_set(const struct device *dev, bool enable)
{
	const struct tach_xec_config * const cfg = dev->config;
	struct tach_xec_data * const data = dev->data;

	if (enable) {
		data->control |= MCHP_TACH_CTRL_CNT_RDY_INT_EN;
	} else {
		data->control &= ~MCHP_TACH_CTRL_CNT_RDY_INT_EN;
	}

	sys_write32(data->control, cfg->base + MCHP_TACH_CONTROL_REG_OFS);
}

/*
 * COUNT_READY_STATUS is set by the hardware on every latch regardless of
 * COUNT_READY_INT_EN, so the interrupt is only armed while something is waiting
 * for it: a fetch, or a registered data ready trigger. Leaving it enabled
 * unconditionally would generate an interrupt every measurement window - up to a
 * few hundred per second - with nothing to consume them. Call with interrupts
 * locked so a fetch and a trigger cannot race for the control register.
 */
static void tach_xec_cnt_rdy_int_update(const struct device *dev)
{
	struct tach_xec_data * const data = dev->data;
	bool enable = data->fetch_pending;

#ifdef CONFIG_TACH_XEC_TRIGGER
	enable = enable || (data->drdy_handler != NULL);
#endif

	tach_xec_cnt_rdy_int_set(dev, enable);
}

#ifdef CONFIG_TACH_XEC_TRIGGER
/*
 * The sensor API calls trigger handlers from a thread, so the ISR only hands the
 * event over. Both signalling mechanisms coalesce, which is what data ready
 * needs: only the most recently latched count is of interest, and a handler
 * slower than the measurement window must not accumulate a backlog of calls.
 */
static void tach_xec_drdy_signal(struct tach_xec_data *data)
{
#if defined(CONFIG_TACH_XEC_TRIGGER_OWN_THREAD)
	k_sem_give(&data->drdy_sem);
#elif defined(CONFIG_TACH_XEC_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->work);
#endif
}

static void tach_xec_process_drdy(const struct device *dev)
{
	struct tach_xec_data * const data = dev->data;
	const struct sensor_trigger *trig = data->drdy_trigger;
	sensor_trigger_handler_t handler = data->drdy_handler;

	/*
	 * The ISR has already stored the latched count, so the handler can read
	 * SENSOR_CHAN_RPM without a sensor_sample_fetch() call of its own.
	 */
	if (handler != NULL) {
		handler(dev, trig);
	}
}

#if defined(CONFIG_TACH_XEC_TRIGGER_OWN_THREAD)
static void tach_xec_thread(void *p1, void *p2, void *p3)
{
	const struct device *dev = p1;
	struct tach_xec_data * const data = dev->data;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (true) {
		k_sem_take(&data->drdy_sem, K_FOREVER);
		tach_xec_process_drdy(dev);
	}
}
#elif defined(CONFIG_TACH_XEC_TRIGGER_GLOBAL_THREAD)
static void tach_xec_work_handler(struct k_work *work)
{
	struct tach_xec_data * const data = CONTAINER_OF(work, struct tach_xec_data, work);

	tach_xec_process_drdy(data->dev);
}
#endif
#endif /* CONFIG_TACH_XEC_TRIGGER */

static void tach_xec_isr(const struct device *dev)
{
	const struct tach_xec_config * const cfg = dev->config;
	struct tach_xec_data * const data = dev->data;
	uint32_t status = sys_read32(cfg->base + MCHP_TACH_STATUS_REG_OFS);

	if ((status & MCHP_TACH_STS_CNT_RDY) != 0U) {
		data->count = sys_read16(cfg->base + TACH_XEC_COUNTER_REG_OFS);
		k_sem_give(&data->latched);
#ifdef CONFIG_TACH_XEC_TRIGGER
		if (data->drdy_handler != NULL) {
			tach_xec_drdy_signal(data);
		}
#endif
	}

	/*
	 * Clear the peripheral status first: the aggregator re-latches from a
	 * source that is still asserted.
	 */
	sys_write32(status & TACH_XEC_STS_RW1C, cfg->base + MCHP_TACH_STATUS_REG_OFS);
	soc_ecia_girq_status_clear(cfg->girq, cfg->girq_pos);
}

static int tach_xec_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct tach_xec_config * const cfg = dev->config;
	struct tach_xec_data * const data = dev->data;
	unsigned int key;
	int ret;

	ARG_UNUSED(chan);

	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);

	key = irq_lock();

	/*
	 * Discard a count latched before this call, along with any interrupt it
	 * raised, so that the value the ISR stores is a fresh measurement.
	 * COUNT_READY_STATUS is R/W1C; writing zero to the other R/W1C bits
	 * leaves them untouched. An interrupt already pending in the NVIC at
	 * this point sees the status cleared and does not signal.
	 */
	sys_write32(MCHP_TACH_STS_CNT_RDY, cfg->base + MCHP_TACH_STATUS_REG_OFS);
	soc_ecia_girq_status_clear(cfg->girq, cfg->girq_pos);
	k_sem_reset(&data->latched);

	data->fetch_pending = true;
	tach_xec_cnt_rdy_int_update(dev);

	irq_unlock(key);

	ret = k_sem_take(&data->latched, K_MSEC(TACH_XEC_LATCH_TIMEOUT_MS));

	/* A registered trigger keeps the interrupt armed past the end of the fetch */
	key = irq_lock();
	data->fetch_pending = false;
	tach_xec_cnt_rdy_int_update(dev);
	irq_unlock(key);

	pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);

	if (ret != 0) {
		LOG_ERR("TACH count not latched within %u ms", TACH_XEC_LATCH_TIMEOUT_MS);

		return -EIO;
	}

	return 0;
}

static int tach_xec_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	const struct tach_xec_config * const cfg = dev->config;
	struct tach_xec_data * const data = dev->data;
	uint64_t numerator;
	uint64_t denominator;
	uint64_t rpm_micro;
	uint16_t count;

	if (chan != SENSOR_CHAN_RPM) {
		return -ENOTSUP;
	}

	count = data->count;

	/* A saturated count is a stopped fan; a zero count cannot be converted */
	if ((count == TACH_XEC_COUNT_STOPPED) || (count == 0U)) {
		val->val1 = 0;
		val->val2 = 0;

		return 0;
	}

	/*
	 * The latched counter holds the number of 100 kHz clocks spanning the
	 * programmed number of TACH edges. Measuring that window in half TACH
	 * periods keeps the arithmetic exact:
	 *
	 *   2 edges -> 1 half period    5 edges -> 4 half periods
	 *   3 edges -> 2 half periods   9 edges -> 8 half periods
	 *
	 * One revolution spans <pulses-per-round> full TACH periods, so
	 *
	 *            60 * 100000 * window_half_periods
	 *   RPM = ---------------------------------------
	 *          2 * pulses_per_round * latched_count
	 *
	 * The result is computed in micro-RPM so the fractional part can be
	 * reported in val2 instead of being truncated away.
	 */
	numerator = (uint64_t)TACH_XEC_SEC_PER_MIN * TACH_XEC_CLOCK_HZ *
		    cfg->window_half_periods * USEC_PER_SEC;
	denominator = (uint64_t)cfg->pulses_per_round * count * 2U;

	/* Round to nearest rather than toward zero */
	rpm_micro = (numerator + (denominator / 2U)) / denominator;

	val->val1 = (int32_t)(rpm_micro / USEC_PER_SEC);
	val->val2 = (int32_t)(rpm_micro % USEC_PER_SEC);

	return 0;
}

#ifdef CONFIG_TACH_XEC_TRIGGER
static int tach_xec_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
				sensor_trigger_handler_t handler)
{
	const struct tach_xec_config * const cfg = dev->config;
	struct tach_xec_data * const data = dev->data;
	unsigned int key;

	/*
	 * The block latches a count every measurement window, which is the data
	 * ready event. Its out of limit interrupt is not exposed: the driver
	 * leaves LIMIT_HI and LIMIT_LO alone.
	 */
	if (trig->type != SENSOR_TRIG_DATA_READY) {
		return -ENOTSUP;
	}

	if ((trig->chan != SENSOR_CHAN_RPM) && (trig->chan != SENSOR_CHAN_ALL)) {
		return -ENOTSUP;
	}

	key = irq_lock();

	if (handler != NULL) {
		/*
		 * Discard a count latched before this call, along with any
		 * interrupt it raised, so the first callback reports a window
		 * that began after the trigger was registered.
		 */
		sys_write32(MCHP_TACH_STS_CNT_RDY, cfg->base + MCHP_TACH_STATUS_REG_OFS);
		soc_ecia_girq_status_clear(cfg->girq, cfg->girq_pos);
	}

	/* Held by pointer: the same object is handed back to the handler */
	data->drdy_trigger = trig;
	data->drdy_handler = handler;

	tach_xec_cnt_rdy_int_update(dev);

	irq_unlock(key);

	return 0;
}
#endif /* CONFIG_TACH_XEC_TRIGGER */

#ifdef CONFIG_PM_DEVICE
static int tach_xec_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct tach_xec_config * const cfg = dev->config;
	struct tach_xec_data * const data = dev->data;
	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		sys_write32(data->control, cfg->base + MCHP_TACH_CONTROL_REG_OFS);
		/* The block resumes mid-count, so any pending event is stale */
		sys_write32(TACH_XEC_STS_RW1C, cfg->base + MCHP_TACH_STATUS_REG_OFS);
		soc_ecia_girq_status_clear(cfg->girq, cfg->girq_pos);
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		/* Bits 31:16 are the read-only counter and must not be written back */
		data->control = sys_read32(cfg->base + MCHP_TACH_CONTROL_REG_OFS) &
				~MCHP_TACH_CTRL_COUNTER_MASK;
		sys_write32(data->control & ~MCHP_TACH_CTRL_EN,
			    cfg->base + MCHP_TACH_CONTROL_REG_OFS);
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}
#endif /* CONFIG_PM_DEVICE */

static int tach_xec_init(const struct device *dev)
{
	const struct tach_xec_config * const cfg = dev->config;
	struct tach_xec_data * const data = dev->data;
	int ret;

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		LOG_ERR("XEC TACH pinctrl init failed (%d)", ret);
		return ret;
	}

	k_sem_init(&data->latched, 0, 1);

#ifdef CONFIG_TACH_XEC_TRIGGER
	data->dev = dev;
#if defined(CONFIG_TACH_XEC_TRIGGER_OWN_THREAD)
	k_sem_init(&data->drdy_sem, 0, 1);

	k_thread_create(&data->thread, data->thread_stack, CONFIG_TACH_XEC_THREAD_STACK_SIZE,
			tach_xec_thread, (void *)dev, NULL, NULL,
			K_PRIO_COOP(CONFIG_TACH_XEC_THREAD_PRIORITY), 0, K_NO_WAIT);
	k_thread_name_set(&data->thread, dev->name);
#elif defined(CONFIG_TACH_XEC_TRIGGER_GLOBAL_THREAD)
	k_work_init(&data->work, tach_xec_work_handler);
#endif
#endif /* CONFIG_TACH_XEC_TRIGGER */

	soc_xec_pcr_sleep_en_clear(cfg->pcr_scr);

	/*
	 * COUNT_READY_INT_EN is left clear here and only set while a fetch is
	 * waiting for a latch or a data ready trigger is registered.
	 */
	data->control = MCHP_TACH_CTRL_READ_MODE_100K_CLOCK | cfg->ctrl_edges |
			MCHP_TACH_CTRL_FILTER_EN | MCHP_TACH_CTRL_EN;

	sys_write32(data->control, cfg->base + MCHP_TACH_CONTROL_REG_OFS);

	/* Discard status latched while the block was being configured */
	sys_write32(TACH_XEC_STS_RW1C, cfg->base + MCHP_TACH_STATUS_REG_OFS);
	soc_ecia_girq_status_clear(cfg->girq, cfg->girq_pos);
	soc_ecia_girq_ctrl(cfg->girq, cfg->girq_pos, MCHP_MEC_ECIA_GIRQ_EN);

	cfg->irq_config_func();

	/* Report a stopped fan until the first successful sample fetch */
	data->count = TACH_XEC_COUNT_STOPPED;

	return 0;
}

static DEVICE_API(sensor, tach_xec_driver_api) = {
	.sample_fetch = tach_xec_sample_fetch,
	.channel_get = tach_xec_channel_get,
#ifdef CONFIG_TACH_XEC_TRIGGER
	.trigger_set = tach_xec_trigger_set,
#endif
};

/* Pre-shifted TACH_EDGES control field for a given number of edges */
#define TACH_XEC_CTRL_EDGES(edges)							\
	((edges) == 2 ? MCHP_TACH_CTRL_EDGES_2 :					\
	 ((edges) == 3 ? MCHP_TACH_CTRL_EDGES_3 :					\
	  ((edges) == 5 ? MCHP_TACH_CTRL_EDGES_5 : MCHP_TACH_CTRL_EDGES_9)))

/* Half TACH periods spanned by a given number of edges */
#define TACH_XEC_HALF_PERIODS(edges)							\
	((edges) == 2 ? 1U : ((edges) == 3 ? 2U : ((edges) == 5 ? 4U : 8U)))

#define TACH_XEC_GIRQ(inst)	MCHP_XEC_ECIA_GIRQ(DT_INST_PROP_BY_IDX(inst, girqs, 0))
#define TACH_XEC_GIRQ_POS(inst)	MCHP_XEC_ECIA_GIRQ_POS(DT_INST_PROP_BY_IDX(inst, girqs, 0))

#define TACH_XEC_DEVICE(inst)								\
	BUILD_ASSERT((DT_INST_PROP(inst, tach_edges) == 2) ||				\
		     (DT_INST_PROP(inst, tach_edges) == 3) ||				\
		     (DT_INST_PROP(inst, tach_edges) == 5) ||				\
		     (DT_INST_PROP(inst, tach_edges) == 9),				\
		     "tach-edges must be 2, 3, 5 or 9");				\
											\
	BUILD_ASSERT(DT_INST_PROP(inst, pulses_per_round) > 0,				\
		     "pulses-per-round must be non-zero");				\
											\
	static struct tach_xec_data tach_xec_data_##inst;				\
											\
	PINCTRL_DT_INST_DEFINE(inst);							\
											\
	static void tach_xec_irq_config_##inst(void)					\
	{										\
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority),		\
			    tach_xec_isr, DEVICE_DT_INST_GET(inst), 0);			\
		irq_enable(DT_INST_IRQN(inst));						\
	}										\
											\
	static const struct tach_xec_config tach_xec_config_##inst = {			\
		.base = (uintptr_t)DT_INST_REG_ADDR(inst),				\
		.ctrl_edges = TACH_XEC_CTRL_EDGES(DT_INST_PROP(inst, tach_edges)),	\
		.window_half_periods =							\
			TACH_XEC_HALF_PERIODS(DT_INST_PROP(inst, tach_edges)),		\
		.pulses_per_round = DT_INST_PROP(inst, pulses_per_round),		\
		.pcr_scr = DT_INST_PROP(inst, pcr_scr),					\
		.girq = TACH_XEC_GIRQ(inst),						\
		.girq_pos = TACH_XEC_GIRQ_POS(inst),					\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),				\
		.irq_config_func = tach_xec_irq_config_##inst,				\
	};										\
											\
	PM_DEVICE_DT_INST_DEFINE(inst, tach_xec_pm_action);				\
											\
	SENSOR_DEVICE_DT_INST_DEFINE(inst,						\
				     tach_xec_init,					\
				     PM_DEVICE_DT_INST_GET(inst),			\
				     &tach_xec_data_##inst,				\
				     &tach_xec_config_##inst,				\
				     POST_KERNEL,					\
				     CONFIG_SENSOR_INIT_PRIORITY,			\
				     &tach_xec_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TACH_XEC_DEVICE)
