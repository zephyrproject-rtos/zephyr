/*
 * Copyright (c) 2016 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <misc/util.h>
#include <kernel.h>
#include <board.h>
#include <errno.h>
#include <spi.h>

#include <clock_control/stm32_clock_control.h>
#include <clock_control.h>

#include <drivers/spi/spi_ll_stm32.h>
#include <spi_ll_stm32.h>

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_SPI_LEVEL
#include <logging/sys_log.h>

#define DEV_CFG(dev)  							\
((const struct spi_stm32_config * const )(dev)->config->config_info)

#define DEV_DATA(dev) 							\
((struct spi_stm32_data * const)(dev)->driver_data)

#ifdef CONFIG_SPI_STM32_INTERRUPT
static void spi_stm32_isr(void *arg)
{
	struct device * const dev = (struct device *) arg;
	const struct spi_stm32_config *cfg = DEV_CFG(dev);
	struct spi_stm32_data *data = DEV_DATA(dev);
	SPI_TypeDef *spi = cfg->spi;

	if (LL_SPI_IsActiveFlag_TXE(spi)) {
		data->tx.process(spi, &data->tx);
	}

	if (LL_SPI_IsActiveFlag_RXNE(spi)) {
		data->rx.process(spi, &data->rx);
	}

	if (!data->rx.len && !data->tx.len) {
		LL_SPI_DisableIT_RXNE(spi);
		LL_SPI_DisableIT_TXE(spi);
		k_sem_give(&data->sync);
	}
}
#endif

static int spi_stm32_configure(struct device *dev, struct spi_config *config)
{
	const struct spi_stm32_config *cfg = DEV_CFG(dev);
	struct spi_stm32_data *data = DEV_DATA(dev);
	const u32_t scaler[] = {
		LL_SPI_BAUDRATEPRESCALER_DIV2,
		LL_SPI_BAUDRATEPRESCALER_DIV4,
		LL_SPI_BAUDRATEPRESCALER_DIV8,
		LL_SPI_BAUDRATEPRESCALER_DIV16,
		LL_SPI_BAUDRATEPRESCALER_DIV32,
		LL_SPI_BAUDRATEPRESCALER_DIV64,
		LL_SPI_BAUDRATEPRESCALER_DIV128,
		LL_SPI_BAUDRATEPRESCALER_DIV256
	};
	u32_t flags = config->config;
	SPI_TypeDef *spi = cfg->spi;
	u32_t clock;
	int br;

	if (SPI_WORD_SIZE_GET(flags) != 8) {
		return -ENOTSUP;
	}

	clock_control_get_rate(device_get_binding(STM32_CLOCK_CONTROL_NAME),
			       (clock_control_subsys_t) &cfg->pclken, &clock);

	for (br = 1 ; br <= ARRAY_SIZE(scaler) ; ++br) {
		u32_t clk = clock >> br;
		if (clk < config->max_sys_freq) {
			break;
		}
	}

	if (br > ARRAY_SIZE(scaler)) {
		return -ENOTSUP;
	}

	LL_SPI_Disable(spi);

	LL_SPI_SetBaudRatePrescaler(spi, scaler[br - 1]);

	if (flags & SPI_MODE_CPOL) {
		LL_SPI_SetClockPolarity(spi, LL_SPI_POLARITY_HIGH);
	} else {
		LL_SPI_SetClockPolarity(spi, LL_SPI_POLARITY_LOW);
	}

	if (flags & SPI_MODE_CPHA) {
		LL_SPI_SetClockPhase(spi, LL_SPI_PHASE_2EDGE);
	} else {
		LL_SPI_SetClockPhase(spi, LL_SPI_PHASE_1EDGE);
	}

	LL_SPI_SetTransferDirection(spi, LL_SPI_FULL_DUPLEX);


	if (flags & SPI_TRANSFER_MASK) {
		LL_SPI_SetTransferBitOrder(spi, LL_SPI_LSB_FIRST);
	} else {
		LL_SPI_SetTransferBitOrder(spi, LL_SPI_MSB_FIRST);
	}

	LL_SPI_DisableCRC(spi);

	if (flags & STM32_SPI_MODE_SLAVE) {
		LL_SPI_SetMode(spi, LL_SPI_MODE_SLAVE);
	} else {
		LL_SPI_SetMode(spi, LL_SPI_MODE_MASTER);
	}

	if (flags & STM32_SPI_NSS_IGNORE) {
		LL_SPI_SetNSSMode(spi, LL_SPI_NSS_SOFT);
	} else if (flags & STM32_SPI_MODE_SLAVE) {
		LL_SPI_SetNSSMode(spi, LL_SPI_NSS_HARD_OUTPUT);
	} else {
		LL_SPI_SetNSSMode(spi, LL_SPI_NSS_HARD_INPUT);
	}

	LL_SPI_SetDataWidth(spi, LL_SPI_DATAWIDTH_8BIT);

#if defined(CONFIG_SOC_SERIES_STM32L4X) || defined(CONFIG_SOC_SERIES_STM32F3X)
	LL_SPI_SetRxFIFOThreshold(spi, LL_SPI_RX_FIFO_TH_QUARTER);
#endif

	LL_SPI_SetStandard(spi, LL_SPI_PROTOCOL_MOTOROLA);

	data->rx.len = 0;
	data->rx.buf = NULL;
	data->tx.len = 0;
	data->tx.buf = NULL;

	return 0;
}

static void spi_stm32_transmit(SPI_TypeDef *spi, struct spi_stm32_buffer_tx *p)
{
	if (!p->len) {
		return;
	}

	LL_SPI_TransmitData8(spi, *p->buf);

	p->buf++;
	p->len--;
}

static void spi_stm32_receive(SPI_TypeDef *spi, struct spi_stm32_buffer_rx *p)
{
	if (!p->len) {
		return;
	}

	*p->buf = LL_SPI_ReceiveData8(spi);

	p->buf++;
	p->len--;
}

static int spi_stm32_transceive(struct device *dev, const void *tx_buf,
			u32_t tx_buf_len,void *rx_buf, u32_t rx_buf_len)
{
	const struct spi_stm32_config *cfg = DEV_CFG(dev);
	struct spi_stm32_data *data = DEV_DATA(dev);
	SPI_TypeDef *spi = cfg->spi;

	__ASSERT(!(rx.buf_len && (rx.buf == NULL)),
		 "spi_stm32_transceive: ERROR - rx NULL buffer");

	__ASSERT(!(tx.buf_len && (tx.buf == NULL)),
		 "spi_stm32_transceive: ERROR - tx NULL buffer");

	data->rx.process = spi_stm32_receive;
	data->rx.len = rx_buf_len;
	data->rx.buf = rx_buf;

	data->tx.process = spi_stm32_transmit;
	data->tx.len = tx_buf_len;
	data->tx.buf = tx_buf;

	LL_SPI_Enable(spi);

#ifdef CONFIG_SPI_STM32_INTERRUPT
	if (rx_buf_len) {
		LL_SPI_EnableIT_RXNE(spi);
	}

	if (tx_buf_len) {
		LL_SPI_EnableIT_TXE(spi);
	}

	k_sem_take(&data->sync, K_FOREVER);
#else
	do {
		if (LL_SPI_IsActiveFlag_TXE(spi)) {
			data->tx.process(spi, &data->tx);
		}

		if (LL_SPI_IsActiveFlag_RXNE(spi) && data->rx.len) {
			data->rx.process(spi, &data->rx);
		}
	} while (data->tx.len || data->rx.len);
#endif

#if defined(CONFIG_SOC_SERIES_STM32L4X) || defined(CONFIG_SOC_SERIES_STM32F3X)
	while (LL_SPI_GetTxFIFOLevel(spi) != LL_SPI_TX_FIFO_EMPTY) {
		(void) LL_SPI_ReceiveData8(spi);
	}
#endif
	if (LL_SPI_GetMode(spi) == LL_SPI_MODE_MASTER) {
		while (LL_SPI_IsActiveFlag_BSY(spi)) {
			/* NOP */
		}
		LL_SPI_Disable(spi);
	}

	return 0;
}

