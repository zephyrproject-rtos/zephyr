/*
 * Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_mspm0_comparator

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/comparator.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>

/*
 * COMP register map.
 * CPU_INT and GEN_EVENT share the same interrupt-controller shape.
 */
struct comp_mspm0_int_regs {
	volatile const uint32_t iidx; /**< Interrupt Index Register, offset: 0x00 */
	uint32_t reserved0;           /**< Reserved, offset: 0x04 - 0x08 */
	volatile uint32_t imask;      /**< Interrupt Mask Register, offset: 0x08 */
	uint32_t reserved1;           /**< Reserved, offset: 0x0C - 0x10 */
	volatile const uint32_t ris;  /**< Raw Interrupt Status Register, offset: 0x10 */
	uint32_t reserved2;           /**< Reserved, offset: 0x14 - 0x18 */
	volatile const uint32_t mis;  /**< Masked Interrupt Status Register, offset: 0x18 */
	uint32_t reserved3;           /**< Reserved, offset: 0x1C - 0x20 */
	volatile uint32_t iset;       /**< Interrupt Set Register, offset: 0x20 */
	uint32_t reserved4;           /**< Reserved, offset: 0x24 - 0x28 */
	volatile uint32_t iclr;       /**< Interrupt Clear Register, offset: 0x28 */
};

struct comp_mspm0_gprcm {
	volatile uint32_t pwren;      /**< Power Enable Register, offset: 0x00 */
	volatile uint32_t rstctl;     /**< Reset Control Register, offset: 0x04 */
	volatile uint32_t clkcfg;     /**< Clock Configuration Register, offset: 0x08 */
	uint32_t reserved[2];         /**< Reserved, offset: 0x0C - 0x14 */
	volatile const uint32_t stat; /**< Status Register, offset: 0x14 */
};

struct comp_mspm0_regs {
	uint32_t reserved0[256];              /**< Reserved, offset: 0x000 - 0x400 */
	volatile uint32_t fsub_0;             /**< Subscriber Port 0, offset: 0x400 */
	volatile uint32_t fsub_1;             /**< Subscriber Port 1, offset: 0x404 */
	uint32_t reserved1[15];               /**< Reserved, offset: 0x408 - 0x444 */
	volatile uint32_t fpub_1;             /**< Publisher Port 1, offset: 0x444 */
	uint32_t reserved2[238];              /**< Reserved, offset: 0x448 - 0x800 */
	struct comp_mspm0_gprcm gprcm;        /**< GPRCM Registers, offset: 0x800 */
	uint32_t reserved3[514];              /**< Reserved, offset: 0x818 - 0x1020 */
	struct comp_mspm0_int_regs cpu_int;   /**< CPU Interrupt Registers, offset: 0x1020 */
	uint32_t reserved4;                   /**< Reserved, offset: 0x104C - 0x1050 */
	struct comp_mspm0_int_regs gen_event; /**< General Event Registers, offset: 0x1050 */
	uint32_t reserved5[25];               /**< Reserved, offset: 0x107C - 0x10E0 */
	volatile const uint32_t evt_mode;     /**< Event Mode Register, offset: 0x10E0 */
	uint32_t reserved6[6];                /**< Reserved, offset: 0x10E4 - 0x10FC */
	volatile const uint32_t desc;         /**< Descriptor Register, offset: 0x10FC */
	volatile uint32_t ctl0;               /**< Control 0 Register, offset: 0x1100 */
	volatile uint32_t ctl1;               /**< Control 1 Register, offset: 0x1104 */
	volatile uint32_t ctl2;               /**< Control 2 Register, offset: 0x1108 */
	volatile uint32_t ctl3;               /**< Control 3 Register, offset: 0x110C */
	uint32_t reserved7[4];                /**< Reserved, offset: 0x1110 - 0x1120 */
	volatile const uint32_t stat;         /**< Status Register, offset: 0x1120 */
};

/* pwren bits */
#define COMP_MSPM0_PWREN_ENABLE     BIT(0)
#define COMP_MSPM0_PWREN_KEY        GENMASK(31, 24)
#define COMP_MSPM0_PWREN_KEY_UNLOCK 0x26U

