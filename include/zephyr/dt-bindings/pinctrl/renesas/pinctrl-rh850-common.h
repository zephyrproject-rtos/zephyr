/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_RH850_COMMON_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_RH850_COMMON_H_

/* Bit positions for fields within psels property */
#define RH850_PORT_SEL_POS  (12U) /* Position of PORT_SEL field in psels property */
#define RH850_PORT_MODE_POS (0U)  /* Position of PORT_MODE field in psels property */

/* Bit masks for extracting fields from psels property */
#define RH850_PORT_SEL_MASK  (0xFFFFF000UL) /* Mask for PORT_SEL field (bits 31:12) */
#define RH850_PORT_MODE_MASK (0xFFFUL)      /* Mask for PORT_MODE field (bits 11:0) */

/* Bit positions for sub-fields within PORT_SEL field */
#define RH850_PORT_PWE_POS   (12U) /* PWE field position within PORT_SEL */
#define RH850_PORT_TYPE_POS  (10U) /* Port type field position within PORT_SEL */
#define RH850_PORT_GROUP_POS (4U)  /* Port group field position within PORT_SEL */
#define RH850_PORT_PIN_POS   (0U)  /* Pin number field position within PORT_SEL */

/* Bit masks for extracting sub-fields from PORT_SEL field */
#define RH850_PORT_PWE_MASK   (0xFF000UL) /* PWE field mask (8 bits) */
#define RH850_PORT_TYPE_MASK  (0xC00UL)   /* Port type field mask (2 bits) */
#define RH850_PORT_GROUP_MASK (0x3F0UL)   /* Port group field mask (6 bits) */
#define RH850_PORT_PIN_MASK   (0xFUL)     /* Pin number field mask (4 bits) */

/* Port Control Register (PCR) bit positions */
#define RH850_PORT_PCRn_m_PCR_PFC_POS    (0U)
#define RH850_PORT_PCRn_m_PCR_PFCE_POS   (1U)
#define RH850_PORT_PCRn_m_PCR_PFCAE_POS  (2U)
#define RH850_PORT_PCRn_m_PCR_PFCEAE_POS (3U)
#define RH850_PORT_PCRn_m_PCR_PM_POS     (4U)
#define RH850_PORT_PCRn_m_PCR_PIPC_POS   (5U)
#define RH850_PORT_PCRn_m_PCR_PMC_POS    (6U)
#define RH850_PORT_PCRn_m_PCR_PPR_POS    (8U)
#define RH850_PORT_PCRn_m_PCR_P_POS      (12U)
#define RH850_PORT_PCRn_m_PCR_PIBC_POS   (16U)
#define RH850_PORT_PCRn_m_PCR_PBDC_POS   (17U)
#define RH850_PORT_PCRn_m_PCR_PD_POS     (18U)
#define RH850_PORT_PCRn_m_PCR_PU_POS     (19U)
#define RH850_PORT_PCRn_m_PCR_PIS_POS    (20U)
#define RH850_PORT_PCRn_m_PCR_PISA_POS   (22U)
#define RH850_PORT_PCRn_m_PCR_PDSC_POS   (24U)
#define RH850_PORT_PCRn_m_PCR_PUCC_POS   (25U)
#define RH850_PORT_PCRn_m_PCR_PODCE_POS  (27U)
#define RH850_PORT_PCRn_m_PCR_PODC_POS   (28U)
#define RH850_PORT_PCRn_m_PCR_PINV_POS   (30U)

