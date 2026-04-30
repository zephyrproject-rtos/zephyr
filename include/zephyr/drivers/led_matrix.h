/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2026 Shankar Sridhar
 *
 * include/zephyr/drivers/led_matrix/led_matrix.h
 *
 * Private API for Charlieplex LED matrix drivers.
 *
 * This header is the contract between the generic led_matrix.c driver and
 * a board-specific HAL (e.g. uno_q_matrix_hal.c).  It is not intended for
 * application use — applications should include <zephyr/drivers/display.h>.
 */

#ifndef ZEPHYR_DRIVERS_LED_MATRIX_LED_MATRIX_H_
#define ZEPHYR_DRIVERS_LED_MATRIX_LED_MATRIX_H_

#include <stdint.h>
#include <stdbool.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Pin pair describing one Charlieplex LED.
 *
 * To light the LED: drive anode_pin HIGH, cathode_pin LOW, all others Hi-Z.
 */
struct led_matrix_pin_pair {
	uint32_t anode_pin;
	uint32_t cathode_pin;
};

/**
 * @brief HAL operations that a board-specific file must implement.
 *
 * led_matrix.c calls these exclusively — it never touches GPIO registers
 * or gpio_dt_spec directly.
 */
struct led_matrix_hal_api {
	/**
	 * @brief Set a single matrix pin to HIGH, LOW, or Hi-Z.
	 *
	 * @param pin_index  Index into the board's GPIO array (0 .. num_pins-1).
	 * @param state       1 = OUTPUT HIGH (anode)
	 *                    0 = OUTPUT LOW  (cathode)
	 *                   -1 = INPUT / Hi-Z (disconnected)
	 */
	void (*led_matrix_set_pin)(uint32_t pin_index, int state);

	/**
	 * @brief Drive all matrix pins to Hi-Z in one call.
	 *
	 * Called at the start of every ISR tick to extinguish the current LED
	 * before lighting the next one.
	 */
	void (*led_matrix_all_pins_highz)(void);

	/**
	 * @brief Configure all GPIO pins and verify they are ready.
	 *
	 * Called once from led_matrix_init().  Returns 0 on success or a
	 * negative errno if any GPIO is not ready.
	 */
	int (*led_matrix_init_pins)(void);
};

/**
 * @brief Compile-time configuration — provided by the board-specific HAL.
 *
 * The HAL instantiates one of these (const, in flash) per device instance
 * and passes a pointer to DEVICE_DT_INST_DEFINE.
 */
struct led_matrix_config {
	/** HAL operations implemented by the board-specific file. */
	const struct led_matrix_hal_api *hal;

	/**
	 * Flat pin-pair lookup table, one entry per LED.
	 * Index: LED position 0 .. (rows * cols - 1).
	 */
	const struct led_matrix_pin_pair *pin_map;

	/** Number of entries in pin_map (rows * cols). */
	uint8_t pin_map_len;

	/** Number of rows in the LED grid. */
	uint8_t rows;

	/** Number of columns in the LED grid. */
	uint8_t cols;
};

/**
 * @brief Runtime state — allocated per device, owned by led_matrix.c.
 *
 * The HAL allocates the storage (static struct led_matrix_data) but never
 * reads or writes any field directly.
 */
struct led_matrix_data {

    /**
     * Save struct device pointer for accessing in timer ISR.
     */ 
    const struct device *dev;

	/**
	 * Flat framebuffer: one byte per LED
	 * Index: row * cols + col.
	 */
	uint8_t framebuffer[104];

	/**
	 * Active-list: indices into pin_map[] for LEDs that are currently ON.
	 * Rebuilt whenever the framebuffer changes.
	 */
	uint8_t active_list[104];

	/** Number of valid entries in active_list[]. */
	uint8_t active_count;

	/** ISR position within active_list[]. */
	uint8_t current_slot;

	/** Stored brightness value (0-255). Wired to on-time scaling later. */
	uint8_t brightness;

	/** When true the ISR keeps all pins Hi-Z (display_blanking_on). */
	bool blanked;

	/** Multiplexing timer — one tick per LED slot. */
	struct k_timer timer;
};

/**
 * @brief Generic LED matrix init function.
 *
 * The board-specific HAL passes this as the init_fn argument to
 * DEVICE_DT_INST_DEFINE.  It calls config->hal->led_matrix_init_pins(),
 * clears the framebuffer, and starts the multiplexing timer.
 *
 * @param dev  Device pointer provided by the Zephyr device model.
 * @return     0 on success, negative errno on failure.
 */
int led_matrix_init(const struct device *dev);

/**
 * @brief display_driver_api instance implemented by led_matrix.c.
 *
 * The board-specific HAL passes &led_matrix_display_api as the api argument
 * to DEVICE_DT_INST_DEFINE so the device participates in the standard Zephyr
 * display subsystem.
 */
extern const struct display_driver_api led_matrix_display_api;

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_LED_MATRIX_LED_MATRIX_H_ */
