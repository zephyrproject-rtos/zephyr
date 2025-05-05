/*
 * Copyright (c) 2025 Texas Instruments
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_COUNTER_TI_AM3352_EQEP_H_
#define ZEPHYR_INCLUDE_DRIVERS_COUNTER_TI_AM3352_EQEP_H_

#include <zephyr/drivers/counter.h>

enum ti_eqep_alarm_channel {
	TI_EQEP_ALARM_CHAN_COMPARE = 0,
	TI_EQEP_ALARM_CHAN_STROBE = 1,
	TI_EQEP_ALARM_CHAN_INDEX = 2,
	TI_EQEP_ALARM_CHAN_TIMEOUT = 3,
	TI_EQEP_ALARM_CHAN_NUM = 4, /* sentinel */
};

enum ti_eqep_reset_mode {
	TI_EQEP_RESET_MODE_INDEX = 0,
	TI_EQEP_RESET_MODE_MAX = 1,
	TI_EQEP_RESET_MODE_INDEX_ONCE = 2,
	TI_EQEP_RESET_MODE_UNIT_TIME = 3,
};

enum ti_eqep_index_latch {
	TI_EQEP_INDEX_LATCH_RESERVED = 0,
	TI_EQEP_INDEX_LATCH_RISING = 1,
	TI_EQEP_INDEX_LATCH_FALLING = 2,
	TI_EQEP_INDEX_LATCH_MARKER = 3,
};

enum ti_eqep_strobe_latch {
	TI_EQEP_STROBE_LATCH_RISING = 0,
	TI_EQEP_STROBE_LATCH_DIR = 1,
};

enum ti_eqep_capture_latch {
	TI_EQEP_CAPTURE_LATCH_CPU = 0,
	TI_EQEP_CAPTURE_LATCH_TIMEOUT = 1,
};

enum ti_eqep_src {
	TI_EQEP_SRC_QUADRATURE = 0, /* Quadrature Count Mode */
	TI_EQEP_SRC_DIRECTION = 1,  /* Direction Count Mode */
	TI_EQEP_SRC_UP = 2,         /* Up Count Mode */
	TI_EQEP_SRC_DOWN = 3,       /* Down Count Mode */
};

struct ti_eqep_qep_cfg {
	enum ti_eqep_reset_mode reset_mode;
	enum ti_eqep_index_latch index_latch;
	enum ti_eqep_strobe_latch strobe_latch;
	enum ti_eqep_capture_latch capture_latch;
};

struct ti_eqep_dec_cfg {
	enum ti_eqep_src source;
	bool rising_edge_only;
	bool swap_inputs;
};

struct ti_eqep_cap_cfg {
	uint8_t clock_prescaler;
	uint8_t unit_position_prescaler;
	bool enable;
};

__syscall int ti_eqep_configure_capture(const struct device *dev,
					const struct ti_eqep_cap_cfg *cap_cfg);

__syscall void ti_eqep_configure_qep(const struct device *dev,
				     const struct ti_eqep_qep_cfg *qep_cfg);

__syscall void ti_eqep_configure_decoder(const struct device *dev,
					 const struct ti_eqep_dec_cfg *dec_cfg);

__syscall int ti_eqep_get_latched_capture_values(const struct device *dev, uint32_t *timer,
						 uint32_t *period);

#endif /* ZEPHYR_INCLUDE_DRIVERS_COUNTER_TI_AM3352_EQEP_H_ */
