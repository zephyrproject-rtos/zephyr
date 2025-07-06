/*
 * Copyright (c) 2024 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SL_CLOCK_MANAGER_TREE_CONFIG_H
#define SL_CLOCK_MANAGER_TREE_CONFIG_H

#include <zephyr/devicetree.h>

#include <em_device.h>

/* Internal macros that must be defined to make parameter validation in the HAL pass,
 * but are unused since Zephyr derives all clock tree configuration from DeviceTree
 */
#define SL_CLOCK_MANAGER_DEFAULT_HF_CLOCK_SOURCE           0xFF
#define SL_CLOCK_MANAGER_DEFAULT_HF_CLOCK_SOURCE_HFRCODPLL 0xFF
#define SL_CLOCK_MANAGER_DEFAULT_LF_CLOCK_SOURCE           0xFC
#define SL_CLOCK_MANAGER_DEFAULT_LF_CLOCK_SOURCE_LFRCO     0xFC

#define SL_CLOCK_MANAGER_INVALID 0xFF

/* SYSCLK */
#define SL_CLOCK_MANAGER_SYSCLK_SOURCE                                                             \
	(DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(sysclk)), DT_NODELABEL(fsrco))                   \
		 ? CMU_SYSCLKCTRL_CLKSEL_FSRCO                                                     \
	 : DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(sysclk)), DT_NODELABEL(hfrcodpll))             \
		 ? CMU_SYSCLKCTRL_CLKSEL_HFRCODPLL                                                 \
	 : DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(sysclk)), DT_NODELABEL(hfxo))                  \
		 ? CMU_SYSCLKCTRL_CLKSEL_HFXO                                                      \
	 : DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(sysclk)), DT_NODELABEL(clkin0))                \
		 ? CMU_SYSCLKCTRL_CLKSEL_CLKIN0                                                    \
		 : SL_CLOCK_MANAGER_INVALID)

#if SL_CLOCK_MANAGER_SYSCLK_SOURCE == SL_CLOCK_MANAGER_INVALID
#error "Invalid clock source selection for SYSCLK"
#endif

#define SL_CLOCK_MANAGER_HCLK_DIVIDER                                                              \
	CONCAT(CMU_SYSCLKCTRL_HCLKPRESC_DIV, DT_PROP(DT_NODELABEL(hclk), clock_div))

#define SL_CLOCK_MANAGER_PCLK_DIVIDER                                                              \
	CONCAT(CMU_SYSCLKCTRL_PCLKPRESC_DIV, DT_PROP(DT_NODELABEL(pclk), clock_div))

/* TRACECLK */
#if defined(_CMU_TRACECLKCTRL_CLKSEL_MASK)
#if DT_NUM_CLOCKS(DT_NODELABEL(traceclk)) == 0
#define SL_CLOCK_MANAGER_TRACECLK_SOURCE CMU_TRACECLKCTRL_CLKSEL_DISABLE
#else
#if defined(CMU_TRACECLKCTRL_CLKSEL_SYSCLK)
/* TRACECLK can be clocked from SYSCLK */
#define SL_CLOCK_MANAGER_TRACECLK_SOURCE                                                           \
	(DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(traceclk)), DT_NODELABEL(sysclk))                \
		 ? CMU_TRACECLKCTRL_CLKSEL_SYSCLK                                                  \
	 : DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(traceclk)), DT_NODELABEL(hfrcodpllrt))         \
		 ? CMU_TRACECLKCTRL_CLKSEL_HFRCODPLLRT                                             \
		 : COND_CODE_1(DT_NODE_EXISTS(DT_NODELABEL(hfrcoem23)), (                          \
			DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(traceclk)),                       \
				DT_NODELABEL(hfrcoem23))                                           \
			? CMU_TRACECLKCTRL_CLKSEL_HFRCOEM23                                        \
			: SL_CLOCK_MANAGER_INVALID), (SL_CLOCK_MANAGER_INVALID)))

#else
/* TRACECLK can be clocked from HCLK */
#define SL_CLOCK_MANAGER_TRACECLK_SOURCE                                                           \
	(DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(traceclk)), DT_NODELABEL(hclk))                  \
		 ? CMU_TRACECLKCTRL_CLKSEL_HCLK                                                    \
	 : DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(traceclk)), DT_NODELABEL(hfrcoem23))           \
		 ? CMU_TRACECLKCTRL_CLKSEL_HFRCOEM23                                               \
		 : SL_CLOCK_MANAGER_INVALID)
