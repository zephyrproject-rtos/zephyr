/*
 * SPDX-FileCopyrightText: Copyright Analog Devices, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT arm_armv7_timer_mem_frame

#include <zephyr/kernel.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/sys_io.h>

LOG_MODULE_REGISTER(counter_arm_timer_mem, CONFIG_COUNTER_LOG_LEVEL);

/* CNTControlBase */
#define CNTCR			0x000U
#define CNTCR_EN		BIT(0)

/* CNTCTLBase. CNTACR0 is frame 0; each following frame is 4 bytes on. */
#define CNTACR0			0x040U
#define CNTACR_RPCT		BIT(0)
#define CNTACR_RFRQ		BIT(2)
#define CNTACR_RWPT		BIT(5)
#define CNTACR_PHYS		(CNTACR_RPCT | CNTACR_RFRQ | CNTACR_RWPT)

/* CNTBaseN physical timer */
#define CNTPCT_LO		0x000U
#define CNTPCT_HI		0x004U
#define CNTFRQ			0x010U
#define CNTP_CVAL_LO		0x020U
#define CNTP_CVAL_HI		0x024U
#define CNTP_CTL		0x02CU
#define CNTP_CTL_ENABLE		BIT(0)
#define CNTP_CTL_IMASK		BIT(1)
#define CNTP_CTL_ISTATUS	BIT(2)

#define ARM_TIMER_MEM_MAX_VALUE	UINT32_MAX

#define DEV_CFG(dev)		((const struct arm_timer_mem_config *)(dev)->config)
#define DEV_DATA(dev)		((struct arm_timer_mem_data *)(dev)->data)

struct arm_timer_mem_data {
	DEVICE_MMIO_NAMED_RAM(base);
	DEVICE_MMIO_NAMED_RAM(control);
	DEVICE_MMIO_NAMED_RAM(ctl);
	struct k_spinlock lock;
	counter_alarm_callback_t callback;
#ifdef CONFIG_COUNTER_64BITS_TICKS
	counter_alarm_callback_64_t callback_64;
#endif
	uint64_t start_offset;
	void *user_data;
	uint32_t freq;
	uint32_t guard_period;
	bool alarm_active;
	bool late_pending;
	bool started;
};

struct arm_timer_mem_config {
	struct counter_config_info info;
	DEVICE_MMIO_NAMED_ROM(base);
	DEVICE_MMIO_NAMED_ROM(control);
	DEVICE_MMIO_NAMED_ROM(ctl);
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	uint8_t frame;
	bool has_control;
	unsigned int irqn;
	void (*irq_config)(const struct device *dev);
};

static uint32_t arm_timer_mem_read(mm_reg_t base, uint32_t off)
{
	return sys_read32(base + off);
}

static void arm_timer_mem_write(mm_reg_t base, uint32_t off, uint32_t val)
{
	sys_write32(val, base + off);
}

static uint64_t arm_timer_mem_read_cntpct(mm_reg_t base)
{
	uint32_t hi;
	uint32_t lo;

	do {
		hi = arm_timer_mem_read(base, CNTPCT_HI);
		lo = arm_timer_mem_read(base, CNTPCT_LO);
	} while (hi != arm_timer_mem_read(base, CNTPCT_HI));

	return ((uint64_t)hi << 32U) | lo;
}

static void arm_timer_mem_write_cval(mm_reg_t base, uint64_t cval)
{
	arm_timer_mem_write(base, CNTP_CVAL_LO, (uint32_t)cval);
	arm_timer_mem_write(base, CNTP_CVAL_HI, (uint32_t)(cval >> 32U));
}

static uint64_t arm_timer_mem_read_cval(mm_reg_t base)
{
	uint32_t hi = arm_timer_mem_read(base, CNTP_CVAL_HI);
	uint32_t lo = arm_timer_mem_read(base, CNTP_CVAL_LO);

	return ((uint64_t)hi << 32U) | lo;
}

static void arm_timer_mem_disarm(mm_reg_t base)
{
	arm_timer_mem_write(base, CNTP_CTL, CNTP_CTL_IMASK);
}