/* Port Control Register (PCR) bit masks */
#define RH850_PORT_PCRn_m_PCR_PFC_Msk    (0x1UL)
#define RH850_PORT_PCRn_m_PCR_PFCE_Msk   (0x2UL)
#define RH850_PORT_PCRn_m_PCR_PFCAE_Msk  (0x4UL)
#define RH850_PORT_PCRn_m_PCR_PFCEAE_Msk (0x8UL)
#define RH850_PORT_PCRn_m_PCR_PM_Msk     (0x10UL)
#define RH850_PORT_PCRn_m_PCR_PIPC_Msk   (0x20UL)
#define RH850_PORT_PCRn_m_PCR_PMC_Msk    (0x40UL)
#define RH850_PORT_PCRn_m_PCR_PPR_Msk    (0x100UL)
#define RH850_PORT_PCRn_m_PCR_P_Msk      (0x1000UL)
#define RH850_PORT_PCRn_m_PCR_PIBC_Msk   (0x10000UL)
#define RH850_PORT_PCRn_m_PCR_PBDC_Msk   (0x20000UL)
#define RH850_PORT_PCRn_m_PCR_PD_Msk     (0x40000UL)
#define RH850_PORT_PCRn_m_PCR_PU_Msk     (0x80000UL)
#define RH850_PORT_PCRn_m_PCR_PIS_Msk    (0x100000UL)
#define RH850_PORT_PCRn_m_PCR_PISA_Msk   (0x400000UL)
#define RH850_PORT_PCRn_m_PCR_PDSC_Msk   (0x1000000UL)
#define RH850_PORT_PCRn_m_PCR_PUCC_Msk   (0x2000000UL)
#define RH850_PORT_PCRn_m_PCR_PODCE_Msk  (0x8000000UL)
#define RH850_PORT_PCRn_m_PCR_PODC_Msk   (0x10000000UL)
#define RH850_PORT_PCRn_m_PCR_PINV_Msk   (0x40000000UL)

/* Pin mode configurations */
#define RH850_CFG_PORT_PIN (0U << RH850_PORT_PCRn_m_PCR_PMC_POS) /* Configure pin as GPIO */
#define RH850_CFG_PERIPHERAL_PIN                                                                   \
	(1U << RH850_PORT_PCRn_m_PCR_PMC_POS) /* Configure pin for peripheral function */

/* Base configurations for alternative functions */
#define RH850_ALT_INx                                                                              \
	(RH850_CFG_PERIPHERAL_PIN | (1U << RH850_PORT_PCRn_m_PCR_PM_POS)) /* input base */
#define RH850_ALT_OUTx                                                                             \
	(RH850_CFG_PERIPHERAL_PIN | (0U << RH850_PORT_PCRn_m_PCR_PM_POS)) /* output base */

/* Alternative input function definitions (1-16) */
#define RH850_ALT_IN1                                                                              \
	(RH850_ALT_INx |                                                                           \
	 (0x00U << RH850_PORT_PCRn_m_PCR_PFC_POS)) /* Alternative input function 1 */
#define RH850_ALT_IN2                                                                              \
	(RH850_ALT_INx |                                                                           \
	 (0x01U << RH850_PORT_PCRn_m_PCR_PFC_POS)) /* Alternative input function 2 */
#define RH850_ALT_IN3                                                                              \
	(RH850_ALT_INx |                                                                           \
	 (0x02U << RH850_PORT_PCRn_m_PCR_PFC_POS)) /* Alternative input function 3 */
#define RH850_ALT_IN4                                                                              \
	(RH850_ALT_INx |                                                                           \
	 (0x03U << RH850_PORT_PCRn_m_PCR_PFC_POS)) /* Alternative input function 4 */
#define RH850_ALT_IN5                                                                              \
	(RH850_ALT_INx |                                                                           \
	 (0x04U << RH850_PORT_PCRn_m_PCR_PFC_POS)) /* Alternative input function 5 */
#define RH850_ALT_IN6                                                                              \
	(RH850_ALT_INx |                                                                           \
	 (0x05U << RH850_PORT_PCRn_m_PCR_PFC_POS)) /* Alternative input function 6 */
#define RH850_ALT_IN7                                                                              \
	(RH850_ALT_INx |                                                                           \
	 (0x06U << RH850_PORT_PCRn_m_PCR_PFC_POS)) /* Alternative input function 7 */
#define RH850_ALT_IN8                                                                              \
	(RH850_ALT_INx |                                                                           \
	 (0x07U << RH850_PORT_PCRn_m_PCR_PFC_POS)) /* Alternative input function 8 */
#define RH850_ALT_IN9                                                                              \
	(RH850_ALT_INx |                                                                           \
	 (0x08U << RH850_PORT_PCRn_m_PCR_PFC_POS)) /* Alternative input function 9 */
