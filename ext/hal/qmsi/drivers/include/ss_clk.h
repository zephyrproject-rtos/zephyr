/*
* Copyright (c) 2017, Intel Corporation
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

#ifndef __SS_CLK_H__
#define __SS_CLK_H__

#include "qm_common.h"
#include "qm_sensor_regs.h"
#include "clk.h"

/**
 * Clock Management for Sensor Subsystem.
 *
 * The clock distribution has three level of gating:
 * 1. SE SoC gating through register CCU_PERIPH_CLK_GATE_CTL
 * 2. SS Soc gating through register IO_CREG_MST0_CTRL (IO_CREG_MST0_CTRL)
 * 3. SS peripheral clk gating
 * Note: the first two are ungated by hardware power-on default (clock gating is
 * done at peripheral level). Thus the only one level of control is enough (and
 * implemented in ss_clk driver) to gate clock on or off to the particular
 * peripheral.
 *
 * @defgroup groupSSClock SS Clock
 * @{
 */

/**
 * Peripheral clocks selection type.
 */
typedef enum {
	SS_CLK_PERIPH_ADC = BIT(31),   /**< ADC clock selector. */
	SS_CLK_PERIPH_I2C_1 = BIT(30), /**< I2C 1 clock selector. */
	SS_CLK_PERIPH_I2C_0 = BIT(29), /**< I2C 0 clock selector. */
	SS_CLK_PERIPH_SPI_1 = BIT(28), /**< SPI 1 clock selector. */
	SS_CLK_PERIPH_SPI_0 = BIT(27), /**< SPI 0 clock selector. */

	/**
	 * GPIO 1 clock selector.
	 *
	 * Special domain peripherals - these do not map onto the standard
	 * register.
	 */
	SS_CLK_PERIPH_GPIO_1 = BIT(1),
	/**
	 * GPIO 0 clock selector.
	 *
	 * Special domain peripherals - these do not map onto the standard
	 * register.
	 */
	SS_CLK_PERIPH_GPIO_0 = BIT(0)
} ss_clk_periph_t;

/**
 * Enable clocking for SS GPIO peripheral.
 *
 * @param [in] gpio GPIO port index.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int ss_clk_gpio_enable(const qm_ss_gpio_t gpio);

/**
 * Disable clocking for SS GPIO peripheral.
 *
 * @param [in] gpio GPIO port index.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int ss_clk_gpio_disable(const qm_ss_gpio_t gpio);

/**
 * Enable clocking for SS SPI peripheral.
 *
 * @param [in] spi SPI port index.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int ss_clk_spi_enable(const qm_ss_spi_t spi);

/**
 * Disable clocking for SS SPI peripheral.
 *
 * @param [in] spi SPI port index.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int ss_clk_spi_disable(const qm_ss_spi_t spi);

/**
 * Enable clocking for SS I2C peripheral.
 *
 * @param [in] i2c I2C port index.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int ss_clk_i2c_enable(const qm_ss_i2c_t i2c);

/**
 * Disable clocking for SS I2C peripheral.
 *
 * @param [in] i2c I2C port index.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int ss_clk_i2c_disable(const qm_ss_i2c_t i2c);

/**
 * Enable the SS ADC clock.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 */
int ss_clk_adc_enable(void);

/**
 * Disable the SS ADC clock.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 */
int ss_clk_adc_disable(void);

/**
 * Set clock divisor for SS ADC.
 *
 * Note: If the system clock speed is changed, the divisor must be recalculated.
 * The minimum supported speed for the SS ADC is 0.14 MHz. So for a system clock
 * speed of 1 MHz, the max value of div is 7, and for 32 MHz, the max value is
 * 224. System clock speeds of less than 1 MHz are not supported by this
 * function.
 *
 * @param [in] div ADC clock divider value.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int ss_clk_adc_set_div(const uint32_t div);

/**
 * @}
 */

#endif /* __SS_CLK_H__ */
