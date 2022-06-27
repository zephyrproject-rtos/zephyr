/*
 * Copyright (c) 2018 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _STM32_I2S_H_
#define _STM32_I2S_H_

#ifdef CONFIG_I2S_STM32_USE_PLLI2S_ENABLE

#if defined(RCC_CFGR_I2SSRC)
/* single selector for the I2S clock source (SEL_1 == SEL_2) */
#define CLK_SEL_1	LL_RCC_I2S1_CLKSOURCE_PLLI2S
#define CLK_SEL_2	LL_RCC_I2S1_CLKSOURCE_PLLI2S
#else
#if defined(RCC_DCKCFGR_I2SSRC)
/* single selector for the I2S clock source (SEL_1 == SEL_2) */
#define CLK_SEL_1	LL_RCC_I2S1_CLKSOURCE_PLL
#define CLK_SEL_2	LL_RCC_I2S1_CLKSOURCE_PLL
#else
#if defined(RCC_DCKCFGR_I2S1SRC) && defined(RCC_DCKCFGR_I2S2SRC)
/* double selector for the I2S clock source (SEL_1 != SEL_2) */
#define CLK_SEL_1	LL_RCC_I2S1_CLKSOURCE_PLLI2S
#define CLK_SEL_2	LL_RCC_I2S2_CLKSOURCE_PLLI2S
#endif /* RCC_DCKCFGR_I2S1SRC && RCC_DCKCFGR_I2S2SRC */
#endif /* RCC_DCKCFGR_I2SSRC */
#endif /* RCC_CFGR_I2SSRC */

#else

#if defined(RCC_CFGR_I2SSRC)
/* single selector for the I2S clock source (SEL_1 == SEL_2) */
#define CLK_SEL_1	LL_RCC_I2S1_CLKSOURCE_PIN
#define CLK_SEL_2	LL_RCC_I2S1_CLKSOURCE_PIN
#else
#if defined(RCC_DCKCFGR_I2SSRC)
/* single selector for the I2S clock source (SEL_1 == SEL_2) */
#define CLK_SEL_1	LL_RCC_I2S1_CLKSOURCE_PLLSRC
#define CLK_SEL_2	LL_RCC_I2S1_CLKSOURCE_PLLSRC
#else
#if defined(RCC_DCKCFGR_I2S1SRC) && defined(RCC_DCKCFGR_I2S2SRC)
/* double selector for the I2S clock source (SEL_1 != SEL_2) */
#define CLK_SEL_1	LL_RCC_I2S1_CLKSOURCE_PLLSRC
#define CLK_SEL_2	LL_RCC_I2S2_CLKSOURCE_PLLSRC
#endif /* RCC_DCKCFGR_I2S1SRC && RCC_DCKCFGR_I2S2SRC */
#endif /* RCC_DCKCFGR_I2SSRC */
#endif /* RCC_CFGR_I2SSRC */

#endif /* CONFIG_I2S_STM32_USE_PLLI2S_ENABLE */

struct queue_item {
	void *mem_block;
	size_t size;
};

/* Minimal ring buffer implementation */
struct ring_buf {
	struct queue_item *buf;
	uint16_t len;
	uint16_t head;
	uint16_t tail;
};

/* Device constant configuration parameters */
struct i2s_stm32_cfg {
	SPI_TypeDef *i2s;
	struct stm32_pclken pclken;
	uint32_t i2s_clk_sel;
	const struct pinctrl_dev_config *pcfg;
	void (*irq_config)(const struct device *dev);
};

struct stream {
	int32_t state;
	struct k_sem sem;

	const struct device *dev_dma;
	uint32_t dma_channel;
	struct dma_config dma_cfg;
	uint8_t priority;
	bool src_addr_increment;
	bool dst_addr_increment;
	uint8_t fifo_threshold;

	struct i2s_config cfg;
	struct ring_buf mem_block_queue;
	void *mem_block;
	bool last_block;
	bool master;
	int (*stream_start)(struct stream *, const struct device *dev);
	void (*stream_disable)(struct stream *, const struct device *dev);
	void (*queue_drop)(struct stream *);
};

/* Device run time data */
struct i2s_stm32_data {
	struct stream rx;
	struct stream tx;
};

#endif	/* _STM32_I2S_H_ */
