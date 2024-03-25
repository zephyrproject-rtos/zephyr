/*
 * Copyright (c) 2020 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_npcx_miwu

/**
 * @file
 * @brief Nuvoton NPCX MIWU driver
 *
 * The device Multi-Input Wake-Up Unit (MIWU) supports the Nuvoton embedded
 * controller (EC) to exit 'Sleep' or 'Deep Sleep' power state which allows chip
 * has better power consumption. Also, it provides signal conditioning such as
 * 'Level' and 'Edge' trigger type and grouping of external interrupt sources
 * of NVIC. The NPCX series has three identical MIWU modules: MIWU0, MIWU1,
 * MIWU2. Together, they support a total of over 140 internal and/or external
 * wake-up input (WUI) sources.
 *
 * This driver uses device tree files to present the relationship between
 * MIWU and the other devices in different npcx series. For npcx7 series,
 * it include:
 *  1. npcxn-miwus-wui-map.dtsi: it presents relationship between wake-up inputs
 *     (WUI) and its source device such as gpio, timer, eSPI VWs and so on.
 *  2. npcxn-miwus-int-map.dtsi: it presents relationship between MIWU group
 *     and NVIC interrupt in npcx series. Please notice it isn't 1-to-1 mapping.
 *     For example, here is the mapping between miwu0's group a & d and IRQ7:
 *
 *     map_miwu0_groups: {
 *         parent = <&miwu0>;
 *         group_ad0: group_ad0_map {
 *             irq        = <7>;
 *             group_mask = <0x09>;
 *         };
 *         ...
 *     };
 *
 *     It will connect IRQ 7 and intc_miwu_isr0() with the argument, group_mask,
 *     by IRQ_CONNECT() during driver initialization function. With group_mask,
 *     0x09, the driver checks the pending bits of group a and group d in ISR.
 *     Then it will execute related callback functions if they have been
 *     registered properly.
 *
 * INCLUDE FILES: soc_miwu.h
 *
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <soc.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/irq_nextlevel.h>
#include <zephyr/drivers/gpio.h>

#include "soc_miwu.h"
#include "soc_gpio.h"

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(intc_miwu, LOG_LEVEL_ERR);

/* MIWU module instances */
#define NPCX_MIWU_DEV(inst) DEVICE_DT_INST_GET(inst),

static const struct device *const miwu_devs[] = {
	DT_INST_FOREACH_STATUS_OKAY(NPCX_MIWU_DEV)
};

BUILD_ASSERT(ARRAY_SIZE(miwu_devs) == NPCX_MIWU_TABLE_COUNT,
		"Size of miwu_devs array must equal to NPCX_MIWU_TABLE_COUNT");

/* Driver config */
struct intc_miwu_config {
	/* miwu controller base address */
	uintptr_t base;
	/* index of miwu controller */
	uint8_t index;
};

/* Driver data */
struct intc_miwu_data {
	/* Callback functions list for each MIWU group */
	sys_slist_t cb_list_grp[8];
#ifdef CONFIG_NPCX_MIWU_BOTH_EDGE_TRIG_WORKAROUND
	uint8_t both_edge_pins[8];
	struct k_spinlock lock;
#endif
};

BUILD_ASSERT(sizeof(struct miwu_io_params) == sizeof(gpio_port_pins_t),
	"Size of struct miwu_io_params must equal to struct gpio_port_pins_t");

BUILD_ASSERT(offsetof(struct miwu_callback, io_cb.params) +
	sizeof(struct miwu_io_params) == sizeof(struct gpio_callback),
	"Failed in size check of miwu_callback and gpio_callback structures!");

BUILD_ASSERT(offsetof(struct miwu_callback, io_cb.params.cb_type) ==
	offsetof(struct miwu_callback, dev_cb.params.cb_type),
	"Failed in offset check of cb_type field of miwu_callback structure");

/* MIWU local functions */
static void intc_miwu_dispatch_isr(sys_slist_t *cb_list, uint8_t mask)
{
	struct miwu_callback *cb, *tmp;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(cb_list, cb, tmp, node) {

		if (cb->io_cb.params.cb_type == NPCX_MIWU_CALLBACK_GPIO) {
			if (BIT(cb->io_cb.params.wui.bit) & mask) {
				__ASSERT(cb->io_cb.handler, "No GPIO callback handler!");
				cb->io_cb.handler(
					npcx_get_gpio_dev(cb->io_cb.params.gpio_port),
					(struct gpio_callback *)cb,
					cb->io_cb.params.pin_mask);
			}
		} else {
			if (BIT(cb->dev_cb.params.wui.bit) & mask) {
				__ASSERT(cb->dev_cb.handler, "No device callback handler!");

				cb->dev_cb.handler(cb->dev_cb.params.source,
						   &cb->dev_cb.params.wui);
			}
		}
	}
}

