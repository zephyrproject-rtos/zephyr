/*
 * Copyright (c) 2023 Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/wakeup_source.h>
#include <zephyr/sys/util.h>
#include <zephyr/devicetree.h>
#include <zephyr/toolchain.h>

#define WAKEUP_SOURCE_VARNAME(node_id, varname)							\
	UTIL_CAT(UTIL_CAT(WAKEUP_SOURCE_NAME(node_id), _), varname)

#define WAKEUP_SOURCE_HAS_WAKEUP_IDS(node_id)							\
	UTIL_AND(										\
		DT_NODE_HAS_PROP(node_id, wakeup_source_ids),					\
		IS_ENABLED(CONFIG_WAKEUP_SOURCE_ID_SUPPORTED)					\
	)

#define WAKEUP_SOURCE_HAS_WAKEUP_IRQ_NUMBERS(node_id)						\
	UTIL_AND(										\
		DT_NUM_IRQS(node_id),								\
		IS_ENABLED(CONFIG_WAKEUP_SOURCE_IRQ_SUPPORTED)					\
	)

#define WAKEUP_SOURCE_HAS_INTERRUPT_GPIOS(node_id)						\
	DT_NODE_HAS_PROP(node_id, interrupt_gpios)

#define WAKEUP_SOURCE_HAS_GPIO_KEYS_GPIOS(node_id)						\
	UTIL_AND(										\
		DT_NODE_HAS_COMPAT(DT_PARENT(node_id), gpio_keys),				\
		DT_NODE_HAS_PROP(node_id, gpios)						\
	)

#define WAKEUP_SOURCE_HAS_WAKEUP_GPIOS(node_id)							\
	UTIL_AND(										\
		UTIL_OR(									\
			WAKEUP_SOURCE_HAS_INTERRUPT_GPIOS(node_id),				\
			WAKEUP_SOURCE_HAS_GPIO_KEYS_GPIOS(node_id)				\
		),										\
		IS_ENABLED(CONFIG_WAKEUP_SOURCE_GPIO_SUPPORTED)					\
	)

#define WAKEUP_SOURCE_COND_HAS_WAKEUP_IDS(node_id, fn)						\
	COND_CODE_1(WAKEUP_SOURCE_HAS_WAKEUP_IDS(node_id), (fn(node_id)), ())

#define WAKEUP_SOURCE_COND_HAS_WAKEUP_IRQ_NUMBERS(node_id, fn)					\
	COND_CODE_1(WAKEUP_SOURCE_HAS_WAKEUP_IRQ_NUMBERS(node_id), (fn(node_id)), ())

#define WAKEUP_SOURCE_COND_HAS_INTERRUPT_GPIOS(node_id, fn)					\
	COND_CODE_1(WAKEUP_SOURCE_HAS_INTERRUPT_GPIOS(node_id), (fn(node_id)), ())

#define WAKEUP_SOURCE_COND_HAS_GPIO_KEYS_GPIOS(node_id, fn)					\
	COND_CODE_1(WAKEUP_SOURCE_HAS_GPIO_KEYS_GPIOS(node_id), (fn(node_id)), ())

#define WAKEUP_SOURCE_COND_HAS_WAKEUP_GPIOS(node_id, fn)					\
	COND_CODE_1(WAKEUP_SOURCE_HAS_WAKEUP_GPIOS(node_id), (fn(node_id)), ())

#define WAKEUP_SOURCE_DEFINE_WAKEUP_IDS(node_id)						\
	static const uint16_t WAKEUP_SOURCE_VARNAME(node_id, wakeup_ids)[] =			\
		DT_PROP(node_id, wakeup_source_ids);

#define WAKEUP_SOURCE_DEFINE_WAKEUP_IRQ_NUMBER_BY_IDX(idx, node_id)				\
	DT_IRQ_BY_IDX(node_id, idx, irq)

#define WAKEUP_SOURCE_DEFINE_WAKEUP_IRQ_NUMBERS(node_id)					\
	static const uint16_t WAKEUP_SOURCE_VARNAME(node_id, wakeup_irq_numbers)[] = {		\
		LISTIFY(									\
			DT_NUM_IRQS(node_id),							\
			WAKEUP_SOURCE_DEFINE_WAKEUP_IRQ_NUMBER_BY_IDX,				\
			(,),									\
			node_id									\
		)										\
	};

#define WAKEUP_SOURCE_DEFINE_INTERRUPT_GPIOS_BY_IDX(idx, node_id)				\
	GPIO_DT_SPEC_GET_BY_IDX(node_id, interrupt_gpios, idx)

#define WAKEUP_SOURCE_DEFINE_INTERRUPT_GPIOS(node_id)						\
	LISTIFY(										\
		DT_PROP_LEN(node_id, interrupt_gpios),						\
		WAKEUP_SOURCE_DEFINE_INTERRUPT_GPIOS_BY_IDX,					\
		(,),										\
		node_id										\
	),

#define WAKEUP_SOURCE_DEFINE_GPIO_KEYS_GPIOS(node_id)						\
	GPIO_DT_SPEC_GET(node_id, gpios),

#define WAKEUP_SOURCE_DEFINE_WAKEUP_GPIOS(node_id)						\
	static const struct gpio_dt_spec WAKEUP_SOURCE_VARNAME(node_id, wakeup_gpios)[] = {	\
		WAKEUP_SOURCE_COND_HAS_INTERRUPT_GPIOS(						\
			node_id,								\
			WAKEUP_SOURCE_DEFINE_INTERRUPT_GPIOS					\
		)										\
		WAKEUP_SOURCE_COND_HAS_GPIO_KEYS_GPIOS(						\
			node_id,								\
			WAKEUP_SOURCE_DEFINE_GPIO_KEYS_GPIOS					\
		)										\
	};

#define WAKEUP_SOURCE_DEFINE_FLAGS(node_id)							\
	static struct wakeup_source_flags WAKEUP_SOURCE_VARNAME(node_id, flags);

#define WAKEUP_SOURCE_ASSIGN_WAKEUP_IDS(node_id)						\
	.wakeup_ids = WAKEUP_SOURCE_VARNAME(node_id, wakeup_ids),				\
	.wakeup_ids_size = ARRAY_SIZE(WAKEUP_SOURCE_VARNAME(node_id, wakeup_ids)),

#define WAKEUP_SOURCE_ASSIGN_WAKEUP_IRQ_NUMBERS(node_id)					\
	.wakeup_irq_numbers = WAKEUP_SOURCE_VARNAME(node_id, wakeup_irq_numbers),		\
	.wakeup_irq_numbers_size =								\
		ARRAY_SIZE(WAKEUP_SOURCE_VARNAME(node_id, wakeup_irq_numbers)),

#define WAKEUP_SOURCE_ASSIGN_WAKEUP_GPIOS(node_id)						\
	.wakeup_gpios = WAKEUP_SOURCE_VARNAME(node_id, wakeup_gpios),				\
	.wakeup_gpios_size = ARRAY_SIZE(WAKEUP_SOURCE_VARNAME(node_id, wakeup_gpios)),

#define WAKEUP_SOURCE_ASSIGN_FLAGS(node_id)							\
	.flags = &WAKEUP_SOURCE_VARNAME(node_id, flags),

#define WAKEUP_SOURCE_ASSIGN_NAME(node_id)							\
	.name = DT_NODE_FULL_NAME(node_id),

#define WAKEUP_SOURCE_DEFINE_WAKEUP_SOURCE(node_id)						\
	const STRUCT_SECTION_ITERABLE(wakeup_source, WAKEUP_SOURCE_NAME(node_id)) = {		\
		WAKEUP_SOURCE_COND_HAS_WAKEUP_IDS(						\
			node_id,								\
			WAKEUP_SOURCE_ASSIGN_WAKEUP_IDS						\
		)										\
		WAKEUP_SOURCE_COND_HAS_WAKEUP_IRQ_NUMBERS(					\
			node_id,								\
			WAKEUP_SOURCE_ASSIGN_WAKEUP_IRQ_NUMBERS					\
		)										\
		WAKEUP_SOURCE_COND_HAS_WAKEUP_GPIOS(						\
			node_id,								\
			WAKEUP_SOURCE_ASSIGN_WAKEUP_GPIOS					\
		)										\
		WAKEUP_SOURCE_ASSIGN_FLAGS(node_id)						\
		WAKEUP_SOURCE_ASSIGN_NAME(node_id)						\
	};

#define WAKEUP_SOURCE_DEFINE(node_id)								\
	WAKEUP_SOURCE_COND_HAS_WAKEUP_IDS(							\
		node_id,									\
		WAKEUP_SOURCE_DEFINE_WAKEUP_IDS							\
	)											\
	WAKEUP_SOURCE_COND_HAS_WAKEUP_IRQ_NUMBERS(						\
		node_id,									\
		WAKEUP_SOURCE_DEFINE_WAKEUP_IRQ_NUMBERS						\
	)											\
	WAKEUP_SOURCE_COND_HAS_WAKEUP_GPIOS(							\
		node_id,									\
		WAKEUP_SOURCE_DEFINE_WAKEUP_GPIOS						\
	)											\
	WAKEUP_SOURCE_DEFINE_FLAGS(node_id)							\
	WAKEUP_SOURCE_DEFINE_WAKEUP_SOURCE(node_id)

WAKEUP_SOURCE_FOREACH(WAKEUP_SOURCE_DEFINE)

static bool wakeup_source_is_enabled(const struct wakeup_source *ws)
{
	return ws->flags->enabled;
}

void sys_wakeup_source_enable(const struct wakeup_source *ws)
{
	ws->flags->enabled = true;
}

void sys_wakeup_source_disable(const struct wakeup_source *ws)
{
	ws->flags->enabled = false;
}

static void wakeup_source_configure(const struct wakeup_source *ws)
{
#ifdef CONFIG_WAKEUP_SOURCE_ID_SUPPORTED
	for (uint16_t i = 0; i < ws->wakeup_ids_size; i++) {
		z_sys_wakeup_source_enable_id(ws->wakeup_ids[i]);
	}
#endif

#ifdef CONFIG_WAKEUP_SOURCE_IRQ_SUPPORTED
	for (uint16_t i = 0; i < ws->wakeup_irq_numbers_size; i++) {
		z_sys_wakeup_source_enable_irq(ws->wakeup_irq_numbers[i]);
	}
#endif

#ifdef CONFIG_WAKEUP_SOURCE_GPIO_SUPPORTED
	for (uint16_t i = 0; i < ws->wakeup_gpios_size; i++) {
		z_sys_wakeup_source_enable_gpio(&ws->wakeup_gpios[i]);
	}
#endif
}

void z_sys_wakeup_sources_configure(void)
{
	STRUCT_SECTION_FOREACH(wakeup_source, ws) {
		if (!wakeup_source_is_enabled(ws)) {
			continue;
		}
		wakeup_source_configure(ws);
	}
}