static const struct spi_driver_api api_funcs = {
	.transceive = spi_stm32_transceive,
	.configure = spi_stm32_configure,
};

static int spi_stm32_init(struct device *dev)
{
	struct spi_stm32_data *data __attribute__((unused)) = DEV_DATA(dev);
	const struct spi_stm32_config *cfg = DEV_CFG(dev);

	__ASSERT_NO_MSG(device_get_binding(STM32_CLOCK_CONTROL_NAME));


	clock_control_on(device_get_binding(STM32_CLOCK_CONTROL_NAME),
			       (clock_control_subsys_t) &cfg->pclken);


#ifdef CONFIG_SPI_STM32_INTERRUPT
	k_sem_init(&data->sync, 0, UINT_MAX);

	cfg->irq_config(dev);
#endif

	return 0;
}

#ifdef CONFIG_SPI_1

#ifdef CONFIG_SPI_STM32_INTERRUPT
static void spi_stm32_irq_config_func_1(struct device *port);
#endif

static const struct spi_stm32_config spi_stm32_cfg_1 = {
	.spi = (SPI_TypeDef *) SPI1_BASE,
	.pclken = {
		.enr = LL_APB2_GRP1_PERIPH_SPI1,
		.bus = STM32_CLOCK_BUS_APB2
	},
#ifdef CONFIG_SPI_STM32_INTERRUPT
	.irq_config = spi_stm32_irq_config_func_1,
#endif
};

