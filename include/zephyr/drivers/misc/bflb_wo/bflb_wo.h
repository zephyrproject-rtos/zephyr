/*
 * Copyright (c) 2026 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for Bouffalolab BL61x GPIO FIFO / Wire Out driver
 * @ingroup bflb_wo_interface
 */

#ifndef ZEPHYR_DRIVERS_MISC_BFLB_WO_BFLB_WO_H_
#define ZEPHYR_DRIVERS_MISC_BFLB_WO_BFLB_WO_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Interfaces for BL61x GPIO FIFO / Wire Out
 * @defgroup bflb_wo_interface Bouffalolab BL61x GPIO FIFO / Wire Out
 * @ingroup misc_interfaces
 *
 * @{
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util_macro.h>

#define BFLB_WO_PIN_NONE	255 /**< No pin selected */
#define BFLB_WO_PIN_CNT		16 /**< Maximum number of pin toggled at the same time */

/** Time size validator */
#define BFLB_WO_VALIDATE_TIME(_node)								\
	BUILD_ASSERT((DT_PROP(_node, time_t0h) < DT_PROP(_node, time_total)),			\
		     "BFLB WO device time-t0h must be smaller than time-total");		\
	BUILD_ASSERT((DT_PROP(_node, time_t1h) < DT_PROP(_node, time_total)),			\
		     "BFLB WO device time-t1h must be smaller than time-total")
/** Time size validator (frequency) */
#define BFLB_WO_VALIDATE_FREQ(_node)								\
	BUILD_ASSERT((DT_PROP(_node, time_t0h) < (Z_HZ_ns / DT_PROP(_node, frequency))),	\
		     "BFLB WO device time-t0h must be smaller than the frequency's period");	\
	BUILD_ASSERT((DT_PROP(_node, time_t1h) < (Z_HZ_ns / DT_PROP(_node, frequency))),	\
		     "BFLB WO device time-t1h must be smaller than the frequency's period")

/** Utility to validate a WO device */
#define BFLB_WO_VALIDATE(_node)									\
	BUILD_ASSERT((DT_PROP_OR(_node, frequency, 0) | DT_PROP_OR(_node, time_total, 0)) != 0,	\
		     "BFLB WO device must have frequency or time_total set");			\
	BUILD_ASSERT(DT_NODE_HAS_PROP(_node, time_t0h),						\
		"BFLB WO device must have time-t0h, device YAML is bad if this was allowed");	\
	BUILD_ASSERT(DT_NODE_HAS_PROP(_node, time_t1h),						\
		"BFLB WO device must have time-t1h, device YAML is bad if this was allowed");	\
	IF_ENABLED(DT_NODE_HAS_PROP(_node, time_total), (BFLB_WO_VALIDATE_TIME(_node)));	\
	IF_ENABLED(DT_NODE_HAS_PROP(_node, frequency), (BFLB_WO_VALIDATE_FREQ(_node)))
/** Utility to validate a WO device (from instance and compat pair) */
#define BFLB_WO_VALIDATE_INST(_inst, _compat) BFLB_WO_VALIDATE(DT_INST(_inst, _compat))

/**
 * @typedef bflb_wo_callback_t
 * @brief Callback type used to inform user async write has completed
 *
 * This callback is called from IRQ context. WO write cannot fail, only
 * data is provided.
 *
 * @param user_data Pointer to user data provided during callback registration
 */
typedef void (*bflb_wo_callback_t) (void *user_data);

/**
 * @struct bflb_wo_config
 * @brief Structure holding the configuration for the GPIO FIFO pulse width, polarity, and pins.
 *
 * Such as:
 *
 * set_invert, unset_invert and park_high are false
 * total_cycles is 9
 * set_cycles is 6
 * unset_cycles is 4
 * 2 frames of data, one with the pin bit set to 1, one with it set to 0
 * The clock cycle unit is XCLK's speed (RC32M or crystal)
 *
 * @code{.unparsed}
 *                    total_cycles
 *           |-----------------------------------|-----------------------------------|
 *
 * clock     |"| |"| |"| |"| |"| |"| |"| |"| |"| |"| |"| |"| |"| |"| |"| |"| |"| |"| |"| |"| |
 *           | |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_|
 *
 * data      1                                   0
 *
 * Pin value """"""""""""""""""""""""|           |"""""""""""""""|
 *                                   |___________|               |___________________________
 *
 *           |-----------------------|-----------|---------------|-------------------|---------
 *           |     set_cycles        |total - set|  unset_cycles |  total - unset    | park
 * @endcode
 *
 * Pins match such as toggle bit = pin & 16 (eg, pin 0 is bit 0, 17 is bit 1, pin 2 is bit 2, etc).
 *
 */
struct bflb_wo_config {
	uint16_t total_cycles; /**< Total clock cycles length of a toggling cycles (9 bits) */
	uint8_t set_cycles; /**< Number of clock cycles a pin is toggled when its bit is set */
	uint8_t unset_cycles; /**< Number of clock cycles a pin is toggled when its bit is unset */
	bool set_invert; /**< Go from low to high instead of high to low when bit is set */
	bool unset_invert; /**< Same but when bit is unset */
	bool park_high; /**< Park pins high instead of low at the end of a sequence */
};

/**
 * Convert frequency to cycles
 *
 * @param frequency Frequency in Hz
 * @param exact Return 0 instead of closest value when an exact match is not possible
 * @return Number of cycles closest to frequency, or 0.
 */
uint16_t bflb_wo_frequency_to_cycles(const uint32_t frequency, const bool exact);

/**
 * Convert time to cycles
 *
 * @param time time in nanoseconds
 * @param exact Return 0 instead of closest value when an exact match is not possible
 * @return Number of cycles closest to time, or 0.
 */
uint16_t bflb_wo_time_to_cycles(const uint32_t time, const bool exact);

/**
 * Configure GPIO FIFO
 *
 * @param config Pointer to the configuration structure
 * @param pins Up to 16 pins, or BFLB_WO_PIN_NONE
 * @param flags Flags for the pins, will be ored with GPIO_OUTPUT
 * @param pin_cnt Number of pins to configure
 * @retval 0 on success
 * @retval -EINVAL if configuration is invalid, or other error codes.
 */
int bflb_wo_configure(const struct bflb_wo_config *const config,
		      const uint8_t *pins, const gpio_flags_t *flags, const size_t pin_cnt);

/**
 * Configure GPIO FIFO
 *
 * @param config Pointer to the configuration structure
 * @param pins Up to 16 pins
 * @param pin_cnt Number of pins to configure
 * @retval 0 on success
 * @retval -EINVAL if configuration is invalid, or other error codes.
 */
int bflb_wo_configure_dt(const struct bflb_wo_config *const config, const struct gpio_dt_spec *pins,
			 const size_t pin_cnt);

/**
 * Write to GPIO FIFO
 *
 * @param data Array containing the sequence of toggles
 * @param len Length of the array
 * @retval 0 on success
 * @retval -EIO or other error codes.
 */
int bflb_wo_write(const uint16_t *const data, const size_t len);

/**
 * Write to GPIO FIFO Asynchronously
 *
 * @param data Array containing the sequence of toggles
 * @param len Length of the array
 * @param cb Function pointer to completion callback.
 * @param user_data Userdata passed to callback.
 * @retval 0 on success
 * @retval -EIO or other error codes.
 */
int bflb_wo_write_async(const uint16_t *const data, const size_t len,
			bflb_wo_callback_t cb, void *user_data);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_MISC_BFLB_WO_BFLB_WO_H_ */