static void arm_timer_mem_alarm_drop(struct arm_timer_mem_data *data)
{
	data->callback = NULL;
#ifdef CONFIG_COUNTER_64BITS_TICKS
	data->callback_64 = NULL;
#endif
	data->user_data = NULL;
	data->alarm_active = false;
	data->late_pending = false;
}

/* Hardware count minus this frame's zero. Valid while started is true. */
static uint64_t arm_timer_mem_frame_ticks(const struct arm_timer_mem_data *data, uint64_t hw)
{
	return hw - data->start_offset;
}

static int arm_timer_mem_request_access(mm_reg_t ctl, uint8_t frame)
{
	uint32_t off = CNTACR0 + (4U * frame);
	uint32_t cntacr;

	cntacr = arm_timer_mem_read(ctl, off);
	arm_timer_mem_write(ctl, off, cntacr | CNTACR_PHYS);
	cntacr = arm_timer_mem_read(ctl, off);
	if ((cntacr & (CNTACR_RPCT | CNTACR_RWPT)) != (CNTACR_RPCT | CNTACR_RWPT)) {
		LOG_ERR("CNTACR%u physical access not granted (0x%x)", frame, cntacr);
		return -EACCES;
	}

	return 0;
}

static int arm_timer_mem_start(const struct device *dev)
{
	const struct arm_timer_mem_config *cfg = dev->config;
	struct arm_timer_mem_data *data = dev->data;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);

	if (data->started) {
		k_spin_unlock(&data->lock, key);
		return 0;
	}

	/*
	 * Without CNTControlBase, secure firmware owns the counter enable. Secure
	 * firmware has already enabled the counter by now. An image with
	 * access to CNTControlBase block sets CNTCR.EN here.
	 */
	if (cfg->has_control) {
		mm_reg_t control = DEVICE_MMIO_NAMED_GET(dev, control);
		uint32_t cntcr;

		cntcr = arm_timer_mem_read(control, CNTCR);
		arm_timer_mem_write(control, CNTCR, cntcr | CNTCR_EN);
	}

	data->start_offset = arm_timer_mem_read_cntpct(DEVICE_MMIO_NAMED_GET(dev, base));
	data->started = true;

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int arm_timer_mem_stop(const struct device *dev)
{
	struct arm_timer_mem_data *data = dev->data;
	mm_reg_t base = DEVICE_MMIO_NAMED_GET(dev, base);
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);

	if (!data->started) {
		k_spin_unlock(&data->lock, key);
		return 0;
	}

	/*
	 * CNTCR.EN enables the counter for every frame of this block.
	 * Clearing it would stop the other frames. This frame stops by
	 * disarming its compare. Its reported count stays at 0 until the
	 * next start, which takes a new start_offset.
	 */
	arm_timer_mem_disarm(base);
	arm_timer_mem_alarm_drop(data);

	data->started = false;

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int arm_timer_mem_get_value(const struct device *dev, uint32_t *ticks)
{
	struct arm_timer_mem_data *data = dev->data;
	mm_reg_t base = DEVICE_MMIO_NAMED_GET(dev, base);
	k_spinlock_key_t key;
	uint64_t now64;

	key = k_spin_lock(&data->lock);

	if (!data->started) {
		*ticks = 0U;
		k_spin_unlock(&data->lock, key);
		return 0;
	}

	now64 = arm_timer_mem_frame_ticks(data, arm_timer_mem_read_cntpct(base));
	*ticks = (uint32_t)now64;

	k_spin_unlock(&data->lock, key);

	return 0;
}

#ifdef CONFIG_COUNTER_64BITS_TICKS
static int arm_timer_mem_get_value_64(const struct device *dev, uint64_t *ticks)
{
	struct arm_timer_mem_data *data = dev->data;
	mm_reg_t base = DEVICE_MMIO_NAMED_GET(dev, base);
	k_spinlock_key_t key;
	uint64_t now64;

	key = k_spin_lock(&data->lock);

	if (!data->started) {
		*ticks = 0ULL;
		k_spin_unlock(&data->lock, key);
		return 0;
	}

	now64 = arm_timer_mem_frame_ticks(data, arm_timer_mem_read_cntpct(base));
	*ticks = now64;

	k_spin_unlock(&data->lock, key);

	return 0;
}
#endif

