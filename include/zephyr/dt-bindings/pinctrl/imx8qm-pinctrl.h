/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_IMX8QM_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_IMX8QM_PINCTRL_H_

/* values for sw_config field */
#define SC_PAD_CONFIG_NORMAL    0U    /* Normal */
#define SC_PAD_CONFIG_OD        1U    /* Open Drain */
#define SC_PAD_CONFIG_OD_IN     2U    /* Open Drain and input */
#define SC_PAD_CONFIG_OUT_IN    3U    /* Output and input */

/* values for lp_config field */
#define SC_PAD_ISO_OFF          0U    /* ISO latch is transparent */
#define SC_PAD_ISO_EARLY        1U    /* Follow EARLY_ISO */
#define SC_PAD_ISO_LATE         2U    /* Follow LATE_ISO */
#define SC_PAD_ISO_ON           3U    /* ISO latched data is held */

/* values for drive strength field */
#define SC_PAD_28FDSOI_DSE_18V_1MA   0U    /* Drive strength of 1mA for 1.8v */
#define SC_PAD_28FDSOI_DSE_18V_2MA   1U    /* Drive strength of 2mA for 1.8v */
#define SC_PAD_28FDSOI_DSE_18V_4MA   2U    /* Drive strength of 4mA for 1.8v */
#define SC_PAD_28FDSOI_DSE_18V_6MA   3U    /* Drive strength of 6mA for 1.8v */
#define SC_PAD_28FDSOI_DSE_18V_8MA   4U    /* Drive strength of 8mA for 1.8v */
#define SC_PAD_28FDSOI_DSE_18V_10MA  5U    /* Drive strength of 10mA for 1.8v */
#define SC_PAD_28FDSOI_DSE_18V_12MA  6U    /* Drive strength of 12mA for 1.8v */
#define SC_PAD_28FDSOI_DSE_18V_HS    7U    /* High-speed drive strength for 1.8v */
#define SC_PAD_28FDSOI_DSE_33V_2MA   0U    /* Drive strength of 2mA for 3.3v */
#define SC_PAD_28FDSOI_DSE_33V_4MA   1U    /* Drive strength of 4mA for 3.3v */
#define SC_PAD_28FDSOI_DSE_33V_8MA   2U    /* Drive strength of 8mA for 3.3v */
#define SC_PAD_28FDSOI_DSE_33V_12MA  3U    /* Drive strength of 12mA for 3.3v */
#define SC_PAD_28FDSOI_DSE_DV_HIGH   0U    /* High drive strength for dual volt */
#define SC_PAD_28FDSOI_DSE_DV_LOW    1U    /* Low drive strength for dual volt */

/* values for pull selection field */
#define SC_PAD_28FDSOI_PS_KEEPER 0U    /* Bus-keeper (only valid for 1.8v) */
#define SC_PAD_28FDSOI_PS_PU     1U    /* Pull-up */
#define SC_PAD_28FDSOI_PS_PD     2U    /* Pull-down */
#define SC_PAD_28FDSOI_PS_NONE   3U    /* No pull (disabled) */

/* values for pad field */
#define SC_P_UART0_RTS_B 23
#define SC_P_UART0_CTS_B 24

/* mux values */
#define IMX8QM_DMA_LPUART2_RX_UART0_RTS_B 2 /* UART0_RTS_B ---> DMA_LPUART2_RX */
#define IMX8QM_DMA_LPUART2_TX_UART0_CTS_B 2 /* DMA_LPUART2_TX ---> UART0_CTS_B */


#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_IMX8QM_PINCTRL_H_ */
