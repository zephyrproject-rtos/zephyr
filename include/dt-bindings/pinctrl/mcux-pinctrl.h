/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MCUX_PINCTRL_H_
#define _MCUX_PINCTRL_H_


/*! @name PCR - Pin Control Register n */
#define PORT_PCR_PS_MASK                         (0x1U)
#define PORT_PCR_PS_SHIFT                        (0U)
#define PORT_PCR_PS(x)                           ((x << PORT_PCR_PS_SHIFT) & PORT_PCR_PS_MASK)
#define PORT_PCR_PE_MASK                         (0x2U)
#define PORT_PCR_PE_SHIFT                        (1U)
#define PORT_PCR_PE(x)                           ((x << PORT_PCR_PE_SHIFT) & PORT_PCR_PE_MASK)
#define PORT_PCR_SRE_MASK                        (0x4U)
#define PORT_PCR_SRE_SHIFT                       (2U)
#define PORT_PCR_SRE(x)                          ((x << PORT_PCR_SRE_SHIFT) & PORT_PCR_SRE_MASK)
#define PORT_PCR_PFE_MASK                        (0x10U)
#define PORT_PCR_PFE_SHIFT                       (4U)
#define PORT_PCR_PFE(x)                          ((x << PORT_PCR_PFE_SHIFT) & PORT_PCR_PFE_MASK)
#define PORT_PCR_ODE_MASK                        (0x20U)
#define PORT_PCR_ODE_SHIFT                       (5U)
#define PORT_PCR_ODE(x)                          ((x << PORT_PCR_ODE_SHIFT) & PORT_PCR_ODE_MASK)
#define PORT_PCR_DSE_MASK                        (0x40U)
#define PORT_PCR_DSE_SHIFT                       (6U)
#define PORT_PCR_DSE(x)                          ((x << PORT_PCR_DSE_SHIFT) & PORT_PCR_DSE_MASK)
#define PORT_PCR_MUX_MASK                        (0x700U)
#define PORT_PCR_MUX_SHIFT                       (8U)
#define PORT_PCR_MUX(x)                          ((x << PORT_PCR_MUX_SHIFT) & PORT_PCR_MUX_MASK)
#define PORT_PCR_LK_MASK                         (0x8000U)
#define PORT_PCR_LK_SHIFT                        (15U)
#define PORT_PCR_LK(x)                           ((x << PORT_PCR_LK_SHIFT) & PORT_PCR_LK_MASK)
#define PORT_PCR_IRQC_MASK                       (0xF0000U)
#define PORT_PCR_IRQC_SHIFT                      (16U)
#define PORT_PCR_IRQC(x)                         ((x << PORT_PCR_IRQC_SHIFT) & PORT_PCR_IRQC_MASK)
#define PORT_PCR_ISF_MASK                        (0x1000000U)
#define PORT_PCR_ISF_SHIFT                       (24U)
#define PORT_PCR_ISF(x)                          ((x << PORT_PCR_ISF_SHIFT) & PORT_PCR_ISF_MASK)

/* Alternate functions */
#define FUNC_ALT_0                0
#define FUNC_ALT_1                1
#define FUNC_ALT_2                2
#define FUNC_ALT_3                3
#define FUNC_ALT_4                4
#define FUNC_ALT_5                5
#define FUNC_ALT_6                6
#define FUNC_ALT_7                7
#define FUNC_ALT_8                8
#define FUNC_ALT_9                9
#define FUNC_ALT_10               10
#define FUNC_ALT_11               11
#define FUNC_ALT_12               12
#define FUNC_ALT_13               13
#define FUNC_ALT_14               14
#define FUNC_ALT_15               15

/* Alternate functions */
#define MCUX_FUNC_ALT_0                PORT_PCR_MUX(FUNC_ALT_0)
#define MCUX_FUNC_ALT_1                PORT_PCR_MUX(FUNC_ALT_1)
#define MCUX_FUNC_ALT_2                PORT_PCR_MUX(FUNC_ALT_2)
#define MCUX_FUNC_ALT_3                PORT_PCR_MUX(FUNC_ALT_3)
#define MCUX_FUNC_ALT_4                PORT_PCR_MUX(FUNC_ALT_4)
#define MCUX_FUNC_ALT_5                PORT_PCR_MUX(FUNC_ALT_5)
#define MCUX_FUNC_ALT_6                PORT_PCR_MUX(FUNC_ALT_6)
#define MCUX_FUNC_ALT_7                PORT_PCR_MUX(FUNC_ALT_7)
#define MCUX_FUNC_ALT_8                PORT_PCR_MUX(FUNC_ALT_8)
#define MCUX_FUNC_ALT_9                PORT_PCR_MUX(FUNC_ALT_9)
#define MCUX_FUNC_ALT_10               PORT_PCR_MUX(FUNC_ALT_10)
#define MCUX_FUNC_ALT_11               PORT_PCR_MUX(FUNC_ALT_11)
#define MCUX_FUNC_ALT_12               PORT_PCR_MUX(FUNC_ALT_12)
#define MCUX_FUNC_ALT_13               PORT_PCR_MUX(FUNC_ALT_13)
#define MCUX_FUNC_ALT_14               PORT_PCR_MUX(FUNC_ALT_14)
#define MCUX_FUNC_ALT_15               PORT_PCR_MUX(FUNC_ALT_15)


#endif	/* _MCUX_PINCTRL_H_ */