#endif /* defined(CMU_TRACECLKCTRL_CLKSEL_SYSCLK) */
#if SL_CLOCK_MANAGER_TRACECLK_SOURCE == SL_CLOCK_MANAGER_INVALID
#error "Invalid clock source selection for TRACECLK"
#endif
#endif /* DT_NUM_CLOCKS(traceclk) */
#endif /* defined(_CMU_TRACECLKCTRL_CLKSEL_MASK) */

#if DT_NODE_HAS_PROP(DT_NODELABEL(traceclk), clock_div)
#define SL_CLOCK_MANAGER_TRACECLK_DIVIDER                                                          \
	CONCAT(CMU_TRACECLKCTRL_PRESC_DIV, DT_PROP(DT_NODELABEL(traceclk), clock_div))
#endif

/* EM01GRPACLK */
#define SL_CLOCK_MANAGER_EM01GRPACLK_SOURCE                                                        \
	(DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(em01grpaclk)), DT_NODELABEL(hfrcodpll))          \
		 ? CMU_EM01GRPACLKCTRL_CLKSEL_HFRCODPLL                                            \
	 : DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(em01grpaclk)), DT_NODELABEL(hfxo))             \
		 ? CMU_EM01GRPACLKCTRL_CLKSEL_HFXO                                                 \
	 : DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(em01grpaclk)), DT_NODELABEL(fsrco))            \
		 ? CMU_EM01GRPACLKCTRL_CLKSEL_FSRCO                                                \
		 : COND_CODE_1(DT_NODE_EXISTS(DT_NODELABEL(hfrcoem23)), (                          \
			DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(em01grpaclk)),                    \
				DT_NODELABEL(hfrcoem23))                                           \
			? CMU_EM01GRPACLKCTRL_CLKSEL_HFRCOEM23                                     \
			: COND_CODE_1(DT_NODE_EXISTS(DT_NODELABEL(hfrcodpllrt)), (                 \
				DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(em01grpaclk)),            \
					DT_NODELABEL(hfrcodpllrt))                                 \
				? CMU_EM01GRPACLKCTRL_CLKSEL_HFRCODPLLRT                           \
				: DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(em01grpaclk)),          \
					DT_NODELABEL(hfxort))                                      \
				? CMU_EM01GRPACLKCTRL_CLKSEL_HFXORT                                \
				: SL_CLOCK_MANAGER_INVALID), (SL_CLOCK_MANAGER_INVALID))),         \
			(SL_CLOCK_MANAGER_INVALID)))

#if SL_CLOCK_MANAGER_EM01GRPACLK_SOURCE == SL_CLOCK_MANAGER_INVALID
#error "Invalid clock source selection for EM01GRPACLK"
#endif

/* EM01GRPBCLK */
#if DT_NODE_EXISTS(DT_NODELABEL(em01grpbclk))
#define SL_CLOCK_MANAGER_EM01GRPBCLK_SOURCE                                                        \
	(DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(em01grpbclk)), DT_NODELABEL(hfrcodpll))          \
		 ? CMU_EM01GRPBCLKCTRL_CLKSEL_HFRCODPLL                                            \
	 : DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(em01grpbclk)), DT_NODELABEL(hfxo))             \
		 ? CMU_EM01GRPBCLKCTRL_CLKSEL_HFXO                                                 \
	 : DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(em01grpbclk)), DT_NODELABEL(fsrco))            \
		 ? CMU_EM01GRPBCLKCTRL_CLKSEL_FSRCO                                                \
	 : DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(em01grpbclk)), DT_NODELABEL(clkin0))           \
		 ? CMU_EM01GRPBCLKCTRL_CLKSEL_CLKIN0                                               \
		 : COND_CODE_1(DT_NODE_EXISTS(DT_NODELABEL(hfrcodpllrt)), (                        \
			DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(em01grpbclk)),                    \
				DT_NODELABEL(hfrcodpllrt))                                         \
			? CMU_EM01GRPBCLKCTRL_CLKSEL_HFRCODPLLRT                                   \
			: DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(em01grpbclk)),                  \
				DT_NODELABEL(hfxort))                                              \
			? CMU_EM01GRPBCLKCTRL_CLKSEL_HFXORT                                        \
			: SL_CLOCK_MANAGER_INVALID), (SL_CLOCK_MANAGER_INVALID)))