static struct spi_stm32_data spi_stm32_dev_data_1;

DEVICE_AND_API_INIT(spi_stm32_1, CONFIG_SPI_1_NAME, &spi_stm32_init,
		    &spi_stm32_dev_data_1, &spi_stm32_cfg_1,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &api_funcs);

#ifdef CONFIG_SPI_STM32_INTERRUPT
static void spi_stm32_irq_config_func_1(struct device *dev)
{
	IRQ_CONNECT(SPI1_IRQn, CONFIG_SPI_1_IRQ_PRI,
		    spi_stm32_isr, DEVICE_GET(spi_stm32_1), 0);
	irq_enable(SPI1_IRQn);
}
#endif

#endif /* CONFIG_SPI_1 */

#ifdef CONFIG_SPI_2

#ifdef CONFIG_SPI_STM32_INTERRUPT
static void spi_stm32_irq_config_func_2(struct device *port);
#endif

static const struct spi_stm32_config spi_stm32_cfg_2 = {
	.spi = (SPI_TypeDef *) SPI2_BASE,
	.pclken = {
		.enr = LL_APB1_GRP1_PERIPH_SPI2,
		.bus = STM32_CLOCK_BUS_APB1
	},
#ifdef CONFIG_SPI_STM32_INTERRUPT
	.irq_config = spi_stm32_irq_config_func_2,
#endif
};

static struct spi_stm32_data spi_stm32_dev_data_2;

DEVICE_AND_API_INIT(spi_stm32_2, CONFIG_SPI_2_NAME, &spi_stm32_init,
		    &spi_stm32_dev_data_2, &spi_stm32_cfg_2,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &api_funcs);

#ifdef CONFIG_SPI_STM32_INTERRUPT
static void spi_stm32_irq_config_func_2(struct device *dev)
{
	IRQ_CONNECT(SPI2_IRQn, CONFIG_SPI_2_IRQ_PRI,
		    spi_stm32_isr, DEVICE_GET(spi_stm32_2), 0);
	irq_enable(SPI2_IRQn);
}
#endif

#endif /* CONFIG_SPI_2 */

#ifdef CONFIG_SPI_3

#ifdef CONFIG_SPI_STM32_INTERRUPT
static void spi_stm32_irq_config_func_3(struct device *port);
#endif

static const  struct spi_stm32_config spi_stm32_cfg_3 = {
	.spi = (SPI_TypeDef *) SPI3_BASE,
	.pclken = {
		.enr = LL_APB1_GRP1_PERIPH_SPI3,
		.bus = STM32_CLOCK_BUS_APB1
	},
#ifdef CONFIG_SPI_STM32_INTERRUPT
	.irq_config = spi_stm32_irq_config_func_3,
#endif
};

static struct spi_stm32_data spi_stm32_dev_data_3;

DEVICE_AND_API_INIT(spi_stm32_3, CONFIG_SPI_3_NAME, &spi_stm32_init,
		    &spi_stm32_dev_data_3, &spi_stm32_cfg_3,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &api_funcs);

#ifdef CONFIG_SPI_STM32_INTERRUPT
static void spi_stm32_irq_config_func_3(struct device *dev)
{
	IRQ_CONNECT(SPI3_IRQn, CONFIG_SPI_3_IRQ_PRI,
		    spi_stm32_isr, DEVICE_GET(spi_stm32_3), 0);
	irq_enable(SPI3_IRQn);
}
#endif

#endif /* CONFIG_SPI_3 */
