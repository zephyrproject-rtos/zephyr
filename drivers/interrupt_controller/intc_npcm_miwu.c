/*
 * Copyright (c) 2026 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_npcm_miwu

/**
 * @file
 * @brief Nuvoton NPCM MIWU driver
 *
 * The Multi-Input Wake-Up Unit (MIWU) lets the Nuvoton NPCM controllers exit
 * the 'Sleep' or 'Deep Sleep' power state. It also provides signal conditioning,
 * such as 'Level' and 'Edge' trigger types, and groups external interrupt
 * sources towards the NVIC. The NPCM series has three identical MIWU modules:
 * MIWU0, MIWU1 and MIWU2. Each module has eight groups of eight wake-up inputs
 * (WUI).
 *
 * The relationship between the MIWU modules and the other devices is described
 * in devicetree:
 *  1. npcm-miwus-wui-map.dtsi maps every wake-up input (WUI) to its source,
 *     such as a GPIO, a timer or an eSPI virtual wire.
 *  2. npcm-miwus-int-map.dtsi maps the inputs of every MIWU group to their NVIC
 *     interrupt. Most groups have one interrupt for all of their inputs; an
 *     entry can also cover a subset of a group ('wui-mask') when the inputs of
 *     a group are routed to different interrupts. A callback can only be
 *     registered on an input that an entry routes to an interrupt.
 *
 * INCLUDE FILES: soc_miwu.h
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>

#include "soc_miwu.h"

/* MIWU multi-registers, 'n' is the group number */
#define NPCM_WKEDG_OFFSET(n)  (0x000U + ((n) * 2U) + (((n) < 5U) ? 0U : 0x1EU))
#define NPCM_WKAEDG_OFFSET(n) (0x001U + ((n) * 2U) + (((n) < 5U) ? 0U : 0x1EU))
#define NPCM_WKPND_OFFSET(n)  (0x00AU + ((n) * 4U) + (((n) < 5U) ? 0U : 0x10U))
#define NPCM_WKPCL_OFFSET(n)  (0x00CU + ((n) * 4U) + (((n) < 5U) ? 0U : 0x10U))
#define NPCM_WKEN_OFFSET(n)   (0x01EU + ((n) * 2U) + (((n) < 5U) ? 0U : 0x12U))
#define NPCM_WKINEN_OFFSET(n) (0x01FU + ((n) * 2U) + (((n) < 5U) ? 0U : 0x12U))
#define NPCM_WKMOD_OFFSET(n)  (0x070U + (n))

#define NPCM_MIWU_REG(base, offset) (*(volatile uint8_t *)((base) + (offset)))

#define NPCM_WKEDG(base, n)  NPCM_MIWU_REG(base, NPCM_WKEDG_OFFSET(n))
#define NPCM_WKAEDG(base, n) NPCM_MIWU_REG(base, NPCM_WKAEDG_OFFSET(n))
#define NPCM_WKPND(base, n)  NPCM_MIWU_REG(base, NPCM_WKPND_OFFSET(n))
#define NPCM_WKPCL(base, n)  NPCM_MIWU_REG(base, NPCM_WKPCL_OFFSET(n))
#define NPCM_WKEN(base, n)   NPCM_MIWU_REG(base, NPCM_WKEN_OFFSET(n))
#define NPCM_WKINEN(base, n) NPCM_MIWU_REG(base, NPCM_WKINEN_OFFSET(n))
#define NPCM_WKMOD(base, n)  NPCM_MIWU_REG(base, NPCM_WKMOD_OFFSET(n))

/*
 * Argument of the group ISR: MIWU table in bits 23-16, inputs routed to the
 * interrupt in bits 15-8, group mask in bits 7-0
 */
#define NPCM_MIWU_ISR_ARG(table, wui_mask, grp_mask)                                               \
	((((uintptr_t)(table)) << 16) | (((uintptr_t)(wui_mask)) << 8) | ((uintptr_t)(grp_mask)))
#define NPCM_MIWU_ISR_ARG_TABLE(arg)    ((uint8_t)(((arg) >> 16) & 0xFFU))
#define NPCM_MIWU_ISR_ARG_WUI_MASK(arg) ((uint8_t)(((arg) >> 8) & 0xFFU))
#define NPCM_MIWU_ISR_ARG_GRP_MASK(arg) ((uint8_t)((arg) & 0xFFU))