/* ctl0 bits */
#define COMP_MSPM0_CTL0_IPSEL GENMASK(2, 0)
#define COMP_MSPM0_CTL0_IPEN  BIT(15)
#define COMP_MSPM0_CTL0_IMSEL GENMASK(18, 16)
#define COMP_MSPM0_CTL0_IMEN  BIT(31)

/* ctl1 bits */
#define COMP_MSPM0_CTL1_ENABLE         BIT(0)
#define COMP_MSPM0_CTL1_MODE           BIT(1)
#define COMP_MSPM0_CTL1_MODE_FAST      0U
#define COMP_MSPM0_CTL1_MODE_ULP       COMP_MSPM0_CTL1_MODE
#define COMP_MSPM0_CTL1_IES            BIT(4)
#define COMP_MSPM0_CTL1_IES_RISING     0U
#define COMP_MSPM0_CTL1_IES_FALLING    COMP_MSPM0_CTL1_IES
#define COMP_MSPM0_CTL1_HYST           GENMASK(6, 5)
#define COMP_MSPM0_CTL1_HYST_NONE      FIELD_PREP(COMP_MSPM0_CTL1_HYST, 0)
#define COMP_MSPM0_CTL1_HYST_10        FIELD_PREP(COMP_MSPM0_CTL1_HYST, 1)
#define COMP_MSPM0_CTL1_HYST_20        FIELD_PREP(COMP_MSPM0_CTL1_HYST, 2)
#define COMP_MSPM0_CTL1_HYST_30        FIELD_PREP(COMP_MSPM0_CTL1_HYST, 3)
#define COMP_MSPM0_CTL1_OUTPOL         BIT(7)
#define COMP_MSPM0_CTL1_OUTPOL_NON_INV 0U
#define COMP_MSPM0_CTL1_FLTEN          BIT(8)
#define COMP_MSPM0_CTL1_FLTDLY         GENMASK(10, 9)
#define COMP_MSPM0_CTL1_FLTDLY_70      FIELD_PREP(COMP_MSPM0_CTL1_FLTDLY, 0)
#define COMP_MSPM0_CTL1_FLTDLY_500     FIELD_PREP(COMP_MSPM0_CTL1_FLTDLY, 1)
#define COMP_MSPM0_CTL1_FLTDLY_1200    FIELD_PREP(COMP_MSPM0_CTL1_FLTDLY, 2)
#define COMP_MSPM0_CTL1_FLTDLY_2700    FIELD_PREP(COMP_MSPM0_CTL1_FLTDLY, 3)
#define COMP_MSPM0_CTL1_WINCOMPEN      BIT(12)

/* ctl2 bits */
#define COMP_MSPM0_CTL2_REFMODE             BIT(0)
#define COMP_MSPM0_CTL2_REFMODE_STATIC      0U
#define COMP_MSPM0_CTL2_REFSRC              GENMASK(5, 3)
#define COMP_MSPM0_CTL2_REFSRC_NONE         FIELD_PREP(COMP_MSPM0_CTL2_REFSRC, 0)
#define COMP_MSPM0_CTL2_REFSRC_VDDA_DAC     FIELD_PREP(COMP_MSPM0_CTL2_REFSRC, 1)
#define COMP_MSPM0_CTL2_REFSRC_VREF_DAC     FIELD_PREP(COMP_MSPM0_CTL2_REFSRC, 2)
#define COMP_MSPM0_CTL2_REFSRC_VREF         FIELD_PREP(COMP_MSPM0_CTL2_REFSRC, 3)
#define COMP_MSPM0_CTL2_REFSRC_VDDA         FIELD_PREP(COMP_MSPM0_CTL2_REFSRC, 5)
#define COMP_MSPM0_CTL2_REFSRC_INT_VREF_DAC FIELD_PREP(COMP_MSPM0_CTL2_REFSRC, 6)
#define COMP_MSPM0_CTL2_REFSRC_INT_VREF     FIELD_PREP(COMP_MSPM0_CTL2_REFSRC, 7)
#define COMP_MSPM0_CTL2_REFSEL              BIT(7)
#define COMP_MSPM0_CTL2_REFSEL_POS          0U
#define COMP_MSPM0_CTL2_REFSEL_NEG          COMP_MSPM0_CTL2_REFSEL
#define COMP_MSPM0_CTL2_DACCTL              BIT(16)
#define COMP_MSPM0_CTL2_DACCTL_COMP_OUT     0U
#define COMP_MSPM0_CTL2_DACCTL_SW           COMP_MSPM0_CTL2_DACCTL
#define COMP_MSPM0_CTL2_DACSW               BIT(17)
#define COMP_MSPM0_CTL2_DACSW_DACCODE0      0U
#define COMP_MSPM0_CTL2_DACSW_DACCODE1      COMP_MSPM0_CTL2_DACSW

