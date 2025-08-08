/*
 * Copyright (C) 2024-2025, Xiaohua Semiconductor Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Driver for External Interrupt controller in HC32 MCUs
 */

#define DT_DRV_COMPAT           xhsc_hc32_extint
#define EXTINT_NODE             DT_INST(0, DT_DRV_COMPAT)

#include <zephyr/device.h>
#include <soc.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/interrupt_controller/intc_hc32.h>
#include <zephyr/irq.h>
#include <errno.h>

#define LOG_LEVEL CONFIG_INTC_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(intc_extint_hc32);


#define INTC_EXTINT_NUM         DT_PROP(EXTINT_NODE, extint_nums)
#define INTC_EXTINT_IRQN_DEF    0xFF

static IRQn_Type extint_irq_table[INTC_EXTINT_NUM] = {[0 ... INTC_EXTINT_NUM - 1] = INTC_EXTINT_IRQN_DEF};

/* wrapper for user callback */
struct hc32_extint_cb {
	hc32_extint_callback_t cb;
	void *user;
};

/* driver data */
struct hc32_extint_data {
	/* callbacks */
	struct hc32_extint_cb cb[INTC_EXTINT_NUM];
#if defined(CONFIG_INTC_EXTINT_USE_SHARE_INTERRUPT)
	uint32_t extint_table;
#endif
};

/**
 * @brief EXTINT ISR handler
 */
static void hc32_extint_isr(const void *arg)
{
	const struct device *dev = DEVICE_DT_GET(EXTINT_NODE);
	struct hc32_extint_data *data = dev->data;

#if defined(CONFIG_INTC_EXTINT_USE_SHARE_INTERRUPT)
	uint32_t ch_idx;

	ARG_UNUSED(arg);
	for (ch_idx = 0; ch_idx < INTC_EXTINT_NUM; ch_idx++) {
		if (SET == EXTINT_GetExtIntStatus(BIT(ch_idx))) {
			EXTINT_ClearExtIntStatus(BIT(ch_idx));
			/* run callback only if one is registered */
			if (data->cb[ch_idx].cb != NULL) {
				data->cb[ch_idx].cb(ch_idx, data->cb[ch_idx].user);
			}
		}
	}
#else
	uint32_t ch = *(uint8_t *)arg;

	if (SET == EXTINT_GetExtIntStatus(BIT(ch))) {
		EXTINT_ClearExtIntStatus(BIT(ch));
		/* run callback only if one is registered */
		if (data->cb[ch].cb != NULL) {
			data->cb[ch].cb(ch, data->cb[ch].user);
		}
	}
#endif
}

/* Configure irq information */
static void hc32_irq_config(uint8_t ch, int irqn, int intsrc)
{
	/* fill irq table */
	extint_irq_table[ch] = (IRQn_Type)irqn;
#if defined(CONFIG_INTC_EXTINT_USE_SHARE_INTERRUPT)
	INTC_ShareIrqCmd(intsrc, ENABLE);
#else
	hc32_intc_irq_signin(irqn, intsrc);
#endif
}

#if defined(CONFIG_INTC_EXTINT_USE_SHARE_INTERRUPT)
#define HC32_EXTINT_INIT_SHARE(node_id, interrupts, idx)        \
	hc32_irq_config(DT_PROP_BY_IDX(node_id, extint_chs, idx),	\
			DT_IRQ_BY_IDX(node_id, idx, irq), 				    \
			DT_IRQ_BY_IDX(node_id, idx, int_src));

#else

#define HC32_EXTINT_INIT(node_id, interrupts, idx)			    \
	static const uint8_t hc32_extint_ch_##idx =                 \
	        DT_PROP_BY_IDX(node_id, extint_chs, idx);           \
	hc32_irq_config(hc32_extint_ch_##idx,			            \
			DT_IRQ_BY_IDX(node_id, idx, irq), 				    \
			DT_IRQ_BY_IDX(node_id, idx, int_src));		        \
	IRQ_CONNECT(DT_IRQ_BY_IDX(node_id, idx, irq),			    \
			DT_IRQ_BY_IDX(node_id, idx, priority),			    \
			hc32_extint_isr, &hc32_extint_ch_##idx,			    \
			0);
#endif

/**
 * @brief initialize intc device driver
 */
static int hc32_intc_init(const struct device *dev)
{
#if defined(CONFIG_INTC_EXTINT_USE_SHARE_INTERRUPT)
	struct hc32_extint_data *data = dev->data;

	data->extint_table = 0;
	DT_FOREACH_PROP_ELEM(EXTINT_NODE, interrupt_names,
			     HC32_EXTINT_INIT_SHARE);
	IRQ_CONNECT(DT_IRQ_BY_IDX(EXTINT_NODE, 0, irq),
		    DT_IRQ_BY_IDX(EXTINT_NODE, 0, priority),
		    hc32_extint_isr, NULL, 0);
#else
	ARG_UNUSED(dev);
	DT_FOREACH_PROP_ELEM(EXTINT_NODE, interrupt_names,
			     HC32_EXTINT_INIT);
#endif

	return 0;
}

