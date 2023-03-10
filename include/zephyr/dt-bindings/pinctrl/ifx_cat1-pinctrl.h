/* Copyright 2022 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Pin control binding helper.
 */

/**
 * Bit definition in PINMUX field
 */
#define SOC_PINMUX_PORT_POS                                (0)
#define SOC_PINMUX_PORT_MASK                               (0xFFul << SOC_PINMUX_PORT_POS)
#define SOC_PINMUX_PIN_POS                                 (8)
#define SOC_PINMUX_PIN_MASK                                (0xFFul << SOC_PINMUX_PIN_POS)
#define SOC_PINMUX_HSIOM_FUNC_POS                          (16)
#define SOC_PINMUX_HSIOM_MASK                              (0xFFul << SOC_PINMUX_HSIOM_FUNC_POS)
#define SOC_PINMUX_SIGNAL_POS                              (24)
#define SOC_PINMUX_SIGNAL_MASK                             (0xFFul << SOC_PINMUX_SIGNAL_POS)

/**
 * Functions are defined using HSIOM SEL
 */
#define HSIOM_SEL_GPIO                                     (0)
#define HSIOM_SEL_GPIO_DSI                                 (1)
#define HSIOM_SEL_DSI_DSI                                  (2)
#define HSIOM_SEL_DSI_GPIO                                 (3)
#define HSIOM_SEL_AMUXA                                    (4)
#define HSIOM_SEL_AMUXB                                    (5)
#define HSIOM_SEL_AMUXA_DSI                                (6)
#define HSIOM_SEL_AMUXB_DSI                                (7)
#define HSIOM_SEL_ACT_0                                    (8)
#define HSIOM_SEL_ACT_1                                    (9)
#define HSIOM_SEL_ACT_2                                    (10)
#define HSIOM_SEL_ACT_3                                    (11)
#define HSIOM_SEL_DS_0                                     (12)
#define HSIOM_SEL_DS_1                                     (13)
#define HSIOM_SEL_DS_2                                     (14)
#define HSIOM_SEL_DS_3                                     (15)
#define HSIOM_SEL_ACT_4                                    (16)
#define HSIOM_SEL_ACT_5                                    (17)
#define HSIOM_SEL_ACT_6                                    (18)
#define HSIOM_SEL_ACT_7                                    (19)
#define HSIOM_SEL_ACT_8                                    (20)
#define HSIOM_SEL_ACT_9                                    (21)
#define HSIOM_SEL_ACT_10                                   (22)
#define HSIOM_SEL_ACT_11                                   (23)
#define HSIOM_SEL_ACT_12                                   (24)
#define HSIOM_SEL_ACT_13                                   (25)
#define HSIOM_SEL_ACT_14                                   (26)
#define HSIOM_SEL_ACT_15                                   (27)
#define HSIOM_SEL_DS_4                                     (28)
#define HSIOM_SEL_DS_5                                     (29)
#define HSIOM_SEL_DS_6                                     (30)
#define HSIOM_SEL_DS_7                                     (31)

/**
 * Macro to set drive mode
 */
#define DT_CAT1_DRIVE_MODE_INFO(peripheral_signal) \
	CAT1_PIN_MAP_DRIVE_MODE_##peripheral_signal

/**
 * Macro to set pin control information (from pinctrl node)
 */
#define DT_CAT1_PINMUX(port, pin, hsiom) \
	((port << SOC_PINMUX_PORT_POS) | \
	 (pin << SOC_PINMUX_PIN_POS) |	 \
	 (hsiom << SOC_PINMUX_HSIOM_FUNC_POS))

/* Redefine DT GPIO label (Px) to CYHAL port macros (CYHAL_PORT_x) */
#define P0  CYHAL_PORT_0
#define P1  CYHAL_PORT_1
#define P2  CYHAL_PORT_2
#define P3  CYHAL_PORT_3
#define P4  CYHAL_PORT_4
#define P5  CYHAL_PORT_5
#define P6  CYHAL_PORT_6
#define P7  CYHAL_PORT_7
#define P8  CYHAL_PORT_8
#define P9  CYHAL_PORT_9
#define P10 CYHAL_PORT_10
#define P11 CYHAL_PORT_11
#define P12 CYHAL_PORT_12
#define P13 CYHAL_PORT_13
#define P14 CYHAL_PORT_14
#define P15 CYHAL_PORT_15
#define P16 CYHAL_PORT_16
#define P17 CYHAL_PORT_17
#define P18 CYHAL_PORT_18
#define P19 CYHAL_PORT_19
#define P20 CYHAL_PORT_20

/* Returns CYHAL GPIO from Board device tree GPIO configuration */
#define DT_GET_CYHAL_GPIO_FROM_DT_GPIOS(node, gpios_prop)				 \
	CYHAL_GET_GPIO(DT_STRING_TOKEN(DT_GPIO_CTLR_BY_IDX(node, gpios_prop, 0), label), \
		       DT_PHA_BY_IDX(node, gpios_prop, 0, pin))