/* ctl3 bits */
#define COMP_MSPM0_CTL3_DACCODE0 GENMASK(7, 0)
#define COMP_MSPM0_CTL3_DACCODE1 GENMASK(23, 16)

/* stat bits */
#define COMP_MSPM0_STAT_OUT BIT(0)

/* cpu_int.imask bits */
#define COMP_MSPM0_INTERRUPT_OUTPUT_EDGE     BIT(1)
#define COMP_MSPM0_INTERRUPT_OUTPUT_EDGE_INV BIT(2)

struct comparator_mspm0_ref_config {
	uint32_t source;
	uint32_t terminal;
	uint32_t dac_control;
	uint32_t dac_input;
	uint8_t dac_code0;
	uint8_t dac_code1;
};

struct comparator_mspm0_config {
	struct comparator_mspm0_ref_config ref_config;
	struct comp_mspm0_regs *regs;
	const struct pinctrl_dev_config *pincfg;
	void (*irq_config_func)(const struct device *dev);
	const struct device *ref;
	uint32_t pos_amux_ch;
	uint32_t neg_amux_ch;
	uint32_t mode;
	uint32_t hysteresis;
	uint32_t filter_delay;
#ifdef CONFIG_COMPARATOR_MSPM0_WINDOW_MODE
	struct comp_mspm0_regs *window_companion_regs;
	uint32_t window_lower_thresh;
	bool window_mode_enable;
#endif
	bool filter_enable;
};

struct comparator_mspm0_data {
	struct k_mutex dev_lock;
	atomic_t trigger_pending;
	void *user_data;
	comparator_callback_t callback;
};

static int comparator_mspm0_get_output(const struct device *dev)
{
	const struct comparator_mspm0_config *config = dev->config;
	struct comparator_mspm0_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->dev_lock, K_FOREVER);
	ret = config->regs->stat & COMP_MSPM0_STAT_OUT;
	k_mutex_unlock(&data->dev_lock);

	return ret;
}

static int comparator_mspm0_set_trigger(const struct device *dev,
					enum comparator_trigger trigger)
{
	const struct comparator_mspm0_config *config = dev->config;
	struct comparator_mspm0_data *data = dev->data;
	uint32_t interrupt_mask = 0;
	int ret = 0;

	k_mutex_lock(&data->dev_lock, K_FOREVER);
	config->regs->cpu_int.imask &=
		~(COMP_MSPM0_INTERRUPT_OUTPUT_EDGE | COMP_MSPM0_INTERRUPT_OUTPUT_EDGE_INV);

