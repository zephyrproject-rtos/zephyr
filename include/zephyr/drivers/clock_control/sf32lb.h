/*
 * Copyright (c) 2025 Core Devices LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _INCLUDE_ZEPHYR_DRIVERS_CLOCK_CONTROL_SF32LB_H_
#define _INCLUDE_ZEPHYR_DRIVERS_CLOCK_CONTROL_SF32LB_H_

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/devicetree/clocks.h>
#include <zephyr/drivers/clock_control.h>

/**
 * @brief Clock Control (SF32LB specifics)
 * @defgroup clock_control_sf32lb Clock Control (SF32LB specifics)
 * @ingroup clock_control_interface
 * @{
 */

/** @brief SF32LB Clock DT spec */
struct sf32lb_clock_dt_spec {
	/** Clock controller (RCC) */
	const struct device *dev;
	/** Clock ID */
	uint16_t id;
};

/**
 * @brief Initialize a `sf32lb_clock_dt_spec` structure from a DT node.
 *
 * @param node_id DT node identifier
 */
#define SF32LB_CLOCK_DT_SPEC_GET(node_id)                                                          \
	{                                                                                          \
		.dev = DEVICE_DT_GET(DT_CLOCKS_CTLR(node_id)),                                     \
		.id = DT_CLOCKS_CELL(node_id, id),                                                 \
	}

/**
 * @brief Same as SF32LB_CLOCK_DT_SPEC_GET but with a default value
 *
 * @param node_id DT node identifier
 * @param default_spec Default value to be used if the node has no clocks property.
 */
#define SF32LB_CLOCK_DT_SPEC_GET_OR(node_id, default_spec)                                         \
	COND_CODE_1(DT_CLOCKS_HAS_IDX(node_id, 0),                                                 \
		    (SF32LB_CLOCK_DT_SPEC_GET(node_id)), (default_spec))

/**
 * @brief Initialize a `sf32lb_clock_dt_spec` structure from a DT instance.
 *
 * @param index DT instance index
 */
#define SF32LB_CLOCK_DT_INST_SPEC_GET(index) SF32LB_CLOCK_DT_SPEC_GET(DT_DRV_INST(index))

/**
 * @brief Initialize a `sf32lb_clock_dt_spec` structure from a parent DT instance.
 *
 * @param index DT instance index
 */
#define SF32LB_CLOCK_DT_INST_PARENT_SPEC_GET(index)                                                \
	{                                                                                          \
		.dev = DEVICE_DT_GET(DT_CLOCKS_CTLR(DT_INST_PARENT(index))),                       \
		.id = DT_CLOCKS_CELL(DT_INST_PARENT(index), id),                                   \
	}

/**
 * @brief Same as SF32LB_CLOCK_DT_INST_SPEC_GET but with a default value.
 *
 * @param index DT instance index
 * @param default_spec Default value to be used if the node has no clocks property.
 */
#define SF32LB_CLOCK_DT_INST_SPEC_GET_OR(index, default_spec)                                      \
	SF32LB_CLOCK_DT_SPEC_GET_OR(DT_DRV_INST(index), default_spec)
/**
 * @brief Check if the clock device is ready
 *
 * @param spec SF32LB clock DT spec
 *
 * @return true If the clock device is ready.
 * @return false If the clock device is not ready.
 */
static inline bool sf32lb_clock_is_ready_dt(const struct sf32lb_clock_dt_spec *spec)
{
	return device_is_ready(spec->dev);
}

/**
 * @brief Turn on a clock using a `sf32lb_clock_dt_spec` structure.
 *
 * @param spec SF32LB clock DT spec
 * @return See clock_control_on().
 */
static inline int sf32lb_clock_control_on_dt(const struct sf32lb_clock_dt_spec *spec)
{
	return clock_control_on(spec->dev, (clock_control_subsys_t)&spec->id);
}

/**
 * @brief Turn off a clock using a `sf32lb_clock_dt_spec` structure.
 *
 * @param spec SF32LB clock DT spec
 * @return See clock_control_off().
 */
static inline int sf32lb_clock_control_off_dt(const struct sf32lb_clock_dt_spec *spec)
{
	return clock_control_off(spec->dev, (clock_control_subsys_t)&spec->id);
}

/**
 * @brief Get the status of a clock using a `sf32lb_clock_dt_spec` structure.
 *
 * @param spec SF32LB clock DT spec
 * @return See clock_control_get_status().
 */
static inline enum clock_control_status sf32lb_clock_control_get_status_dt(
		const struct sf32lb_clock_dt_spec *spec)
{
	return clock_control_get_status(spec->dev, (clock_control_subsys_t)&spec->id);
}

/**
 * @brief Get the clock rate using a `sf32lb_clock_dt_spec` structure.
 *
 * @param spec SF32LB clock DT spec
 * @param[out] rate Stored clock rate in Hz
 *
 * @return See clock_control_get_rate().
 */
static inline uint32_t sf32lb_clock_control_get_rate_dt(const struct sf32lb_clock_dt_spec *spec,
						uint32_t *rate)
{
	return clock_control_get_rate(spec->dev, (clock_control_subsys_t)&spec->id, rate);
}

/** @} */

#endif /* _INCLUDE_ZEPHYR_DRIVERS_CLOCK_CONTROL_SF32LB_H_ */
