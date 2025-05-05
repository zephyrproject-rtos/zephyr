/*
 * Copyright (c) 2025 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/counter/ti_am3352_eqep.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ti_eqep);

#define DT_DRV_COMPAT ti_am3352_eqep

#define TI_EQEP_MAX_TOP_VALUE (0xFFFFFFFFU)

struct ti_eqep_regs {
	volatile uint32_t QPOSCNT;  /**< QEP Position Counter Register, offset: 0x00 */
	volatile uint32_t QPOSINIT; /**< Position Counter Initialization Register, offset: 0x04 */
	volatile uint32_t QPOSMAX;  /**< Maximum Position Counter Register, offset: 0x08 */
	volatile uint32_t QPOSCMP;  /**< Position Compare Register, offset: 0x0C */
	volatile uint32_t QPOSILAT; /**< Index Position Latch Register, offset: 0x10 */
	volatile uint32_t QPOSSLAT; /**< Strobe Position Latch Register, offset: 0x14 */
	volatile uint32_t QPOSLAT;  /**< QEP Position Counter Latch Register, offset: 0x18 */
	uint8_t RESERVED_1[0x4];    /**< Reserved, offset: 0x1C - 0x20 */
	volatile uint32_t QUPRD;    /**< QEP Unit Timer Period, offset: 0x20 */
	uint8_t RESERVED_2[0x4];    /**< Reserved, offset: 0x24 - 0x28 */
	volatile uint16_t QDECCTL;  /**< Quadrature Decoder, offset: 0x28 */
	volatile uint16_t QEPCTL;   /**< QEP Control Register, offset: 0x2A */
	volatile uint16_t QCAPCTL;  /**< QEP Capture Control Register, offset: 0x2C */
	volatile uint16_t QPOSCTL;  /**< QEP Position Compare Control Register, offset: 0x2E */
	volatile uint16_t INTEN;    /**< QEP Interrupt Enable Register, offset: 0x30 */
	volatile uint16_t INTFLG;   /**< QEP Interrupt Flag Register, offset: 0x32 */
	volatile uint16_t INTCLR;   /**< QEP Interrupt Clear Register, offset: 0x34 */
	uint8_t RESERVED_3[0x2];    /**< Reserved, offset: 0x36 - 0x38 */
	volatile uint16_t QEPSTS;   /**< QEP Status Register, offset: 0x38 */
	uint8_t RESERVED_4[0x4];    /**< Reserved, offset: 0x3A - 0x3E */
	volatile uint16_t QCTLAT;   /**< QEP Capture Timer Latch Register, offset: 0x3E */
	volatile uint16_t QCPRDLAT; /**< QEP Capture Period Latch Register, offset: 0x40 */
};

/* Quadrature Decoder Control Register */
#define TI_EQEP_QDECCTL_QSRC GENMASK(15, 14)
#define TI_EQEP_QDECCTL_XCR  BIT(11)
#define TI_EQEP_QDECCTL_SWAP BIT(10)

/* QEP Control Register */
#define TI_EQEP_QEPCTL_PCRM     GENMASK(13, 12)
#define TI_EQEP_QEPCTL_PCRM_MAX (0x1)
#define TI_EQEP_QEPCTL_SWI      BIT(7)
#define TI_EQEP_QEPCTL_SEL      BIT(6)
#define TI_EQEP_QEPCTL_IEL      GENMASK(5, 4)
#define TI_EQEP_QEPCTL_QPEN     BIT(3)
#define TI_EQEP_QEPCTL_QCLM     BIT(2)
#define TI_EQEP_QEPCTL_UTE      BIT(1)

/* Position Compare Control Register */
#define TI_EQEP_QPOSCTL_PCSHDW BIT(15)
#define TI_EQEP_QPOSCTL_PCE    BIT(12)

/* QEP Status Register */
#define TI_EQEP_QEPSTS_FIDF BIT(6)
#define TI_EQEP_QEPSTS_QDF  BIT(5)
#define TI_EQEP_QEPSTS_QDLF BIT(4)