	if (trigger == COMPARATOR_TRIGGER_NONE) {
		config->regs->cpu_int.iclr =
			COMP_MSPM0_INTERRUPT_OUTPUT_EDGE | COMP_MSPM0_INTERRUPT_OUTPUT_EDGE_INV;
		atomic_clear(&data->trigger_pending);
		goto out;
	}
#ifdef CONFIG_COMPARATOR_MSPM0_WINDOW_MODE
	if (config->window_mode_enable) {
		config->regs->ctl1 = (config->regs->ctl1 & ~COMP_MSPM0_CTL1_IES) |
				     (COMP_MSPM0_CTL1_IES_RISING | COMP_MSPM0_CTL1_IES_FALLING);

		interrupt_mask =
			COMP_MSPM0_INTERRUPT_OUTPUT_EDGE | COMP_MSPM0_INTERRUPT_OUTPUT_EDGE_INV;

	} else {
#endif
		switch (trigger) {
		case COMPARATOR_TRIGGER_RISING_EDGE:
			config->regs->ctl1 = (config->regs->ctl1 & ~COMP_MSPM0_CTL1_IES) |
					     COMP_MSPM0_CTL1_IES_RISING;
			interrupt_mask = COMP_MSPM0_INTERRUPT_OUTPUT_EDGE;
			break;

		case COMPARATOR_TRIGGER_FALLING_EDGE:
			config->regs->ctl1 = (config->regs->ctl1 & ~COMP_MSPM0_CTL1_IES) |
					     COMP_MSPM0_CTL1_IES_FALLING;
			interrupt_mask = COMP_MSPM0_INTERRUPT_OUTPUT_EDGE;
			break;

		case COMPARATOR_TRIGGER_BOTH_EDGES:
			config->regs->ctl1 =
				(config->regs->ctl1 & ~COMP_MSPM0_CTL1_IES) |
				(COMP_MSPM0_CTL1_IES_RISING | COMP_MSPM0_CTL1_IES_FALLING);
			interrupt_mask = COMP_MSPM0_INTERRUPT_OUTPUT_EDGE |
					 COMP_MSPM0_INTERRUPT_OUTPUT_EDGE_INV;
			break;

		default:
			ret = -EINVAL;
			goto out;
		}
#ifdef CONFIG_COMPARATOR_MSPM0_WINDOW_MODE
	}
#endif

	config->regs->cpu_int.imask |= interrupt_mask;
out:
	k_mutex_unlock(&data->dev_lock);
	return ret;
}

static int comparator_mspm0_set_trigger_callback(const struct device *dev,
						 comparator_callback_t callback,
						 void *user_data)
{
	const struct comparator_mspm0_config *config = dev->config;
	struct comparator_mspm0_data *data = dev->data;
	uint32_t imask;

	k_mutex_lock(&data->dev_lock, K_FOREVER);
	imask = config->regs->cpu_int.imask &
		(COMP_MSPM0_INTERRUPT_OUTPUT_EDGE | COMP_MSPM0_INTERRUPT_OUTPUT_EDGE_INV);
	config->regs->cpu_int.imask &= ~imask;
	data->callback = callback;
	data->user_data = user_data;
	atomic_clear(&data->trigger_pending);
	if (imask != 0) {
		config->regs->cpu_int.imask |= imask;
	}
	k_mutex_unlock(&data->dev_lock);

	return 0;
}

static int comparator_mspm0_trigger_is_pending(const struct device *dev)
{
	struct comparator_mspm0_data *data = dev->data;

	return atomic_cas(&data->trigger_pending, 1, 0) ? 1 : 0;
}

static void comparator_mspm0_isr(const struct device *dev)
{
	const struct comparator_mspm0_config *config = dev->config;
	struct comparator_mspm0_data *data = dev->data;

	if (config->regs->cpu_int.iidx) {
		if (data->callback) {
			data->callback(dev, data->user_data);
		} else {
			atomic_set(&data->trigger_pending, 1);
		}
	}
}

