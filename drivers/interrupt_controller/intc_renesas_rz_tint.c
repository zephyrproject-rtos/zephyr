/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rz_tint

#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/interrupt_controller/gic.h>
#include <zephyr/drivers/interrupt_controller/intc_rz_tint.h>
#include <zephyr/dt-bindings/interrupt-controller/arm-gic.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(rz_intc, CONFIG_INTC_LOG_LEVEL);

struct intc_rz_tint_config {
	uint8_t tint;
	uint8_t max_gpioint;
	uint32_t irq;
	uint32_t prio;
};

struct intc_rz_tint_data {
	uint8_t port;
	uint8_t pin;
	uint8_t gpioint;
	enum intc_rz_tint_trigger trigger_type;
	intc_rz_tint_callback_t callback;
	void *callback_data;
};

#define RZ_INTC_BASE   DT_REG_ADDR(DT_NODELABEL(intc))
#define RZ_INTC_TSCR   (RZ_INTC_BASE + DT_REG_ADDR_BY_NAME(DT_NODELABEL(intc), tscr))
#define RZ_INTC_TITSR0 (RZ_INTC_BASE + DT_REG_ADDR_BY_NAME(DT_NODELABEL(intc), titsr0))
#define RZ_INTC_TSSR0  (RZ_INTC_BASE + DT_REG_ADDR_BY_NAME(DT_NODELABEL(intc), tssr0))
#define RZ_INTC_INTSEL (RZ_INTC_BASE + DT_REG_ADDR_BY_NAME(DT_NODELABEL(intc), intsel))

#define REG_TITSR_READ(tint)        sys_read32(RZ_INTC_TITSR0 + ((tint) / 16) * 4)
#define REG_TITSR_WRITE(tint, v)    sys_write32((v), RZ_INTC_TITSR0 + ((tint) / 16) * 4)
#define REG_TITSR_TITSEL_MASK(tint) (BIT_MASK(2) << ((tint % 16) * 2))

#define REG_TSSR_READ(tint)       sys_read32(RZ_INTC_TSSR0 + ((tint) / 4) * 4)
#define REG_TSSR_WRITE(tint, v)   sys_write32((v), RZ_INTC_TSSR0 + ((tint) / 4) * 4)
#define REG_TSSR_TSSEL_MASK(tint) (BIT_MASK(7) << ((tint % 4) * 8))
#define REG_TSSR_TIEN_MASK(tint)  (BIT(7) << ((tint % 4) * 8))

#if defined(CONFIG_RENESAS_RZ_TINT_SUPPORT_STATUS_CLEAR_REG)
/* V2H, V2N */
#define REG_TSCTR_READ(tint)     sys_read32(RZ_INTC_TSCR)
#define REG_TSCLR_WRITE(tint, v) sys_write32((v), RZ_INTC_TSCR + 4)
#define TINT_STATUS_READ(tint)   REG_TSCTR_READ(tint)
#define TINT_STATUS_CLEAR(tint)  REG_TSCLR_WRITE(tint, TINT_STATUS_READ(tint) | BIT(tint))
#else
#define REG_TSCR_READ(tint)     sys_read32(RZ_INTC_TSCR)
#define REG_TSCR_WRITE(tint, v) sys_write32((v), RZ_INTC_TSCR)
#define TINT_STATUS_READ(tint)  REG_TSCR_READ(tint)
#define TINT_STATUS_CLEAR(tint) REG_TSCR_WRITE(tint, TINT_STATUS_READ(tint) & ~BIT(tint))
#endif

#define OFFSET(irq)                   ((irq) - 353 - COND_CODE_1(CONFIG_GIC, (32), (0)))
#define REG_INTSEL_READ(irq)          sys_read32(RZ_INTC_INTSEL + (OFFSET(irq) / 3) * 4)
#define REG_INTSEL_WRITE(irq, v)      sys_write32((v), RZ_INTC_INTSEL + (OFFSET(irq) / 3) * 4)
#define REG_INTSEL_SPIk_SEL_MASK(irq) (BIT_MASK(10) << ((OFFSET(irq) % 3) * 10))

static const uint8_t gpioint_table[] = DT_PROP(DT_NODELABEL(intc), gpioint_table);

static inline void intc_rz_tint_clear_irq_status(const struct device *dev)
{
	const struct intc_rz_tint_config *config = dev->config;
	uint8_t tint = config->tint;

	TINT_STATUS_CLEAR(tint);

	/*
	 * User's manual: Clear Timing of Interrupt Cause
	 * Dummy read is required after write
	 */
	TINT_STATUS_READ(tint);
}

int intc_rz_tint_enable(const struct device *dev)
{
	const struct intc_rz_tint_config *config = dev->config;

	irq_enable(config->irq);

	return 0;
}

int intc_rz_tint_disable(const struct device *dev)
{
	const struct intc_rz_tint_config *config = dev->config;

	irq_disable(config->irq);

	return 0;
}