#define RH850_ALT_IN10                                                                             \
	(RH850_ALT_INx |                                                                           \
	 (0x09U << RH850_PORT_PCRn_m_PCR_PFC_POS)) /* Alternative input function 10 */
#define RH850_ALT_IN11                                                                             \
	(RH850_ALT_INx |                                                                           \
	 (0x0AU << RH850_PORT_PCRn_m_PCR_PFC_POS)) /* Alternative input function 11 */
#define RH850_ALT_IN12                                                                             \
	(RH850_ALT_INx |                                                                           \
	 (0x0BU << RH850_PORT_PCRn_m_PCR_PFC_POS)) /* Alternative input function 12 */
#define RH850_ALT_IN13                                                                             \
	(RH850_ALT_INx |                                                                           \
	 (0x0CU << RH850_PORT_PCRn_m_PCR_PFC_POS)) /* Alternative input function 13 */
#define RH850_ALT_IN14                                                                             \
	(RH850_ALT_INx |                                                                           \
	 (0x0DU << RH850_PORT_PCRn_m_PCR_PFC_POS)) /* Alternative input function 14 */
#define RH850_ALT_IN15                                                                             \
	(RH850_ALT_INx |                                                                           \
	 (0x0EU << RH850_PORT_PCRn_m_PCR_PFC_POS)) /* Alternative input function 15 */
#define RH850_ALT_IN16                                                                             \
	(RH850_ALT_INx |                                                                           \
	 (0x0FU << RH850_PORT_PCRn_m_PCR_PFC_POS)) /* Alternative input function 16 */

/* Alternative output function definitions (1-16) */
#define RH850_ALT_OUT1                                                                             \
	(RH850_ALT_OUTx |                                                                          \
	 (0x00U << RH850_PORT_PCRn_m_PCR_PFC_POS)) /* Alternative output function 1 */
#define RH850_ALT_OUT2                                                                             \
	(RH850_ALT_OUTx |                                                                          \
	 (0x01U << RH850_PORT_PCRn_m_PCR_PFC_POS)) /* Alternative output function 2 */
#define RH850_ALT_OUT3                                                                             \
	(RH850_ALT_OUTx |                                                                          \
	 (0x02U << RH850_PORT_PCRn_m_PCR_PFC_POS)) /* Alternative output function 3 */
#define RH850_ALT_OUT4                                                                             \
	(RH850_ALT_OUTx |                                                                          \
	 (0x03U << RH850_PORT_PCRn_m_PCR_PFC_POS)) /* Alternative output function 4 */
#define RH850_ALT_OUT5                                                                             \
	(RH850_ALT_OUTx |                                                                          \
	 (0x04U << RH850_PORT_PCRn_m_PCR_PFC_POS)) /* Alternative output function 5 */
#define RH850_ALT_OUT6                                                                             \
	(RH850_ALT_OUTx |                                                                          \
	 (0x05U << RH850_PORT_PCRn_m_PCR_PFC_POS)) /* Alternative output function 6 */
#define RH850_ALT_OUT7                                                                             \
	(RH850_ALT_OUTx |                                                                          \
	 (0x06U << RH850_PORT_PCRn_m_PCR_PFC_POS)) /* Alternative output function 7 */
#define RH850_ALT_OUT8                                                                             \
	(RH850_ALT_OUTx |                                                                          \
	 (0x07U << RH850_PORT_PCRn_m_PCR_PFC_POS)) /* Alternative output function 8 */
#define RH850_ALT_OUT9                                                                             \
	(RH850_ALT_OUTx |                                                                          \
	 (0x08U << RH850_PORT_PCRn_m_PCR_PFC_POS)) /* Alternative output function 9 */
#define RH850_ALT_OUT10                                                                            \
	(RH850_ALT_OUTx |                                                                          \
	 (0x09U << RH850_PORT_PCRn_m_PCR_PFC_POS)) /* Alternative output function 10 */
#define RH850_ALT_OUT11                                                                            \
	(RH850_ALT_OUTx |                                                                          \
	 (0x0AU << RH850_PORT_PCRn_m_PCR_PFC_POS)) /* Alternative output function 11 */
#define RH850_ALT_OUT12                                                                            \
	(RH850_ALT_OUTx |                                                                          \
	 (0x0BU << RH850_PORT_PCRn_m_PCR_PFC_POS)) /* Alternative output function 12 */
