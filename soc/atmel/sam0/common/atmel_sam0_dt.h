/*
 * Copyright (c) 2020 Linaro Ltd.
 * Copyright (c) 2021-2024 Gerson Fernando Budke <nandojve@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Atmel SAM0 MCU family devicetree helper macros
 */

/* clang-format off */

#ifndef _ATMEL_SAM0_DT_H_
#define _ATMEL_SAM0_DT_H_

#define ATMEL_SAM0_DT_INST_CELL_REG_ADDR_OFFSET(n, cell)		\
	(volatile uint32_t *)						\
	(DT_REG_ADDR(DT_INST_PHANDLE_BY_NAME(n, clocks, cell)) +	\
	 DT_INST_CLOCKS_CELL_BY_NAME(n, cell, offset))

#define ATMEL_SAM0_DT_INST_CELL_PERIPH_MASK(n, name, cell)		\
	BIT(DT_INST_CLOCKS_CELL_BY_NAME(n, name, cell))

/* Helper macro to get register address that control peripheral clock
 * enable bit.
 */
#define ATMEL_SAM0_DT_INST_MCLK_PM_REG_ADDR_OFFSET(n)			\
	COND_CODE_1(DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(mclk)),	\
		(ATMEL_SAM0_DT_INST_CELL_REG_ADDR_OFFSET(n, mclk)),	\
		(ATMEL_SAM0_DT_INST_CELL_REG_ADDR_OFFSET(n, pm)))

/* Helper macro to get peripheral clock bit mask.
 */
#define ATMEL_SAM0_DT_INST_MCLK_PM_PERIPH_MASK(n, cell)			\
	COND_CODE_1(DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(mclk)),	\
		(ATMEL_SAM0_DT_INST_CELL_PERIPH_MASK(n, mclk, cell)),	\
		(ATMEL_SAM0_DT_INST_CELL_PERIPH_MASK(n, pm, cell)))

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

#endif /* _ATMEL_SAM0_SOC_DT_H_ */

/* clang-format on */