int intc_rz_tint_set_type(const struct device *dev, enum intc_rz_tint_trigger trig)
{
	const struct intc_rz_tint_config *config = dev->config;
	struct intc_rz_tint_data *data = dev->data;
	uint8_t tint = config->tint;
	uint32_t reg_val = REG_TITSR_READ(tint);
	uint32_t flags = IRQ_TYPE_LEVEL;
	uint32_t set = 0;

	switch (trig) {
	case RZ_TINT_FAILING_EDGE:
		set = 1;
		break;
	case RZ_TINT_RISING_EDGE:
		set = 0;
		break;
	case RZ_TINT_LOW_LEVEL:
		set = 3;
		break;
	case RZ_TINT_HIGH_LEVEL:
		set = 2;
		break;
	case RZ_TINT_BOTH_EDGE:
	default:
		return -ENOTSUP;
	}

	/* Select interrupt type */
	reg_val = (reg_val & ~REG_TITSR_TITSEL_MASK(tint)) |
		  FIELD_PREP(REG_TITSR_TITSEL_MASK(tint), set);
	REG_TITSR_WRITE(tint, reg_val);

	/*
	 * User's manual: Precaution when Changing Interrupt Settings
	 * When changing the TINT interrupt detection method to the edge type,
	 * write 0 to the TSTATn bit of TSCR.
	 */
	if ((trig == RZ_TINT_RISING_EDGE) || (trig == RZ_TINT_FAILING_EDGE)) {
		flags = IRQ_TYPE_EDGE;
		intc_rz_tint_clear_irq_status(dev);
	}

	/* Set interrupt type for GIC, and clear pending interrupt */
#ifdef CONFIG_GIC
	arm_gic_irq_set_priority(config->irq, config->prio, flags);
	arm_gic_irq_clear_pending(config->irq);
#else
	NVIC_ClearPendingIRQ(config->irq);
#endif

	data->trigger_type = trig;

	return 0;
}

void intc_rz_tint_isr(const struct device *dev)
{
	const struct intc_rz_tint_config *config = dev->config;
	struct intc_rz_tint_data *data = dev->data;

	intc_rz_tint_clear_irq_status(dev);

	/* Clear pending interrupt */
#ifdef CONFIG_GIC
	arm_gic_irq_clear_pending(config->irq);
#else
	NVIC_ClearPendingIRQ(config->irq);
#endif

	if (data->callback) {
		data->callback(data->callback_data);
	}
}

static int intc_rz_tint_init(const struct device *dev)
{
	struct intc_rz_tint_data *data = dev->data;

#if defined(CONFIG_RENESAS_RZ_INTC_SELECT_INTERRUPT)
	const struct intc_rz_tint_config *config = dev->config;
	uint8_t tint = config->tint;
	uint32_t irq = config->irq;
	uint32_t reg_val = REG_INTSEL_READ(irq);

	reg_val &= ~REG_INTSEL_SPIk_SEL_MASK(irq);
	reg_val |= FIELD_PREP(REG_INTSEL_SPIk_SEL_MASK(irq), tint);

	REG_INTSEL_WRITE(irq, reg_val);
#endif
	return intc_rz_tint_set_type(dev, data->trigger_type);
};

int intc_rz_tint_connect(const struct device *dev, uint8_t port, uint8_t pin)
{
	const struct intc_rz_tint_config *config = dev->config;
	struct intc_rz_tint_data *data = dev->data;

	uint8_t tint = config->tint;

	/* Map to GPIOINT */
	uint8_t gpioint = gpioint_table[port] + pin;

	if (gpioint > config->max_gpioint) {
		return -EINVAL;
	}

	uint32_t reg_val = REG_TSSR_READ(tint);

	reg_val &= ~(REG_TSSR_TSSEL_MASK(tint) | REG_TSSR_TIEN_MASK(tint));
	reg_val |= FIELD_PREP(REG_TSSR_TSSEL_MASK(tint), gpioint);
	reg_val |= FIELD_PREP(REG_TSSR_TIEN_MASK(tint), 1U);
	REG_TSSR_WRITE(tint, reg_val);

	data->gpioint = gpioint;
	data->port = port;
	data->pin = pin;

	return 0;
}

int intc_rz_tint_set_callback(const struct device *dev, intc_rz_tint_callback_t cb, void *arg)
{
	struct intc_rz_tint_data *data = dev->data;

	data->callback = cb;
	data->callback_data = arg;

	return 0;
}

#define TINT_RZ_IRQ_CONNECT(index, isr)                                                            \
	IRQ_CONNECT(DT_INST_IRQ_BY_IDX(index, 0, irq), DT_INST_IRQ_BY_IDX(index, 0, priority),     \
		    isr, DEVICE_DT_INST_GET(index),                                                \
		    COND_CODE_1(CONFIG_GIC, (DT_INST_IRQ_BY_IDX(index, 0, flags)), (0)));

#define INTC_RZ_TINT_INIT(index)                                                                   \
	static const struct intc_rz_tint_config intc_rz_tint_config##index = {                     \
		.tint = DT_INST_REG_ADDR(index),                                                   \
		.irq = DT_INST_IRQ_BY_IDX(index, 0, irq),                                          \
		.prio = DT_INST_IRQ_BY_IDX(index, 0, priority),                                    \
		.max_gpioint = DT_PROP(DT_INST_PARENT(index), max_gpioint),                        \
	};                                                                                         \
	struct intc_rz_tint_data intc_rz_tint_data##index = {                                      \
		.trigger_type = DT_INST_ENUM_IDX_OR(index, trigger_type, 0),                       \
	};                                                                                         \
	static int intc_rz_tint_init##index(const struct device *dev)                              \
	{                                                                                          \
		TINT_RZ_IRQ_CONNECT(index, intc_rz_tint_isr)                                       \
		return intc_rz_tint_init(dev);                                                     \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(index, intc_rz_tint_init##index, NULL, &intc_rz_tint_data##index,    \
			      &intc_rz_tint_config##index, PRE_KERNEL_2,                           \
			      CONFIG_INTC_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(INTC_RZ_TINT_INIT)