static int comparator_mspm0_init(const struct device *dev)
{
	const struct comparator_mspm0_config *config = dev->config;
	struct comparator_mspm0_data *data = dev->data;

	k_mutex_init(&data->dev_lock);
	config->regs->gprcm.pwren = FIELD_PREP(COMP_MSPM0_PWREN_KEY, COMP_MSPM0_PWREN_KEY_UNLOCK) |
				    COMP_MSPM0_PWREN_ENABLE;
	if ((config->regs->gprcm.pwren & COMP_MSPM0_PWREN_ENABLE) != COMP_MSPM0_PWREN_ENABLE) {
		return -EIO;
	}

	k_busy_wait(k_cyc_to_us_ceil32(CONFIG_MSPM0_PERIPH_STARTUP_DELAY));

	pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);

	/* configure channels */
	config->regs->ctl0 = config->pos_amux_ch | config->neg_amux_ch | COMP_MSPM0_CTL0_IPEN |
			     COMP_MSPM0_CTL0_IMEN;

	/* configure mode, hysteresis and polarity */
	config->regs->ctl1 = config->mode | config->hysteresis | COMP_MSPM0_CTL1_OUTPOL_NON_INV;

	/* configure filter */
	if ((config->mode == COMP_MSPM0_CTL1_MODE_FAST) && config->filter_enable) {
		config->regs->ctl1 |= (COMP_MSPM0_CTL1_FLTEN | config->filter_delay);
	}

	/* configure reference for the dac */
	if (config->ref_config.source != COMP_MSPM0_CTL2_REFSRC_NONE) {
		/*
		 * REFMODE = static or sampled operation
		 * REFSRC = reference for comparator
		 * REFSEL = terminal on which reference is applied
		 * DACCTL = whether comparator output or DACSRC is the mux for DACCODE0/1
		 * DACSW = DACCODE0/1 selection mux
		 */
		config->regs->ctl2 = COMP_MSPM0_CTL2_REFMODE_STATIC | config->ref_config.source |
				     config->ref_config.terminal | config->ref_config.dac_control |
				     config->ref_config.dac_input;

		/* DAC input codes */
		config->regs->ctl3 = ((uint32_t)config->ref_config.dac_code1 << 16) | config->ref_config.dac_code0;

		if (config->ref_config.terminal == COMP_MSPM0_CTL2_REFSEL_NEG) {
			/* neg reference terminal is selected then only enable positive channel
			 * selection */
			config->regs->ctl0 &= ~COMP_MSPM0_CTL0_IMEN;
		} else {
			/* pos reference terminal is selected then only enable negative channel
			 * selection */
			config->regs->ctl0 &= ~COMP_MSPM0_CTL0_IPEN;
		}

#ifdef CONFIG_REGULATOR_MSPM0_VREF
		if (config->ref &&
		    ((config->ref_config.source == COMP_MSPM0_CTL2_REFSRC_INT_VREF_DAC) ||
		     (config->ref_config.source == COMP_MSPM0_CTL2_REFSRC_INT_VREF))) {
			if (regulator_enable(config->ref) < 0) {
				return -ENODEV;
			}
		}
#endif
	}
#ifdef CONFIG_COMPARATOR_MSPM0_WINDOW_MODE
	if (config->window_mode_enable && config->window_companion_regs) {
		config->regs->ctl1 |= COMP_MSPM0_CTL1_WINCOMPEN;

		config->window_companion_regs->ctl1 &= ~COMP_MSPM0_CTL1_WINCOMPEN;
		config->window_companion_regs->ctl0 =
			(config->window_companion_regs->ctl0 & ~COMP_MSPM0_CTL0_IPSEL) |
			FIELD_PREP(COMP_MSPM0_CTL0_IPSEL, 7);
		config->window_companion_regs->ctl0 =
			(config->window_companion_regs->ctl0 & ~COMP_MSPM0_CTL0_IMSEL) |
			config->window_lower_thresh;
		config->window_companion_regs->ctl1 |= COMP_MSPM0_CTL1_ENABLE;
	}
#endif

	config->irq_config_func(dev);

	/* enable comparator */
	config->regs->ctl1 |= COMP_MSPM0_CTL1_ENABLE;

	return 0;
}

static DEVICE_API(comparator, comparator_mspm0_api) = {
	.get_output = comparator_mspm0_get_output,
	.set_trigger = comparator_mspm0_set_trigger,
	.set_trigger_callback = comparator_mspm0_set_trigger_callback,
	.trigger_is_pending = comparator_mspm0_trigger_is_pending,
};