/*
 * Program the compare for one alarm. A late alarm is programmed only
 * when the caller asked for it to fire. The compare is the tick the
 * application asked for, which is what the callback reports.
 */
static void arm_timer_mem_alarm_start(const struct device *dev, uint64_t target64, bool late,
				     bool irq_on_late)
{
	const struct arm_timer_mem_config *cfg = dev->config;
	struct arm_timer_mem_data *data = dev->data;
	mm_reg_t base = DEVICE_MMIO_NAMED_GET(dev, base);

	arm_timer_mem_disarm(base);

	if (late && !irq_on_late) {
		arm_timer_mem_alarm_drop(data);
	} else {

		arm_timer_mem_write_cval(base, target64 + data->start_offset);
		arm_timer_mem_write(base, CNTP_CTL, CNTP_CTL_ENABLE);

		/*
		 * Latch a late alarm. late_pending is clear for a stale
		 * interrupt, so that one is not delivered.
		 */
		if (late) {
			data->late_pending = true;
			k_irq_set_pending(cfg->irqn);
		}
	}
}

static int arm_timer_mem_set_alarm(const struct device *dev, uint8_t chan_id,
				   const struct counter_alarm_cfg *alarm_cfg)
{
	struct arm_timer_mem_data *data = dev->data;
	uint64_t now64;
	uint64_t target64;
	uint32_t ticks = alarm_cfg->ticks;
	uint32_t flags = alarm_cfg->flags;
	bool absolute = (flags & COUNTER_ALARM_CFG_ABSOLUTE) != 0U;
	bool irq_on_late;
	bool late;
	k_spinlock_key_t key;
	int err;

	ARG_UNUSED(chan_id);

	key = k_spin_lock(&data->lock);

	if (!data->started) {
		k_spin_unlock(&data->lock, key);
		return -EINVAL;
	}

	if (data->alarm_active) {
		k_spin_unlock(&data->lock, key);
		return -EBUSY;
	}

	now64 = arm_timer_mem_frame_ticks(data,
				      arm_timer_mem_read_cntpct(DEVICE_MMIO_NAMED_GET(dev, base)));

	if (absolute) {
		irq_on_late = (flags & COUNTER_ALARM_CFG_EXPIRE_WHEN_LATE) != 0U;
		target64 = (now64 & ~((uint64_t)ARM_TIMER_MEM_MAX_VALUE)) | ticks;
		if (target64 <= now64) {
			target64 += ((uint64_t)ARM_TIMER_MEM_MAX_VALUE) + 1U;
		}
	} else {
		irq_on_late = true;
		target64 = now64 + ticks;
	}

	arm_timer_mem_alarm_drop(data);
	data->callback = alarm_cfg->callback;
	data->user_data = alarm_cfg->user_data;
	data->alarm_active = true;

	if (absolute) {
		uint32_t max_rel_val = ARM_TIMER_MEM_MAX_VALUE - data->guard_period;
		uint32_t diff;

		/*
		 * Use the count read above. Reading again is too late on
		 * a slow counter: the tick then looks due only after the
		 * counter wraps, and the alarm is accepted by mistake.
		 * diff == 0 is the current tick. A large diff is a tick
		 * that just passed and is still inside the guard period.
		 */
		diff = (uint32_t)target64 - (uint32_t)now64;
		late = (diff == 0U) || ((diff - 1U) >= max_rel_val);
	} else {
		/*
		 * Read the count again. Within one tick of the target
		 * the alarm is late.
		 */
		now64 = arm_timer_mem_frame_ticks(data, arm_timer_mem_read_cntpct(
							  DEVICE_MMIO_NAMED_GET(dev, base)));
		late = (now64 + 1ULL) >= target64;
	}

	arm_timer_mem_alarm_start(dev, target64, late, irq_on_late);
	err = 0;
	/* An absolute alarm that is already late returns -ETIME. */
	if (late && absolute) {
		err = -ETIME;
	}

	k_spin_unlock(&data->lock, key);

	return err;
}

