/*
 * Copyright (c) 2026 MediaTek Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief MediaTek external interrupt (EINT) consumer interface.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_INTC_MTK_EINT_H_
#define ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_INTC_MTK_EINT_H_

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/sys/slist.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Condition on which an external interrupt line fires.
 *
 * A line detects one condition at a time.  The controller has no dual-edge
 * mode, so a consumer that wants both edges selects the edge opposite to the
 * level the line currently sits at, and turns it around with
 * eint_mtk_set_polarity() after every event; this enumeration deliberately
 * offers no value for it.
 */
enum eint_mtk_trigger {
	/** Transition from low to high. */
	EINT_MTK_TRIG_EDGE_RISING,
	/** Transition from high to low. */
	EINT_MTK_TRIG_EDGE_FALLING,
	/** Line held high. */
	EINT_MTK_TRIG_LEVEL_HIGH,
	/** Line held low. */
	EINT_MTK_TRIG_LEVEL_LOW,
};

/**
 * @brief Called when an external interrupt fires on a registered line.
 *
 * @param dev  Device that registered the callback.
 * @param line Line that fired, counted from the start of the controller.
 * @param arg  Argument supplied at registration.
 */
typedef void (*eint_mtk_cb_handler_t)(const struct device *dev, uint8_t line, void *arg);

/**
 * @brief Registration for a contiguous range of external interrupt lines.
 *
 * The consumer owns this storage and keeps it alive for as long as the callback
 * is registered.  Initialise it with eint_mtk_init_callback() rather than by
 * hand.
 */
struct eint_mtk_callback {
	/** @cond INTERNAL_HIDDEN */
	sys_snode_t node;
	/** @endcond */

	/** Handler to call when a line in the range fires. */
	eint_mtk_cb_handler_t cb_handler;

	/** Device passed to the handler. */
	const struct device *cb_dev;

	/** Argument passed to the handler. */
	void *cb_arg;

	/** First line of the range. */
	uint8_t first_line;

	/** Number of lines in the range, counted from the first. */
	uint8_t num_lines;
};

/**
 * @cond INTERNAL_HIDDEN
 *
 * For internal driver use only, skip these in public documentation.
 */
__subsystem struct eint_mtk_driver_api {
	int (*add_callback)(const struct device *dev, struct eint_mtk_callback *callback);
	void (*remove_callback)(const struct device *dev, struct eint_mtk_callback *callback);
	int (*enable)(const struct device *dev, uint8_t line);
	int (*disable)(const struct device *dev, uint8_t line);
	bool (*is_enabled)(const struct device *dev, uint8_t line);
	int (*set_trigger)(const struct device *dev, uint8_t line, enum eint_mtk_trigger trig);
	int (*set_polarity)(const struct device *dev, uint8_t line, bool high);
};
/** @endcond */

/**
 * @brief Fill in a registration for a range of lines.
 *
 * @param callback   Registration to initialise.
 * @param first_line First line of the range.
 * @param num_lines  Number of lines in the range, counted from @p first_line.
 * @param cb_handler Handler to call when a line in the range fires.
 * @param cb_dev     Device passed to the handler.
 * @param cb_arg     Argument passed to the handler.
 *
 * @retval 0 on success.
 * @retval -EINVAL A required argument is NULL or the range is empty.
 */
static inline int eint_mtk_init_callback(struct eint_mtk_callback *callback, uint8_t first_line,
					 uint8_t num_lines, eint_mtk_cb_handler_t cb_handler,
					 const struct device *cb_dev, void *cb_arg)
{
	if ((callback == NULL) || (cb_handler == NULL) || (cb_dev == NULL) || (num_lines == 0U)) {
		return -EINVAL;
	}

	callback->cb_handler = cb_handler;
	callback->cb_dev = cb_dev;
	callback->cb_arg = cb_arg;
	callback->first_line = first_line;
	callback->num_lines = num_lines;

	return 0;
}

/**
 * @brief Register for events on a range of lines.
 *
 * Ranges may not overlap: one registration owns a given line.
 *
 * @param dev      External interrupt controller.
 * @param callback Registration filled in by eint_mtk_init_callback().
 *
 * @retval 0 on success.
 * @retval -EINVAL The registration is malformed, reaches past the last line
 *                 of the controller, or overlaps one already registered.
 */
static inline int eint_mtk_add_callback(const struct device *dev,
					struct eint_mtk_callback *callback)
{
	return DEVICE_API_GET(eint_mtk, dev)->add_callback(dev, callback);
}

/**
 * @brief Drop a registration.
 *
 * Lines the registration covered are left as they are; disable them first if
 * they should stop firing.
 *
 * @param dev      External interrupt controller.
 * @param callback Registration to drop.
 */
static inline void eint_mtk_remove_callback(const struct device *dev,
					    struct eint_mtk_callback *callback)
{
	DEVICE_API_GET(eint_mtk, dev)->remove_callback(dev, callback);
}

/**
 * @brief Let a line deliver events.
 *
 * @param dev  External interrupt controller.
 * @param line Line to enable.
 *
 * @retval 0 on success.
 * @retval -EINVAL The line is past the last one of the controller.
 */
static inline int eint_mtk_enable(const struct device *dev, uint8_t line)
{
	return DEVICE_API_GET(eint_mtk, dev)->enable(dev, line);
}

/**
 * @brief Stop a line delivering events.
 *
 * @param dev  External interrupt controller.
 * @param line Line to disable.
 *
 * @retval 0 on success.
 * @retval -EINVAL The line is past the last one of the controller.
 */
static inline int eint_mtk_disable(const struct device *dev, uint8_t line)
{
	return DEVICE_API_GET(eint_mtk, dev)->disable(dev, line);
}

/**
 * @brief Report whether a line may deliver events.
 *
 * @param dev  External interrupt controller.
 * @param line Line to query.
 *
 * @return true if the line is enabled, false if it is disabled or out of range.
 */
static inline bool eint_mtk_is_enabled(const struct device *dev, uint8_t line)
{
	return DEVICE_API_GET(eint_mtk, dev)->is_enabled(dev, line);
}

/**
 * @brief Choose the condition on which a line fires.
 *
 * Any event already latched on the line is discarded, so a line enabled after
 * this call does not fire on a condition that predates it.  The line's enable
 * state is left alone.
 *
 * @param dev  External interrupt controller.
 * @param line Line to configure.
 * @param trig Condition to detect.
 *
 * @retval 0 on success.
 * @retval -EINVAL The line is past the last one of the controller, or the
 *                 condition is not one of @ref eint_mtk_trigger.
 */
static inline int eint_mtk_set_trigger(const struct device *dev, uint8_t line,
				       enum eint_mtk_trigger trig)
{
	return DEVICE_API_GET(eint_mtk, dev)->set_trigger(dev, line, trig);
}

/**
 * @brief Turn a line's condition around, keeping any event already latched.
 *
 * Selects a rising edge or a high level if @p high is true, and a falling edge
 * or a low level otherwise; whether the line detects edges or levels is left as
 * eint_mtk_set_trigger() chose it.  Unlike that call, this one discards
 * nothing, so an edge that arrives while a consumer re-arms a line still fires.
 *
 * @param dev  External interrupt controller.
 * @param line Line to configure.
 * @param high Respond to the high side rather than the low side.
 *
 * @retval 0 on success.
 * @retval -EINVAL The line is past the last one of the controller.
 */
static inline int eint_mtk_set_polarity(const struct device *dev, uint8_t line, bool high)
{
	return DEVICE_API_GET(eint_mtk, dev)->set_polarity(dev, line, high);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_INTC_MTK_EINT_H_ */