#define COMPARATOR_MSPM0_DEFINE(n)								   \
	PINCTRL_DT_INST_DEFINE(n);								   \
												   \
	static void comparator_mspm0_irq_config_##n(const struct device *dev)			   \
	{											   \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),				   \
			    comparator_mspm0_isr,						   \
			    DEVICE_DT_INST_GET(n), 0);						   \
		irq_enable(DT_INST_IRQN(n));							   \
	}											   \
												   \
	static const struct comparator_mspm0_config						   \
		comparator_mspm0_config_##n = {							   \
		.regs = (struct comp_mspm0_regs *)DT_INST_REG_ADDR(n),				   \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),					   \
		.pos_amux_ch = FIELD_PREP(COMP_MSPM0_CTL0_IPSEL,				   \
					  DT_INST_PROP_OR(n, positive_inputs, 0)),		   \
		.neg_amux_ch = FIELD_PREP(COMP_MSPM0_CTL0_IMSEL,				   \
					  DT_INST_PROP_OR(n, negative_inputs, 0)),		   \
		.mode = _CONCAT(COMP_MSPM0_CTL1_MODE_,						   \
				DT_INST_STRING_UPPER_TOKEN_OR(n, ti_mode, FAST)),		   \
		.hysteresis = _CONCAT(COMP_MSPM0_CTL1_HYST_,					   \
				      DT_INST_STRING_UPPER_TOKEN_OR(n, ti_hysteresis, NONE)),      \
		.ref_config = {									   \
			.source = _CONCAT(COMP_MSPM0_CTL2_REFSRC_, DT_INST_STRING_UPPER_TOKEN_OR(n, \
							       ti_reference_source, NONE)),        \
			.terminal = _CONCAT(COMP_MSPM0_CTL2_REFSEL_,				   \
					    DT_INST_STRING_UPPER_TOKEN_OR(n,			   \
					    ti_reference_terminal, NEG)),			   \
			.dac_code0 = DT_INST_PROP_OR(n, ti_reference_dac_code0, 128),              \
			.dac_code1 = DT_INST_PROP_OR(n, ti_reference_dac_code1, 128),              \
			.dac_control = _CONCAT(COMP_MSPM0_CTL2_DACCTL_,				   \
					       DT_INST_STRING_UPPER_TOKEN_OR(n,                    \
					       ti_reference_dac_control, COMP_OUT)),		   \
			.dac_input = _CONCAT(COMP_MSPM0_CTL2_DACSW_DACCODE,				   \
					     DT_INST_PROP_OR(n, ti_reference_dac_input, 0)),       \
		},										   \
		.filter_enable = DT_INST_PROP_OR(n, ti_filter_enable, false),			   \
		.filter_delay = _CONCAT(COMP_MSPM0_CTL1_FLTDLY_,				   \
					DT_INST_PROP_OR(n, ti_filter_delay, 70)),		   \
		IF_ENABLED(CONFIG_COMPARATOR_MSPM0_WINDOW_MODE, (				   \
			.window_mode_enable = DT_INST_PROP_OR(n, ti_window_mode_enable, false),	   \
			.window_companion_regs = COND_CODE_1(					   \
				DT_INST_NODE_HAS_PROP(n, ti_window_companion),			   \
				((struct comp_mspm0_regs *)DT_REG_ADDR(DT_PHANDLE(DT_DRV_INST(n), \
								     ti_window_companion))),	   \
								     (NULL)),			   \
			.window_lower_thresh = FIELD_PREP(COMP_MSPM0_CTL0_IMSEL,			   \
						       DT_INST_PROP_OR(n,			   \
						       ti_window_lower_threshold, 0)),		   \
		))										   \
		.irq_config_func = comparator_mspm0_irq_config_##n,				   \
		COND_CODE_1(DT_INST_NODE_HAS_PROP(n, vref),					   \
			(.ref = DEVICE_DT_GET(DT_PHANDLE(DT_DRV_INST(n), vref)),),		   \
			(.ref = NULL,))								   \
	};											   \
												   \
	static struct comparator_mspm0_data comparator_mspm0_data_##n;				   \
												   \
	DEVICE_DT_INST_DEFINE(n,								   \
		comparator_mspm0_init,								   \
		NULL,										   \
		&comparator_mspm0_data_##n,							   \
		&comparator_mspm0_config_##n,							   \
		POST_KERNEL,									   \
		CONFIG_COMPARATOR_INIT_PRIORITY,						   \
		&comparator_mspm0_api);

DT_INST_FOREACH_STATUS_OKAY(COMPARATOR_MSPM0_DEFINE)
