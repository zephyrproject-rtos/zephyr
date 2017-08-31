#ifndef _RDA_DMA_H_
#define _RDA_DMA_H_

#include "soc_registers.h"

#define RDA_HW_ACCELERATE_ENABLE   1

typedef enum _dma_mode {
	NORMAL_MODE  = 0,
	AES_ENC_MODE = 1,
	TX_CRC_MODE  = 2,
	AES_DEC_MODE = 5,
	RX_CRC_MODE  = 6
} dma_mode;

typedef enum _dma_int_out_dscp {
	AHB_DMA_DONE = (1<<0),
	PRNG_ALERT = (1<<1),
	TRNG_ON_FLY_TEST_FAIL = (1<<2),
	TRNG_START_TEST_FAIL = (1<<3),
	TRNG_DATA_READY = (1<<4),
	CIOS_DONE = (1<<5)
} dma_int_out_dscp;

typedef enum _dma_ctrl_hsizem {
	DMA_CTL_HSM_BYTE,    /*!< DMA ctrl hsizem 8 bits */
	DMA_CTL_HSM_2BYTES,  /*!< DMA ctrl hsizem 16 bits */
	DMA_CTL_HSM_4BYTES   /*!< DMA ctrl hsizem 32 bits */
} dma_ctrl_hsizem;

typedef enum _dma_ctrl_fix_src {
	DMA_CTL_SRC_ADDR_INC, /*!< DMA ctrl fix src off */
	DMA_CTL_FIX_SRC_ADDR  /*!< DMA ctrl fix src on */
} dma_ctrl_fix_src;

typedef enum _dma_ctrl_fix_dst {
	DMA_CTL_DST_ADDR_INC, /*!< DMA ctrl fix dst off */
	DMA_CTL_FIX_DST_ADDR  /*!< DMA ctrl fix dst on */
} dma_ctrl_fix_dst;

#endif /* _RDA_DMA_H_ */