/* Capture Control Register */
#define TI_EQEP_QCAPCTL_CEN      BIT(15)
#define TI_EQEP_QCAPCTL_CCPS     GENMASK(6, 4)
#define TI_EQEP_QCAPCTL_CCPS_MAX (0x7)
#define TI_EQEP_QCAPCTL_UPPS     GENMASK(3, 0)
#define TI_EQEP_QCAPCTL_UPPS_MAX (0xB)

enum ti_eqep_int {
	TI_EQEP_INT_GLOB = BIT(0),  /* Global Interrupt */
	TI_EQEP_INT_QDCI = BIT(3),  /* Quadrature Direction Change */
	TI_EQEP_INT_PCUI = BIT(5),  /* Position Counter Underflow */
	TI_EQEP_INT_PCOI = BIT(6),  /* Position Counter Overflow */
	TI_EQEP_INT_PCMI = BIT(8),  /* Position Counter Compare */
	TI_EQEP_INT_SELI = BIT(9),  /* Strobe Event */
	TI_EQEP_INT_IELI = BIT(10), /* Index Event */
	TI_EQEP_INT_UTOI = BIT(11), /* Timeout Event */
};

#define DEV_CFG(dev)  ((const struct ti_eqep_cfg *)(dev)->config)
#define DEV_DATA(dev) ((struct ti_eqep_data *)(dev)->data)
#define DEV_REGS(dev) ((struct ti_eqep_regs *)DEVICE_MMIO_NAMED_GET(dev, mmio))

struct ti_eqep_cfg {
	struct counter_config_info info;

	DEVICE_MMIO_NAMED_ROM(mmio);

	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	const struct pinctrl_dev_config *pcfg;
	void (*irq_config_func)();
	uint32_t clock_frequency;
};

struct ti_eqep_data {
	DEVICE_MMIO_NAMED_RAM(mmio);
	enum ti_eqep_src source;
	counter_top_callback_t top_callback;
	void *top_user_data;
	counter_alarm_callback_t alarm_callback[TI_EQEP_ALARM_CHAN_NUM];
	void *alarm_user_data[TI_EQEP_ALARM_CHAN_NUM];
	uint32_t alarm_guard_period;
};

static bool ti_eqep_is_counting_up(const struct device *dev)
{
	struct ti_eqep_regs *regs = DEV_REGS(dev);
	struct ti_eqep_data *data = DEV_DATA(dev);

	switch (data->source) {
	case TI_EQEP_SRC_UP: {
		return true;
	}
	case TI_EQEP_SRC_DOWN: {
		return false;
	}
	default: {
		return !!(regs->QEPSTS & TI_EQEP_QEPSTS_QDF);
	}
	}
}

static void ti_eqep_reset_counter(const struct device *dev, uint32_t top_value)
{
	struct ti_eqep_regs *regs = DEV_REGS(dev);

	/* set initial value */
	if (ti_eqep_is_counting_up(dev)) {
		regs->QPOSINIT = 0;
	} else {
		regs->QPOSINIT = top_value;
	}

	/* initialize counter */
	regs->QEPCTL |= TI_EQEP_QEPCTL_SWI;
}

static int ti_eqep_start(const struct device *dev)
{
	/* Quadrature Position Counter Enable */
	DEV_REGS(dev)->QEPCTL |= TI_EQEP_QEPCTL_QPEN;

	return 0;
}

static int ti_eqep_stop(const struct device *dev)
{
	/* Quadrature Position Software Reset */
	DEV_REGS(dev)->QEPCTL &= ~TI_EQEP_QEPCTL_QPEN;

	return 0;
}

static int ti_eqep_get_value(const struct device *dev, uint32_t *ticks)
{
	*ticks = DEV_REGS(dev)->QPOSCNT;

	return 0;
}

static uint32_t ti_eqep_get_freq(const struct device *dev)
{
	const struct ti_eqep_cfg *cfg = DEV_CFG(dev);
	uint32_t frequency;
	int ret;

	if (cfg->clock_dev != NULL) {
		ret = clock_control_get_rate(cfg->clock_dev, cfg->clock_subsys, &frequency);

		if (ret == 0) {
			return frequency;
		}
	}

	return cfg->clock_frequency;
}

static uint32_t ti_eqep_get_top_value(const struct device *dev)
{
	return DEV_REGS(dev)->QPOSMAX;
}