#ifdef CONFIG_COUNTER_64BITS_TICKS
static int arm_timer_mem_set_alarm_64(const struct device *dev, uint8_t chan_id,
				      const struct counter_alarm_cfg_64 *alarm_cfg)
{
	struct arm_timer_mem_data *data = dev->data;
	uint64_t now64;
	uint64_t target64;
	uint64_t ticks = alarm_cfg->ticks;
	uint32_t flags = alarm_cfg->flags;
	bool absolute = (flags & COUNTER_ALARM_CFG_ABSOLUTE) != 0U;
	bool irq_on_late;
	bool late;
	k_spinlock_key_t key;
	int err;

	ARG_UNUSED(chan_id);

	key = k_spin_lock(&data->lock);

	if (!data->started) {
		k_spin_unlock(&data->lock, key);
		return -EINVAL;
	}

	if (data->alarm_active) {
		k_spin_unlock(&data->lock, key);
		return -EBUSY;
	}

	now64 = arm_timer_mem_frame_ticks(data,
				      arm_timer_mem_read_cntpct(DEVICE_MMIO_NAMED_GET(dev, base)));

	if (absolute) {
		irq_on_late = (flags & COUNTER_ALARM_CFG_EXPIRE_WHEN_LATE) != 0U;
		target64 = ticks;
	} else {
		irq_on_late = true;
		target64 = now64 + ticks;
	}

	arm_timer_mem_alarm_drop(data);
	data->callback_64 = alarm_cfg->callback;
	data->user_data = alarm_cfg->user_data;
	data->alarm_active = true;

	/*
	 * Read the count again. Within one tick of the target the
	 * alarm is late.
	 */
	now64 = arm_timer_mem_frame_ticks(data,
				      arm_timer_mem_read_cntpct(DEVICE_MMIO_NAMED_GET(dev, base)));
	late = (now64 + 1ULL) >= target64;

	arm_timer_mem_alarm_start(dev, target64, late, irq_on_late);
	err = 0;
	/* An absolute alarm that is already late returns -ETIME. */
	if (late && absolute) {
		err = -ETIME;
	}

	k_spin_unlock(&data->lock, key);

	return err;
}
#endif

static int arm_timer_mem_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	struct arm_timer_mem_data *data = dev->data;
	k_spinlock_key_t key;

	ARG_UNUSED(chan_id);

	key = k_spin_lock(&data->lock);
	arm_timer_mem_disarm(DEVICE_MMIO_NAMED_GET(dev, base));
	arm_timer_mem_alarm_drop(data);
	k_spin_unlock(&data->lock, key);

	return 0;
}

static int arm_timer_mem_set_top_value(const struct device *dev,
				       const struct counter_top_cfg *top_cfg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(top_cfg);

	return -ENOTSUP;
}

#ifdef CONFIG_COUNTER_64BITS_TICKS
static int arm_timer_mem_set_top_value_64(const struct device *dev,
					  const struct counter_top_cfg_64 *top_cfg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(top_cfg);

	return -ENOTSUP;
}
#endif

/* ISTATUS is UNKNOWN while ENABLE is clear. Both bits mean the compare is due. */
static bool arm_timer_mem_compare_due(uint32_t ctl)
{
	return (ctl & (CNTP_CTL_ENABLE | CNTP_CTL_ISTATUS)) ==
	       (CNTP_CTL_ENABLE | CNTP_CTL_ISTATUS);
}

static uint32_t arm_timer_mem_get_pending_int(const struct device *dev)
{
	mm_reg_t base = DEVICE_MMIO_NAMED_GET(dev, base);

	return arm_timer_mem_compare_due(arm_timer_mem_read(base, CNTP_CTL)) ? 1U : 0U;
}

static uint32_t arm_timer_mem_get_top_value(const struct device *dev)
{
	const struct arm_timer_mem_config *cfg = dev->config;

	return cfg->info.max_top_value;
}

#ifdef CONFIG_COUNTER_64BITS_TICKS
static uint64_t arm_timer_mem_get_top_value_64(const struct device *dev)
{
	const struct arm_timer_mem_config *cfg = dev->config;

	return cfg->info.max_top_value_64;
}
#endif

static int arm_timer_mem_set_guard_period(const struct device *dev, uint32_t ticks,
					  uint32_t flags)
{
	struct arm_timer_mem_data *data = dev->data;

	if ((flags & ~COUNTER_GUARD_PERIOD_LATE_TO_SET) != 0U) {
		return -ENOTSUP;
	}

	if (ticks >= ARM_TIMER_MEM_MAX_VALUE) {
		return -EINVAL;
	}

	data->guard_period = ticks;

	return 0;
}