/* Node '/npcm-miwus-int-map/map-miwu<table>-groups' of a MIWU instance */
#define NPCM_MIWU_INT_MAP(inst)                                                                    \
	DT_PATH(npcm_miwus_int_map,                                                                \
		UTIL_CAT(UTIL_CAT(map_miwu, DT_INST_PROP(inst, index)), _groups))

/*
 * MIWU module instances, indexed by the 'index' property of each module. A
 * disabled module leaves its slot NULL, which intc_miwu_get_dev() reports.
 */
#define NPCM_MIWU_DEV_ENTRY(inst) [DT_INST_PROP(inst, index)] = DEVICE_DT_INST_GET(inst),

static const struct device *const miwu_devs[NPCM_MIWU_TABLE_COUNT] = {
	DT_INST_FOREACH_STATUS_OKAY(NPCM_MIWU_DEV_ENTRY)};

/* LISTIFY() in NPCM_MIWU_INIT() needs the group count as a literal */
BUILD_ASSERT(NPCM_MIWU_GROUP_COUNT == 8, "NPCM_MIWU_INIT() assumes eight MIWU groups");

/* Driver config */
struct intc_miwu_config {
	/* miwu controller base address */
	uintptr_t base;
	/* Inputs of each group that the interrupt map routes to an interrupt */
	uint8_t routed[NPCM_MIWU_GROUP_COUNT];
	/* Connect and enable the interrupts of all groups of the controller */
	void (*irq_config_func)(void);
};

/* Driver data */
struct intc_miwu_data {
	/* Callback functions list for each MIWU group */
	sys_slist_t cb_list_grp[NPCM_MIWU_GROUP_COUNT];
	/* Serializes the register updates and the callback list changes */
	struct k_spinlock lock;
};

/* MIWU instance of a wake-up input, or NULL if the input has no MIWU mapping */
static const struct device *intc_miwu_get_dev(const struct npcm_wui *wui)
{
	if (wui->table >= NPCM_MIWU_TABLE_COUNT) {
		return NULL;
	}

	return miwu_devs[wui->table];
}

/* Set or clear the bit of a wake-up input in a per-group register */
static void intc_miwu_update_bit(const struct npcm_wui *wui, uint32_t offset, bool set)
{
	const struct device *dev = intc_miwu_get_dev(wui);
	const struct intc_miwu_config *config;
	struct intc_miwu_data *data;
	k_spinlock_key_t key;

	if (dev == NULL) {
		return;
	}

	config = dev->config;
	data = dev->data;

	key = k_spin_lock(&data->lock);
	if (set) {
		NPCM_MIWU_REG(config->base, offset) |= BIT(wui->bit);
	} else {
		NPCM_MIWU_REG(config->base, offset) &= ~BIT(wui->bit);
	}
	k_spin_unlock(&data->lock, key);
}

static bool intc_miwu_trig_is_valid(enum miwu_int_mode mode, enum miwu_int_trig trig)
{
	switch (mode) {
	case NPCM_MIWU_MODE_LEVEL:
		return (trig == NPCM_MIWU_TRIG_LOW) || (trig == NPCM_MIWU_TRIG_HIGH);
	case NPCM_MIWU_MODE_EDGE:
		return (trig == NPCM_MIWU_TRIG_LOW) || (trig == NPCM_MIWU_TRIG_HIGH) ||
		       (trig == NPCM_MIWU_TRIG_BOTH);
	default:
		return false;
	}
}

/* MIWU local functions */
static void intc_miwu_dispatch_isr(sys_slist_t *cb_list, uint8_t mask)
{
	struct miwu_callback *cb, *tmp;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(cb_list, cb, tmp, node) {
		if ((BIT(cb->wui.bit) & mask) != 0U) {
			__ASSERT(cb->handler != NULL, "No callback handler!");
			cb->handler(cb->source, &cb->wui);
		}
	}
}

static void intc_miwu_isr_pri(uint8_t wui_table, uint8_t wui_group, uint8_t wui_mask)
{
	const struct intc_miwu_config *config = miwu_devs[wui_table]->config;
	struct intc_miwu_data *data = miwu_devs[wui_table]->data;
	const uintptr_t base = config->base;
	uint8_t mask = NPCM_WKPND(base, wui_group) & NPCM_WKEN(base, wui_group) & wui_mask;

	/* Clear pending bits before dispatch ISR */
	if (mask != 0U) {
		NPCM_WKPCL(base, wui_group) = mask;
	}

	/* Dispatch the callbacks registered on the pending wake-up inputs */
	intc_miwu_dispatch_isr(&data->cb_list_grp[wui_group], mask);
}

