/*
 * Copyright 2022-2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_siul2_eirq

#include <soc.h>
#include <zephyr/irq.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/math_extras.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/interrupt_controller/intc_nxp_siul2_eirq.h>

/* SIUL2 External Interrupt Controller registers (offsets from DISR0) */
/* SIUL2 DMA/Interrupt Status Flag */
#define SIUL2_DISR0             0x0
/* SIUL2 DMA/Interrupt Request Enable */
#define SIUL2_DIRER0            0x8
/* SIUL2 DMA/Interrupt Request Select */
#define SIUL2_DIRSR0            0x10
/* SIUL2 Interrupt Rising-Edge Event Enable */
#define SIUL2_IREER0            0x18
/* SIUL2 Interrupt Falling-Edge Event Enable */
#define SIUL2_IFEER0            0x20
/* SIUL2 Interrupt Filter Enable */
#define SIUL2_IFER0             0x28
/* SIUL2 Interrupt Filter Maximum Counter Register */
#define SIUL2_IFMCR(n)          (0x30 + 0x4 * (n))
#define SIUL2_IFMCR_MAXCNT_MASK GENMASK(3, 0)
#define SIUL2_IFMCR_MAXCNT(v)   FIELD_PREP(SIUL2_IFMCR_MAXCNT_MASK, (v))
/* SIUL2 Interrupt Filter Clock Prescaler Register */
#define SIUL2_IFCPR             0xb0
#define SIUL2_IFCPR_IFCP_MASK   GENMASK(3, 0)
#define SIUL2_IFCPR_IFCP(v)     FIELD_PREP(SIUL2_IFCPR_IFCP_MASK, (v))

/* Handy accessors */
#define REG_READ(r)     sys_read32(config->base + (r))
#define REG_WRITE(r, v) sys_write32((v), config->base + (r))

#define GLITCH_FILTER_DISABLED (SIUL2_IFMCR_MAXCNT_MASK + 1)

struct nxp_siul2_eirq_config {
	mem_addr_t base;
	const struct pinctrl_dev_config *pincfg;
	uint8_t filter_clock_prescaler;
	uint8_t max_filter_counter[CONFIG_NXP_SIUL2_EIRQ_EXT_INTERRUPTS_MAX];
};

struct nxp_siul2_eirq_cb {
	nxp_siul2_eirq_callback_t cb;
	uint8_t pin;
	void *data;
};

struct nxp_siul2_eirq_data {
	struct nxp_siul2_eirq_cb *cb;
};

static inline void nxp_siul2_eirq_interrupt_handler(const struct device *dev, uint32_t irq_idx)
{
	const struct nxp_siul2_eirq_config *config = dev->config;
	struct nxp_siul2_eirq_data *data = dev->data;
	uint32_t mask = GENMASK(CONFIG_NXP_SIUL2_EIRQ_EXT_INTERRUPTS_GROUP - 1, 0);
	uint32_t pending;
	uint8_t irq;

	pending = nxp_siul2_eirq_get_pending(dev);
	pending &= mask << (irq_idx * CONFIG_NXP_SIUL2_EIRQ_EXT_INTERRUPTS_GROUP);

	while (pending) {
		mask = LSB_GET(pending);
		irq = u64_count_trailing_zeros(mask);

		/* Clear status flag */
		REG_WRITE(SIUL2_DISR0, REG_READ(SIUL2_DISR0) | mask);

		if (data->cb[irq].cb != NULL) {
			data->cb[irq].cb(data->cb[irq].pin, data->cb[irq].data);
		}

		pending ^= mask;
	}
}

int nxp_siul2_eirq_set_callback(const struct device *dev, uint8_t irq, uint8_t pin,
			      nxp_siul2_eirq_callback_t cb, void *arg)
{
	struct nxp_siul2_eirq_data *data = dev->data;

	__ASSERT_NO_MSG(irq < CONFIG_NXP_SIUL2_EIRQ_EXT_INTERRUPTS_MAX);

	if ((data->cb[irq].cb == cb) && (data->cb[irq].data == arg)) {
		return 0;
	}

	if (data->cb[irq].cb) {
		return -EBUSY;
	}

	data->cb[irq].cb = cb;
	data->cb[irq].pin = pin;
	data->cb[irq].data = arg;

	return 0;
}