static uint32_t arm_timer_mem_get_guard_period(const struct device *dev, uint32_t flags)
{
	struct arm_timer_mem_data *data = dev->data;

	ARG_UNUSED(flags);

	return data->guard_period;
}

static int arm_timer_mem_freq_store(const struct device *dev)
{
	const struct arm_timer_mem_config *cfg = dev->config;
	struct arm_timer_mem_data *data = dev->data;
	uint32_t rate;

	/*
	 * CNTFRQ is the frame's copy of the system counter frequency.
	 * When it reads zero, the clock named by the block has the rate.
	 */
	rate = arm_timer_mem_read(DEVICE_MMIO_NAMED_GET(dev, base), CNTFRQ);
	if (rate == 0U) {
		int err;

		if (cfg->clock_dev == NULL) {
			return -EINVAL;
		}

		err = clock_control_get_rate(cfg->clock_dev, cfg->clock_subsys, &rate);
		if (err != 0) {
			return err;
		}
	}

	if (rate == 0U) {
		return -EINVAL;
	}

	data->freq = rate;

	return 0;
}

static uint32_t arm_timer_mem_get_freq(const struct device *dev)
{
	struct arm_timer_mem_data *data = dev->data;

	return data->freq;
}

static void arm_timer_mem_isr(const struct device *dev)
{
	struct arm_timer_mem_data *data = dev->data;
	mm_reg_t base = DEVICE_MMIO_NAMED_GET(dev, base);
	counter_alarm_callback_t cb;
	void *user_data;
#ifdef CONFIG_COUNTER_64BITS_TICKS
	counter_alarm_callback_64_t cb64;
#endif
	uint32_t ctl;
	uint64_t ticks;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);

	/*
	 * A pending interrupt can outlive cancel_alarm(). Deliver only
	 * an alarm that is still active. ISTATUS means the compare is
	 * due. late_pending is a late alarm latched in the controller.
	 * A stale interrupt has neither, and is ignored.
	 */
	ctl = arm_timer_mem_read(base, CNTP_CTL);
	if (!data->alarm_active ||
	    (!arm_timer_mem_compare_due(ctl) && !data->late_pending)) {
		k_spin_unlock(&data->lock, key);
		return;
	}

	arm_timer_mem_disarm(base);
	ticks = arm_timer_mem_frame_ticks(data, arm_timer_mem_read_cval(base));

	cb = data->callback;
#ifdef CONFIG_COUNTER_64BITS_TICKS
	cb64 = data->callback_64;
#endif
	user_data = data->user_data;
	arm_timer_mem_alarm_drop(data);

	k_spin_unlock(&data->lock, key);

#ifdef CONFIG_COUNTER_64BITS_TICKS
	if (cb64 != NULL) {
		cb64(dev, 0U, ticks, user_data);
		return;
	}
#endif

	if (cb != NULL) {
		cb(dev, 0U, (uint32_t)ticks, user_data);
	}
}

static int arm_timer_mem_init(const struct device *dev)
{
	const struct arm_timer_mem_config *cfg = dev->config;
	int err;

	DEVICE_MMIO_NAMED_MAP(dev, base, K_MEM_CACHE_NONE);
	if (cfg->has_control) {
		DEVICE_MMIO_NAMED_MAP(dev, control, K_MEM_CACHE_NONE);
	}
	DEVICE_MMIO_NAMED_MAP(dev, ctl, K_MEM_CACHE_NONE);

	/*
	 * The clock may be shared with other devices. Leave it on if a
	 * later step fails. A block with no clocks property takes its
	 * rate from CNTFRQ.
	 */
	if (cfg->clock_dev != NULL) {
		if (!device_is_ready(cfg->clock_dev)) {
			LOG_ERR("Clock control device not ready");
			return -ENODEV;
		}

		err = clock_control_on(cfg->clock_dev, cfg->clock_subsys);
		if (err < 0 && err != -EALREADY) {
			LOG_ERR("Failed to enable clock");
			return err;
		}
	}

	err = arm_timer_mem_request_access(DEVICE_MMIO_NAMED_GET(dev, ctl), cfg->frame);
	if (err != 0) {
		return err;
	}

	err = arm_timer_mem_freq_store(dev);
	if (err != 0) {
		LOG_ERR("Failed to get counter frequency");
		return err;
	}

	arm_timer_mem_disarm(DEVICE_MMIO_NAMED_GET(dev, base));
	cfg->irq_config(dev);

	return 0;
}

