/*
 * Copyright (c) 2022 Gerson Fernando Budke
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DT_BINDINGS_PINCTRL_ATMEL_SAM_H_
#define DT_BINDINGS_PINCTRL_ATMEL_SAM_H_

/*
 * @name Atmel SAM gpio port list.
 * @{
 */

#define SAM_PINMUX_PORT_a		0U
#define SAM_PINMUX_PORT_b		1U
#define SAM_PINMUX_PORT_c		2U
#define SAM_PINMUX_PORT_d		3U
#define SAM_PINMUX_PORT_e		4U
#define SAM_PINMUX_PORT_f		5U
#define SAM_PINMUX_PORT_g		6U
#define SAM_PINMUX_PORT_h		7U
#define SAM_PINMUX_PORT_i		8U
#define SAM_PINMUX_PORT_j		9U
#define SAM_PINMUX_PORT_k		10U
#define SAM_PINMUX_PORT_l		11U
#define SAM_PINMUX_PORT_m		12U
#define SAM_PINMUX_PORT_n		13U
#define SAM_PINMUX_PORT_o		14U
#define SAM_PINMUX_PORT_p		15U

/**
 * @name Atmel SAM peripheral list.
 * @{
 */

/** GPIO */
#define SAM_PINMUX_PERIPH_gpio		0U
/** Peripherals */
#define SAM_PINMUX_PERIPH_a		0U
#define SAM_PINMUX_PERIPH_b		1U
#define SAM_PINMUX_PERIPH_c		2U
#define SAM_PINMUX_PERIPH_d		3U
#define SAM_PINMUX_PERIPH_e		4U
#define SAM_PINMUX_PERIPH_f		5U
#define SAM_PINMUX_PERIPH_g		6U
#define SAM_PINMUX_PERIPH_h		7U
#define SAM_PINMUX_PERIPH_i		8U
#define SAM_PINMUX_PERIPH_j		9U
#define SAM_PINMUX_PERIPH_k		10U
#define SAM_PINMUX_PERIPH_l		11U
#define SAM_PINMUX_PERIPH_m		12U
#define SAM_PINMUX_PERIPH_n		13U
/** Extra */
#define SAM_PINMUX_PERIPH_x		0U
/** System */
#define SAM_PINMUX_PERIPH_s		0U
/** LPM */
#define SAM_PINMUX_PERIPH_lpm		0U
/** Wake-up pin sources */
#define SAM_PINMUX_PERIPH_wkup0		0U
#define SAM_PINMUX_PERIPH_wkup1		1U
#define SAM_PINMUX_PERIPH_wkup2		2U
#define SAM_PINMUX_PERIPH_wkup3		3U
#define SAM_PINMUX_PERIPH_wkup4		4U
#define SAM_PINMUX_PERIPH_wkup5		5U
#define SAM_PINMUX_PERIPH_wkup6		6U
#define SAM_PINMUX_PERIPH_wkup7		7U
#define SAM_PINMUX_PERIPH_wkup8		8U
#define SAM_PINMUX_PERIPH_wkup9		9U
#define SAM_PINMUX_PERIPH_wkup10	10U
#define SAM_PINMUX_PERIPH_wkup11	11U
#define SAM_PINMUX_PERIPH_wkup12	12U
#define SAM_PINMUX_PERIPH_wkup13	13U
#define SAM_PINMUX_PERIPH_wkup14	14U
#define SAM_PINMUX_PERIPH_wkup15	15U
/** @} */

/**
 * @name Atmel SAM pin function list.
 * @{
 */

/** Selects pin to be used as GPIO */
#define SAM_PINMUX_FUNC_gpio		0U
/** Selects pin to be used as by some peripheral */
#define SAM_PINMUX_FUNC_periph		1U
/** Selects pin to be used as extra function */
#define SAM_PINMUX_FUNC_extra		2U
/** Selects pin to be used as system function */
#define SAM_PINMUX_FUNC_system		3U
/** Selects and configure pin to be used in Low Power Mode */
#define SAM_PINMUX_FUNC_lpm		4U
/** Selects and configure wake-up pin sources Low Power Mode */
#define SAM_PINMUX_FUNC_wakeup		5U

