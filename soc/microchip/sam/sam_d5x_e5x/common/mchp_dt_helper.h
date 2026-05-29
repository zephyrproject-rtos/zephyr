/*
 * Copyright (c) 2025 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file mchp_dt_helper.h
 * @brief device tree helper macros.
 *
 * This file contains the device tree helper macros.
 */

#ifndef SOC_MICROCHIP_SAM_D5X_E5X_COMMON_MCHP_DT_HELPER_H_

/* Helper macros for use with DMAC controller
 * return 0xff as default value if there is no 'dmas' property
 */
/* clang-format off */
#define MCHP_DT_INST_DMA_CELL(n, name, cell)				\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, dmas),			\
		    (DT_INST_DMAS_CELL_BY_NAME(n, name, cell)),		\
		    (0xff))

#define MCHP_DT_INST_DMA_TRIGSRC(n, name)	MCHP_DT_INST_DMA_CELL(n, name, trigsrc)

#define MCHP_DT_INST_DMA_CHANNEL(n, name)	MCHP_DT_INST_DMA_CELL(n, name, channel)

#define MCHP_DT_INST_DMA_CTLR(n, name)					\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, dmas),			\
		    (DT_INST_DMAS_CTLR_BY_NAME(n, name)),		\
		    (DT_INVALID_NODE))
/* clang-format on */

#endif /* SOC_MICROCHIP_SAM_D5X_E5X_COMMON_MCHP_DT_HELPER_H_ */
