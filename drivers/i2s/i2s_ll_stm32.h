/*
 * Copyright (c) 2018 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _STM32_I2S_H_
#define _STM32_I2S_H_

struct queue_item {
	void *mem_block;
	size_t size;
};

/* Device constant configuration parameters */
struct i2s_stm32_cfg {
	SPI_TypeDef *i2s;
	const struct stm32_pclken *pclken;
	size_t pclk_len;
	const struct pinctrl_dev_config *pcfg;
	void (*irq_config)(const struct device *dev);
	bool master_clk_sel;
};

struct stream {
	int32_t state;
	struct k_msgq *msgq;

	const struct device *dev_dma;
	uint32_t dma_channel;
	struct dma_config dma_cfg;
	uint8_t priority;
	bool src_addr_increment;
	bool dst_addr_increment;
	uint8_t fifo_threshold;
	bool tx_stop_for_drain;

	struct i2s_config cfg;
	void *mem_block;
	bool last_block;
	bool master;
	int (*stream_start)(struct stream *, const struct device *dev);
	void (*stream_disable)(struct stream *, const struct device *dev);
};

/* Device run time data */
struct i2s_stm32_data {
	struct stream rx;
	struct stream tx;
};

/* checks that DMA Tx packet is fully transmitted over the I2S */
static inline uint32_t ll_func_i2s_dma_busy(SPI_TypeDef *i2s)
{
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_i2s)
	return LL_SPI_IsActiveFlag_TXC(i2s) == 0;
#else
	/* the I2S Tx empty and busy flags are needed */
	return (LL_SPI_IsActiveFlag_TXE(i2s) &&
		!LL_SPI_IsActiveFlag_BSY(i2s));
#endif
}

#endif	/* _STM32_I2S_H_ */
