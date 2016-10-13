/*
 * Copyright (c) 2016, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the Intel Corporation nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL CORPORATION OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __QM_SS_GPIO_H__
#define __QM_SS_GPIO_H__

#include "qm_common.h"
#include "qm_sensor_regs.h"

/**
 * General Purpose IO for Sensor Subsystem.
 *
 * @defgroup groupSSGPIO SS GPIO
 * @{
 */

/**
 * GPIO SS pin states.
 */
typedef enum {
	QM_SS_GPIO_LOW = 0,  /**< Pin level high. */
	QM_SS_GPIO_HIGH,     /**< Pin level low. */
	QM_SS_GPIO_STATE_NUM /**< Number of states. */
} qm_ss_gpio_state_t;

/**
* SS GPIO port configuration type.
*
* Each bit in the registers control a GPIO pin.
*/
typedef struct {
	uint32_t direction;    /**< SS GPIO direction, 0b: input, 1b: output. */
	uint32_t int_en;       /**< Interrupt enable. */
	uint32_t int_type;     /**< Interrupt type, 0b: level; 1b: edge. */
	uint32_t int_polarity; /**< Interrupt polarity, 0b: low, 1b: high. */
	uint32_t int_debounce; /**< Debounce on/off. */

#if HAS_SS_GPIO_INTERRUPT_BOTHEDGE
	uint32_t int_bothedge; /**< Interrupt on rising and falling edges */
#endif

	/**
	 * User callback.
	 *
	 * Called for any interrupt on the Sensor Subsystem GPIO.
	 *
	 * @param[in] data The callback user data.
	 * @param[in] int_status Bitfield of triggered pins.
	 */
	void (*callback)(void *data, uint32_t int_status);
	void *callback_data; /**< Callback user data. */
} qm_ss_gpio_port_config_t;

/**
 * Set SS GPIO port configuration.
 *
 * This includes the direction of the pins, if interrupts are enabled or not,
 * the level on which an interrupt is generated, the polarity of interrupts
 * and if GPIO-debounce is enabled or not. If interrupts are enabled it also
 * registers the user defined callback function.
 *
 * @param[in] gpio SS GPIO port index to configure.
 * @param[in] cfg New configuration for SS GPIO port. This must not be NULL.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_ss_gpio_set_config(const qm_ss_gpio_t gpio,
			  const qm_ss_gpio_port_config_t *const cfg);

/**
 * Read the current value of a single pin on a given SS GPIO port.
 *
 * @param[in] gpio SS GPIO port index.
 * @param[in] pin Pin of SS GPIO port to read.
 * @param[out] state QM_GPIO_LOW for low or QM_GPIO_HIGH for high. This must not
 * be NULL.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_ss_gpio_read_pin(const qm_ss_gpio_t gpio, const uint8_t pin,
			qm_ss_gpio_state_t *const state);

/**
 * Set a single pin on a given SS GPIO port.
 *
 * @param[in] gpio SS GPIO port index.
 * @param[in] pin Pin of SS GPIO port to set.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_ss_gpio_set_pin(const qm_ss_gpio_t gpio, const uint8_t pin);

/**
 * Clear a single pin on a given SS GPIO port.
 *
 * @param[in] gpio SS GPIO port index.
 * @param[in] pin Pin of SS GPIO port to clear.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_ss_gpio_clear_pin(const qm_ss_gpio_t gpio, const uint8_t pin);

/**
 * Set or clear a single SS GPIO pin using a state variable.
 *
 * @param[in] gpio GPIO port index.
 * @param[in] pin Pin of GPIO port to update.
 * @param[in] state QM_GPIO_LOW for low or QM_GPIO_HIGH for high.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_ss_gpio_set_pin_state(const qm_ss_gpio_t gpio, const uint8_t pin,
			     const qm_ss_gpio_state_t state);

/**
 * Get SS GPIO port values.
 *
 * Read entire SS GPIO port. Each bit of the val parameter is set to the current
 * value of each pin on the port. Maximum 32 pins per port.
 *
 * @param[in] gpio SS GPIO port index.
 * @param[out] port Value of all pins on GPIO port. This must not be NULL.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_ss_gpio_read_port(const qm_ss_gpio_t gpio, uint32_t *const port);

/**
 * Get SS GPIO port values.
 *
 * Write entire SS GPIO port. Each pin on the SS GPIO port is set to the
 * corresponding value set in the val parameter. Maximum 32 pins per port.
 *
 * @param[in] gpio SS GPIO port index.
 * @param[in] val Value of all pins on SS GPIO port.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_ss_gpio_write_port(const qm_ss_gpio_t gpio, const uint32_t val);

/**
 * @}
 */

#endif /* __QM_SS_GPIO_H__ */