void nxp_siul2_eirq_unset_callback(const struct device *dev, uint8_t irq)
{
	struct nxp_siul2_eirq_data *data = dev->data;

	__ASSERT_NO_MSG(irq < CONFIG_NXP_SIUL2_EIRQ_EXT_INTERRUPTS_MAX);

	data->cb[irq].cb = NULL;
	data->cb[irq].pin = 0;
	data->cb[irq].data = NULL;
}

void nxp_siul2_eirq_enable_interrupt(const struct device *dev, uint8_t irq,
				   enum nxp_siul2_eirq_trigger trigger)
{
	const struct nxp_siul2_eirq_config *config = dev->config;
	uint32_t reg_val;

	__ASSERT_NO_MSG(irq < CONFIG_NXP_SIUL2_EIRQ_EXT_INTERRUPTS_MAX);

	/* Configure trigger */
	reg_val = REG_READ(SIUL2_IREER0);
	if ((trigger == NXP_SIUl2_EIRQ_RISING_EDGE) || (trigger == NXP_SIUl2_EIRQ_BOTH_EDGES)) {
		reg_val |= BIT(irq);
	} else {
		reg_val &= ~BIT(irq);
	}
	REG_WRITE(SIUL2_IREER0, reg_val);

	reg_val = REG_READ(SIUL2_IFEER0);
	if ((trigger == NXP_SIUl2_EIRQ_FALLING_EDGE) || (trigger == NXP_SIUl2_EIRQ_BOTH_EDGES)) {
		reg_val |= BIT(irq);
	} else {
		reg_val &= ~BIT(irq);
	}
	REG_WRITE(SIUL2_IFEER0, reg_val);

	/* Clear status flag and unmask interrupt */
	REG_WRITE(SIUL2_DISR0, REG_READ(SIUL2_DISR0) | BIT(irq));
	REG_WRITE(SIUL2_DIRER0, REG_READ(SIUL2_DIRER0) | BIT(irq));
}

void nxp_siul2_eirq_disable_interrupt(const struct device *dev, uint8_t irq)
{
	const struct nxp_siul2_eirq_config *config = dev->config;

	__ASSERT_NO_MSG(irq < CONFIG_NXP_SIUL2_EIRQ_EXT_INTERRUPTS_MAX);

	/* Disable triggers */
	REG_WRITE(SIUL2_IREER0, REG_READ(SIUL2_IREER0) & ~BIT(irq));
	REG_WRITE(SIUL2_IFEER0, REG_READ(SIUL2_IFEER0) & ~BIT(irq));

	/* Clear status flag and mask interrupt */
	REG_WRITE(SIUL2_DISR0, REG_READ(SIUL2_DISR0) | BIT(irq));
	REG_WRITE(SIUL2_DIRER0, REG_READ(SIUL2_DIRER0) & ~BIT(irq));
}

uint32_t nxp_siul2_eirq_get_pending(const struct device *dev)
{
	const struct nxp_siul2_eirq_config *config = dev->config;

	return REG_READ(SIUL2_DISR0) & REG_READ(SIUL2_DIRER0);
}

static int nxp_siul2_eirq_init(const struct device *dev)
{
	const struct nxp_siul2_eirq_config *config = dev->config;
	uint8_t irq;
	int err;

	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err) {
		return err;
	}

	/* Disable triggers, clear status flags and mask all interrupts */
	REG_WRITE(SIUL2_IREER0, 0U);
	REG_WRITE(SIUL2_IFEER0, 0U);
	REG_WRITE(SIUL2_DISR0, 0xffffffff);
	REG_WRITE(SIUL2_DIRER0, 0U);

	/* Select the request type as interrupt */
	REG_WRITE(SIUL2_DIRSR0, 0U);

	/* Configure glitch filters */
	REG_WRITE(SIUL2_IFCPR, SIUL2_IFCPR_IFCP(config->filter_clock_prescaler));

	for (irq = 0; irq < CONFIG_NXP_SIUL2_EIRQ_EXT_INTERRUPTS_MAX; irq++) {
		if (config->max_filter_counter[irq] < GLITCH_FILTER_DISABLED) {
			REG_WRITE(SIUL2_IFMCR(irq),
				  SIUL2_IFMCR_MAXCNT(config->max_filter_counter[irq]));
			REG_WRITE(SIUL2_IFER0, REG_READ(SIUL2_IFER0) | BIT(irq));
		} else {
			REG_WRITE(SIUL2_IFER0, REG_READ(SIUL2_IFER0) & ~BIT(irq));
		}
	}

	return 0;
}