#ifdef CONFIG_NPCX_MIWU_BOTH_EDGE_TRIG_WORKAROUND
static void npcx_miwu_set_pseudo_both_edge(uint8_t table, uint8_t group, uint8_t bit)
{
	const struct intc_miwu_config *config = miwu_devs[table]->config;
	const uint32_t base = config->base;
	uint8_t pmask = BIT(bit);

	if (IS_BIT_SET(NPCX_WKST(base, group), bit)) {
		/* Current signal level is high, set falling edge triger. */
		NPCX_WKEDG(base, group) |= pmask;
	} else {
		/* Current signal level is low, set rising edge triger. */
		NPCX_WKEDG(base, group) &= ~pmask;
	}
}
#endif

static void intc_miwu_isr_pri(int wui_table, int wui_group)
{
	const struct intc_miwu_config *config = miwu_devs[wui_table]->config;
	struct intc_miwu_data *data = miwu_devs[wui_table]->data;
	const uint32_t base = config->base;
	uint8_t mask = NPCX_WKPND(base, wui_group) & NPCX_WKEN(base, wui_group);

#ifdef CONFIG_NPCX_MIWU_BOTH_EDGE_TRIG_WORKAROUND
	uint8_t new_mask = mask;

	while (new_mask != 0) {
		uint8_t pending_bit = find_lsb_set(new_mask) - 1;
		uint8_t pending_mask = BIT(pending_bit);

		NPCX_WKPCL(base, wui_group) = pending_mask;
		if ((data->both_edge_pins[wui_group] & pending_mask) != 0) {
			npcx_miwu_set_pseudo_both_edge(wui_table, wui_group, pending_bit);
		}

		new_mask &= ~pending_mask;
	};
#else
	/* Clear pending bits before dispatch ISR */
	if (mask) {
		NPCX_WKPCL(base, wui_group) = mask;
	}
#endif

	/* Dispatch registered gpio isrs */
	intc_miwu_dispatch_isr(&data->cb_list_grp[wui_group], mask);
}

/* Platform specific MIWU functions */
void npcx_miwu_irq_enable(const struct npcx_wui *wui)
{
	const struct intc_miwu_config *config = miwu_devs[wui->table]->config;
	const uint32_t base = config->base;

#ifdef CONFIG_NPCX_MIWU_BOTH_EDGE_TRIG_WORKAROUND
	k_spinlock_key_t key;
	struct intc_miwu_data *data = miwu_devs[wui->table]->data;

	key = k_spin_lock(&data->lock);
#endif

	NPCX_WKEN(base, wui->group) |= BIT(wui->bit);

#ifdef CONFIG_NPCX_MIWU_BOTH_EDGE_TRIG_WORKAROUND
	if ((data->both_edge_pins[wui->group] & BIT(wui->bit)) != 0) {
		npcx_miwu_set_pseudo_both_edge(wui->table, wui->group, wui->bit);
	}
	k_spin_unlock(&data->lock, key);
#endif
}

void npcx_miwu_irq_disable(const struct npcx_wui *wui)
{
	const struct intc_miwu_config *config = miwu_devs[wui->table]->config;
	const uint32_t base = config->base;

	NPCX_WKEN(base, wui->group) &= ~BIT(wui->bit);
}

void npcx_miwu_io_enable(const struct npcx_wui *wui)
{
	const struct intc_miwu_config *config = miwu_devs[wui->table]->config;
	const uint32_t base = config->base;

	NPCX_WKINEN(base, wui->group) |= BIT(wui->bit);
}

void npcx_miwu_io_disable(const struct npcx_wui *wui)
{
	const struct intc_miwu_config *config = miwu_devs[wui->table]->config;
	const uint32_t base = config->base;

	NPCX_WKINEN(base, wui->group) &= ~BIT(wui->bit);
}

bool npcx_miwu_irq_get_state(const struct npcx_wui *wui)
{
	const struct intc_miwu_config *config = miwu_devs[wui->table]->config;
	const uint32_t base = config->base;

	return IS_BIT_SET(NPCX_WKEN(base, wui->group), wui->bit);
}