static void intc_miwu_isr(const void *arg)
{
	uint8_t wui_table = NPCM_MIWU_ISR_ARG_TABLE((uintptr_t)arg);
	uint8_t wui_mask = NPCM_MIWU_ISR_ARG_WUI_MASK((uintptr_t)arg);
	uint8_t grp_mask = NPCM_MIWU_ISR_ARG_GRP_MASK((uintptr_t)arg);
	uint8_t wui_group = 0U;

	/* Check all MIWU groups belong to the same irq */
	while (grp_mask != 0U) {
		if ((grp_mask & BIT(0)) != 0U) {
			intc_miwu_isr_pri(wui_table, wui_group, wui_mask);
		}
		wui_group++;
		grp_mask >>= 1;
	}
}

/* Platform specific MIWU functions */
void npcm_miwu_irq_enable(const struct npcm_wui *wui)
{
	intc_miwu_update_bit(wui, NPCM_WKEN_OFFSET(wui->group), true);
}

void npcm_miwu_irq_disable(const struct npcm_wui *wui)
{
	intc_miwu_update_bit(wui, NPCM_WKEN_OFFSET(wui->group), false);
}

void npcm_miwu_io_enable(const struct npcm_wui *wui)
{
	intc_miwu_update_bit(wui, NPCM_WKINEN_OFFSET(wui->group), true);
}

void npcm_miwu_io_disable(const struct npcm_wui *wui)
{
	intc_miwu_update_bit(wui, NPCM_WKINEN_OFFSET(wui->group), false);
}

bool npcm_miwu_irq_get_state(const struct npcm_wui *wui)
{
	const struct device *dev = intc_miwu_get_dev(wui);
	const struct intc_miwu_config *config;

	if (dev == NULL) {
		return false;
	}

	config = dev->config;

	return IS_BIT_SET(NPCM_WKEN(config->base, wui->group), wui->bit);
}

bool npcm_miwu_irq_get_and_clear_pending(const struct npcm_wui *wui)
{
	const struct device *dev = intc_miwu_get_dev(wui);
	const struct intc_miwu_config *config;
	bool pending;

	if (dev == NULL) {
		return false;
	}

	config = dev->config;
	pending = IS_BIT_SET(NPCM_WKPND(config->base, wui->group), wui->bit);

	/* WKPCL is write-1-to-clear, so no read-modify-write is needed */
	if (pending) {
		NPCM_WKPCL(config->base, wui->group) = BIT(wui->bit);
	}

	return pending;
}

int npcm_miwu_interrupt_configure(const struct npcm_wui *wui, enum miwu_int_mode mode,
				  enum miwu_int_trig trig)
{
	const struct device *dev = intc_miwu_get_dev(wui);
	const struct intc_miwu_config *config;
	struct intc_miwu_data *data;
	uint8_t pmask = BIT(wui->bit);
	k_spinlock_key_t key;
	uintptr_t base;

	/* Reject the request before any register is changed */
	if ((dev == NULL) || !intc_miwu_trig_is_valid(mode, trig)) {
		return -EINVAL;
	}

	config = dev->config;
	data = dev->data;
	base = config->base;

	key = k_spin_lock(&data->lock);

	/* Disable interrupt of wake-up input source before configuring it */
	NPCM_WKEN(base, wui->group) &= ~pmask;

	if (mode == NPCM_MIWU_MODE_LEVEL) {
		/* Level detection: WKEDG selects active high (0) or active low (1) */
		NPCM_WKMOD(base, wui->group) |= pmask;
		if (trig == NPCM_MIWU_TRIG_HIGH) {
			NPCM_WKEDG(base, wui->group) &= ~pmask;
		} else {
			NPCM_WKEDG(base, wui->group) |= pmask;
		}
	} else {
		/* Edge detection: WKAEDG selects any edge, else WKEDG selects the edge */
		NPCM_WKMOD(base, wui->group) &= ~pmask;
		if (trig == NPCM_MIWU_TRIG_BOTH) {
			NPCM_WKAEDG(base, wui->group) |= pmask;
		} else {
			NPCM_WKAEDG(base, wui->group) &= ~pmask;
			if (trig == NPCM_MIWU_TRIG_HIGH) {
				NPCM_WKEDG(base, wui->group) &= ~pmask;
			} else {
				NPCM_WKEDG(base, wui->group) |= pmask;
			}
		}
	}

