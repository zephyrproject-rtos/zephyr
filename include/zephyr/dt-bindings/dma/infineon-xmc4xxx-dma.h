/*
 * Copyright (c) 2022 Andriy Gelman <andriy.gelman@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_DMA_INFINEON_XMC4XXX_DMA_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_DMA_INFINEON_XMC4XXX_DMA_H_

#define XMC4XXX_DMA_REQUEST_SOURCE_POS	0
#define XMC4XXX_DMA_REQUEST_SOURCE_MASK 0xf

#define XMC4XXX_DMA_LINE_POS  4
#define XMC4XXX_DMA_LINE_MASK 0xf

#define XMC4XXX_DMA_GET_REQUEST_SOURCE(mx)                                                         \
	((mx >> XMC4XXX_DMA_REQUEST_SOURCE_POS) & XMC4XXX_DMA_REQUEST_SOURCE_MASK)

#define XMC4XXX_DMA_GET_LINE(mx) ((mx >> XMC4XXX_DMA_LINE_POS) & XMC4XXX_DMA_LINE_MASK)

#define XMC4XXX_SET_CONFIG(line, rs)                                                               \
	((line) << XMC4XXX_DMA_LINE_POS | (rs) << XMC4XXX_DMA_REQUEST_SOURCE_POS)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_DMA_INFINEON_XMC4XXX_DMA_H_ */