static int ti_eqep_set_top_value(const struct device *dev, const struct counter_top_cfg *top_cfg)
{
	struct ti_eqep_regs *regs = DEV_REGS(dev);
	struct ti_eqep_data *data = DEV_DATA(dev);
	bool reset = true;

	data->top_callback = top_cfg->callback;
	data->top_user_data = top_cfg->user_data;

	if (top_cfg->flags & COUNTER_TOP_CFG_DONT_RESET) {
		reset = false;

		if (top_cfg->flags & COUNTER_TOP_CFG_RESET_WHEN_LATE) {
			reset = top_cfg->ticks < regs->QPOSCNT;
		}

		if (reset) {
			return -ETIME;
		}
	}

	regs->QPOSMAX = top_cfg->ticks;
	if (reset) {
		ti_eqep_reset_counter(dev, top_cfg->ticks);
	}

	return 0;
}

static ALWAYS_INLINE bool ti_eqep_is_late(const struct device *dev, uint32_t ticks, uint32_t now,
					  uint32_t top, bool counting_up)
{
	uint32_t guard = DEV_DATA(dev)->alarm_guard_period;

	return ((counting_up && ticks > now && ticks <= (now + guard) % top) ||
		(!counting_up && ticks < now && ticks >= (now + top - guard) % top));
}

int ti_eqep_set_alarm(const struct device *dev, uint8_t chan_id,
		      const struct counter_alarm_cfg *alarm_cfg)
{
	struct ti_eqep_data *data = DEV_DATA(dev);
	struct ti_eqep_regs *regs = DEV_REGS(dev);
	enum ti_eqep_alarm_channel chan = chan_id;
	uint32_t ticks = alarm_cfg->ticks;

	if (alarm_cfg->callback == NULL) {
		return -EINVAL;
	}

	if (data->alarm_callback[chan] != NULL) {
		return -EBUSY;
	}

	if (chan == TI_EQEP_ALARM_CHAN_COMPARE) {
		uint32_t now = regs->QPOSCNT;
		uint32_t top = regs->QPOSMAX;
		bool counting_up = ti_eqep_is_counting_up(dev);

		if (alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) {
			/* ignore when ticks are absolute, late and outside guard */
			if ((alarm_cfg->flags & COUNTER_ALARM_CFG_EXPIRE_WHEN_LATE) &&
			    ti_eqep_is_late(dev, ticks, now, top, counting_up)) {
				return -EBUSY;
			}
		} else if (counting_up) {
			ticks += now;
		} else {
			ticks -= now;
		}
	}

	/* write callback function */
	data->alarm_callback[chan] = alarm_cfg->callback;
	data->alarm_user_data[chan] = alarm_cfg->user_data;

	switch (chan) {
	case TI_EQEP_ALARM_CHAN_COMPARE: {
		/* enable compare interrupt */
		regs->INTEN |= TI_EQEP_INT_PCMI;

		/* disable shadow load */
		regs->QPOSCTL &= ~TI_EQEP_QPOSCTL_PCSHDW;

		/* enable compare */
		regs->QPOSCTL |= TI_EQEP_QPOSCTL_PCE;

		/* write ticks to compare register */
		regs->QPOSCMP = ticks;

		break;
	}
	case TI_EQEP_ALARM_CHAN_STROBE: {
		/* enable strobe event interrupt */
		regs->INTEN |= TI_EQEP_INT_SELI;

		break;
	}
	case TI_EQEP_ALARM_CHAN_INDEX: {
		/* enable index event interrupt */
		regs->INTEN |= TI_EQEP_INT_IELI;

		break;
	}
	case TI_EQEP_ALARM_CHAN_TIMEOUT: {
		/* enable timeout event interrupt */
		regs->INTEN |= TI_EQEP_INT_UTOI;

		/* enable timeout */
		regs->QEPCTL |= TI_EQEP_QEPCTL_UTE;

		/* set ticks as timeout period */
		regs->QUPRD = ticks;

		break;
	}
	default: {
		/* unreachable */
	}
	}

	return 0;
}

