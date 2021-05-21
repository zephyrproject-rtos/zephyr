/*
 * Copyright (c) 2019 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Microchip XEC MCU family General Purpose Input Output (GPIO) defines.
 *
 */

#ifndef _MICROCHIP_MEC_SOC_GPIO_H_
#define _MICROCHIP_MEC_SOC_GPIO_H_

#define MCHP_GPIO_000_036	0
#define MCHP_GPIO_040_076	1
#define MCHP_GPIO_100_136	2
#define MCHP_GPIO_140_176	3
#define MCHP_GPIO_200_236	4
#define MCHP_GPIO_240_276	5
#define MCHP_GPIO_MAX_PORT	6
#define MCHP_NUM_GPIO_PORTS	6

/* pin internal pull up/down or repeater mode */
#define MCHP_GPIO_CFG_PULLS_POS		0U
#define MCHP_GPIO_CFG_PULLS_MASK	(3U << 0)
#define MCHP_GPIO_CFG_PULLS_NONE	0U
#define MCHP_GPIO_CFG_PULLS_UP		(1U << 0)
#define MCHP_GPIO_CFG_PULLS_DN		(2U << 0)
#define MCHP_GPIO_CFG_PULLS_REPEAT	(3U << 0)

/* power gate: GPIO pin enabled when specified voltage rail is on */
#define MCHP_GPIO_CFG_PG_POS		2U
#define MCHP_GPIO_CFG_PG_MASK		(3U << 2)
#define MCHP_GPIO_CFG_PG_VTR		0U
#define MCHP_GPIO_CFG_PG_VCC		(1U << 2)
/* pin powered down (no input or output) */
#define MCHP_GPIO_CFG_PG_UNPWRD		(2U << 2)

/* enable pin interrupt detection */
#define MCHP_GPIO_CFG_IDET_POS		4U
#define MCHP_GPIO_CFG_IDET_MASK		(7 << 4)
#define MCHP_GPIO_CFG_IDET_DIS		0U
#define MCHP_GPIO_CFG_IDET_LVL_LO	(1U << 4)
#define MCHP_GPIO_CFG_IDET_LVL_HI	(2U << 4)
#define MCHP_GPIO_CFG_IDET_EDGE_RISING	(3U << 4)
#define MCHP_GPIO_CFG_IDET_EDGE_FALLING	(4U << 4)
#define MCHP_GPIO_CFG_IDET_EDGE_BOTH	(5U << 4)

/* open drain output buffer */
#define MCHP_GPIO_CFG_OPEN_DRAIN_EN_POS	7U
#define MCHP_GPIO_CFG_OPEN_DRAIN_EN	BIT(7)

/* direction is output */
#define MCHP_GPIO_CFG_OUT_POS		8U
#define MCHP_GPIO_CFG_OUT		BIT(8)

/* Select parallel output */
#define MCHP_GPIO_CFG_PAROUT_EN_POS	9U
#define MCHP_GPIO_CFG_PAROUT_EN		BIT(9)

/* function polarity */
#define MCHP_GPIO_CFG_FUNC_POL_INV_POS	10U
#define MCHP_GPIO_CFG_FUNC_POL_INV	BIT(10)

/* Disable input pad. Output still functional */
#define MCHP_GPIO_CFG_INPAD_DIS_POS	11U
#define MCHP_GPIO_CFG_INPAD_DIS		BIT(11)

/* function */
#define MCHP_GPIO_CFG_FUNC_POS		12U
#define MCHP_GPIO_CFG_FUNC_MASK		(7U << 12)
#define MCHP_GPIO_CFG_FUNC_0		0U
#define MCHP_GPIO_CFG_FUNC_GPIO		(MCHP_GPIO_CFG_FUNC_0)
#define MCHP_GPIO_CFG_FUNC_1		(1U << 12)
#define MCHP_GPIO_CFG_FUNC_2		(2U << 12)
#define MCHP_GPIO_CFG_FUNC_3		(3U << 12)
#define MCHP_GPIO_CFG_FUNC_4		(4U << 12)
#define MCHP_GPIO_CFG_FUNC_5		(5U << 12)
#define MCHP_GPIO_CFG_FUNC_6		(6U << 12)
#define MCHP_GPIO_CFG_FUNC_7		(7U << 12)

/* initial output state */
#define MCHP_GPIO_CFG_OUT_INIT_HI_POS	15U
#define MCHP_GPIO_CFG_OUT_INIT_HI	BIT(15)

/* Fast Speed (slew) */
#define MCHP_GPIO_CFG_SPD_POS		16U
#define MCHP_GPIO_CFG_SPD_FAST		BIT(16)

/* Output drive strength */
#define MCHP_GPIO_CFG_DRVS_POS		17U
#define MCHP_GPIO_CFG_DRVS_MASK		(3U << 17)
#define MCHP_GPIO_CFG_DRVS_2MA		0U
#define MCHP_GPIO_CFG_DRVS_4MA		(1U << 17)
#define MCHP_GPIO_CFG_DRVS_8MA		(2U << 17)
#define MCHP_GPIO_CFG_DRVS_12MA		(3U << 17)

int soc_gpio_input(uint32_t pin_id, uint32_t *pin_state);
int soc_gpio_output(uint32_t pin_id, uint32_t value);
int soc_gpio_inport(uint32_t gpio_port, uint32_t *value);
int soc_gpio_outport(uint32_t gpio_port, uint32_t *value);
int soc_gpio_outport_set(uint32_t gpio_port, uint32_t value);
int soc_gpio_output_en(uint32_t pin_id, uint32_t enable);
int soc_gpio_func(uint32_t pin_id, uint32_t *func);
int soc_gpio_func_set(uint32_t pin_id, uint32_t func);
int soc_gpio_config(uint32_t pin_id, uint32_t cfg);

#endif /* _MICROCHIP_MEC_SOC_GPIO_H_ */
