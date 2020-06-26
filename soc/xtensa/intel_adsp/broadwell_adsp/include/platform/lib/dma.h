/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright(c) 2016 Intel Corporation. All rights reserved.
 *
 * Author: Liam Girdwood <liam.r.girdwood@linux.intel.com>
 */

#ifdef __SOF_LIB_DMA_H__

#ifndef __PLATFORM_LIB_DMA_H__
#define __PLATFORM_LIB_DMA_H__

#define PLATFORM_NUM_DMACS		2

/* max number of supported DMA channels */
#define PLATFORM_MAX_DMA_CHAN	8

#define DMA_ID_DMAC0			0
#define DMA_ID_DMAC1			1

#define DMA_HANDSHAKE_SSP1_RX		0
#define DMA_HANDSHAKE_SSP1_TX		1
#define DMA_HANDSHAKE_SSP0_RX		2
#define DMA_HANDSHAKE_SSP0_TX		3
#define DMA_HANDSHAKE_OBFF_0		4
#define DMA_HANDSHAKE_OBFF_1		5
#define DMA_HANDSHAKE_OBFF_2		6
#define DMA_HANDSHAKE_OBFF_3		7
#define DMA_HANDSHAKE_OBFF_4		8
#define DMA_HANDSHAKE_OBFF_5		9
#define DMA_HANDSHAKE_OBFF_6		10
#define DMA_HANDSHAKE_OBFF_7		11
#define DMA_HANDSHAKE_OBFF_8		12
#define DMA_HANDSHAKE_OBFF_9		13
#define DMA_HANDSHAKE_OBFF_10		14
#define DMA_HANDSHAKE_OBFF_11		15

#define dma_chan_irq(dma, chan) dma_irq(dma)
#define dma_chan_irq_name(dma, chan) dma_irq_name(dma)

#endif /* __PLATFORM_LIB_DMA_H__ */

#else

#error "This file shouldn't be included from outside of sof/lib/dma.h"

#endif /* __SOF_LIB_DMA_H__ */
