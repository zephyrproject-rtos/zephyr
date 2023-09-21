/*
 * Copyright (c) 2022 Schlumberger
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_XMC4XXX_INTC_H_
#define ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_XMC4XXX_INTC_H_

/**
 * @brief Enable interrupt for specific port_id and pin combination
 *
 * @param port_id Port index
 * @param pin pin Pin the port
 * @param mode Level or edge interrupt
 * @param trig Trigger edge type (falling, rising or both)
 * @param fn Callback function
 * @param user_data Parameter to the interrupt callback
 *
 * @retval  0 On success
 * @retval -ENOTSUP If the specific port_id/pin combination is not supported or
 * not defined in the dts
 * @retval -EBUSY  If the interrupt line is already used by a different port_id/pin
 * @retval -EINVAL If the trigger combination is invalid
 *
 */

int intc_xmc4xxx_gpio_enable_interrupt(int port_id, int pin, enum gpio_int_mode mode,
		enum gpio_int_trig trig, void(*fn)(const struct device*, int), void *user_data);

/**
 * @brief Disable interrupt for specific port_id and pin combination
 *
 * @param port_id Port index
 * @param pin pin Pin the port
 * @retval 0 On susccess
 * @retval -EINVAL If the specific port_id and pin combination has no interrupt
 * enabled
 */

int intc_xmc4xxx_gpio_disable_interrupt(int port_id, int pin);

#endif /* ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_XMC4XXX_INTC_H_ */