static int ti_eqep_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	struct ti_eqep_data *data = DEV_DATA(dev);
	struct ti_eqep_regs *regs = DEV_REGS(dev);
	enum ti_eqep_alarm_channel chan = chan_id;

	data->alarm_callback[chan] = NULL;
	data->alarm_user_data[chan] = NULL;

	switch (chan) {
	case TI_EQEP_ALARM_CHAN_COMPARE: {
		/* disable compare interrupt */
		regs->INTEN &= ~TI_EQEP_INT_PCMI;

		/* disable compare */
		regs->QPOSCTL &= ~TI_EQEP_QPOSCTL_PCE;

		break;
	}
	case TI_EQEP_ALARM_CHAN_STROBE: {
		/* disable strobe event interrupt */
		regs->INTEN &= ~TI_EQEP_INT_SELI;

		break;
	}
	case TI_EQEP_ALARM_CHAN_INDEX: {
		/* disable index event interrupt */
		regs->INTEN &= ~TI_EQEP_INT_IELI;

		break;
	}
	case TI_EQEP_ALARM_CHAN_TIMEOUT: {
		/* disable timeout event interrupt */
		regs->INTEN &= ~TI_EQEP_INT_UTOI;

		/* disable timeout */
		regs->QEPCTL &= ~TI_EQEP_QEPCTL_UTE;

		/* reset timeout period */
		regs->QUPRD = 0;

		break;
	}
	default: {
		/* unreachable */
	}
	}

	return 0;
}

static uint32_t ti_eqep_get_pending_int(const struct device *dev)
{
	return DEV_REGS(dev)->INTFLG;
}

static uint32_t ti_eqep_get_guard_period(const struct device *dev, uint32_t flags)
{
	struct ti_eqep_data *data = DEV_DATA(dev);

	if (flags & COUNTER_GUARD_PERIOD_LATE_TO_SET) {
		return data->alarm_guard_period;
	}

	return 0;
}

static int ti_eqep_set_guard_period(const struct device *dev, uint32_t ticks, uint32_t flags)
{
	struct ti_eqep_data *data = DEV_DATA(dev);

	if (flags & COUNTER_GUARD_PERIOD_LATE_TO_SET) {
		data->alarm_guard_period = ticks;
	}

	return 0;
}

static int ti_eqep_init(const struct device *dev)
{
	const struct ti_eqep_cfg *cfg = DEV_CFG(dev);
	struct ti_eqep_regs *regs;
	int ret;

	DEVICE_MMIO_NAMED_MAP(dev, mmio, K_MEM_CACHE_NONE);
	regs = DEV_REGS(dev);

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Fail to configure pinctrl\n");
		return ret;
	}

	/* irq connect */
	cfg->irq_config_func();

	/* set max value */
	regs->QPOSMAX = cfg->info.max_top_value;

	/* reset counter */
	ti_eqep_reset_counter(dev, cfg->info.max_top_value);

	/* enable overflow/underflow interrupts */
	regs->INTEN |= (TI_EQEP_INT_PCOI | TI_EQEP_INT_PCUI);

	return 0;
}

