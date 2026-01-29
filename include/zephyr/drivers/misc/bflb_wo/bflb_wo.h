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

/**
 * @brief Interfaces for BL61x GPIO FIFO / Wire Out
 * @defgroup bflb_wo_interface Bouffalolab BL61x GPIO FIFO / Wire Out
 * @ingroup misc_interfaces
 *
 * @{
 */

#include <zephyr/sys/util_macro.h>

#define BFLB_WO_PIN_NONE	255
#define BFLB_WO_PIN_CNT		16

#define BFLB_WO_PINS_EXPAND_16
#define BFLB_WO_PINS_EXPAND_15 BFLB_WO_PIN_NONE,
#define BFLB_WO_PINS_EXPAND_14 BFLB_WO_PIN_NONE, BFLB_WO_PIN_NONE,
#define BFLB_WO_PINS_EXPAND_13 BFLB_WO_PINS_EXPAND_15 BFLB_WO_PINS_EXPAND_14
#define BFLB_WO_PINS_EXPAND_12 BFLB_WO_PINS_EXPAND_14 BFLB_WO_PINS_EXPAND_14
#define BFLB_WO_PINS_EXPAND_11 BFLB_WO_PINS_EXPAND_13 BFLB_WO_PINS_EXPAND_14
#define BFLB_WO_PINS_EXPAND_10 BFLB_WO_PINS_EXPAND_12 BFLB_WO_PINS_EXPAND_14
#define BFLB_WO_PINS_EXPAND_9 BFLB_WO_PINS_EXPAND_11 BFLB_WO_PINS_EXPAND_14
#define BFLB_WO_PINS_EXPAND_8 BFLB_WO_PINS_EXPAND_12 BFLB_WO_PINS_EXPAND_12
#define BFLB_WO_PINS_EXPAND_7 BFLB_WO_PINS_EXPAND_11 BFLB_WO_PINS_EXPAND_12
#define BFLB_WO_PINS_EXPAND_6 BFLB_WO_PINS_EXPAND_10 BFLB_WO_PINS_EXPAND_12
#define BFLB_WO_PINS_EXPAND_5 BFLB_WO_PINS_EXPAND_9 BFLB_WO_PINS_EXPAND_12
#define BFLB_WO_PINS_EXPAND_4 BFLB_WO_PINS_EXPAND_8 BFLB_WO_PINS_EXPAND_12
#define BFLB_WO_PINS_EXPAND_3 BFLB_WO_PINS_EXPAND_5 BFLB_WO_PINS_EXPAND_14
#define BFLB_WO_PINS_EXPAND_2 BFLB_WO_PINS_EXPAND_4 BFLB_WO_PINS_EXPAND_14
#define BFLB_WO_PINS_EXPAND_1 BFLB_WO_PINS_EXPAND_15 BFLB_WO_PINS_EXPAND_2
#define BFLB_WO_PINS_EXPAND( _I ) CONCAT(BFLB_WO_PINS_EXPAND_, _I)
/* This macro can be used to generate the pin map in the structure */
#define BFLB_WO_PINS( ... ) { __VA_ARGS__ , BFLB_WO_PINS_EXPAND(NUM_VA_ARGS( __VA_ARGS__ )) }

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
 *
 *                    total_cycles
 *           |-----------------------------------|-----------------------------------|
 *
 * clock     |‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_
 *
 * data      1                                   0
 *
 * Pin value ‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾|⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽|‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾|⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽⎽
 *
 *           |-----------------------|-----------|---------------|-------------------|---------
 *           |     set_cycles        |total - set|  unset_cycles |  total - unset    | park
 *
 * Pins match such as toggle bit = pin & 16 (eg, pin 0 is bit 0, 17 is bit 1, pin 2 is bit 2, etc).
 *
 */
struct bflb_wo_config {
	uint8_t pins[16]; /* Up to 16 pins, or BFLB_WO_PIN_NONE */
	uint16_t pull_down; /* Pull down mask for the pins */
	uint16_t pull_up; /* Pull up mask for the pins */
	uint16_t total_cycles; /* Total clock cycles length of a toggling cycles (9 bits) */
	uint8_t set_cycles; /* Number of clock cycles a pin is toggled when its bit is set */
	uint8_t unset_cycles; /* Number of clock cycles a pin is toggled when its bit is not set */
	bool set_invert; /* Go from low to high instead of high to low when bit is set */
	bool unset_invert; /* Same but when bit is unset */
	bool park_high; /* Park pins high instead of low at the end of a sequence */
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
 * @retval 0 on success
 * @retval -EINVAL if configuration is invalid, or other error codes.
 */
int bflb_wo_configure(const struct bflb_wo_config *const config);

/**
 * Write to GPIO FIFO
 *
 * @param data Array containing the sequence of toggles
 * @param len Length of the array
 * @retval 0 on success
 * @retval -EIO or other error codes.
 */
int bflb_wo_write(const uint16_t *const data, const size_t len);

#ifdef CONFIG_BFLB_WO_PROTOCOLS

#include <zephyr/drivers/uart.h>

/**
 * Write WS2812B protocol to GPIO FIFO
 *
 * @param pin Output pin
 * @param grb Array containing the colors
 * @param len Length of the array
 * @retval 0 on success
 * @retval -EIO or other error codes.
 */
int bflb_wo_ws2812b(const uint32_t *const rgb, const size_t len, const uint8_t pin);

/**
 * Write UART protocol to GPIO FIFO
 * @param data String to send
 * @param pin Output pin
 * @param uart_cfg UART configuration
 * @retval 0 on success
 * @retval -EIO or other error codes.
 */
int bflb_wo_uart(const char *const data, const uint8_t pin,
		 const struct uart_config *const uart_cfg);

#endif /* CONFIG_BFLB_WO_PROTOCOLS */

/**
 * @}
 */

#endif /* ZEPHYR_DRIVERS_MISC_BFLB_WO_BFLB_WO_H_ */
