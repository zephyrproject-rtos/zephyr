/*
 * Copyright (c) 2021 Microchip Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Microchip XEC MCU family I2C port support.
 *
 */

#ifndef _MICROCHIP_MEC_SOC_DMA_H_
#define _MICROCHIP_MEC_SOC_DMA_H_

#define MEC_DMA_CHAN_0		0
#define MEC_DMA_CHAN_1		1
#define MEC_DMA_CHAN_2		2
#define MEC_DMA_CHAN_3		3
#define MEC_DMA_CHAN_4		4
#define MEC_DMA_CHAN_5		5
#define MEC_DMA_CHAN_6		6
#define MEC_DMA_CHAN_7		7
#define MEC_DMA_CHAN_8		8
#define MEC_DMA_CHAN_9		9
#define MEC_DMA_CHAN_10		10
#define MEC_DMA_CHAN_11		11
#define MEC_DMA_CHAN_12		12
#define MEC_DMA_CHAN_13		13
#define MEC_DMA_CHAN_14		14
#define MEC_DMA_CHAN_15		15
#define MEC_DMA_CHAN_MAX	16

#define DMA_FLAG_UNITS_1BYTE	1U
#define DMA_FLAG_UNITS_2BYTES	2U
#define DMA_FLAG_UNITS_4BYTES	4U
#define DMA_FLAG_MEM2DEV	BIT(8)
#define DMA_FLAG_INCR_MEM	BIT(9)
#define DMA_FLAG_INCR_DEV	BIT(10)
#define DMA_FLAG_LOCK_CHAN	BIT(11)

#define DMA_FLAG_UNITS_MASK	0x07U

#define DMA_START_IEN		BIT(0)
#define DMA_START_SWFC		BIT(1) /* software flow control */

int soc_dma_init(void);
int soc_dma_chan_init(uint8_t chan);
int soc_dma_chan_cfg_mem(uint8_t chan, uintptr_t mbase, uint32_t sz);
int soc_dma_chan_cfg_device(uint8_t chan, uint32_t dev_dma_id,
			    uintptr_t dev_addr);
int soc_dma_chan_cfg_unit_size(uint8_t chan, uint8_t units);
int soc_dma_chan_cfg_dir(uint8_t chan, uint8_t mem2dev);
int soc_dma_chan_cfg_addr_incr(uint8_t chan, uint8_t incr_flags);
int soc_dma_chan_cfg_flags(uint8_t chan, uint32_t flags);
int soc_dma_chan_activate(uint8_t chan, int activate);
int mec_dma_start(uint8_t chan, uint32_t start_flags);

#endif /* _MICROCHIP_MEC_SOC_DMA_H_ */
