/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Texas Instruments
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup ti_am3352_eqep_interface
 * @brief Public API for TI AM3352 Enhanced Quadrature Encoder Pulse (EQEP) counter
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_COUNTER_TI_AM3352_EQEP_H_
#define ZEPHYR_INCLUDE_DRIVERS_COUNTER_TI_AM3352_EQEP_H_

#include <zephyr/drivers/counter.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief TI AM3352 EQEP counter device-specific API extension
 * @defgroup ti_am3352_eqep_interface TI AM3352 EQEP counter
 * @since 4.4.0
 * @version 0.1.0
 * @ingroup counter_interface
 * @{
 */

/** Number of EQEP alarm channels */
#define TI_EQEP_ALARM_CHAN_NUM (4)

/**
 * @brief EQEP Event Source Identifiers (Logical Channels)
 *
 * These represent different interrupt sources within the EQEP peripheral,
 * not independent hardware channels. Each corresponds to a distinct functional
 * unit that can generate events and trigger callbacks through the counter API.
 */
enum ti_eqep_alarm_channel {
	/** Alarm for position compare unit */
	TI_EQEP_ALARM_CHAN_COMPARE = 0,
	/** Alarm for strobe event */
	TI_EQEP_ALARM_CHAN_STROBE = 1,
	/** Alarm for index event */
	TI_EQEP_ALARM_CHAN_INDEX = 2,
	/** Alarm for unit timeout event */
	TI_EQEP_ALARM_CHAN_TIMEOUT = 3,
};

/**
 * @brief Position counter reset mode
 */
enum ti_eqep_reset_mode {
	/** Reset counter on index event */
	TI_EQEP_RESET_MODE_INDEX = 0,
	/** Reset counter after maximum value */
	TI_EQEP_RESET_MODE_MAX = 1,
	/** Reset counter on only first index event */
	TI_EQEP_RESET_MODE_INDEX_ONCE = 2,
	/** Reset counter on unit timeout event */
	TI_EQEP_RESET_MODE_UNIT_TIME = 3,
};

/**
 * @brief Index event latch mode
 */
enum ti_eqep_index_latch {
	/** Reserved value */
	TI_EQEP_INDEX_LATCH_RESERVED = 0,
	/** Latch counter on index's rising edge */
	TI_EQEP_INDEX_LATCH_RISING = 1,
	/** Latch counter on index's falling edge */
	TI_EQEP_INDEX_LATCH_FALLING = 2,
	/** Latch counter and direction flag on index marker */
	TI_EQEP_INDEX_LATCH_MARKER = 3,
};

/**
 * @brief Strobe event latch mode
 */
enum ti_eqep_strobe_latch {
	/** Latch counter on strobe's rising edge */
	TI_EQEP_STROBE_LATCH_RISING = 0,
	/** Latch counter on strobe's rising edge if clockwise and falling edge if anti-clockwise */
	TI_EQEP_STROBE_LATCH_DIR = 1,
};

/**
 * @brief Capture unit latch mode
 */
enum ti_eqep_capture_latch {
	/** Latch capture timer and capture period on counter read */
	TI_EQEP_CAPTURE_LATCH_CPU = 0,
	/** Latch counter, capture timer and capture period on timeout */
	TI_EQEP_CAPTURE_LATCH_TIMEOUT = 1,
};

/**
 * @brief Configuration for QEP control register
 */
struct ti_eqep_qep_cfg {
	/** Counter reset mode */
	enum ti_eqep_reset_mode reset_mode: 2;
	/** Index event latch mode */
	enum ti_eqep_index_latch index_latch: 2;
	/** Strobe event latch mode */
	enum ti_eqep_strobe_latch strobe_latch: 1;
	/** Capture unit latch mode */
	enum ti_eqep_capture_latch capture_latch: 1;
} __packed;

/**
 * @brief Position counter source selection
 */
enum ti_eqep_src {
	/** Quadrature Count Mode */
	TI_EQEP_SRC_QUADRATURE = 0,
	/** Direction Count Mode */
	TI_EQEP_SRC_DIRECTION = 1,
	/** Up Count Mode */
	TI_EQEP_SRC_UP = 2,
	/** Down Count Mode */
	TI_EQEP_SRC_DOWN = 3,
};

/**
 * @brief Configuration for decoder unit register
 */
struct ti_eqep_dec_cfg {
	/** Counter source selection */
	enum ti_eqep_src source: 2;
	/** Only increment/decrement on the rising edges */
	bool rising_edge_only: 1;
	/** Swap quadrature clock inputs */
	bool swap_inputs: 1;
} __packed;

/**
 * @brief Capture timer clock prescaler values
 */
enum ti_eqep_cap_clk_prescaler {
	/** CAPCLK = EQEP_FICLK/1 */
	TI_EQEP_CAP_CLK_DIV_1 = 0,
	/** CAPCLK = EQEP_FICLK/2 */
	TI_EQEP_CAP_CLK_DIV_2 = 1,
	/** CAPCLK = EQEP_FICLK/4 */
	TI_EQEP_CAP_CLK_DIV_4 = 2,
	/** CAPCLK = EQEP_FICLK/8 */
	TI_EQEP_CAP_CLK_DIV_8 = 3,
	/** CAPCLK = EQEP_FICLK/16 */
	TI_EQEP_CAP_CLK_DIV_16 = 4,
	/** CAPCLK = EQEP_FICLK/32 */
	TI_EQEP_CAP_CLK_DIV_32 = 5,
	/** CAPCLK = EQEP_FICLK/64 */
	TI_EQEP_CAP_CLK_DIV_64 = 6,
	/** CAPCLK = EQEP_FICLK/128 */
	TI_EQEP_CAP_CLK_DIV_128 = 7,
};

/**
 * @brief Unit position event prescaler values
 */
enum ti_eqep_unit_pos_prescaler {
	/** UPEVNT = QCLK/1 */
	TI_EQEP_UNIT_POS_DIV_1 = 0,
	/** UPEVNT = QCLK/2 */
	TI_EQEP_UNIT_POS_DIV_2 = 1,
	/** UPEVNT = QCLK/4 */
	TI_EQEP_UNIT_POS_DIV_4 = 2,
	/** UPEVNT = QCLK/8 */
	TI_EQEP_UNIT_POS_DIV_8 = 3,
	/** UPEVNT = QCLK/16 */
	TI_EQEP_UNIT_POS_DIV_16 = 4,
	/** UPEVNT = QCLK/32 */
	TI_EQEP_UNIT_POS_DIV_32 = 5,
	/** UPEVNT = QCLK/64 */
	TI_EQEP_UNIT_POS_DIV_64 = 6,
	/** UPEVNT = QCLK/128 */
	TI_EQEP_UNIT_POS_DIV_128 = 7,
	/** UPEVNT = QCLK/256 */
	TI_EQEP_UNIT_POS_DIV_256 = 8,
	/** UPEVNT = QCLK/512 */
	TI_EQEP_UNIT_POS_DIV_512 = 9,
	/** UPEVNT = QCLK/1024 */
	TI_EQEP_UNIT_POS_DIV_1024 = 10,
	/** UPEVNT = QCLK/2048 */
	TI_EQEP_UNIT_POS_DIV_2048 = 11,
};

/**
 * @brief Configuration for capture unit register
 */
struct ti_eqep_cap_cfg {
	/** Enable the capture unit */
	bool enable: 1;
	/** Capture timer clock prescaler */
	enum ti_eqep_cap_clk_prescaler clock_prescaler: 3;
	/** Unit position event prescaler */
	enum ti_eqep_unit_pos_prescaler unit_position_prescaler: 4;
} __packed;

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
 */
__syscall void ti_eqep_configure_capture(const struct device *dev,
					 const struct ti_eqep_cap_cfg *cap_cfg);

/**
 * @brief Get latched capture timer and period values
 *
 * @param[in]  dev     Pointer to EQEP counter device
 * @param[out] timer   Latched capture timer value
 * @param[out] period  Latched capture period value
 * @param[in]  scale   Scale values by capture clock prescaler
 *
 * @retval 0 if success
 * @retval -EINVAL if out parameters are NULL
 * @retval -ENOTSUP if capture unit is not enabled
 */
__syscall int ti_eqep_get_latched_capture_values(const struct device *dev, uint32_t *timer,
						 uint32_t *period, bool scale);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#include <zephyr/syscalls/ti_am3352_eqep.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_COUNTER_TI_AM3352_EQEP_H_ */
