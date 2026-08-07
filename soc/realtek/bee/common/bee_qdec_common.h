/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file bee_qdec_common.h
 * @brief Realtek Bee Series Common QDEC Abstraction Layer.
 */

#ifndef ZEPHYR_DRIVERS_COMMON_BEE_QDEC_H_
#define ZEPHYR_DRIVERS_COMMON_BEE_QDEC_H_

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <stdbool.h>
#include <stdint.h>

#if defined(CONFIG_SOC_SERIES_RTL87X2G)
#include <rtl_aon_qdec.h>
#include <rtl_rcc.h>
#elif defined(CONFIG_SOC_SERIES_RTL8752H)
#include <rtl876x_qdec.h>
#include <rtl876x_rcc.h>
#include <rtl876x_nvic.h>
#include <vector_table.h>
#else
#error "Unsupported Realtek Bee SoC series"
#endif

/**
 * @defgroup bee_qdec_abstraction QDEC Abstraction Interface
 * @{
 */

enum bee_qdec_axis {
	BEE_QDEC_AXIS_X = 0,
	BEE_QDEC_AXIS_Y,
	BEE_QDEC_AXIS_Z,
	BEE_QDEC_AXIS_MAX
};

enum bee_qdec_event_type {
	BEE_QDEC_EVENT_NEW_DATA,
	BEE_QDEC_EVENT_ILLEGAL,
	BEE_QDEC_EVENT_OVERFLOW,
	BEE_QDEC_EVENT_UNDERFLOW,
};

/**
 * @brief Configuration for a single axis
 */
struct bee_qdec_axis_config {
	bool enable;
	uint8_t counts_per_revolution; /* 2 or 4 */
	uint32_t debounce_time_ms;
};

/**
 * @brief Unified QDEC Operations Structure.
 */
struct bee_qdec_ops {
	/**
	 * @brief Initialize the QDEC hardware.
	 * @param reg Base address of the QDEC register.
	 * @param axis_cfg Array of configuration for X, Y, Z axes.
	 * @return 0 on success, negative errno on failure.
	 */
	int (*init)(uint32_t reg, const struct bee_qdec_axis_config *axis_cfgs);

	/**
	 * @brief Enable the QDEC axis.
	 */
	void (*enable)(uint32_t reg, enum bee_qdec_axis axis);

	/**
	 * @brief Disable the QDEC axis.
	 */
	void (*disable)(uint32_t reg, enum bee_qdec_axis axis);

	/**
	 * @brief Get the current raw accumulation count.
	 */
	uint16_t (*get_count)(uint32_t reg, enum bee_qdec_axis axis);

	/**
	 * @brief Enable interrupt for specific event.
	 */
	void (*int_enable)(uint32_t reg, enum bee_qdec_axis axis, enum bee_qdec_event_type type);

	/**
	 * @brief Disable interrupt for specific event.
	 */
	void (*int_disable)(uint32_t reg, enum bee_qdec_axis axis, enum bee_qdec_event_type type);

	/**
	 * @brief Clear interrupt pending status / flag.
	 */
	void (*int_clear)(uint32_t reg, enum bee_qdec_axis axis, enum bee_qdec_event_type type);

	/**
	 * @brief Check interrupt status / flag state.
	 * @return true if set, false otherwise.
	 */
	bool (*get_status)(uint32_t reg, enum bee_qdec_axis axis, enum bee_qdec_event_type type);
};

/**
 * @brief Retrieve the QDEC Operations structure.
 * @return const struct bee_qdec_ops* Pointer to the operations structure.
 */
const struct bee_qdec_ops *bee_qdec_get_ops(void);

/** @} */

#endif /* ZEPHYR_DRIVERS_COMMON_BEE_QDEC_H_ */