bool npcx_miwu_irq_get_and_clear_pending(const struct npcx_wui *wui)
{
	const struct intc_miwu_config *config = miwu_devs[wui->table]->config;
	const uint32_t base = config->base;
#ifdef CONFIG_NPCX_MIWU_BOTH_EDGE_TRIG_WORKAROUND
	k_spinlock_key_t key;
	struct intc_miwu_data *data = miwu_devs[wui->table]->data;
#endif

	bool pending = IS_BIT_SET(NPCX_WKPND(base, wui->group), wui->bit);

	if (pending) {
#ifdef CONFIG_NPCX_MIWU_BOTH_EDGE_TRIG_WORKAROUND
		key = k_spin_lock(&data->lock);

		NPCX_WKPCL(base, wui->group) = BIT(wui->bit);

		if ((data->both_edge_pins[wui->group] & BIT(wui->bit)) != 0) {
			npcx_miwu_set_pseudo_both_edge(wui->table, wui->group, wui->bit);
		}
		k_spin_unlock(&data->lock, key);
#else
		NPCX_WKPCL(base, wui->group) = BIT(wui->bit);
#endif
	}

	return pending;
}

int npcx_miwu_interrupt_configure(const struct npcx_wui *wui,
		enum miwu_int_mode mode, enum miwu_int_trig trig)
{
	const struct intc_miwu_config *config = miwu_devs[wui->table]->config;
	const uint32_t base = config->base;
	uint8_t pmask = BIT(wui->bit);
	int ret = 0;
#ifdef CONFIG_NPCX_MIWU_BOTH_EDGE_TRIG_WORKAROUND
	struct intc_miwu_data *data = miwu_devs[wui->table]->data;
	k_spinlock_key_t key;
#endif

	/* Disable interrupt of wake-up input source before configuring it */
	npcx_miwu_irq_disable(wui);

#ifdef CONFIG_NPCX_MIWU_BOTH_EDGE_TRIG_WORKAROUND
	key = k_spin_lock(&data->lock);
	data->both_edge_pins[wui->group] &= ~BIT(wui->bit);
#endif
	/* Handle interrupt for level trigger */
	if (mode == NPCX_MIWU_MODE_LEVEL) {
		/* Set detection mode to level */
		NPCX_WKMOD(base, wui->group) |= pmask;
		switch (trig) {
		/* Enable interrupting on level high */
		case NPCX_MIWU_TRIG_HIGH:
			NPCX_WKEDG(base, wui->group) &= ~pmask;
			break;
		/* Enable interrupting on level low */
		case NPCX_MIWU_TRIG_LOW:
			NPCX_WKEDG(base, wui->group) |= pmask;
			break;
		default:
			ret = -EINVAL;
			goto early_exit;
		}
	/* Handle interrupt for edge trigger */
	} else {
		/* Set detection mode to edge */
		NPCX_WKMOD(base, wui->group) &= ~pmask;
		switch (trig) {
		/* Handle interrupting on falling edge */
		case NPCX_MIWU_TRIG_LOW:
			NPCX_WKAEDG(base, wui->group) &= ~pmask;
			NPCX_WKEDG(base, wui->group) |= pmask;
			break;
		/* Handle interrupting on rising edge */
		case NPCX_MIWU_TRIG_HIGH:
			NPCX_WKAEDG(base, wui->group) &= ~pmask;
			NPCX_WKEDG(base, wui->group) &= ~pmask;
			break;
		/* Handle interrupting on both edges */
		case NPCX_MIWU_TRIG_BOTH:
#ifdef CONFIG_NPCX_MIWU_BOTH_EDGE_TRIG_WORKAROUND
			NPCX_WKAEDG(base, wui->group) &= ~pmask;
			data->both_edge_pins[wui->group] |= BIT(wui->bit);
#else
			/* Enable any edge */
			NPCX_WKAEDG(base, wui->group) |= pmask;
#endif
			break;
		default:
			ret = -EINVAL;
			goto early_exit;
		}
	}

	/* Enable wake-up input sources */
	NPCX_WKINEN(base, wui->group) |= pmask;

	/*
	 * Clear pending bit since it might be set if WKINEN bit is
	 * changed.
	 */
	NPCX_WKPCL(base, wui->group) |= pmask;

#ifdef CONFIG_NPCX_MIWU_BOTH_EDGE_TRIG_WORKAROUND
	if ((data->both_edge_pins[wui->group] & BIT(wui->bit)) != 0) {
		npcx_miwu_set_pseudo_both_edge(wui->table, wui->group, wui->bit);
	}
#endif

early_exit:
#ifdef CONFIG_NPCX_MIWU_BOTH_EDGE_TRIG_WORKAROUND
	k_spin_unlock(&data->lock, key);
#endif
	return ret;
}

void npcx_miwu_init_gpio_callback(struct miwu_callback *callback,
				const struct npcx_wui *io_wui, int port)
{
	/* Initialize WUI and GPIO settings in unused bits field */
	callback->io_cb.params.wui.table = io_wui->table;
	callback->io_cb.params.wui.bit   = io_wui->bit;
	callback->io_cb.params.gpio_port = port;
	callback->io_cb.params.cb_type = NPCX_MIWU_CALLBACK_GPIO;
	callback->io_cb.params.wui.group = io_wui->group;
}