#define RH850_ALT_OUT13                                                                            \
	(RH850_ALT_OUTx |                                                                          \
	 (0x0CU << RH850_PORT_PCRn_m_PCR_PFC_POS)) /* Alternative output function 13 */
#define RH850_ALT_OUT14                                                                            \
	(RH850_ALT_OUTx |                                                                          \
	 (0x0DU << RH850_PORT_PCRn_m_PCR_PFC_POS)) /* Alternative output function 14 */
#define RH850_ALT_OUT15                                                                            \
	(RH850_ALT_OUTx |                                                                          \
	 (0x0EU << RH850_PORT_PCRn_m_PCR_PFC_POS)) /* Alternative output function 15 */
#define RH850_ALT_OUT16                                                                            \
	(RH850_ALT_OUTx |                                                                          \
	 (0x0FU << RH850_PORT_PCRn_m_PCR_PFC_POS)) /* Alternative output function 16 */

/* GPIO mode (same as port pin mode) */
#define RH850_GPIO (RH850_CFG_PORT_PIN) /* Configure pin as general-purpose I/O */

/* Pin drive strength levels */
#define RH850_DRIVE_STRENGTH_VERY_LOW 0U /* Very low drive strength */
#define RH850_DRIVE_STRENGTH_LOW      1U /* Low drive strength */
#define RH850_DRIVE_STRENGTH_MEDIUM   2U /* Medium drive strength */
#define RH850_DRIVE_STRENGTH_HIGH     3U /* High drive strength */

/* Input buffer characteristic type selection */
#define DISABLE_INPUT_BUFFER  (0U << RH850_PORT_PCRn_m_PCR_PIBC_POS)
#define SHMT1_INPUT_BUFFER    ((1U << RH850_PORT_PCRn_m_PCR_PIBC_POS) | \
							(0U << RH850_PORT_PCRn_m_PCR_PIS_POS))
#define SHMT4_INPUT_BUFFER  ((1U << RH850_PORT_PCRn_m_PCR_PIBC_POS) | \
							(1U << RH850_PORT_PCRn_m_PCR_PIS_POS))
#define TTL_INPUT_BUFFER    ((1U << RH850_PORT_PCRn_m_PCR_PIBC_POS) | \
							(4U << RH850_PORT_PCRn_m_PCR_PIS_POS))
#define SHMTMSC_INPUT_BUFFER  ((1U << RH850_PORT_PCRn_m_PCR_PIBC_POS) | \
							(5U << RH850_PORT_PCRn_m_PCR_PIS_POS))

/**
 * @brief Maps DT enum index to RH850 input buffer configuration value.
 */
#define RH850_INPUT_BUFFER_CFG(node_id) \
	rh850_input_buffer_tbl[DT_ENUM_IDX(node_id, renesas_input_buffer_selection)]


/**
 * @brief Maps DT drive-* properties to RH850 output drive mode.
 */
#define RH850_OUTPUT_DRIVE_MODE(node_id)       \
	(DT_PROP(node_id, drive_push_pull) ? 1U :  \
	DT_PROP(node_id, drive_open_drain)  ? 2U : \
	DT_PROP(node_id, drive_open_source) ? 3U : \
	0U)

/**
 * @brief Create a pin selection value for RH850 pinctrl configuration.
 *
 * This macro combines port number and pin number into the PORT_SEL field, and
 * concatenates the mode parameter with "RH850_" prefix to form the PORT_MODE field.
 * The resulting value can be used in device tree pinctrl configurations.
 *
 * @param port_num Port number identifier - combined with pin_num for PORT_SEL field.
 * @param pin_num Pin number within the port - combined with port_num for PORT_SEL field.
 * @param mode Pin mode suffix (will be prefixed with "RH850_" to form complete mode constant).
 * @return Complete pin selection value with PORT_SEL and PORT_MODE fields properly positioned.
 */
#define RH850_PSEL(port_num, pin_num, mode)                                                        \
	(((port_num | pin_num) << RH850_PORT_SEL_POS) | ((RH850_##mode) << RH850_PORT_MODE_POS))

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_RH850_COMMON_H_ */