#if SL_CLOCK_MANAGER_EM01GRPBCLK_SOURCE == SL_CLOCK_MANAGER_INVALID
#error "Invalid clock source selection for EM01GRPBCLK"
#endif
#endif /* DT_NODE_EXISTS(em01grpbclk)*/

/* EM01GRPCCLK */
#if DT_NODE_EXISTS(DT_NODELABEL(em01grpcclk))
#define SL_CLOCK_MANAGER_EM01GRPCCLK_SOURCE                                                        \
	(DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(em01grpcclk)), DT_NODELABEL(hfrcodpll))          \
		 ? CMU_EM01GRPCCLKCTRL_CLKSEL_HFRCODPLL                                            \
	 : DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(em01grpcclk)), DT_NODELABEL(hfxo))             \
		 ? CMU_EM01GRPCCLKCTRL_CLKSEL_HFXO                                                 \
	 : DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(em01grpcclk)), DT_NODELABEL(fsrco))            \
		 ? CMU_EM01GRPCCLKCTRL_CLKSEL_FSRCO                                                \
		 : COND_CODE_1(DT_NODE_EXISTS(DT_NODELABEL(hfrcoem23)), (                          \
			DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(em01grpcclk)),                    \
				DT_NODELABEL(hfrcodpllrt))                                         \
			? CMU_EM01GRPACLKCTRL_CLKSEL_HFRCODPLLRT                                   \
			: DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(em01grpcclk)),                  \
				DT_NODELABEL(hfxort))                                              \
			? CMU_EM01GRPACLKCTRL_CLKSEL_HFXORT                                        \
			: DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(em01grpcclk)),                  \
				DT_NODELABEL(hfrcoem23))                                           \
			? CMU_EM01GRPCCLKCTRL_CLKSEL_HFRCOEM23                                     \
			: SL_CLOCK_MANAGER_INVALID), (SL_CLOCK_MANAGER_INVALID)))

#if SL_CLOCK_MANAGER_EM01GRPCCLK_SOURCE == SL_CLOCK_MANAGER_INVALID
#error "Invalid clock source selection for EM01GRPCCLK"
#endif
#endif /* DT_NODE_EXISTS(em01grpcclk)*/

/* IADCCLK */
#if DT_NODE_EXISTS(DT_NODELABEL(iadcclk))
#define SL_CLOCK_MANAGER_IADCCLK_SOURCE                                                            \
	(DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(iadcclk)), DT_NODELABEL(em01grpaclk))            \
		 ? CMU_IADCCLKCTRL_CLKSEL_EM01GRPACLK                                              \
	 : DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(iadcclk)), DT_NODELABEL(fsrco))                \
		 ? CMU_IADCCLKCTRL_CLKSEL_FSRCO                                                    \
		 : COND_CODE_1(DT_NODE_EXISTS(DT_NODELABEL(hfrcoem23)), (                          \
			DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(iadcclk)),                        \
				DT_NODELABEL(hfrcoem23))                                           \
			? CMU_IADCCLKCTRL_CLKSEL_HFRCOEM23                                         \
			: SL_CLOCK_MANAGER_INVALID), (SL_CLOCK_MANAGER_INVALID)))

#if SL_CLOCK_MANAGER_IADCCLK_SOURCE == SL_CLOCK_MANAGER_INVALID
#error "Invalid clock source selection for IADCCLK"
#endif
#endif /* DT_NODE_EXISTS(iadcclk) */

/* LESENSEHFCLK */
#if DT_NODE_EXISTS(DT_NODELABEL(lesensehfclk))
#define SL_CLOCK_MANAGER_LESENSEHFCLK_SOURCE                                                       \
	(DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(lesensehfclk)), DT_NODELABEL(hfrcoem23))         \
		 ? CMU_LESENSEHFCLKCTRL_CLKSEL_HFRCOEM23                                           \
	 : DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(lesensehfclk)), DT_NODELABEL(fsrco))           \
		 ? CMU_LESENSEHFCLKCTRL_CLKSEL_FSRCO                                               \
		 : SL_CLOCK_MANAGER_INVALID)

