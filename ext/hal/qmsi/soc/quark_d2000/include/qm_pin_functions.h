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

#ifndef __QM_PIN_FUNCTIONS_H__
#define __QM_PIN_FUNCTIONS_H__
/**
 * SoC Pins definition.
 *
 * @defgroup group SOC_PINS
 * @{
 */

#include "qm_common.h"

/*
 * This file provides an abstraction layer for pin numbers and pin functions.
 */

/* Package pins to pin IDs. */

/* QFN40 package. */
#define QM_PIN_ID_QFN40_31 QM_PIN_ID_0
#define QM_PIN_ID_QFN40_32 QM_PIN_ID_1
#define QM_PIN_ID_QFN40_33 QM_PIN_ID_2
#define QM_PIN_ID_QFN40_34 QM_PIN_ID_3
#define QM_PIN_ID_QFN40_35 QM_PIN_ID_4
#define QM_PIN_ID_QFN40_36 QM_PIN_ID_5
#define QM_PIN_ID_QFN40_37 QM_PIN_ID_6
#define QM_PIN_ID_QFN40_38 QM_PIN_ID_7
#define QM_PIN_ID_QFN40_39 QM_PIN_ID_8
#define QM_PIN_ID_QFN40_11 QM_PIN_ID_9
#define QM_PIN_ID_QFN40_2 QM_PIN_ID_10
#define QM_PIN_ID_QFN40_3 QM_PIN_ID_11
#define QM_PIN_ID_QFN40_4 QM_PIN_ID_12
#define QM_PIN_ID_QFN40_5 QM_PIN_ID_13
#define QM_PIN_ID_QFN40_6 QM_PIN_ID_14
#define QM_PIN_ID_QFN40_7 QM_PIN_ID_15
#define QM_PIN_ID_QFN40_8 QM_PIN_ID_16
#define QM_PIN_ID_QFN40_9 QM_PIN_ID_17
#define QM_PIN_ID_QFN40_10 QM_PIN_ID_18
#define QM_PIN_ID_QFN40_18 QM_PIN_ID_19
#define QM_PIN_ID_QFN40_13 QM_PIN_ID_20
#define QM_PIN_ID_QFN40_14 QM_PIN_ID_21
#define QM_PIN_ID_QFN40_15 QM_PIN_ID_22
#define QM_PIN_ID_QFN40_16 QM_PIN_ID_23
#define QM_PIN_ID_QFN40_21 QM_PIN_ID_24

/* Pin function name to pin function number. */

/* Pin ID 0. */
#define QM_PIN_0_FN_GPIO_0 QM_PMUX_FN_0
#define QM_PIN_0_FN_AIN_0 QM_PMUX_FN_1
#define QM_PIN_0_FN_SPI0_M_CS_B_0 QM_PMUX_FN_2

/* Pin ID 1. */
#define QM_PIN_1_FN_GPIO_1 QM_PMUX_FN_0
#define QM_PIN_1_FN_AIN_1 QM_PMUX_FN_1
#define QM_PIN_1_FN_SPI0_M_CS_B_1 QM_PMUX_FN_2

/* Pin ID 2. */
#define QM_PIN_2_FN_GPIO_2 QM_PMUX_FN_0
#define QM_PIN_2_FN_AIN_2 QM_PMUX_FN_1
#define QM_PIN_2_FN_SPI0_M_CS_B_2 QM_PMUX_FN_2

/* Pin ID 3. */
#define QM_PIN_3_FN_GPIO_3 QM_PMUX_FN_0
#define QM_PIN_3_FN_AIN_3 QM_PMUX_FN_1
#define QM_PIN_3_FN_SPI0_M_CS_B_3 QM_PMUX_FN_2

/* Pin ID 4. */
#define QM_PIN_4_FN_GPIO_4 QM_PMUX_FN_0
#define QM_PIN_4_FN_AIN_4 QM_PMUX_FN_1
#define QM_PIN_4_FN_RTC_CLK_OUT QM_PMUX_FN_2

/* Pin ID 5. */
#define QM_PIN_5_FN_GPIO_5 QM_PMUX_FN_0
#define QM_PIN_5_FN_AIN_5 QM_PMUX_FN_1
#define QM_PIN_5_FN_SYS_CLK_OUT QM_PMUX_FN_2

/* Pin ID 6. */
#define QM_PIN_6_FN_GPIO_6 QM_PMUX_FN_0
#define QM_PIN_6_FN_AIN_6 QM_PMUX_FN_1
#define QM_PIN_6_FN_I2C0_SCL QM_PMUX_FN_2

/* Pin ID 7. */
#define QM_PIN_7_FN_GPIO_7 QM_PMUX_FN_0
#define QM_PIN_7_FN_AIN_7 QM_PMUX_FN_1
#define QM_PIN_7_FN_I2C0_SDA QM_PMUX_FN_2