static void ti_eqep_isr(const struct device *dev)
{
	struct ti_eqep_regs *regs = DEV_REGS(dev);
	struct ti_eqep_data *data = DEV_DATA(dev);
	uint16_t flg = regs->INTFLG;

	if (flg & (TI_EQEP_INT_PCOI | TI_EQEP_INT_PCUI)) {
		if (data->top_callback) {
			data->top_callback(dev, data->top_user_data);
		}

		/* clear overflow/underflow */
		regs->INTCLR |= (TI_EQEP_INT_PCOI | TI_EQEP_INT_PCUI);
	} else if (flg & TI_EQEP_INT_PCMI) {
		enum ti_eqep_alarm_channel chan = TI_EQEP_ALARM_CHAN_COMPARE;
		counter_alarm_callback_t cb = data->alarm_callback[chan];
		void *user_data = data->alarm_user_data[chan];

		data->alarm_callback[chan] = NULL;
		data->alarm_user_data[chan] = NULL;

		/* disable compare */
		regs->QPOSCTL &= ~TI_EQEP_QPOSCTL_PCE;

		if (cb != NULL) {
			cb(dev, chan, regs->QPOSCMP, user_data);
		}

		/* clear compare interrupt */
		regs->INTCLR |= TI_EQEP_INT_PCMI;
	} else if (flg & TI_EQEP_INT_SELI) {
		enum ti_eqep_alarm_channel chan = TI_EQEP_ALARM_CHAN_STROBE;
		counter_alarm_callback_t cb = data->alarm_callback[chan];
		void *user_data = data->alarm_user_data[chan];

		data->alarm_callback[chan] = NULL;
		data->alarm_user_data[chan] = NULL;

		if (cb != NULL) {
			cb(dev, chan, regs->QPOSSLAT, user_data);
		}

		/* clear strobe interrupt */
		regs->INTCLR |= TI_EQEP_INT_SELI;
	} else if (flg & TI_EQEP_INT_IELI) {
		enum ti_eqep_alarm_channel chan = TI_EQEP_ALARM_CHAN_INDEX;
		counter_alarm_callback_t cb = data->alarm_callback[chan];
		void *user_data = data->alarm_user_data[chan];

		data->alarm_callback[chan] = NULL;
		data->alarm_user_data[chan] = NULL;

		if (cb != NULL) {
			cb(dev, chan, regs->QPOSILAT, user_data);
		}

		/* clear index interrupt */
		regs->INTCLR |= TI_EQEP_INT_IELI;
	} else if (flg & TI_EQEP_INT_UTOI) {
		enum ti_eqep_alarm_channel chan = TI_EQEP_ALARM_CHAN_TIMEOUT;
		counter_alarm_callback_t cb = data->alarm_callback[chan];
		void *user_data = data->alarm_user_data[chan];

		data->alarm_callback[chan] = NULL;
		data->alarm_user_data[chan] = NULL;

		if (cb != NULL) {
			cb(dev, chan, regs->QPOSLAT, user_data);
		}

		/* clear timeout interrupt */
		regs->INTCLR |= TI_EQEP_INT_UTOI;
	} else {
		LOG_ERR("unknown interrupt %u encountered, clearing", flg);

		regs->INTCLR |= flg;
	}

	/* clear global interrupt */
	regs->INTCLR |= TI_EQEP_INT_GLOB;
}

static DEVICE_API(counter, ti_eqep_api) = {
	.start = ti_eqep_start,
	.stop = ti_eqep_stop,
	.get_value = ti_eqep_get_value,
	.set_alarm = ti_eqep_set_alarm,
	.cancel_alarm = ti_eqep_cancel_alarm,
	.set_top_value = ti_eqep_set_top_value,
	.get_pending_int = ti_eqep_get_pending_int,
	.get_top_value = ti_eqep_get_top_value,
	.get_guard_period = ti_eqep_get_guard_period,
	.set_guard_period = ti_eqep_set_guard_period,
	.get_freq = ti_eqep_get_freq,
	.is_counting_up = ti_eqep_is_counting_up,
};

int z_impl_ti_eqep_configure_capture(const struct device *dev,
				     const struct ti_eqep_cap_cfg *cap_cfg)
{
	struct ti_eqep_regs *regs = DEV_REGS(dev);

	uint16_t qcapctl = regs->QCAPCTL;

	if (cap_cfg->enable) {
		if (qcapctl & TI_EQEP_QCAPCTL_CEN) {
			LOG_ERR("capture unit must be disabled before changing prescaler");
			return -ENOTSUP;
		}

		if (cap_cfg->clock_prescaler > TI_EQEP_QCAPCTL_CCPS_MAX) {
			LOG_ERR("clock prescaler too large for capture");
			return -EINVAL;
		}

		if (cap_cfg->unit_position_prescaler > TI_EQEP_QCAPCTL_UPPS_MAX) {
			LOG_ERR("unit position prescaler too large for capture");
			return -EINVAL;
		}

		/* enable capture */
		qcapctl &= ~(TI_EQEP_QCAPCTL_CCPS | TI_EQEP_QCAPCTL_UPPS);
		qcapctl |= FIELD_PREP(TI_EQEP_QCAPCTL_CCPS, cap_cfg->clock_prescaler) |
			   FIELD_PREP(TI_EQEP_QCAPCTL_UPPS, cap_cfg->unit_position_prescaler) |
			   TI_EQEP_QCAPCTL_CEN;
	} else {
		/* disable capture */
		qcapctl &= ~TI_EQEP_QCAPCTL_CEN;
	}

	/* write back */
	regs->QCAPCTL = qcapctl;

	return 0;
}

