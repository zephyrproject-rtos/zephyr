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

#ifndef __SCSS_H__
#define __SCSS_H__

#include "qm_common.h"
#include "qm_soc_regs.h"
#include <x86intrin.h>

/**
 * System control subsystem for Quark Microcontrollers.
 *
 * @defgroup groupSCSS System Control Subsystem
 * @{
 */

#define OSC0_SI_FREQ_SEL_MASK (0xFFFFFCFF)

/**
 * When using an external crystal, this value must be set to the number of
 * system ticks per micro second. The expected value is 32 ticks for a 32MHz
 * crystal.
 */
#define SYS_TICKS_PER_US_XTAL (32)
#define SYS_TICKS_PER_US_32MHZ (32)
#define SYS_TICKS_PER_US_16MHZ (16)
#define SYS_TICKS_PER_US_8MHZ (8)
#define SYS_TICKS_PER_US_4MHZ (4)

/**
 * System clock divider type.
 */
typedef enum {
	CLK_SYS_DIV_1,
	CLK_SYS_DIV_2,
	CLK_SYS_DIV_4,
	CLK_SYS_DIV_8,
#if (QUARK_D2000)
	CLK_SYS_DIV_16,
	CLK_SYS_DIV_32,
	CLK_SYS_DIV_64,
	CLK_SYS_DIV_128,
#endif
	CLK_SYS_DIV_NUM
} clk_sys_div_t;

/**
 * System clock mode type.
 */
typedef enum {
	CLK_SYS_HYB_OSC_32MHZ,
	CLK_SYS_HYB_OSC_16MHZ,
	CLK_SYS_HYB_OSC_8MHZ,
	CLK_SYS_HYB_OSC_4MHZ,
	CLK_SYS_RTC_OSC,
	CLK_SYS_CRYSTAL_OSC
} clk_sys_mode_t;

/**
 * Peripheral clock divider type.
 */
typedef enum {
	CLK_PERIPH_DIV_1,
	CLK_PERIPH_DIV_2,
	CLK_PERIPH_DIV_4,
	CLK_PERIPH_DIV_8
} clk_periph_div_t;

/**
 * GPIO clock debounce divider type.
 */

typedef enum {
	CLK_GPIO_DB_DIV_1,
	CLK_GPIO_DB_DIV_2,
	CLK_GPIO_DB_DIV_4,
	CLK_GPIO_DB_DIV_8,
	CLK_GPIO_DB_DIV_16,
	CLK_GPIO_DB_DIV_32,
	CLK_GPIO_DB_DIV_64,
	CLK_GPIO_DB_DIV_128
} clk_gpio_db_div_t;

/**
 * External crystal clock divider type.
 */
typedef enum {
	CLK_EXT_DIV_1,
	CLK_EXT_DIV_2,
	CLK_EXT_DIV_4,
	CLK_EXT_DIV_8
} clk_ext_div_t;

/**
 * RTC clock divider type.
 */
typedef enum {
	CLK_RTC_DIV_1,
	CLK_RTC_DIV_2,
	CLK_RTC_DIV_4,
	CLK_RTC_DIV_8,
	CLK_RTC_DIV_16,
	CLK_RTC_DIV_32,
	CLK_RTC_DIV_64,
	CLK_RTC_DIV_128,
	CLK_RTC_DIV_256,
	CLK_RTC_DIV_512,
	CLK_RTC_DIV_1024,
	CLK_RTC_DIV_2048,
	CLK_RTC_DIV_4096,
	CLK_RTC_DIV_8192,
	CLK_RTC_DIV_16384,
	CLK_RTC_DIV_32768
} clk_rtc_div_t;

/**
 * SCSS peripheral clock register type.
 */
