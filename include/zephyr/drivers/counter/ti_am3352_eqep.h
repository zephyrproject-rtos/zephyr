/*
 * Copyright (c) 2025 Texas Instruments
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_COUNTER_TI_AM3352_EQEP_H_
#define ZEPHYR_INCLUDE_DRIVERS_COUNTER_TI_AM3352_EQEP_H_

#include <zephyr/drivers/counter.h>

/**
 * @brief Type to represent an alarm channel.
 *
 * These are not real channels and are only used to set callbacks for various events that EQEP
 * supports.
 */
#define TI_EQEP_ALARM_CHAN_NUM (4)
enum ti_eqep_alarm_channel {
	/* Alarm for position compare unit */
	TI_EQEP_ALARM_CHAN_COMPARE = 0,
	/* Alarm for strobe event */
	TI_EQEP_ALARM_CHAN_STROBE = 1,
	/* Alarm for index event */
	TI_EQEP_ALARM_CHAN_INDEX = 2,
	/* Alarm for unit timeout event */
	TI_EQEP_ALARM_CHAN_TIMEOUT = 3,
};

/**
 * @brief Position counter reset mode
 */
enum ti_eqep_reset_mode {
	/* Reset counter on index event */
	TI_EQEP_RESET_MODE_INDEX = 0,
	/* Reset counter after maximum value */
	TI_EQEP_RESET_MODE_MAX = 1,
	/* Reset counter on only first index event */
	TI_EQEP_RESET_MODE_INDEX_ONCE = 2,
	/* Reset counter on unit timeout event */
	TI_EQEP_RESET_MODE_UNIT_TIME = 3,
};

/**
 * @brief Index event latch mode
 */
enum ti_eqep_index_latch {
	/* Reserved value */
	TI_EQEP_INDEX_LATCH_RESERVED = 0,
	/* Latch counter on index's rising edge */
	TI_EQEP_INDEX_LATCH_RISING = 1,
	/* Latch counter on index's falling edge */
	TI_EQEP_INDEX_LATCH_FALLING = 2,
	/* Latch counter and direction flag on index marker */
	TI_EQEP_INDEX_LATCH_MARKER = 3,
};

/**
 * @brief Strobe event latch mode
 */
enum ti_eqep_strobe_latch {
	/* Latch counter on strobe's rising edge */
	TI_EQEP_STROBE_LATCH_RISING = 0,
	/* Latch counter on strobe's rising edge if clockwise and falling edge if anti-clockwise */
	TI_EQEP_STROBE_LATCH_DIR = 1,
};

/**
 * @brief Capture unit latch mode
 */
enum ti_eqep_capture_latch {
	/* Latch capture timer and capture period on counter read */
	TI_EQEP_CAPTURE_LATCH_CPU = 0,
	/* Latch counter, capture timer and capture period on timeout */
	TI_EQEP_CAPTURE_LATCH_TIMEOUT = 1,
};

/**
 * @brief Position counter source selection
 */
enum ti_eqep_src {
	/* Quadrature Count Mode */
	TI_EQEP_SRC_QUADRATURE = 0,
	/* Direction Count Mode */
	TI_EQEP_SRC_DIRECTION = 1,
	/* Up Count Mode */
	TI_EQEP_SRC_UP = 2,
	/* Down Count Mode */
	TI_EQEP_SRC_DOWN = 3,
};

/**
 * @brief Configuration for QEP control register
 */
struct ti_eqep_qep_cfg {
	/* Counter reset mode */
	enum ti_eqep_reset_mode reset_mode;
	/* Index event latch mode */
	enum ti_eqep_index_latch index_latch;
	/* Strobe event latch mode */
	enum ti_eqep_strobe_latch strobe_latch;
	/* Capture unit latch mode */
	enum ti_eqep_capture_latch capture_latch;
};

/**
 * @brief Configuration for decoder unit register
 */
struct ti_eqep_dec_cfg {
	/* Counter source selection */
	enum ti_eqep_src source;
	/* Only increment/decrement on the rising edges */
	bool rising_edge_only;
	/* Swap quadrature clock inputs */
	bool swap_inputs;
};

/**
 * @brief Configuration for capture unit register
 */
struct ti_eqep_cap_cfg {
	/* Enable the capture unit */
	bool enable;
	/* Capture timer clock prescaler */
	uint8_t clock_prescaler;
	/* Unit position event prescaler */
	uint8_t unit_position_prescaler;
};

/**
 * @brief Configure QEP Control
 *
 * @param[in] dev     Pointer to EQEP counter device
 * @param[in] qep_cfg Pointer to QEP control configuration
 *
 */
__syscall void ti_eqep_configure_qep(const struct device *dev,
				     const struct ti_eqep_qep_cfg *qep_cfg);

/**
 * @brief Configure Decoder Unit
 *
 * @param[in] dev     Pointer to EQEP counter device
 * @param[in] dec_cfg Pointer to decoder configuration
 *
 */
__syscall void ti_eqep_configure_decoder(const struct device *dev,
					 const struct ti_eqep_dec_cfg *dec_cfg);

/**
 * @brief Configure Edge Capture Unit
 *
 * @param[in] dev     Pointer to EQEP counter device
 * @param[in] cap_cfg Pointer to edge capture configuration
 *
 * @retval 0 if success
 * @retval -ENOTSUP if capture unit is not disabled when configuring
 * @retval -EINVAL if cap_cfg values are not valid
 */
__syscall int ti_eqep_configure_capture(const struct device *dev,
					const struct ti_eqep_cap_cfg *cap_cfg);

/**
 * @brief
 *
 * @param[in]  dev     Pointer to EQEP counter device
 * @param[out] timer   Latched capture timer value
 * @param[out] period  Latched capture period value
 *
 * @retval 0 if success
 * @retval -EINVAL if out parameters are NULL
 * @retval -ENOTSUP if capture unit is not enabled
 */
__syscall int ti_eqep_get_latched_capture_values(const struct device *dev, uint32_t *timer,
						 uint32_t *period);

#endif /* ZEPHYR_INCLUDE_DRIVERS_COUNTER_TI_AM3352_EQEP_H_ */
