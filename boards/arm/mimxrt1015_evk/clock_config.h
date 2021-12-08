/*
 * Copyright (c) 2021, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CLOCK_CONFIG_H_
#define _CLOCK_CONFIG_H_

#include "fsl_common.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/*!< Board xtal0 frequency in Hz */
#define BOARD_XTAL0_CLK_HZ                         24000000U

/*!< Board xtal32k frequency in Hz */
#define BOARD_XTAL32K_CLK_HZ                          32768U
/*******************************************************************************
 ************************ BOARD_InitBootClocks function ************************
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus*/

/*!
 * @brief This function executes default configuration of clocks.
 *
 */
void BOARD_InitBootClocks(void);

#if defined(__cplusplus)
}
#endif /* __cplusplus*/

/*******************************************************************************
 ************************** Configuration clock_init ***************************
 ******************************************************************************/
/*******************************************************************************
 * Definitions for clock_init configuration
 ******************************************************************************/
/*!< Core clock frequency: 500000000Hz */
#define CLOCK_INIT_CORE_CLOCK                     500000000U

/* Clock outputs (values are in Hz): */
#define CLOCK_INIT_AHB_CLK_ROOT                       500000000UL
#define CLOCK_INIT_CKIL_SYNC_CLK_ROOT                 32768UL
#define CLOCK_INIT_CLKO1_CLK                          0UL
#define CLOCK_INIT_CLKO2_CLK                          0UL
#define CLOCK_INIT_CLK_1M                             1000000UL
#define CLOCK_INIT_CLK_24M                            24000000UL
#define CLOCK_INIT_ENET_500M_REF_CLK                  500000000UL
#define CLOCK_INIT_FLEXIO1_CLK_ROOT                   30000000UL
#define CLOCK_INIT_FLEXSPI_CLK_ROOT                   90000000UL
#define CLOCK_INIT_GPT1_IPG_CLK_HIGHFREQ              62500000UL
#define CLOCK_INIT_GPT2_IPG_CLK_HIGHFREQ              62500000UL
#define CLOCK_INIT_IPG_CLK_ROOT                       125000000UL
#define CLOCK_INIT_LPI2C_CLK_ROOT                     10000000UL
#define CLOCK_INIT_LPSPI_CLK_ROOT                     90000000UL
#define CLOCK_INIT_MQS_MCLK                           41538461UL
#define CLOCK_INIT_PERCLK_CLK_ROOT                    62500000UL
#define CLOCK_INIT_SAI1_CLK_ROOT                      41538461UL
#define CLOCK_INIT_SAI1_MCLK1                         41538461UL
#define CLOCK_INIT_SAI1_MCLK2                         41538461UL
#define CLOCK_INIT_SAI1_MCLK3                         30000000UL
#define CLOCK_INIT_SAI2_CLK_ROOT                      41538461UL
#define CLOCK_INIT_SAI2_MCLK1                         41538461UL
#define CLOCK_INIT_SAI2_MCLK2                         0UL
#define CLOCK_INIT_SAI2_MCLK3                         30000000UL
#define CLOCK_INIT_SAI3_CLK_ROOT                      41538461UL
#define CLOCK_INIT_SAI3_MCLK1                         41538461UL
#define CLOCK_INIT_SAI3_MCLK2                         0UL
#define CLOCK_INIT_SAI3_MCLK3                         30000000UL
#define CLOCK_INIT_SPDIF0_CLK_ROOT                    30000000UL
#define CLOCK_INIT_SPDIF0_EXTCLK_OUT                  0UL
#define CLOCK_INIT_TRACE_CLK_ROOT                     99000000UL
#define CLOCK_INIT_UART_CLK_ROOT                      80000000UL
#define CLOCK_INIT_USBPHY1_CLK                        480000000UL

/*! @brief Usb1 PLL set for clock_init configuration.
 */
extern const clock_usb_pll_config_t usb1PllConfig_clock_init;
/*! @brief Sys PLL for clock_init configuration.
 */
extern const clock_sys_pll_config_t sysPllConfig_clock_init;
/*! @brief Enet PLL set for clock_init configuration.
 */
extern const clock_enet_pll_config_t enetPllConfig_clock_init;

/*******************************************************************************
 * API for clock_init configuration
 ******************************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus*/

/*!
 * @brief This function executes configuration of clocks.
 *
 */
void clock_init(void);

#if defined(__cplusplus)
}
#endif  /* __cplusplus*/

#endif  /* _CLOCK_CONFIG_H_ */