typedef enum {
	CLK_PERIPH_REGISTER = BIT(0),
	CLK_PERIPH_CLK = BIT(1),
	CLK_PERIPH_I2C_M0 = BIT(2),
#if (QUARK_SE)
	CLK_PERIPH_I2C_M1 = BIT(3),
#endif
	CLK_PERIPH_SPI_S = BIT(4),
	CLK_PERIPH_SPI_M0 = BIT(5),
#if (QUARK_SE)
	CLK_PERIPH_SPI_M1 = BIT(6),
#endif
	CLK_PERIPH_GPIO_INTERRUPT = BIT(7),
	CLK_PERIPH_GPIO_DB = BIT(8),
#if (QUARK_SE)
	CLK_PERIPH_I2S = BIT(9),
#endif
	CLK_PERIPH_WDT_REGISTER = BIT(10),
	CLK_PERIPH_RTC_REGISTER = BIT(11),
	CLK_PERIPH_PWM_REGISTER = BIT(12),
	CLK_PERIPH_GPIO_REGISTER = BIT(13),
	CLK_PERIPH_SPI_M0_REGISTER = BIT(14),
#if (QUARK_SE)
	CLK_PERIPH_SPI_M1_REGISTER = BIT(15),
#endif
	CLK_PERIPH_SPI_S_REGISTER = BIT(16),
	CLK_PERIPH_UARTA_REGISTER = BIT(17),
	CLK_PERIPH_UARTB_REGISTER = BIT(18),
	CLK_PERIPH_I2C_M0_REGISTER = BIT(19),
#if (QUARK_SE)
	CLK_PERIPH_I2C_M1_REGISTER = BIT(20),
	CLK_PERIPH_I2S_REGISTER = BIT(21),
	CLK_PERIPH_ALL = 0x3FFFFF
#elif(QUARK_D2000)
	CLK_PERIPH_ADC = BIT(22),
	CLK_PERIPH_ADC_REGISTER = BIT(23),
	CLK_PERIPH_ALL = 0xCFFFFF
#else
#error "Unsupported / unspecified interrupt controller detected."
#endif
} clk_periph_t;

/**
 * Change the operating mode and clock divisor of the system
 * clock source. Changing this clock speed affects all
 * peripherals.
 *
 * @param [in] mode System clock source operating mode
 * @param [in] div System clock divisor.
 * @return qm_rc_t QM_RC_OK on success, error code otherwise.
 */
qm_rc_t clk_sys_set_mode(const clk_sys_mode_t mode, const clk_sys_div_t div);

/**
 * Change ADC clock divider value. The new divider value is set to N, where N is
 * the value set by the function and is between 1 and 1024.
 *
 * @brief Change divider value of ADC clock.
 * @param [in] div Divider value for the ADC clock.
 * @return qm_rc_t QM_RC_OK on success, error code otherwise.
 */
qm_rc_t clk_adc_set_div(const uint16_t div);

/**
 * Change Peripheral clock divider value. The maximum divisor is
 * /8. These peripherals include GPIO Interrupt, SPI, I2C and
 * ADC.
 *
 * @brief Change divider value of peripheral clock.
 * @param [in] div Divider value for the peripheral clock.
 * @return qm_rc_t QM_RC_OK on success, error code otherwise.
 */
qm_rc_t clk_periph_set_div(const clk_periph_div_t div);

/**
 * Change GPIO debounce clock divider value. The maximum divisor
 * is /128.
 *
 * @brief Change divider value of GPIO debounce clock.
 * @param [in] div Divider value for the GPIO debounce clock.
 * @return qm_rc_t QM_RC_OK on success, error code otherwise.
 */
qm_rc_t clk_gpio_db_set_div(const clk_gpio_db_div_t div);

/**
 * Change External clock divider value. The maximum divisor is
 * /8.
 *
 * @brief Change divider value of external clock.
 * @param [in] div Divider value for the external clock.
 * @return qm_rc_t QM_RC_OK on success, error code otherwise.
 */
qm_rc_t clk_ext_set_div(const clk_ext_div_t div);

/**
 * Change RTC divider value. The maximum divisor is /32768.
 *
 * @brief Change divider value of RTC.
 * @param [in] div Divider value for the RTC.
 * @return qm_rc_t QM_RC_OK on success, error code otherwise.
 */
qm_rc_t clk_rtc_set_div(const clk_rtc_div_t div);

/**
 * Enable clocks for peripherals / registers.
 *
 * @param [in] clocks Which peripheral and register clocks to enable.
 * @return qm_rc_t QM_RC_OK on success, error code otherwise.
 */
qm_rc_t clk_periph_enable(const clk_periph_t clocks);

/**
 * Disable clocks for peripherals / registers.
 *
 * @param [in] clocks Which peripheral and register clocks to disable.
 * @return qm_rc_t QM_RC_OK on success, error code otherwise.
 */
qm_rc_t clk_periph_disable(const clk_periph_t clocks);

/**
 * Get number of system ticks per micro second.
 *
 * @return uint32_t Number of system ticks per micro second.
 */
uint32_t clk_sys_get_ticks_per_us(void);

/**
 * @brief Idle loop the processor for at least the value given in microseconds.
 *
 * This function will wait until at least the given number of microseconds has
 * elapsed since calling this function. Note it is dependent on the system clock
 * speed. The delay parameter does not include, calling the function, returning
 * from it, calculation setup and while loops.
 *
 * @param [in] microseconds Minimum number of micro seconds to delay for.
 * @return void.
 */
void clk_sys_udelay(uint32_t microseconds);
/**
 * @}
 */

#endif /* __SCSS_H__ */
