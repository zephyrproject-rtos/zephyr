/*
 * Copyright (c) 2025-2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file intc_mchp_eic_g1.h
 * @brief EIC driver header file for Microchip eic g1 peripheral.
 * This can be used to access the APIs implemented for the eic driver.
 */

#ifndef INCLUDE_ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_MCHP_EIC_G1_H_
#define INCLUDE_ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_MCHP_EIC_G1_H_

#include <soc.h>
#include <zephyr/types.h>
#include <zephyr/drivers/gpio.h>

/**
 * @typedef mchp_eic_callback_t
 * @brief EIC ISR callback used to notify the GPIO layer of an interrupt.
 *
 * @param[in] pins Bitmask of GPIO pins that triggered (bit n => pin n).
 * @param[in] data User/driver context pointer associated with the GPIO port.
 *
 * @note Called from interrupt context.
 */
typedef void (*mchp_eic_callback_t)(uint32_t pins, void *data);

/**
 * @enum mchp_eic_trigger
 * @brief EIC interrupt trigger selection.
 */
enum mchp_eic_trigger {
	/** Interrupt on rising edge. */
	MCHP_EIC_RISING = 1,
	/** Interrupt on falling edge. */
	MCHP_EIC_FALLING,
	/** Interrupt on both rising and falling edges. */
	MCHP_EIC_BOTH,
	/** Interrupt when input is high (level). */
	MCHP_EIC_HIGH,
	/** Interrupt when input is low (level). */
	MCHP_EIC_LOW,
};

/**
 * @enum mchp_gpio_port_id
 * @brief Microchip GPIO port identifiers.
 */
enum mchp_gpio_port_id {
	/** GPIO Port 0. */
	MCHP_PORT_ID0 = 0,
	/** GPIO Port 1. */
	MCHP_PORT_ID1,
	/** GPIO Port 2. */
	MCHP_PORT_ID2,
	/** GPIO Port 3. */
	MCHP_PORT_ID3,
	/** Number of ports (sentinel). */
	MCHP_PORT_ID_MAX,
};

/**
 * @enum mchp_eic_line
 * @brief Microchip EIC (External Interrupt Controller) line identifiers.
 */
enum mchp_eic_line {
	/** EIC line 0. */
	EIC_LINE_0 = 0,
	/** EIC line 1. */
	EIC_LINE_1,
	/** EIC line 2. */
	EIC_LINE_2,
	/** EIC line 3. */
	EIC_LINE_3,
	/** EIC line 4. */
	EIC_LINE_4,
	/** EIC line 5. */
	EIC_LINE_5,
	/** EIC line 6. */
	EIC_LINE_6,
	/** EIC line 7. */
	EIC_LINE_7,
	/** EIC line 8. */
	EIC_LINE_8,
	/** EIC line 9. */
	EIC_LINE_9,
	/** EIC line 10. */
	EIC_LINE_10,
	/** EIC line 11. */
	EIC_LINE_11,
	/** EIC line 12. */
	EIC_LINE_12,
	/** EIC line 13. */
	EIC_LINE_13,
	/** EIC line 14. */
	EIC_LINE_14,
	/** EIC line 15. */
	EIC_LINE_15,
	/** Number of EIC lines (sentinel). */
	EIC_LINE_MAX,
};

/**
 * @struct eic_config_params
 * @brief Parameters used to configure a Microchip EIC line for a GPIO pin.
 */
struct eic_config_params {
	/** Trigger type (edge/level) for the EIC line. */
	enum mchp_eic_trigger trig_type;

	/** GPIO pin number within the port. */
	uint8_t pin_num;

	/** GPIO port identifier */
	uint8_t port_id;

	/** Enable/disable input debouncing for this line. */
	bool debounce;

	/** Pointer to GPIO driver/user context passed back in the callback. */
	void *gpio_data;

	/** Base address of the GPIO port registers */
	port_group_registers_t *port_addr;

	/** Callback invoked from EIC ISR to notify the GPIO layer. */
	mchp_eic_callback_t eic_line_callback;
};

/**
 * @brief Configure and enable an EIC interrupt for a given pin.
 *
 * This function configures the EIC interrupt for the specified pin, including setting up
 * the trigger type, enabling input, configuring debounce if required,
 * and updating the internal data structures to reflect the new assignment of pin to an eic line.
 *
 * @param eic_pin_config Pointer to the EIC pin configuration structure.
 *
 * @return 0 on success, or a negative error code on failure:
 *         - -ENOTSUP if no EIC line is available for the pin.
 *         - -EBUSY if the EIC line is already in use.
 */
int eic_mchp_config_interrupt(struct eic_config_params *eic_pin_config);

/**
 * @brief Get the pending EIC interrupts for a given port.
 *
 * This function checks which pins in the specified port have pending EIC interrupts.
 * It returns a bitmask where each set bit corresponds to a pin with a pending interrupt.
 *
 * @param port_id The identifier of the port to check for pending interrupts.
 *
 * @return A bitmask indicating which pins in the port have pending interrupts.
 *         Each bit set in the return value corresponds to a pin number with a pending interrupt.
 *
 */
uint32_t eic_mchp_interrupt_pending(uint8_t port_id);

/**
 * @brief Disables an EIC interrupt for a given pin configuration.
 *
 * This function disables the EIC interrupt associated with the specified pin configuration.
 * It checks if the pin is currently assigned to an EIC line, disables the interrupt line if so,
 * and updates the internal data structures to mark the line as free.
 *
 * @param eic_pin_config Pointer to the EIC pin configuration structure.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int eic_mchp_disable_interrupt(struct eic_config_params *eic_pin_config);

#endif /*INCLUDE_ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_MCHP_EIC_G1_H_*/