void z_impl_ti_eqep_configure_qep(const struct device *dev, const struct ti_eqep_qep_cfg *qep_cfg)
{
	struct ti_eqep_regs *regs = DEV_REGS(dev);
	uint16_t qepctl = regs->QEPCTL;

	/* configure reset mode and latch conditions */
	qepctl &= ~(TI_EQEP_QEPCTL_PCRM | TI_EQEP_QEPCTL_IEL | TI_EQEP_QEPCTL_SEL |
		    TI_EQEP_QEPCTL_QCLM);
	qepctl |= FIELD_PREP(TI_EQEP_QEPCTL_PCRM, qep_cfg->reset_mode) |
		  FIELD_PREP(TI_EQEP_QEPCTL_IEL, qep_cfg->index_latch) |
		  FIELD_PREP(TI_EQEP_QEPCTL_SEL, qep_cfg->strobe_latch) |
		  FIELD_PREP(TI_EQEP_QEPCTL_QCLM, qep_cfg->capture_latch);

	/* write register */
	regs->QEPCTL = qepctl;
}

void z_impl_ti_eqep_configure_decoder(const struct device *dev,
				      const struct ti_eqep_dec_cfg *dec_cfg)
{
	struct ti_eqep_regs *regs = DEV_REGS(dev);
	struct ti_eqep_data *data = DEV_DATA(dev);
	uint16_t qdecctl = regs->QDECCTL;

	/* set source mode */
	qdecctl &= ~TI_EQEP_QDECCTL_QSRC;
	qdecctl |= FIELD_PREP(TI_EQEP_QDECCTL_QSRC, dec_cfg->source);

	/* set swap bit */
	if (dec_cfg->swap_inputs) {
		qdecctl |= TI_EQEP_QDECCTL_SWAP;
	} else {
		qdecctl &= ~TI_EQEP_QDECCTL_SWAP;
	}

	/* set xcr */
	if (dec_cfg->rising_edge_only) {
		/* rising edge only - 1x */
		qdecctl |= TI_EQEP_QDECCTL_XCR;
	} else {
		/* both rising and falling - 2x */
		qdecctl &= ~TI_EQEP_QDECCTL_XCR;
	}

	/* write back */
	regs->QDECCTL = qdecctl;

	/* save source */
	data->source = dec_cfg->source;

	/* reset counter */
	ti_eqep_reset_counter(dev, regs->QPOSMAX);
}

int z_impl_ti_eqep_get_latched_capture_values(const struct device *dev, uint32_t *timer,
					      uint32_t *period, bool scale)
{
	struct ti_eqep_regs *regs = DEV_REGS(dev);
	uint16_t qcapctl = regs->QCAPCTL;

	if (timer == NULL || period == NULL) {
		return -EINVAL;
	}

	if (!(qcapctl & TI_EQEP_QCAPCTL_CEN)) {
		return -ENOTSUP;
	}

	*timer = regs->QCTLAT;
	*period = regs->QCPRDLAT;

	if (scale) {
		uint16_t ccps = 1 << FIELD_GET(TI_EQEP_QCAPCTL_CCPS, qcapctl);

		*timer *= ccps;
		*period *= ccps;
	}

	return 0;
}

#define TI_EQEP_CLK_CONFIG(n)                                                                      \
	COND_CODE_1(DT_INST_CLOCKS_HAS_NAME(n, fck), (                                             \
			.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_NAME(n, fck)),           \
			.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL_BY_NAME(       \
				n, fck, clk_id),                                                   \
			.clock_frequency = 0                                                       \
		), (                                                                               \
			.clock_dev = NULL,                                                         \
			.clock_subsys = NULL,                                                      \
			.clock_frequency = DT_INST_PROP_OR(n, clock_frequency, 0)                  \
		)                                                                                  \
	)