/* Pin ID 8. */
#define QM_PIN_8_FN_GPIO_8 QM_PMUX_FN_0
#define QM_PIN_8_FN_AIN_8 QM_PMUX_FN_1
#define QM_PIN_8_FN_SPI_S_SCK QM_PMUX_FN_2

/* Pin ID 9. */
#define QM_PIN_9_FN_GPIO_9 QM_PMUX_FN_0
#define QM_PIN_9_FN_AIN_9 QM_PMUX_FN_1
#define QM_PIN_9_FN_SPI_S_MOSI QM_PMUX_FN_2

/* Pin ID 10. */
#define QM_PIN_10_FN_GPIO_10 QM_PMUX_FN_0
#define QM_PIN_10_FN_AIN_10 QM_PMUX_FN_1
#define QM_PIN_10_FN_SPI_S_MISO QM_PMUX_FN_2

/* Pin ID 11. */
#define QM_PIN_11_FN_GPIO_11 QM_PMUX_FN_0
#define QM_PIN_11_FN_AIN_11 QM_PMUX_FN_1
#define QM_PIN_11_FN_SPI_S_CS_B QM_PMUX_FN_2

/* Pin ID 12. */
#define QM_PIN_12_FN_GPIO_12 QM_PMUX_FN_0
#define QM_PIN_12_FN_AIN_12 QM_PMUX_FN_1
#define QM_PIN_12_FN_UART0_TXD QM_PMUX_FN_2

/* Pin ID 13. */
#define QM_PIN_13_FN_GPIO_13 QM_PMUX_FN_0
#define QM_PIN_13_FN_AIN_13 QM_PMUX_FN_1
#define QM_PIN_13_FN_UART0_RXD QM_PMUX_FN_2

/* Pin ID 14. */
#define QM_PIN_14_FN_GPIO_14 QM_PMUX_FN_0
#define QM_PIN_14_FN_AIN_14 QM_PMUX_FN_1
#define QM_PIN_14_FN_UART0_RTS QM_PMUX_FN_2

/* Pin ID 15. */
#define QM_PIN_15_FN_GPIO_15 QM_PMUX_FN_0
#define QM_PIN_15_FN_AIN_15 QM_PMUX_FN_1
#define QM_PIN_15_FN_UART0_CTS QM_PMUX_FN_2

/* Pin ID 16. */
#define QM_PIN_16_FN_GPIO_16 QM_PMUX_FN_0
#define QM_PIN_16_FN_AIN_16 QM_PMUX_FN_1
#define QM_PIN_16_FN_SPI0_M_SCK QM_PMUX_FN_2

/* Pin ID 17. */
#define QM_PIN_17_FN_GPIO_17 QM_PMUX_FN_0
#define QM_PIN_17_FN_AIN_17 QM_PMUX_FN_1
#define QM_PIN_17_FN_SPI0_M_MOSI QM_PMUX_FN_2

/* Pin ID 18. */
#define QM_PIN_18_FN_GPIO_18 QM_PMUX_FN_0
#define QM_PIN_18_FN_AIN_18 QM_PMUX_FN_1
#define QM_PIN_18_FN_SPI0_M_MISO QM_PMUX_FN_2

/* Pin ID 19. */
#define QM_PIN_19_FN_TDO QM_PMUX_FN_0
#define QM_PIN_19_FN_GPIO_19 QM_PMUX_FN_1
#define QM_PIN_19_FN_PWM_0 QM_PMUX_FN_2

/* Pin ID 20. */
#define QM_PIN_20_FN_TRST_N QM_PMUX_FN_0
#define QM_PIN_20_FN_GPIO_20 QM_PMUX_FN_1
#define QM_PIN_20_FN_UART1_TXD QM_PMUX_FN_2

/* Pin ID 21. */
#define QM_PIN_21_FN_TCK QM_PMUX_FN_0
#define QM_PIN_21_FN_GPIO_21 QM_PMUX_FN_1
#define QM_PIN_21_FN_UART1_RXD QM_PMUX_FN_2

/* Pin ID 22. */
#define QM_PIN_22_FN_TMS QM_PMUX_FN_0
#define QM_PIN_22_FN_GPIO_22 QM_PMUX_FN_1
#define QM_PIN_22_FN_UART1_RTS QM_PMUX_FN_2

/* Pin ID 23. */
#define QM_PIN_23_FN_TDI QM_PMUX_FN_0
#define QM_PIN_23_FN_GPIO_23 QM_PMUX_FN_1
#define QM_PIN_23_FN_UART1_CTS QM_PMUX_FN_2

/* Pin ID 24. */
#define QM_PIN_24_FN_GPIO_24 QM_PMUX_FN_0
#define QM_PIN_24_FN_LPD_SIG_OUT QM_PMUX_FN_1
#define QM_PIN_24_FN_PWM_1 QM_PMUX_FN_2

/**
 * @}
 */

#endif /* __QM_PIN_FUNCTIONS_H__ */
