/*
 * Copyright (c) 2023 Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_WAKEUP_SOURCE_H_
#define ZEPHYR_INCLUDE_SYS_WAKEUP_SOURCE_H_

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup sys_wakeup_source System wakeup source
 * @ingroup os_services
 * @{
 */

/** @cond INTERNAL_HIDDEN */

/** Mutable wakeup source flags */
struct wakeup_source_flags {
	uint8_t enabled : 1;
};

/** Required information to enable a wakeup source */
struct wakeup_source {
#ifdef CONFIG_WAKEUP_SOURCE_ID_SUPPORTED
	/** Vendor specific wakeup source identifiers */
	const uint16_t *wakeup_ids;
	/** Number of vendor specific wakeup source identifier */
	uint16_t wakeup_ids_size;
#endif
#ifdef CONFIG_WAKEUP_SOURCE_IRQ_SUPPORTED
	/** Wakeup interrupt numbers */
	const uint16_t *wakeup_irq_numbers;
	/** Number of wakeup interrupt numbers */
	uint16_t wakeup_irq_numbers_size;
#endif
#ifdef CONFIG_WAKEUP_SOURCE_GPIO_SUPPORTED
	/** Wakeup GPIOs */
	const struct gpio_dt_spec *wakeup_gpios;
	/** Number of wakeup GPIOs */
	uint16_t wakeup_gpios_size;
#endif
	/** Name of wakeup source */
	const char *name;
	/** Mutable wakeup source flags */
	struct wakeup_source_flags *flags;
};

/** Enable wakeup source by id hook */
void z_sys_wakeup_source_enable_id(uint16_t id);

/** Enable IRQ as wakeup source hook */
void z_sys_wakeup_source_enable_irq(uint16_t irq_number);

/** Enable GPIO as wakeup source hook */
void z_sys_wakeup_source_enable_gpio(const struct gpio_dt_spec *gpio);

/** Configure all enabled wakeup sources */
void z_sys_wakeup_sources_configure(void);

/** @endcond */

/** Enable a wakeup source */
void sys_wakeup_source_enable(const struct wakeup_source *ws);

/** Disable a wakeup source */
void sys_wakeup_source_disable(const struct wakeup_source *ws);

#ifdef __cplusplus
}
#endif

/** @cond INTERNAL_HIDDEN */

#define WAKEUP_SOURCE_NAME(node_id) \
	UTIL_CAT(__wakeup_source_, DT_DEP_ORD(node_id))

#define IS_WAKEUP_SOURCE(node_id) \
	IS_ENABLED(DT_PROP(node_id, wakeup_source))

#define WAKEUP_SOURCE_FOREACH_INTERNAL(node_id, fn) \
	COND_CODE_1(IS_WAKEUP_SOURCE(node_id), (fn(node_id)), ())

/** @endcond */

#define WAKEUP_SOURCE_DT_GET(node_id) \
	&WAKEUP_SOURCE_NAME(node_id)

#define WAKEUP_SOURCE_FOREACH(fn) \
	DT_FOREACH_VARGS_HELPER(WAKEUP_SOURCE_FOREACH_INTERNAL, fn)

/** @cond INTERNAL_HIDDEN */

#define WAKEUP_SOURCE_DEFINE_EXTERN(node_id) \
	extern const struct wakeup_source WAKEUP_SOURCE_NAME(node_id);

WAKEUP_SOURCE_FOREACH(WAKEUP_SOURCE_DEFINE_EXTERN)

/** @endcond */

/** @} */

#endif /* ZEPHYR_INCLUDE_SYS_WAKEUP_H_ */
