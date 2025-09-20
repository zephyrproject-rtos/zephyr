/*
 * Copyright (c) 2022 Teslabs Engineering S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_AT32_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_AT32_H_

#include <zephyr/device.h>

#define CRM_CFG_OFFSET    0x08U
#define CRM_AHB1EN_OFFSET 0x30U
#define CRM_AHB2EN_OFFSET 0x34U
#define CRM_AHB3EN_OFFSET 0x38U
#define CRM_APB1EN_OFFSET 0x40U
#define CRM_APB2EN_OFFSET 0x44U

#define CRM_CFG_AHBDIV_POS  4U
#define CRM_CFG_AHBDIV_MSK  (BIT_MASK(4) << CRM_CFG_AHBDIV_POS)
#define CRM_CFG_APB1DIV_POS 10U
#define CRM_CFG_APB1DIV_MSK (BIT_MASK(3) << CRM_CFG_APB1DIV_POS)
#define CRM_CFG_APB2DIV_POS 13U
#define CRM_CFG_APB2DIV_MSK (BIT_MASK(3) << CRM_CFG_APB2DIV_POS)

/**
 * @brief Obtain a reference to the AT32 clock controller.
 *
 * There is a single clock controller in the AT32: ctrl. The device can be
 * used without checking for it to be ready since it has no initialization
 * code subject to failures.
 */
#define AT32_CLOCK_CONTROLLER DEVICE_DT_GET(DT_NODELABEL(cctl))

/* To enable use of IS_ENABLED utility macro, these symbols
 * should not be defined directly using DT_SAME_NODE.
 */
#define DT_CRM_CLOCKS_CTRL DT_CLOCKS_CTLR(DT_NODELABEL(crm))
#if DT_SAME_NODE(DT_CRM_CLOCKS_CTRL, DT_NODELABEL(pll))
#define AT32_SYSCLK_SRC_PLL 1
#endif
#if DT_SAME_NODE(DT_CRM_CLOCKS_CTRL, DT_NODELABEL(clk_hick))
#define AT32_SYSCLK_SRC_HICK 1
#endif
#if DT_SAME_NODE(DT_CRM_CLOCKS_CTRL, DT_NODELABEL(clk_hext))
#define AT32_SYSCLK_SRC_HEXT 1
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(pll), okay) && DT_NODE_HAS_PROP(DT_NODELABEL(pll), clocks)
#define DT_PLL_CLOCKS_CTRL DT_CLOCKS_CTLR(DT_NODELABEL(pll))
#if DT_SAME_NODE(DT_PLL_CLOCKS_CTRL, DT_NODELABEL(clk_hick))
#define AT32_PLL_SRC_HICK 1
#endif
#if DT_SAME_NODE(DT_PLL_CLOCKS_CTRL, DT_NODELABEL(clk_hext))
#define AT32_PLL_SRC_HEXT 1
#endif
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(clk_hext), fixed_clock, okay)
#define AT32_HEXT_ENABLED 1
#define AT32_HEXT_FREQ    DT_PROP(DT_NODELABEL(clk_hext), clock_frequency)
#elif DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(clk_hext), at_at32_hext, okay)
#define AT32_HEXT_ENABLED 1
#define AT32_HEXT_FREQ    DT_PROP(DT_NODELABEL(clk_hext), clock_frequency)
#else
#define AT32_HEXT_ENABLED 0
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_AT32_H_ */
