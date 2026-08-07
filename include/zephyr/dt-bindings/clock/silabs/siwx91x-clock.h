/* Copyright (c) 2024-2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_SILABS_SIWX91X_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_SILABS_SIWX91X_CLOCK_H_

/* Reference clocks interpretation: 
 * ie: HP_REF_ULP
 * ULP reference from HP domain
 */

/* Clock managed by the HP (High Power) driver */
#define SIWX91X_CLK_UART0          0
#define SIWX91X_CLK_UART1          1
#define SIWX91X_CLK_I2C0           2
#define SIWX91X_CLK_I2C1           3
#define SIWX91X_CLK_UDMA           4
#define SIWX91X_CLK_PWM            5
#define SIWX91X_CLK_GSPI           6
#define SIWX91X_CLK_QSPI           7
#define SIWX91X_CLK_QSPI2          8
#define SIWX91X_CLK_I2S            9
#define SIWX91X_CLK_STATIC_I2S     10
#define SIWX91X_CLK_GPDMA          11
#define SIWX91X_CLK_RNG            12
#define SIWX91X_CLK_GPIO           13
#define SIWX91X_CLK_SSI            14
#define SIWX91X_CLK_PIN_OUT        15 /* OUT_CLK */
#define SIWX91X_CLK_ICACHE         16

#define SIWX91X_CLK_CPU            17 /* PROC     */
#define SIWX91X_CLK_CPU_LP         18 /* MCU_LP - Not Supported*/

#define SIWX91X_CLK_HP_REF_ULP     19 /* HP_ULP   */
#define SIWX91X_CLK_HP_REF_PLL     20 /* PLL_REF  */

#define SIWX91X_CLK_PLL_SOC        21 /* SOC_PLL  */
#define SIWX91X_CLK_PLL_INTF       22 /* INTF_PLL */
#define SIWX91X_CLK_PLL_I2S        23 /* I2S_PLL  */

/* Clock managed by the ULP (Ultra Low Power) driver */
#define SIWX91X_CLK_ULP_UART       24
#define SIWX91X_CLK_ULP_I2C        25
#define SIWX91X_CLK_ULP_UDMA       26
#define SIWX91X_CLK_ULP_I2S        27
#define SIWX91X_CLK_ULP_STATIC_I2S 28
#define SIWX91X_CLK_ULP_ADC        29
#define SIWX91X_CLK_ULP_SSI        30
#define SIWX91X_CLK_ULP_TIMER      31
#define SIWX91X_CLK_ULP_GPIO       32

#define SIWX91X_CLK_ULP_REF_AON    33 /* SLP_SENSOR */
#define SIWX91X_CLK_ULP_REF_CPU    34 /* ULP_PROC*/

/* Clock managed by the AON (Always-On) driver */
#define SIWX91X_CLK_XTAL_MHZ       35
#define SIWX91X_CLK_XTAL_KHZ       36
#define SIWX91X_CLK_RC_MHZ         37
#define SIWX91X_CLK_RC_KHZ         38

#define SIWX91X_CLK_WATCHDOG       39
#define SIWX91X_CLK_RTC            40
#define SIWX91X_CLK_SYSRTC         41
#define SIWX91X_CLK_UULP_GPIO      42

#define SIWX91X_CLK_REF_HP         43 /* HP_REF*/
#define SIWX91X_CLK_REF_ULP        44 /* ULP_REF*/
#define SIWX91X_CLK_AON_REF_HF     45 /* UULP_HF_REF*/
#define SIWX91X_CLK_AON_REF_LF     46 /* UULP_LF_REF*/

/* Other */
#define SIWX91X_CLK_GATED          47

#endif
