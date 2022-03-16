/*
 * Copyright (c) 2020 Linaro Ltd.
 * Copyright (c) 2021 Gerson Fernando Budke
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Atmel SAM0 MCU family devicetree helper macros
 */

#ifndef _ATMEL_SAM0_DT_H_
#define _ATMEL_SAM0_DT_H_

/* Helper macro to get MCLK register address for corresponding
 * that has corresponding clock enable bit.
 */
#define MCLK_MASK_DT_INT_REG_ADDR(n) \
	(DT_REG_ADDR(DT_INST_PHANDLE_BY_NAME(n, clocks, mclk)) + \
	 DT_INST_CLOCKS_CELL_BY_NAME(n, mclk, offset))

/* Helper macros for use with ATMEL SAM0 DMAC controller
 * return 0xff as default value if there is no 'dmas' property
 */
#define ATMEL_SAM0_DT_INST_DMA_CELL(n, name, cell)		\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, dmas),		\
		    (DT_INST_DMAS_CELL_BY_NAME(n, name, cell)),	\
		    (0xff))
#define ATMEL_SAM0_DT_INST_DMA_TRIGSRC(n, name) \
	ATMEL_SAM0_DT_INST_DMA_CELL(n, name, trigsrc)
#define ATMEL_SAM0_DT_INST_DMA_CHANNEL(n, name) \
	ATMEL_SAM0_DT_INST_DMA_CELL(n, name, channel)
#define ATMEL_SAM0_DT_INST_DMA_CTLR(n, name)			\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, dmas),		\
		    (DT_INST_DMAS_CTLR_BY_NAME(n, name)),	\
		    (DT_INVALID_NODE))


/* Use to check if a sercom 'n' is enabled for a given 'compat' */
#define ATMEL_SAM0_DT_SERCOM_CHECK(n, compat) \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(sercom##n), compat, okay)

/* Use to check if TCC 'n' is enabled for a given 'compat' */
#define ATMEL_SAM0_DT_TCC_CHECK(n, compat) \
	DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(tcc##n), compat, okay)

/* Common macro for use to set HCLK_FREQ_HZ */
#define ATMEL_SAM0_DT_CPU_CLK_FREQ_HZ \
	DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency)

/* Devicetree related macros to construct pin mux config data */

/* Get PIN associated with pinctrl-0 pin at index 'i' */
#define ATMEL_SAM0_PIN(node_id, i) \
	DT_PHA(DT_PINCTRL_0(node_id, i), atmel_pins, pin)

/* Get PIO register address associated with pinctrl-0 pin at index 'i' */
#define ATMEL_SAM0_PIN_TO_PORT_REG_ADDR(node_id, i) \
	DT_REG_ADDR(DT_PHANDLE(DT_PINCTRL_0(node_id, i), atmel_pins))

/* Get peripheral cfg associated with pinctrl-0 pin at index 'i' */
#define ATMEL_SAM0_PIN_PERIPH(node_id, i) \
	DT_PHA(DT_PINCTRL_0(node_id, i), atmel_pins, peripheral)

/* Helper function for ATMEL_SAM_PIN_FLAGS */
#define ATMEL_SAM0_PIN_FLAG(node_id, i, flag) \
	DT_PROP(DT_PINCTRL_0(node_id, i), flag)

/* Convert DT flags to SoC flags */
#define ATMEL_SAM0_PIN_FLAGS(node_id, i) \
	(ATMEL_SAM0_PIN_FLAG(node_id, i, bias_pull_up) << SOC_PORT_PULLUP_POS | \
	 ATMEL_SAM0_PIN_FLAG(node_id, i, bias_pull_down) << SOC_PORT_PULLUP_POS | \
	 ATMEL_SAM0_PIN_FLAG(node_id, i, input_enable) << SOC_PORT_INPUT_ENABLE_POS | \
	 ATMEL_SAM0_PIN_FLAG(node_id, i, output_enable) << SOC_PORT_OUTPUT_ENABLE_POS | \
	 ATMEL_SAM0_PIN_FLAG(node_id, i, pinmux_enable) << SOC_PORT_PMUXEN_ENABLE_POS)

/* Construct a soc_port_pin element for pin cfg */
#define ATMEL_SAM0_DT_PORT(node_id, idx)					\
	{									\
		(PortGroup *)ATMEL_SAM0_PIN_TO_PORT_REG_ADDR(node_id, idx),	\
		ATMEL_SAM0_PIN(node_id, idx),					\
		ATMEL_SAM0_PIN_PERIPH(node_id, idx) << SOC_PORT_FUNC_POS |	\
		ATMEL_SAM0_PIN_FLAGS(node_id, idx)				\
	}

/* Get the number of pins for pinctrl-0 */
#define ATMEL_SAM0_DT_NUM_PINS(node_id) DT_NUM_PINCTRLS_BY_IDX(node_id, 0)

#define ATMEL_SAM0_DT_INST_NUM_PINS(inst) \
	ATMEL_SAM0_DT_NUM_PINS(DT_DRV_INST(inst))

/* internal macro to structure things for use with UTIL_LISTIFY */
#define ATMEL_SAM0_DT_PIN_ELEM(idx, node_id) ATMEL_SAM0_DT_PORT(node_id, idx)

/* Construct an array initializer for soc_port_pin for a device instance */
#define ATMEL_SAM0_DT_PINS(node_id)				\
	{ LISTIFY(ATMEL_SAM0_DT_NUM_PINS(node_id),		\
		  ATMEL_SAM0_DT_PIN_ELEM, (,), node_id)		\
	}

#define ATMEL_SAM0_DT_INST_PINS(inst) \
	ATMEL_SAM0_DT_PINS(DT_DRV_INST(inst))

#endif /* _ATMEL_SAM0_SOC_DT_H_ */