void npcx_miwu_init_dev_callback(struct miwu_callback *callback,
				const struct npcx_wui *dev_wui,
				miwu_dev_callback_handler_t handler,
				const struct device *source)
{
	/* Initialize WUI and input device settings */
	callback->dev_cb.params.wui.table = dev_wui->table;
	callback->dev_cb.params.wui.group = dev_wui->group;
	callback->dev_cb.params.wui.bit   = dev_wui->bit;
	callback->dev_cb.params.source = source;
	callback->dev_cb.params.cb_type = NPCX_MIWU_CALLBACK_DEV;
	callback->dev_cb.handler = handler;
}

int npcx_miwu_manage_callback(struct miwu_callback *cb, bool set)
{
	struct npcx_wui *wui;
	struct intc_miwu_data *data;
	sys_slist_t *cb_list;

	if (cb->io_cb.params.cb_type == NPCX_MIWU_CALLBACK_GPIO) {
		wui = &cb->io_cb.params.wui;
	} else {
		wui = &cb->dev_cb.params.wui;
	}

	data = miwu_devs[wui->table]->data;
	cb_list = &data->cb_list_grp[wui->group];
	if (!sys_slist_is_empty(cb_list)) {
		if (!sys_slist_find_and_remove(cb_list, &cb->node)) {
			if (!set) {
				return -EINVAL;
			}
		}
	}

	if (set) {
		sys_slist_prepend(cb_list, &cb->node);
	}

	return 0;
}

/* MIWU driver registration */
#define NPCX_MIWU_ISR_FUNC(index) _CONCAT(intc_miwu_isr, index)
#define NPCX_MIWU_INIT_FUNC(inst) _CONCAT(intc_miwu_init, inst)
#define NPCX_MIWU_INIT_FUNC_DECL(inst) \
	static int intc_miwu_init##inst(const struct device *dev)

/* MIWU ISR implementation */
#define NPCX_MIWU_ISR_FUNC_IMPL(inst)                                          \
	static void intc_miwu_isr##inst(void *arg)                             \
	{                                                                      \
		uint8_t grp_mask = (uint32_t)arg;                              \
		int group = 0;                                                 \
									       \
		/* Check all MIWU groups belong to the same irq */             \
		do {                                                           \
			if (grp_mask & 0x01)                                   \
				intc_miwu_isr_pri(inst, group);                \
			group++;                                               \
			grp_mask = grp_mask >> 1;                              \
									       \
		} while (grp_mask != 0);                                       \
	}

/* MIWU init function implementation */
#define NPCX_MIWU_INIT_FUNC_IMPL(inst)                                         \
	static int intc_miwu_init##inst(const struct device *dev)              \
	{                                                                      \
		int i;                                                         \
		const struct intc_miwu_config *config = dev->config;           \
		const uint32_t base = config->base;                            \
									       \
		/* Clear all MIWUs' pending and enable bits of MIWU device */  \
		for (i = 0; i < NPCX_MIWU_GROUP_COUNT; i++) {                  \
			NPCX_WKEN(base, i) = 0;                                \
			NPCX_WKPCL(base, i) = 0xFF;                            \
		}                                                              \
									       \
		/* Config IRQ and MWIU group directly */                       \
		DT_FOREACH_CHILD(NPCX_DT_NODE_FROM_MIWU_MAP(inst),             \
			NPCX_DT_MIWU_IRQ_CONNECT_IMPL_CHILD_FUNC)              \
		return 0;                                                      \
	}                                                                      \

#define NPCX_MIWU_INIT(inst)                                                   \
	NPCX_MIWU_INIT_FUNC_DECL(inst);                                        \
									       \
	static const struct intc_miwu_config miwu_config_##inst = {	       \
		.base = DT_REG_ADDR(DT_NODELABEL(miwu##inst)),                 \
		.index = DT_PROP(DT_NODELABEL(miwu##inst), index),             \
	};                                                                     \
	struct intc_miwu_data miwu_data_##inst;				       \
									       \
	DEVICE_DT_INST_DEFINE(inst,					       \
			    NPCX_MIWU_INIT_FUNC(inst),                         \
			    NULL,					       \
			    &miwu_data_##inst, &miwu_config_##inst,            \
			    PRE_KERNEL_1,                                      \
			    CONFIG_INTC_INIT_PRIORITY, NULL);                  \
									       \
	NPCX_MIWU_ISR_FUNC_IMPL(inst)                                          \
									       \
	NPCX_MIWU_INIT_FUNC_IMPL(inst)

DT_INST_FOREACH_STATUS_OKAY(NPCX_MIWU_INIT)