static DEVICE_API(counter, arm_timer_mem_api) = {
	.start = arm_timer_mem_start,
	.stop = arm_timer_mem_stop,
	.get_value = arm_timer_mem_get_value,
	.set_alarm = arm_timer_mem_set_alarm,
	.cancel_alarm = arm_timer_mem_cancel_alarm,
	.set_top_value = arm_timer_mem_set_top_value,
	.get_pending_int = arm_timer_mem_get_pending_int,
	.get_top_value = arm_timer_mem_get_top_value,
	.get_guard_period = arm_timer_mem_get_guard_period,
	.set_guard_period = arm_timer_mem_set_guard_period,
	.get_freq = arm_timer_mem_get_freq,
#ifdef CONFIG_COUNTER_64BITS_TICKS
	.get_value_64 = arm_timer_mem_get_value_64,
	.set_alarm_64 = arm_timer_mem_set_alarm_64,
	.get_top_value_64 = arm_timer_mem_get_top_value_64,
	.set_top_value_64 = arm_timer_mem_set_top_value_64,
#endif
};

#define ARM_TIMER_MEM_INIT(n)							\
	static void arm_timer_mem_irq_config_##n(const struct device *dev)	\
	{									\
		ARG_UNUSED(dev);						\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),		\
			    arm_timer_mem_isr, DEVICE_DT_INST_GET(n),		\
			    COND_CODE_1(DT_INST_IRQ_HAS_CELL(n, flags),		\
					(DT_INST_IRQ(n, flags)), (0U)));	\
		irq_enable(DT_INST_IRQN(n));					\
	}									\
										\
	static struct arm_timer_mem_data arm_timer_mem_data_##n;		\
										\
	static const struct arm_timer_mem_config arm_timer_mem_config_##n = {	\
		.info = {							\
			COND_CODE_1(CONFIG_COUNTER_64BITS_TICKS,		\
				(.max_top_value_64 = UINT64_MAX,),		\
				(.max_top_value = ARM_TIMER_MEM_MAX_VALUE,))	\
			.flags = COUNTER_CONFIG_INFO_COUNT_UP,			\
			.channels = 1U,						\
		},								\
		DEVICE_MMIO_NAMED_ROM_INIT(base, DT_DRV_INST(n)),		\
		COND_CODE_1(DT_REG_HAS_NAME(DT_INST_PARENT(n), control),	\
			(DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(control,		\
							    DT_INST_PARENT(n)),),\
			(.control = {0},))					\
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(ctl, DT_INST_PARENT(n)),	\
		COND_CODE_1(DT_NODE_HAS_PROP(DT_INST_PARENT(n), clocks),	\
			(.clock_dev = DEVICE_DT_GET(DT_CLOCKS_CTLR(		\
					     DT_INST_PARENT(n))),		\
			 .clock_subsys = (clock_control_subsys_t)		\
				DT_CLOCKS_CELL(DT_INST_PARENT(n), clkid),),	\
			(.clock_dev = NULL,))					\
		.frame = DT_INST_PROP(n, frame_number),				\
		.has_control = DT_REG_HAS_NAME(DT_INST_PARENT(n), control),	\
		.irqn = DT_INST_IRQN(n),					\
		.irq_config = arm_timer_mem_irq_config_##n,			\
	};									\
										\
	DEVICE_DT_INST_DEFINE(n,						\
			      arm_timer_mem_init,				\
			      NULL,						\
			      &arm_timer_mem_data_##n,				\
			      &arm_timer_mem_config_##n,			\
			      POST_KERNEL,					\
			      CONFIG_COUNTER_INIT_PRIORITY,			\
			      &arm_timer_mem_api);

DT_INST_FOREACH_STATUS_OKAY(ARM_TIMER_MEM_INIT)
