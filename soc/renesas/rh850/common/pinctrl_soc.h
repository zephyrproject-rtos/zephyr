/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_RENESAS_RH850_COMMON_PINCTRL_SOC_H_
#define ZEPHYR_SOC_RENESAS_RH850_COMMON_PINCTRL_SOC_H_

#include <zephyr/devicetree.h>
#include <zephyr/types.h>

#include <zephyr/dt-bindings/pinctrl/renesas/pinctrl-rh850-common.h>

/**
 * @brief Type to hold a Renesas RH850 pin's pinctrl configuration.
 */
struct rh850_pinctrl_soc_pin {
	/** Port identifier */
	uint32_t port_id;
	/** Pin number 0..15 */
	uint32_t pin_num;
	/** Pin configuration */
	uint32_t cfg;
};

typedef struct rh850_pinctrl_soc_pin pinctrl_soc_pin_t;

/**
 * @brief Maps DT input-buffer-selection enum index to input buffer
 * configuration value for pin controller.
 */
static const uint32_t rh850_input_buffer_tbl[] = {
	DISABLE_INPUT_BUFFER,   /* disable */
	SHMT1_INPUT_BUFFER,     /* schmitt1 */
	SHMT4_INPUT_BUFFER,     /* schmitt4 */
	TTL_INPUT_BUFFER,       /* ttl */
	SHMTMSC_INPUT_BUFFER    /* schmittmsc */
};

/**
 * @brief Utility macro to initialize each pin.
 *
 * @param node_id Node identifier.
 * @param prop Property name.
 * @param idx Property entry index.
 */
#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)                                    \
	{.port_id = RH850_GET_PORT_ID(DT_PROP_BY_IDX(node_id, prop, idx)),                  \
	 .pin_num = RH850_GET_PIN_NUM(DT_PROP_BY_IDX(node_id, prop, idx)),                  \
	 .cfg = RH850_GET_PORT_MODE(DT_PROP_BY_IDX(node_id, prop, idx)) |                   \
		(DT_PROP(node_id, renesas_direct_control) << RH850_PORT_PCRn_m_PCR_PIPC_POS) |  \
		(DT_PROP(node_id, bias_pull_up) << RH850_PORT_PCRn_m_PCR_PU_POS) |              \
		(DT_PROP(node_id, bias_pull_down) << RH850_PORT_PCRn_m_PCR_PD_POS) |            \
		(RH850_OUTPUT_DRIVE_MODE(node_id) << RH850_PORT_PCRn_m_PCR_PODCE_POS) |         \
		(DT_PROP(node_id, input_enable) << RH850_PORT_PCRn_m_PCR_PM_POS) |              \
		(DT_PROP(node_id, output_high) << RH850_PORT_PCRn_m_PCR_P_POS) |                \
		(DT_ENUM_IDX(node_id, drive_strength) << RH850_PORT_PCRn_m_PCR_PDSC_POS) |      \
		(RH850_INPUT_BUFFER_CFG(node_id)) |                                             \
		(DT_PROP(node_id, renesas_bidirect_control) << RH850_PORT_PCRn_m_PCR_PBDC_POS)},

/**
 * @brief Utility macro to initialize state pins contained in a given property.
 *
 * @param node_id Node identifier.
 * @param prop Property name describing state pins.
 */
#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)                                                   \
	{                                                                                          \
		DT_FOREACH_CHILD_VARGS(DT_PHANDLE(node_id, prop), DT_FOREACH_PROP_ELEM, psels,     \
				       Z_PINCTRL_STATE_PIN_INIT)                                   \
	}

/* Bit position in bsp_io_port_t structure of FSP */
#define RH850_FSP_PORT_PWE_POS   (24)
#define RH850_FSP_PORT_TYPE_POS  (16)
#define RH850_FSP_PORT_GROUP_POS (8)

/**
 * @brief Extract PORT_SEL field from pinctrl configuration value.
 *
 * @param pinctrl Pin control configuration value from psels property.
 * @return PORT_SEL field value.
 */
#define RH850_GET_PORT_SEL(pinctrl) ((pinctrl & RH850_PORT_SEL_MASK) >> RH850_PORT_SEL_POS)

/**
 * @brief Extract PORT_MODE field from pinctrl configuration value.
 *
 * @param pinctrl Pin control configuration value from psels property.
 * @return PORT_MODE field value.
 */
#define RH850_GET_PORT_MODE(pinctrl) ((pinctrl & RH850_PORT_MODE_MASK) >> RH850_PORT_MODE_POS)

/**
 * @brief Convert PORT_SEL field to FSP bsp_io_port_t structure format.
 *
 * @param pinctrl Pin control configuration value from psels property.
 * @return Port ID in FSP bsp_io_port_t format with PWE, TYPE, and GROUP fields positioned
 * correctly.
 */
#define RH850_GET_PORT_ID(pinctrl)                                                                 \
	((((RH850_GET_PORT_SEL(pinctrl) & RH850_PORT_PWE_MASK) >> RH850_PORT_PWE_POS)              \
	  << RH850_FSP_PORT_PWE_POS) |                                                             \
	 (((RH850_GET_PORT_SEL(pinctrl) & RH850_PORT_TYPE_MASK) >> RH850_PORT_TYPE_POS)            \
	  << RH850_FSP_PORT_TYPE_POS) |                                                            \
	 (((RH850_GET_PORT_SEL(pinctrl) & RH850_PORT_GROUP_MASK) >> RH850_PORT_GROUP_POS)          \
	  << RH850_FSP_PORT_GROUP_POS))

/**
 * @brief Extract pin number from pinctrl configuration value.
 *
 * @param pinctrl Pin control configuration value from psels property.
 * @return Pin number within the port.
 */