#if SL_CLOCK_MANAGER_LESENSEHFCLK_SOURCE == SL_CLOCK_MANAGER_INVALID
#error "Invalid clock source selection for LESENSEHFCLK"
#endif
#endif /* DT_NODE_EXISTS(lesensehfclk) */

/* EM23GRPACLK */
#define SL_CLOCK_MANAGER_EM23GRPACLK_SOURCE                                                        \
	(DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(em23grpaclk)), DT_NODELABEL(lfrco))              \
		 ? CMU_EM23GRPACLKCTRL_CLKSEL_LFRCO                                                \
	 : DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(em23grpaclk)), DT_NODELABEL(lfxo))             \
		 ? CMU_EM23GRPACLKCTRL_CLKSEL_LFXO                                                 \
	 : DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(em23grpaclk)), DT_NODELABEL(ulfrco))           \
		 ? CMU_EM23GRPACLKCTRL_CLKSEL_ULFRCO                                               \
		 : SL_CLOCK_MANAGER_INVALID)

#if SL_CLOCK_MANAGER_EM23GRPACLK_SOURCE == SL_CLOCK_MANAGER_INVALID
#error "Invalid clock source selection for EM23GRPACLK"
#endif

/* EM4GRPACLK */
#define SL_CLOCK_MANAGER_EM4GRPACLK_SOURCE                                                         \
	(DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(em4grpaclk)), DT_NODELABEL(lfrco))               \
		 ? CMU_EM4GRPACLKCTRL_CLKSEL_LFRCO                                                 \
	 : DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(em4grpaclk)), DT_NODELABEL(lfxo))              \
		 ? CMU_EM4GRPACLKCTRL_CLKSEL_LFXO                                                  \
	 : DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(em4grpaclk)), DT_NODELABEL(ulfrco))            \
		 ? CMU_EM4GRPACLKCTRL_CLKSEL_ULFRCO                                                \
		 : SL_CLOCK_MANAGER_INVALID)

#if SL_CLOCK_MANAGER_EM4GRPACLK_SOURCE == SL_CLOCK_MANAGER_INVALID
#error "Invalid clock source selection for EM4GRPACLK"
#endif

/* RTCCCLK */
#if DT_NODE_EXISTS(DT_NODELABEL(rtccclk))
#define SL_CLOCK_MANAGER_RTCCCLK_SOURCE                                                            \
	(DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(rtccclk)), DT_NODELABEL(lfrco))                  \
		 ? CMU_RTCCCLKCTRL_CLKSEL_LFRCO                                                    \
	 : DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(rtccclk)), DT_NODELABEL(lfxo))                 \
		 ? CMU_RTCCCLKCTRL_CLKSEL_LFXO                                                     \
	 : DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(rtccclk)), DT_NODELABEL(ulfrco))               \
		 ? CMU_RTCCCLKCTRL_CLKSEL_ULFRCO                                                   \
		 : SL_CLOCK_MANAGER_INVALID)

#if SL_CLOCK_MANAGER_RTCCCLK_SOURCE == SL_CLOCK_MANAGER_INVALID
#error "Invalid clock source selection for RTCCCLK"
#endif
#endif /* DT_NODE_EXISTS(rtccclk) */

/* SYSRTCCLK */
#if DT_NODE_EXISTS(DT_NODELABEL(sysrtcclk))
#define SL_CLOCK_MANAGER_SYSRTCCLK_SOURCE                                                          \
	(DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(sysrtcclk)), DT_NODELABEL(lfrco))                \
		 ? CMU_SYSRTC0CLKCTRL_CLKSEL_LFRCO                                                 \
	 : DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(sysrtcclk)), DT_NODELABEL(lfxo))               \
		 ? CMU_SYSRTC0CLKCTRL_CLKSEL_LFXO                                                  \
	 : DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(sysrtcclk)), DT_NODELABEL(ulfrco))             \
		 ? CMU_SYSRTC0CLKCTRL_CLKSEL_ULFRCO                                                \
		 : SL_CLOCK_MANAGER_INVALID)