#define NXP_SIUl2_EIRQ_ISR_DEFINE(idx, n)                                                          \
	static void nxp_siul2_eirq_isr##idx##_##n(const struct device *dev)                        \
	{                                                                                          \
		nxp_siul2_eirq_interrupt_handler(dev, idx);                                        \
	}

#define _NXP_SIUl2_EIRQ_IRQ_CONFIG(idx, n)                                                         \
	do {                                                                                       \
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, idx, irq), DT_INST_IRQ_BY_IDX(n, idx, priority), \
			    nxp_siul2_eirq_isr##idx##_##n, DEVICE_DT_INST_GET(n),                  \
			    COND_CODE_1(CONFIG_GIC, (DT_INST_IRQ_BY_IDX(n, idx, flags)), (0)));    \
		irq_enable(DT_INST_IRQ_BY_IDX(n, idx, irq));                                       \
	} while (false);

#define NXP_SIUl2_EIRQ_IRQ_CONFIG(n)                                                               \
	LISTIFY(DT_NUM_IRQS(DT_DRV_INST(n)), _NXP_SIUl2_EIRQ_IRQ_CONFIG, (), n)

#define NXP_SIUl2_EIRQ_FILTER_CONFIG(idx, n)                                                       \
	COND_CODE_1(DT_NODE_EXISTS(DT_INST_CHILD(n, irq_##idx)),                                   \
		    (DT_PROP_OR(DT_INST_CHILD(n, irq_##idx), max_filter_counter,                   \
				GLITCH_FILTER_DISABLED)),                                          \
		    (GLITCH_FILTER_DISABLED))

#define NXP_SIUl2_EIRQ_INIT_DEVICE(n)                                                              \
	LISTIFY(DT_NUM_IRQS(DT_DRV_INST(n)), NXP_SIUl2_EIRQ_ISR_DEFINE, (), n)                     \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static const struct nxp_siul2_eirq_config nxp_siul2_eirq_conf_##n = {                      \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                       \
		.filter_clock_prescaler = DT_INST_PROP_OR(n, filter_prescaler, 0),                 \
		.max_filter_counter = {LISTIFY(CONFIG_NXP_SIUL2_EIRQ_EXT_INTERRUPTS_MAX,           \
					       NXP_SIUl2_EIRQ_FILTER_CONFIG, (,), n)},             \
	};                                                                                         \
	static struct nxp_siul2_eirq_cb                                                            \
	nxp_siul2_eirq_cb_##n[CONFIG_NXP_SIUL2_EIRQ_EXT_INTERRUPTS_MAX];                           \
	static struct nxp_siul2_eirq_data nxp_siul2_eirq_data_##n = {                              \
		.cb = nxp_siul2_eirq_cb_##n,                                                       \
	};                                                                                         \
	static int nxp_siul2_eirq_init_##n(const struct device *dev)                               \
	{                                                                                          \
		int err;                                                                           \
                                                                                                   \
		err = nxp_siul2_eirq_init(dev);                                                    \
		if (err) {                                                                         \
			return err;                                                                \
		}                                                                                  \
                                                                                                   \
		NXP_SIUl2_EIRQ_IRQ_CONFIG(n);                                                      \
                                                                                                   \
		return 0;                                                                          \
	}                                                                                          \
	DEVICE_DT_INST_DEFINE(n, nxp_siul2_eirq_init_##n, NULL, &nxp_siul2_eirq_data_##n,          \
			      &nxp_siul2_eirq_conf_##n, PRE_KERNEL_2, CONFIG_INTC_INIT_PRIORITY,   \
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(NXP_SIUl2_EIRQ_INIT_DEVICE)