#define TI_EQEP_INIT(n)                                                                            \
	static void ti_eqep_irq_config_func_##n(void)                                              \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), ti_eqep_isr,                \
			    DEVICE_DT_INST_GET(n), DT_INST_IRQ(n, flags));                         \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}                                                                                          \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
                                                                                                   \
	static struct ti_eqep_cfg ti_eqep_config_##n = {                                           \
		DEVICE_MMIO_NAMED_ROM_INIT(mmio, DT_DRV_INST(n)),                                  \
		.info =                                                                            \
			{                                                                          \
				.max_top_value = TI_EQEP_MAX_TOP_VALUE,                            \
				.channels = TI_EQEP_ALARM_CHAN_NUM,                                \
			},                                                                         \
		TI_EQEP_CLK_CONFIG(n),                                                             \
		.irq_config_func = ti_eqep_irq_config_func_##n,                                    \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
	};                                                                                         \
                                                                                                   \
	static struct ti_eqep_data ti_eqep_data_##n;                                               \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, ti_eqep_init, NULL, &ti_eqep_data_##n, &ti_eqep_config_##n,       \
			      POST_KERNEL, CONFIG_COUNTER_INIT_PRIORITY, &ti_eqep_api);

DT_INST_FOREACH_STATUS_OKAY(TI_EQEP_INIT)

#ifdef CONFIG_USERSPACE
#include <zephyr/internal/syscall_handler.h>

int z_vrfy_ti_eqep_configure_capture(const struct device *dev,
				     const struct ti_eqep_cap_cfg *cap_cfg)
{
	struct ti_eqep_cap_cfg cfg_copy;

	K_OOPS(K_SYSCALL_SPECIFIC_DRIVER(dev, K_OBJ_DRIVER_COUNTER, &ti_eqep_api));
	K_OOPS(k_usermode_from_copy(&cfg_copy, cap_cfg, sizeof(cfg_copy)));

	return z_impl_ti_eqep_configure_capture(dev, (const struct ti_eqep_cap_cfg *)&cfg_copy);
}
#include <zephyr/syscalls/ti_eqep_configure_capture_mrsh.c>

void z_vrfy_ti_eqep_configure_qep(const struct device *dev, const struct ti_eqep_qep_cfg *qep_cfg)
{
	struct ti_eqep_qep_cfg cfg_copy;

	K_OOPS(K_SYSCALL_SPECIFIC_DRIVER(dev, K_OBJ_DRIVER_COUNTER, &ti_eqep_api));
	K_OOPS(k_usermode_from_copy(&cfg_copy, qep_cfg, sizeof(cfg_copy)));

	z_impl_ti_eqep_configure_qep(dev, (const struct ti_eqep_qep_cfg *)&cfg_copy);
}
#include <zephyr/syscalls/ti_eqep_configure_qep_mrsh.c>

void z_vrfy_ti_eqep_configure_decoder(const struct device *dev,
				      const struct ti_eqep_dec_cfg *dec_cfg)
{
	struct ti_eqep_dec_cfg cfg_copy;

	K_OOPS(K_SYSCALL_SPECIFIC_DRIVER(dev, K_OBJ_DRIVER_COUNTER, &ti_eqep_api));
	K_OOPS(k_usermode_from_copy(&cfg_copy, dec_cfg, sizeof(cfg_copy)));

	z_impl_ti_eqep_configure_decoder(dev, (const struct ti_eqep_dec_cfg *)&cfg_copy);
}
#include <zephyr/syscalls/ti_eqep_configure_decoder_mrsh.c>

static int z_vrfy_ti_eqep_get_latched_capture_values(const struct device *dev, uint32_t *timer,
						     uint32_t *period, bool scale)
{
	K_OOPS(K_SYSCALL_SPECIFIC_DRIVER(dev, K_OBJ_DRIVER_COUNTER, &ti_eqep_api));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(timer, sizeof(uint32_t)));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(period, sizeof(uint32_t)));

	z_impl_ti_eqep_get_latched_capture_values(dev, timer, period, scale);
}
#include <zephyr/syscalls/ti_eqep_get_latched_capture_values_mrsh.c>

#endif /* CONFIG_USERSPACE */