#if SL_CLOCK_MANAGER_SYSRTCCLK_SOURCE == SL_CLOCK_MANAGER_INVALID
#error "Invalid clock source selection for SYSRTCCLK"
#endif
#endif /* DT_NODE_EXISTS(sysrtcclk) */

/* WDOG0CLK */
#define SL_CLOCK_MANAGER_WDOG0CLK_SOURCE                                                           \
	(DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(wdog0clk)), DT_NODELABEL(lfrco))                 \
		 ? CMU_WDOG0CLKCTRL_CLKSEL_LFRCO                                                   \
	 : DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(wdog0clk)), DT_NODELABEL(lfxo))                \
		 ? CMU_WDOG0CLKCTRL_CLKSEL_LFXO                                                    \
	 : DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(wdog0clk)), DT_NODELABEL(ulfrco))              \
		 ? CMU_WDOG0CLKCTRL_CLKSEL_ULFRCO                                                  \
	 : DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(wdog0clk)), DT_NODELABEL(hclkdiv1024))         \
		 ? CMU_WDOG0CLKCTRL_CLKSEL_HCLKDIV1024                                             \
		 : SL_CLOCK_MANAGER_INVALID)

#if SL_CLOCK_MANAGER_WDOG0CLK_SOURCE == SL_CLOCK_MANAGER_INVALID
#error "Invalid clock source selection for WDOG0CLK"
#endif

/* WDOG1CLK */
#if DT_NODE_EXISTS(DT_NODELABEL(wdog1clk))
#define SL_CLOCK_MANAGER_WDOG1CLK_SOURCE                                                           \
	(DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(wdog1clk)), DT_NODELABEL(lfrco))                 \
		 ? CMU_WDOG1CLKCTRL_CLKSEL_LFRCO                                                   \
	 : DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(wdog1clk)), DT_NODELABEL(lfxo))                \
		 ? CMU_WDOG1CLKCTRL_CLKSEL_LFXO                                                    \
	 : DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(wdog1clk)), DT_NODELABEL(ulfrco))              \
		 ? CMU_WDOG1CLKCTRL_CLKSEL_ULFRCO                                                  \
	 : DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(wdog1clk)), DT_NODELABEL(hclkdiv1024))         \
		 ? CMU_WDOG1CLKCTRL_CLKSEL_HCLKDIV1024                                             \
		 : SL_CLOCK_MANAGER_INVALID)

#if SL_CLOCK_MANAGER_WDOG1CLK_SOURCE == SL_CLOCK_MANAGER_INVALID
#error "Invalid clock source selection for WDOG1CLK"
#endif
#endif /* DT_NODE_EXISTS(wdog1clk) */

/* LCDCLK */
#if DT_NODE_EXISTS(DT_NODELABEL(lcdclk))
#define SL_CLOCK_MANAGER_LCDCLK_SOURCE                                                             \
	(DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(lcdclk)), DT_NODELABEL(lfrco))                   \
		 ? CMU_LCDCLKCTRL_CLKSEL_LFRCO                                                     \
	 : DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(lcdclk)), DT_NODELABEL(lfxo))                  \
		 ? CMU_LCDCLKCTRL_CLKSEL_LFXO                                                      \
	 : DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(lcdclk)), DT_NODELABEL(ulfrco))                \
		 ? CMU_LCDCLKCTRL_CLKSEL_ULFRCO                                                    \
		 : SL_CLOCK_MANAGER_INVALID)

#if SL_CLOCK_MANAGER_LCDCLK_SOURCE == SL_CLOCK_MANAGER_INVALID
#error "Invalid clock source selection for LCDCLK"
#endif
#endif /* DT_NODE_EXISTS(lcdclk) */

/* PCNT0CLK */
/* FIXME: allow clock selection from S0 pin */
#if DT_NODE_EXISTS(DT_NODELABEL(pcnt0clk))
#if DT_NUM_CLOCKS(DT_NODELABEL(pcnt0clk)) == 0
#define SL_CLOCK_MANAGER_PCNT0CLK_SOURCE CMU_PCNT0CLKCTRL_CLKSEL_DISABLED
#else
#define SL_CLOCK_MANAGER_PCNT0CLK_SOURCE                                                           \
	(DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(pcnt0clk)), DT_NODELABEL(em23grpaclk))           \
		 ? CMU_PCNT0CLKCTRL_CLKSEL_EM23GRPACLK                                             \
		 : SL_CLOCK_MANAGER_INVALID)