/** @} */

/**
 * @name Atmel SAM pinmux bit field mask and positions.
 * @{
 */

/** Pinmux bit field position. */
#define SAM_PINCTRL_PINMUX_POS		(16U)
/** Pinmux bit field mask. */
#define SAM_PINCTRL_PINMUX_MASK		(0xFFFF)

/** Port field mask. */
#define SAM_PINMUX_PORT_MSK		(0xFU)
/** Port field position. */
#define SAM_PINMUX_PORT_POS		(0U)
/** Pin field mask. */
#define SAM_PINMUX_PIN_MSK		(0x1FU)
/** Pin field position. */
#define SAM_PINMUX_PIN_POS		(SAM_PINMUX_PORT_POS + 4U)
/** Function field mask. */
#define SAM_PINMUX_FUNC_MSK		(0x7U)
/** Function field position. */
#define SAM_PINMUX_FUNC_POS		(SAM_PINMUX_PIN_POS + 5U)
/** Peripheral field mask. */
#define SAM_PINMUX_PERIPH_MSK		(0xFU)
/** Peripheral field position. */
#define SAM_PINMUX_PERIPH_POS		(SAM_PINMUX_FUNC_POS + 3U)

/** @} */

/**
 * @brief Atmel SAM pinmux bit field.
 * @anchor SAM_PINMUX
 *
 * Fields:
 *
 * -   0..3: port
 * -   4..8: pin_num
 * -  9..11: func
 * - 12..15: pin_mux
 *
 * @param port    Port ('A'..'P')
 * @param pin_num Pin  (0..31)
 * @param func    Function (GPIO, Peripheral, System, Extra, LPM - 0..4)
 * @param pin_mux Peripheral based on the Function selected (0..15)
 */
#define SAM_PINMUX(port, pin_num, pin_mux, func)			  \
	((((SAM_PINMUX_PORT_##port) & SAM_PINMUX_PORT_MSK)		  \
	 << SAM_PINMUX_PORT_POS)					| \
	 (((pin_num) & SAM_PINMUX_PIN_MSK)				  \
	 << SAM_PINMUX_PIN_POS)						| \
	 (((SAM_PINMUX_FUNC_##func) & SAM_PINMUX_FUNC_MSK)		  \
	 << SAM_PINMUX_FUNC_POS)					| \
	 (((SAM_PINMUX_PERIPH_##pin_mux) & SAM_PINMUX_PERIPH_MSK)	  \
	 << SAM_PINMUX_PERIPH_POS))

/**
 * Obtain Pinmux value from pinctrl_soc_pin_t configuration.
 *
 * @param pincfg pinctrl_soc_pin_t bit field value.
 */
#define SAM_PINMUX_GET(pincfg) \
	(((pincfg) >> SAM_PINCTRL_PINMUX_POS) & SAM_PINCTRL_PINMUX_MASK)

#define SAM_PINMUX_PORT_GET(pincfg) \
	((SAM_PINMUX_GET(pincfg) >> SAM_PINMUX_PORT_POS) \
	 & SAM_PINMUX_PORT_MSK)

#define SAM_PINMUX_PIN_GET(pincfg) \
	((SAM_PINMUX_GET(pincfg) >> SAM_PINMUX_PIN_POS) \
	 & SAM_PINMUX_PIN_MSK)

#define SAM_PINMUX_FUNC_GET(pincfg) \
	((SAM_PINMUX_GET(pincfg) >> SAM_PINMUX_FUNC_POS) \
	 & SAM_PINMUX_FUNC_MSK)

#define SAM_PINMUX_PERIPH_GET(pincfg) \
	((SAM_PINMUX_GET(pincfg) >> SAM_PINMUX_PERIPH_POS) \
	 & SAM_PINMUX_PERIPH_MSK)

#endif  /* DT_BINDINGS_PINCTRL_ATMEL_SAM_H_ */
