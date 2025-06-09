/*
 * Copyright (c) 2024 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief TI CC23X0 MCU family devicetree helper macros
 */

#ifndef _TI_CC23X0_DT_H_
#define _TI_CC23X0_DT_H_

/* Common macro to get CPU clock frequency */
#define TI_CC23X0_DT_CPU_CLK_FREQ_HZ \
	DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency)

/*
 * Helper macros for use with TI CC23X0 DMA controller
 * return 0xff as default value if there is no 'dmas' property
 */
#define TI_CC23X0_DT_INST_DMA_CELL(n, name, cell)		\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, dmas),		\
		    (DT_INST_DMAS_CELL_BY_NAME(n, name, cell)),	\
		    (0xff))

#define TI_CC23X0_DT_INST_DMA_TRIGSRC(n, name) \
	TI_CC23X0_DT_INST_DMA_CELL(n, name, trigsrc)

#define TI_CC23X0_DT_INST_DMA_CHANNEL(n, name) \
	TI_CC23X0_DT_INST_DMA_CELL(n, name, channel)

#define TI_CC23X0_DT_INST_DMA_CTLR(n, name)			\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, dmas),		\
		    (DT_INST_DMAS_CTLR_BY_NAME(n, name)),	\
		    (DT_INVALID_NODE))

#endif /* _TI_CC23X0_DT_H_ */