#if SL_CLOCK_MANAGER_PCNT0CLK_SOURCE == SL_CLOCK_MANAGER_INVALID
#error "Invalid clock source selection for PCNT0CLK"
#endif
#endif /* DT_NUM_CLOCKS(pcnt0clk) */
#endif /* DT_NODE_EXISTS(pcnt0clk) */

/* EUART0CLK */
#if DT_NODE_EXISTS(DT_NODELABEL(euart0clk))
#if DT_NUM_CLOCKS(DT_NODELABEL(euart0clk)) == 0
#define SL_CLOCK_MANAGER_EUART0CLK_SOURCE CMU_EUART0CLKCTRL_CLKSEL_DISABLED
#else
#define SL_CLOCK_MANAGER_EUART0CLK_SOURCE                                                          \
	(DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(euart0clk)), DT_NODELABEL(em01grpaclk))          \
		 ? CMU_EUART0CLKCTRL_CLKSEL_EM01GRPACLK                                            \
	 : DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(euart0clk)), DT_NODELABEL(em23grpaclk))        \
		 ? CMU_EUART0CLKCTRL_CLKSEL_EM23GRPACLK                                            \
		 : SL_CLOCK_MANAGER_INVALID)

#if SL_CLOCK_MANAGER_EUART0CLK_SOURCE == SL_CLOCK_MANAGER_INVALID
#error "Invalid clock source selection for EUART0CLK"
#endif
#endif /* DT_NUM_CLOCKS(euart0clk) */
#endif /* DT_NODE_EXISTS(euart0clk) */

/* EUSART0CLK */
#if DT_NODE_EXISTS(DT_NODELABEL(eusart0clk))
#if DT_NUM_CLOCKS(DT_NODELABEL(eusart0clk)) == 0
#define SL_CLOCK_MANAGER_EUSART0CLK_SOURCE CMU_EUSART0CLKCTRL_CLKSEL_DISABLED
#else
#if DT_NODE_EXISTS(DT_NODELABEL(em01grpbclk))
/* If EM01GRPB clock exists, EUSART0 is in EM01GRPA or EM23GRPA */
#define SL_CLOCK_MANAGER_EUSART0CLK_SOURCE                                                         \
	(DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(eusart0clk)), DT_NODELABEL(em01grpaclk))         \
		 ? CMU_EUSART0CLKCTRL_CLKSEL_EM01GRPACLK                                           \
	 : DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(eusart0clk)), DT_NODELABEL(em23grpaclk))       \
		 ? CMU_EUSART0CLKCTRL_CLKSEL_EM23GRPACLK                                           \
	 : DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(eusart0clk)), DT_NODELABEL(fsrco))             \
		 ? CMU_EUSART0CLKCTRL_CLKSEL_FSRCO                                                 \
		 : SL_CLOCK_MANAGER_INVALID)
#else
/* Otherwise, EUSART0 is in EM01GRPC or directly connected to oscillators */
#define SL_CLOCK_MANAGER_EUSART0CLK_SOURCE                                                         \
	(DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(eusart0clk)), DT_NODELABEL(em01grpcclk))         \
		 ? CMU_EUSART0CLKCTRL_CLKSEL_EM01GRPCCLK                                           \
	 : DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(eusart0clk)), DT_NODELABEL(hfrcoem23))         \
		 ? CMU_EUSART0CLKCTRL_CLKSEL_HFRCOEM23                                             \
	 : DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(eusart0clk)), DT_NODELABEL(lfrco))             \
		 ? CMU_EUSART0CLKCTRL_CLKSEL_LFRCO                                                 \
	 : DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(eusart0clk)), DT_NODELABEL(lfxo))              \
		 ? CMU_EUSART0CLKCTRL_CLKSEL_LFXO                                                  \
		 : SL_CLOCK_MANAGER_INVALID)
#endif /* DT_NODE_EXISTS(em01grpbclk) */
#if SL_CLOCK_MANAGER_EUSART0CLK_SOURCE == SL_CLOCK_MANAGER_INVALID
#error "Invalid clock source selection for EUSART0CLK"
#endif
#endif /* DT_NUM_CLOCKS(eusart0clk) */
#endif /* DT_NODE_EXISTS(eusart0clk) */