static struct hc32_extint_data extint_data;
DEVICE_DT_DEFINE(EXTINT_NODE, &hc32_intc_init,
		 NULL,
		 &extint_data, NULL,
		 PRE_KERNEL_1, CONFIG_INTC_INIT_PRIORITY,
		 NULL);

void hc32_extint_enable(int port, int pin)
{
	int irqnum = 0;
#if defined(CONFIG_INTC_EXTINT_USE_SHARE_INTERRUPT)
	const struct device *const dev = DEVICE_DT_GET(EXTINT_NODE);
	struct hc32_extint_data *data = dev->data;
#endif

	if (pin >= INTC_EXTINT_NUM) {
		__ASSERT_NO_MSG(pin);
	}
	/* Get matching extint irq number from irq_table */
	irqnum = extint_irq_table[pin];
	if (irqnum == INTC_EXTINT_IRQN_DEF) {
		__ASSERT_NO_MSG(pin);
	}

	/* Enable requested pin interrupt */
	GPIO_ExtIntCmd((uint8_t)port, BIT((uint32_t)pin), ENABLE);
#if defined(CONFIG_INTC_EXTINT_USE_SHARE_INTERRUPT)
	if (0 == data->extint_table) {
		/* Enable extint irq interrupt */
		irq_enable(irqnum);
	}
	SET_REG32_BIT(data->extint_table, BIT((uint32_t)pin));
#else
	/* Enable extint irq interrupt */
	irq_enable(irqnum);
#endif
}

void hc32_extint_disable(int port, int pin)
{
	int irqnum = 0;
#if defined(CONFIG_INTC_EXTINT_USE_SHARE_INTERRUPT)
	const struct device *const dev = DEVICE_DT_GET(EXTINT_NODE);
	struct hc32_extint_data *data = dev->data;
#endif

	if (pin >= INTC_EXTINT_NUM) {
		__ASSERT_NO_MSG(pin);
	}
	/* Get matching extint irq number from irq_table */
	irqnum = extint_irq_table[pin];
	if (irqnum == INTC_EXTINT_IRQN_DEF) {
		__ASSERT_NO_MSG(pin);
	}

	/* Disable requested pin interrupt */
	GPIO_ExtIntCmd((uint8_t)port, BIT((uint32_t)pin), DISABLE);
#if defined(CONFIG_INTC_EXTINT_USE_SHARE_INTERRUPT)
	CLR_REG32_BIT(data->extint_table, BIT((uint32_t)pin));
	if (0 == data->extint_table) {
		/* Disable extint irq interrupt */
		irq_disable(irqnum);
	}
#else
	/* Disable extint irq interrupt */
	irq_disable(irqnum);
#endif
}

void hc32_extint_trigger(int pin, int trigger)
{
	stc_extint_init_t stcExtIntInit;

	if (pin >= INTC_EXTINT_NUM) {
		__ASSERT_NO_MSG(pin);
	}

	/* ExtInt config */
	(void)EXTINT_StructInit(&stcExtIntInit);
	switch (trigger) {
	case HC32_EXTINT_TRIG_FALLING:
		stcExtIntInit.u32Edge = EXTINT_TRIG_FALLING;
		break;
	case HC32_EXTINT_TRIG_RISING:
		stcExtIntInit.u32Edge = EXTINT_TRIG_RISING;
		break;
	case HC32_EXTINT_TRIG_BOTH:
		stcExtIntInit.u32Edge = EXTINT_TRIG_BOTH;
		break;
	case HC32_EXTINT_TRIG_LOW_LVL:
		stcExtIntInit.u32Edge = EXTINT_TRIG_LOW;
		break;
	default:
		__ASSERT_NO_MSG(trigger);
		break;
	}
	EXTINT_Init(BIT((uint32_t)pin), &stcExtIntInit);
}

int  hc32_extint_set_callback(int pin, hc32_extint_callback_t cb, void *user)
{
	const struct device *const dev = DEVICE_DT_GET(EXTINT_NODE);
	struct hc32_extint_data *data = dev->data;

	if (pin >= INTC_EXTINT_NUM) {
		__ASSERT_NO_MSG(pin);
	}

	if ((data->cb[pin].cb == cb) && (data->cb[pin].user == user)) {
		return 0;
	}
	/* if callback already exists/maybe-running return busy */
	if (data->cb[pin].cb != NULL) {
		return -EBUSY;
	}
	data->cb[pin].cb   = cb;
	data->cb[pin].user = user;

	return 0;
}

void hc32_extint_unset_callback(int pin)
{
	const struct device *const dev = DEVICE_DT_GET(EXTINT_NODE);
	struct hc32_extint_data *data = dev->data;

	if (pin >= INTC_EXTINT_NUM) {
		__ASSERT_NO_MSG(pin);
	}

	data->cb[pin].cb   = NULL;
	data->cb[pin].user = NULL;
}