	/* Enable wake-up input sources */
	NPCM_WKINEN(base, wui->group) |= pmask;

	/* Clear pending bit since it might be set if WKINEN bit is changed */
	NPCM_WKPCL(base, wui->group) = pmask;

	k_spin_unlock(&data->lock, key);

	return 0;
}

void npcm_miwu_init_dev_callback(struct miwu_callback *callback, const struct npcm_wui *dev_wui,
				 miwu_dev_callback_handler_t handler, const struct device *source)
{
	callback->wui = *dev_wui;
	callback->source = source;
	callback->handler = handler;
}

int npcm_miwu_manage_callback(struct miwu_callback *cb, bool set)
{
	const struct device *dev = intc_miwu_get_dev(&cb->wui);
	const struct intc_miwu_config *config;
	struct intc_miwu_data *data;
	sys_slist_t *cb_list;
	k_spinlock_key_t key;
	bool removed;

	if (dev == NULL) {
		return -EINVAL;
	}

	config = dev->config;
	data = dev->data;

	/* A callback on an input that no interrupt is routed from would never run */
	if (set && ((config->routed[cb->wui.group] & BIT(cb->wui.bit)) == 0U)) {
		return -ENOTSUP;
	}

	cb_list = &data->cb_list_grp[cb->wui.group];

	key = k_spin_lock(&data->lock);
	removed = sys_slist_find_and_remove(cb_list, &cb->node);
	if (set) {
		sys_slist_prepend(cb_list, &cb->node);
	}
	k_spin_unlock(&data->lock, key);

	return (set || removed) ? 0 : -EINVAL;
}

static int intc_miwu_init(const struct device *dev)
{
	const struct intc_miwu_config *config = dev->config;
	const uintptr_t base = config->base;

	/* Clear all MIWUs' pending and enable bits of MIWU device */
	for (uint8_t group = 0U; group < NPCM_MIWU_GROUP_COUNT; group++) {
		NPCM_WKEN(base, group) = 0U;
		NPCM_WKPCL(base, group) = 0xFFU;
	}

	config->irq_config_func();

	return 0;
}

/* Connect and enable the interrupt of one child of the group-interrupt map */
#define NPCM_MIWU_IRQ_CONNECT(child, inst)                                                         \
	IRQ_CONNECT(DT_PROP(child, irq), DT_PROP(child, irq_prio), intc_miwu_isr,                  \
		    (void *)NPCM_MIWU_ISR_ARG(DT_INST_PROP(inst, index), DT_PROP(child, wui_mask), \
					      DT_PROP(child, group_mask)),                         \
		    0);                                                                            \
	irq_enable(DT_PROP(child, irq));

/* Inputs of 'group' that one child of the group-interrupt map routes */
#define NPCM_MIWU_ROUTED_BY_CHILD(child, group)                                                    \
	| (IS_BIT_SET(DT_PROP(child, group_mask), group) ? DT_PROP(child, wui_mask) : 0)

/* Inputs of 'group' that the group-interrupt map of an instance routes */
#define NPCM_MIWU_ROUTED(group, inst)                                                              \
	(0 DT_FOREACH_CHILD_VARGS(NPCM_MIWU_INT_MAP(inst), NPCM_MIWU_ROUTED_BY_CHILD, group))

#define NPCM_MIWU_INIT(inst)                                                                       \
	static void intc_miwu_irq_config_##inst(void)                                              \
	{                                                                                          \
		DT_FOREACH_CHILD_VARGS(NPCM_MIWU_INT_MAP(inst), NPCM_MIWU_IRQ_CONNECT, inst)       \
	}                                                                                          \
                                                                                                   \
	static const struct intc_miwu_config miwu_config_##inst = {                                \
		.base = DT_INST_REG_ADDR(inst),                                                    \
		.routed = {LISTIFY(8, NPCM_MIWU_ROUTED, (,), inst)},                               \
		.irq_config_func = intc_miwu_irq_config_##inst,                                    \
	};                                                                                         \
                                                                                                   \
	static struct intc_miwu_data miwu_data_##inst;                                             \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, intc_miwu_init, NULL, &miwu_data_##inst, &miwu_config_##inst,  \
			      PRE_KERNEL_1, CONFIG_INTC_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(NPCM_MIWU_INIT)
