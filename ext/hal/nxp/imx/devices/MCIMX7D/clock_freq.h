/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __CLOCK_FREQ_H__
#define __CLOCK_FREQ_H__

#include "device_imx.h"

/*!
 * @addtogroup clock_freq_helper
 * @{
 */

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @brief Get clock frequency applies to the GPT module
 *
 * @param base GPT base pointer.
 * @return clock frequency (in HZ) applies to the GPT module
 */
uint32_t get_gpt_clock_freq(GPT_Type *base);

/*!
 * @brief Get clock frequency applies to the ECSPI module
 *
 * @param base ECSPI base pointer.
 * @return clock frequency (in HZ) applies to the ECSPI module
 */
uint32_t get_ecspi_clock_freq(ECSPI_Type *base);

/*!
 * @brief Get clock frequency applies to the FLEXCAN module
 *
 * @param base CAN base pointer.
 * @return clock frequency (in HZ) applies to the FLEXCAN module
 */
uint32_t get_flexcan_clock_freq(CAN_Type *base);

/*!
 * @brief Get clock frequency applies to the I2C module
 *
 * @param base I2C base pointer.
 * @return clock frequency (in HZ) applies to the I2C module
 */
uint32_t get_i2c_clock_freq(I2C_Type *base);

/*!
 * @brief Get clock frequency applies to the UART module
 *
 * @param base UART base pointer.
 * @return clock frequency (in HZ) applies to the UART module
 */
uint32_t get_uart_clock_freq(UART_Type *base);

#if defined(__cplusplus)
}
#endif

/*! @}*/

#endif /* __CLOCK_FREQ_H__ */
/*******************************************************************************
 * EOF
 ******************************************************************************/
