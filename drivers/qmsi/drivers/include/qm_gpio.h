/*
 * Copyright (c) 2015, Intel Corporation
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

#ifndef __QM_GPIO_H__
#define __QM_GPIO_H__

#include "qm_common.h"
#include "qm_soc_regs.h"

/**
 * General Purpose IO for Quark Microcontrollers.
 *
 * @defgroup groupGPIO GPIO
 * @{
 */

/**
* GPIO port configuration type. Each bit in the registers control a GPIO pin.
*/
typedef struct {
	uint32_t direction;    /* GPIO direction, 0b: input, 1b: output */
	uint32_t int_en;       /* Interrupt enable */
	uint32_t int_type;     /* Interrupt type, 0b: level; 1b: edge */
	uint32_t int_polarity; /* Interrupt polarity, 0b: low, 1b: high */
	uint32_t int_debounce; /* Debounce on/off */
	uint32_t int_bothedge; /* Interrupt on both rising and falling edges */
	void (*callback)(uint32_t int_status); /* Callback function */
} qm_gpio_port_config_t;

/**
 * GPIO Interrupt Service Routine
 */
void qm_gpio_isr_0(void);
#if (HAS_AON_GPIO)
void qm_aon_gpio_isr_0(void);
#endif /* HAS_AON_GPIO */

/**
 * Set GPIO port configuration. This includes if interrupts are enabled or not,
 * the level on which an interrupt is generated, the polarity of interrupts and
 * if GPIO-debounce is enabled or not. If interrupts are enabled it also
 * registers an ISR with the user defined callback function.
 *
 * @brief Set GPIO port configuration.
 * @param[in] gpio GPIO port index to configure.
 * @param[in] cfg New configuration for GPIO port.
 * @return qm_rc_t QM_RC_OK on success, error code otherwise.
 */
qm_rc_t qm_gpio_set_config(const qm_gpio_t gpio,
			   const qm_gpio_port_config_t *const cfg);

/**
 * Get GPIO port configuration. This includes if interrupts are enabled or not,
 * the level on which an interrupt is generated, the polarity of interrupts and
 * if GPIO-debounce is enabled or not.
 *
 * @brief Get GPIO port configuration.
 * @param[in] gpio GPIO port index to read the configuration of.
 * @param[out] cfg Current configuration for GPIO port.
 * @return qm_rc_t QM_RC_OK on success, error code otherwise.
 */
qm_rc_t qm_gpio_get_config(const qm_gpio_t gpio,
			   qm_gpio_port_config_t *const cfg);

/**
 * Read the current value of a single pin on a given GPIO port.
 *
 * @param[in] gpio GPIO port index.
 * @param[in] pin Pin of GPIO port to read.
 * @return bool Value of the pin specified on GPIO port.
 */
bool qm_gpio_read_pin(const qm_gpio_t gpio, const uint8_t pin);

/**
 * Set a single pin on a given GPIO port.
 *
 * @param[in] gpio GPIO port index.
 * @param[in] pin Pin of GPIO port to set.
 * @return qm_rc_t QM_RC_OK on success, error code otherwise.
 */
qm_rc_t qm_gpio_set_pin(const qm_gpio_t gpio, const uint8_t pin);

/**
 * Clear a single pin on a given GPIO port.
 *
 * @param[in] gpio GPIO port index.
 * @param[in] pin Pin of GPIO port to clear.
 * @return qm_rc_t QM_RC_OK on success, error code otherwise.
 */
qm_rc_t qm_gpio_clear_pin(const qm_gpio_t gpio, const uint8_t pin);

/**
 * Read entire GPIO port. Each bit of the val parameter is set to the current
 * value of each pin on the port. Maximum 32 pins per port.
 *
 * @brief Get GPIO port values.
 * @param[in] gpio GPIO port index.
 * @return uint32_t Value of all pins on GPIO port.
 */
uint32_t qm_gpio_read_port(const qm_gpio_t gpio);

/**
 * Write entire GPIO port. Each pin on the GPIO port is set to the
 * corresponding value set in the val parameter. Maximum 32 pins per port.
 *
 * @brief Get GPIO port values.
 * @param[in] gpio GPIO port index.
 * @param[in] val Value of all pins on GPIO port.
 * @return qm_rc_t QM_RC_OK on success, error code otherwise.
 */
qm_rc_t qm_gpio_write_port(const qm_gpio_t gpio, const uint32_t val);

/**
 * @}
 */

#endif /* __QM_GPIO_H__ */
