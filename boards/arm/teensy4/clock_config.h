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
/*!< Core clock frequency: 600000000Hz */
#define CLOCK_INIT_CORE_CLOCK                     600000000U

/* Clock outputs (values are in Hz): */
#define CLOCK_INIT_AHB_CLK_ROOT                       600000000UL
#define CLOCK_INIT_CAN_CLK_ROOT                       40000000UL
#define CLOCK_INIT_CKIL_SYNC_CLK_ROOT                 32768UL
#define CLOCK_INIT_CLKO1_CLK                          0UL
#define CLOCK_INIT_CLKO2_CLK                          0UL
#define CLOCK_INIT_CLK_1M                             1000000UL
#define CLOCK_INIT_CLK_24M                            24000000UL
#define CLOCK_INIT_CSI_CLK_ROOT                       24000000UL
#define CLOCK_INIT_ENET2_125M_CLK                     0UL
#define CLOCK_INIT_ENET2_REF_CLK                      0UL
#define CLOCK_INIT_ENET2_TX_CLK                       0UL
#define CLOCK_INIT_ENET_125M_CLK                      50000000UL
#define CLOCK_INIT_ENET_25M_REF_CLK                   25000000UL
#define CLOCK_INIT_ENET_REF_CLK                       0UL
#define CLOCK_INIT_ENET_TX_CLK                        0UL
#define CLOCK_INIT_FLEXIO1_CLK_ROOT                   30000000UL
#define CLOCK_INIT_FLEXIO2_CLK_ROOT                   30000000UL
#define CLOCK_INIT_FLEXSPI2_CLK_ROOT                  102857142UL
#define CLOCK_INIT_FLEXSPI_CLK_ROOT                   90000000UL
#define CLOCK_INIT_GPT1_IPG_CLK_HIGHFREQ              75000000UL
#define CLOCK_INIT_GPT2_IPG_CLK_HIGHFREQ              75000000UL
#define CLOCK_INIT_IPG_CLK_ROOT                       150000000UL
#define CLOCK_INIT_LCDIF_CLK_ROOT                     9300000UL
#define CLOCK_INIT_LPI2C_CLK_ROOT                     10000000UL
#define CLOCK_INIT_LPSPI_CLK_ROOT                     90000000UL
#define CLOCK_INIT_LVDS1_CLK                          1200000000UL
#define CLOCK_INIT_MQS_MCLK                           41538461UL
#define CLOCK_INIT_PERCLK_CLK_ROOT                    75000000UL
#define CLOCK_INIT_PLL7_MAIN_CLK                      24000000UL
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
#define CLOCK_INIT_SEMC_CLK_ROOT                      163862068UL
#define CLOCK_INIT_SPDIF0_CLK_ROOT                    30000000UL
#define CLOCK_INIT_SPDIF0_EXTCLK_OUT                  0UL
#define CLOCK_INIT_TRACE_CLK_ROOT                     99000000UL
#define CLOCK_INIT_UART_CLK_ROOT                      80000000UL
#define CLOCK_INIT_USBPHY1_CLK                        480000000UL
#define CLOCK_INIT_USBPHY2_CLK                        0UL
#define CLOCK_INIT_USDHC1_CLK_ROOT                    198000000UL
#define CLOCK_INIT_USDHC2_CLK_ROOT                    198000000UL

/*! @brief Arm PLL set for clock_init configuration.
 */
extern const clock_arm_pll_config_t armPllConfig_clock_init;
/*! @brief Usb1 PLL set for clock_init configuration.
 */
extern const clock_usb_pll_config_t usb1PllConfig_clock_init;
/*! @brief Sys PLL for clock_init configuration.
 */
extern const clock_sys_pll_config_t sysPllConfig_clock_init;
/*! @brief Video PLL set for clock_init configuration.
 */
extern const clock_video_pll_config_t videoPllConfig_clock_init;
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