/* SYSTICKCLK */
#if DT_NODE_EXISTS(DT_NODELABEL(systickclk))
#define SL_CLOCK_MANAGER_SYSTICKCLK_SOURCE                                                         \
	(DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(systickclk)), DT_NODELABEL(hclk)) ? 0            \
	 : DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(systickclk)), DT_NODELABEL(em23grpaclk))       \
		 ? 1                                                                               \
		 : SL_CLOCK_MANAGER_INVALID)

#if SL_CLOCK_MANAGER_SYSTICKCLK_SOURCE == SL_CLOCK_MANAGER_INVALID
#error "Invalid clock source selection for SYSTICKCLK"
#endif
#endif /* DT_NODE_EXISTS(systickclk) */

/* VDAC0CLK */
#if DT_NODE_EXISTS(DT_NODELABEL(vdac0clk))
#if DT_NUM_CLOCKS(DT_NODELABEL(vdac0clk)) == 0
#define SL_CLOCK_MANAGER_VDAC0CLK_SOURCE CMU_VDAC0CLKCTRL_CLKSEL_DISABLED
#else
#define SL_CLOCK_MANAGER_VDAC0CLK_SOURCE                                                           \
	(DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(vdac0clk)), DT_NODELABEL(em01grpaclk))           \
		 ? CMU_VDAC0CLKCTRL_CLKSEL_EM01GRPACLK                                             \
	 : DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(vdac0clk)), DT_NODELABEL(em23grpaclk))         \
		 ? CMU_VDAC0CLKCTRL_CLKSEL_EM23GRPACLK                                             \
	 : DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(vdac0clk)), DT_NODELABEL(fsrco))               \
		 ? CMU_VDAC0CLKCTRL_CLKSEL_FSRCO                                                   \
	 : DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(vdac0clk)), DT_NODELABEL(hfrcoem23))           \
		 ? CMU_VDAC0CLKCTRL_CLKSEL_HFRCOEM23                                               \
		 : SL_CLOCK_MANAGER_INVALID)

#if SL_CLOCK_MANAGER_VDAC0CLK_SOURCE == SL_CLOCK_MANAGER_INVALID
#error "Invalid clock source selection for VDAC0CLK"
#endif
#endif /* DT_NUM_CLOCKS(vdac0clk) */
#endif /* DT_NODE_EXISTS(vdac0clk) */

/* VDAC1CLK */
#if DT_NODE_EXISTS(DT_NODELABEL(vdac1clk))
#if DT_NUM_CLOCKS(DT_NODELABEL(vdac1clk)) == 0
#define SL_CLOCK_MANAGER_VDAC1CLK_SOURCE CMU_VDAC1CLKCTRL_CLKSEL_DISABLED
#else
#define SL_CLOCK_MANAGER_VDAC1CLK_SOURCE                                                           \
	(DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(vdac1clk)), DT_NODELABEL(em01grpaclk))           \
		 ? CMU_VDAC1CLKCTRL_CLKSEL_EM01GRPACLK                                             \
	 : DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(vdac1clk)), DT_NODELABEL(em23grpaclk))         \
		 ? CMU_VDAC1CLKCTRL_CLKSEL_EM23GRPACLK                                             \
	 : DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(vdac1clk)), DT_NODELABEL(fsrco))               \
		 ? CMU_VDAC1CLKCTRL_CLKSEL_FSRCO                                                   \
	 : DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(vdac1clk)), DT_NODELABEL(hfrcoem23))           \
		 ? CMU_VDAC1CLKCTRL_CLKSEL_HFRCOEM23                                               \
		 : SL_CLOCK_MANAGER_INVALID)

#if SL_CLOCK_MANAGER_VDAC1CLK_SOURCE == SL_CLOCK_MANAGER_INVALID
#error "Invalid clock source selection for VDAC1CLK"
#endif
#endif /* DT_NUM_CLOCKS(vdac1clk) */
#endif /* DT_NODE_EXISTS(vdac1clk) */

#endif /* SL_CLOCK_MANAGER_TREE_CONFIG_H */