#define RH850_GET_PIN_NUM(pinctrl)                                                                 \
	((RH850_GET_PORT_SEL(pinctrl) & RH850_PORT_PIN_MASK) >> RH850_PORT_PIN_POS)

/**
 * @brief Extract pin mode configuration from pin configuration value.
 *
 * @param pincfg Pin configuration value containing mode settings.
 * @return Combined mode configuration including PFC, PFCE, PFCAE, PFCEAE, PM, and PMC fields.
 */
#define RH850_GET_MODE(pincfg)                                                                     \
	((pincfg & (RH850_PORT_PCRn_m_PCR_PFC_Msk | RH850_PORT_PCRn_m_PCR_PFCE_Msk |               \
		    RH850_PORT_PCRn_m_PCR_PFCAE_Msk | RH850_PORT_PCRn_m_PCR_PFCEAE_Msk |           \
		    RH850_PORT_PCRn_m_PCR_PM_Msk | RH850_PORT_PCRn_m_PCR_PMC_Msk)) >>              \
	 RH850_PORT_PCRn_m_PCR_PFC_POS)

/**
 * @brief Extract pull-up configuration from pin configuration value.
 *
 * @param pincfg Pin configuration value.
 * @return Pull-up enable status (1 = enabled, 0 = disabled).
 */
#define RH850_GET_PULL_UP(pincfg)                                                                \
	((pincfg & RH850_PORT_PCRn_m_PCR_PU_Msk) >> RH850_PORT_PCRn_m_PCR_PU_POS)

/**
 * @brief Extract pull-down configuration from pin configuration value.
 *
 * @param pincfg Pin configuration value.
 * @return Pull-down enable status (1 = enabled, 0 = disabled).
 */
#define RH850_GET_PULL_DOWN(pincfg)                                                              \
	((pincfg & RH850_PORT_PCRn_m_PCR_PD_Msk) >> RH850_PORT_PCRn_m_PCR_PD_POS)

/**
 * @brief Extract drive output pin mode configuration from pin configuration value.
 *
 * @param pincfg Pin configuration value.
 * @return Encoded value of drive output pin mode
 * 0x1 = Push-Pull
 * 0x2 = Drive open drain
 * 0x3 = Drive open Source
 */
#define RH850_GET_OPEN_DRAIN(pincfg)                                                           \
	((pincfg & (RH850_PORT_PCRn_m_PCR_PODC_Msk |                                               \
		RH850_PORT_PCRn_m_PCR_PODCE_Msk)) >> RH850_PORT_PCRn_m_PCR_PODCE_POS)

/**
 * @brief Extract input mode configuration from pin configuration value.
 *
 * @param pincfg Pin configuration value.
 * @return Input mode status (1 = input, 0 = output).
 */
#define RH850_GET_INPUT_MODE(pincfg)                                                           \
	((pincfg & RH850_PORT_PCRn_m_PCR_PM_Msk) >> RH850_PORT_PCRn_m_PCR_PM_POS)

/**
 * @brief Extract output level from pin configuration value.
 *
 * @param pincfg Pin configuration value.
 * @return Output pin value (1 = high, 0 = low).
 */
#define RH850_GET_OUTPUT(pincfg)                                                               \
	((pincfg & RH850_PORT_PCRn_m_PCR_P_Msk) >> RH850_PORT_PCRn_m_PCR_P_POS)

/**
 * @brief Extract drive strength configuration from pin configuration value.
 *
 * @param pincfg Pin configuration value.
 * @return Drive strength setting combining PDSC and PUCC fields.
 */
#define RH850_GET_DRIVE_STRENGTH(pincfg)                                                       \
	((pincfg & (RH850_PORT_PCRn_m_PCR_PDSC_Msk | RH850_PORT_PCRn_m_PCR_PUCC_Msk)) >>           \
	 RH850_PORT_PCRn_m_PCR_PDSC_POS)

/**
 * @brief Extract direct I/O control configuration from pin configuration value.
 *
 * @param pincfg Pin configuration value.
 * @return Direct I/O control enable status (1 = enabled, 0 = disabled).
 */
#define RH850_GET_DIRECT_CONTROL(pincfg)                                                        \
	((pincfg & RH850_PORT_PCRn_m_PCR_PIPC_Msk) >> RH850_PORT_PCRn_m_PCR_PIPC_POS)

/**
 * @brief Extract Bi-Direction control configuration from pin configuration value.
 *
 * @param pincfg Pin configuration value.
 * @return Bi-Direction control enable status (1 = enabled, 0 = disabled).
 */
#define RH850_GET_BIDIRECT_CONTROL(pincfg)                                                        \
	((pincfg & RH850_PORT_PCRn_m_PCR_PBDC_Msk) >> RH850_PORT_PCRn_m_PCR_PBDC_POS)

/**
 * @brief Extract Input buffer characteristics from pin configuration value.
 *
 * @param pincfg Pin configuration value.
 * @return Encoded value of input buffer characteristic selection
 * 0x0 = Input buffer disable
 * 0x1 = SHMT1 input buffer
 * 0x11 = SHMT4 input buffer
 * 0x41 = TTL input buffer
 * 0x51 = SHMTMSC input buffer
 * Other values are invalid
 */
#define RH850_GET_INPUT_BUFFER(pincfg)                                                         \
	((pincfg & (RH850_PORT_PCRn_m_PCR_PIBC_Msk | RH850_PORT_PCRn_m_PCR_PIS_Msk |               \
		RH850_PORT_PCRn_m_PCR_PISA_Msk)) >> RH850_PORT_PCRn_m_PCR_PIBC_POS)

#endif /* ZEPHYR_SOC_RENESAS_RH850_COMMON_PINCTRL_SOC_H_ */
