/*
 * Copyright (c) 2016 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <kernel.h>

#include <board.h>
#include <spi.h>
#include <clock_control.h>

#include <misc/util.h>

#include <clock_control/stm32_clock_control.h>
#include "spi_stm32lx.h"
#include <drivers/spi/spi_stm32lx.h>

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_SPI_LEVEL
#include <logging/sys_log.h>

/* convenience defines */
#define DEV_CFG(dev)							\
	((const struct spi_stm32lx_config * const)(dev)->config->config_info)
#define DEV_DATA(dev)							\
	((struct spi_stm32lx_data * const)(dev)->driver_data)
#define SPI_STRUCT(dev)							\
	((volatile struct spi_stm32lx *)(DEV_CFG(dev))->base)


#ifdef CONFIG_SPI_STM32LX_INTERRUPT
static void spi_stm32lx_isr(void *arg)
{
	struct device * const dev = (struct device *)arg;

	volatile struct spi_stm32lx *spi = SPI_STRUCT(dev);
	struct spi_stm32lx_data *data = DEV_DATA(dev);
	volatile u8_t *dr8 = &spi->dr.dr_as_u8;

	if (spi->sr.bit.txe && data->current.tx_len) {
		*dr8 = *data->current.tx_buf;
		data->current.tx_buf++;
		data->current.tx_len--;
	}

	if (spi->sr.bit.rxne && data->current.rx_len) {
		*data->current.rx_buf = *dr8;
		data->current.rx_buf++;
		data->current.rx_len--;
	}

	if (!data->current.rx_len && !data->current.tx_len) {
		spi->cr2.bit.rxneie = 0;
		spi->cr2.bit.txeie = 0;

		k_sem_give(&data->device_sync_sem);
	}
}
#endif

#define BR_SCALER_COUNT		8
static const u32_t baud_rate_scaler[] = {
	2, 4, 8, 16, 32, 64, 128, 256
};

/* Only support 8bit data size */
#define DATA_SIZE_8_CFG		(3 + 4)

static int spi_stm32lx_configure(struct device *dev, struct spi_config *config)
{
	volatile struct spi_stm32lx *spi = SPI_STRUCT(dev);
	struct spi_stm32lx_data *data = DEV_DATA(dev);
	const struct spi_stm32lx_config *cfg = DEV_CFG(dev);
	u32_t flags = config->config;
	u32_t clock;
	int br;

	clock_control_get_rate(data->clock,
			(clock_control_subsys_t *)&cfg->pclken, &clock);

	if (SPI_WORD_SIZE_GET(flags) != 8) {
		return -ENOTSUP;
	}

	for (br = 0 ; br < BR_SCALER_COUNT ; ++br) {
		u32_t clk = clock / baud_rate_scaler[br];

		if (clk < config->max_sys_freq) {
			break;
		}
	}

	if (br >= BR_SCALER_COUNT) {
		return -ENOTSUP;
	}

	/* Disable Peripheral */
	spi->cr1.bit.spe = 0;

	/* Setup baud rate prescaler */
	spi->cr1.bit.br = br;

	/* Setup polarity flags */
	if (flags & SPI_MODE_CPOL) {
		spi->cr1.bit.cpol = 1;
	} else {
		spi->cr1.bit.cpol = 0;
	}

	if (flags & SPI_MODE_CPHA) {
		spi->cr1.bit.cpha = 1;
	} else {
		spi->cr1.bit.cpha = 0;
	}

	/* Full Duplex 2-lines */
	spi->cr1.bit.rxonly = 0;
	spi->cr1.bit.bidimode = 0;

	/* Setup transfer bit mode */
	if (flags & SPI_TRANSFER_MASK) {
		spi->cr1.bit.lsbfirst = 1;
	} else {
		spi->cr1.bit.lsbfirst = 0;
	}

	/* Disable CRC Feature */
	spi->cr1.bit.crcen = 0;

	/* Slave Support */
	if (flags & STM32LX_SPI_SLAVE_MODE) {
		spi->cr1.bit.mstr = 0;

		/* NSS Management */
		if (flags & STM32LX_SPI_SLAVE_NSS_IGNORE) {
			spi->cr1.bit.ssm = 1;
			spi->cr1.bit.ssi = 0;
		} else {
			spi->cr1.bit.ssm = 0;
		}

		data->current.is_slave = 1;
	} else {
		spi->cr1.bit.mstr = 1;
		spi->cr1.bit.ssm = 0;

		/* NSS Management */
		if (flags & STM32LX_SPI_MASTER_NSS_IGNORE) {
			spi->cr2.bit.ssoe = 0;
		} else {
			spi->cr2.bit.ssoe = 1;
			spi->cr2.bit.nssp = 1;
		}

		data->current.is_slave = 0;
	}

	/* Setup Data size */
	spi->cr2.bit.ds = DATA_SIZE_8_CFG;
	spi->cr2.bit.frxth = 1;

	/* Motorola Format */
	spi->cr2.bit.frf = 0;

	data->current.rx_len = 0;
	data->current.rx_buf = NULL;
	data->current.tx_len = 0;
	data->current.tx_buf = NULL;

	return 0;
}

static int spi_stm32lx_slave_select(struct device *dev, u32_t slave)
{
	/* NOP */

	return 0;
}

static int spi_stm32lx_transceive(struct device *dev,
				  const void *tx_buf, u32_t tx_buf_len,
				  void *rx_buf, u32_t rx_buf_len)
{
	volatile struct spi_stm32lx *spi = SPI_STRUCT(dev);
	struct spi_stm32lx_data *data = DEV_DATA(dev);
	volatile u8_t *dr8 = (volatile u8_t *)&spi->dr.dr_as_u8;

	__ASSERT(!((tx_buf_len && (tx_buf == NULL)) ||
		  (rx_buf_len && (rx_buf == NULL))),
		 "spi_stm32lx_transceive: ERROR - NULL buffer");

	data->current.rx_len = rx_buf_len;
	data->current.rx_buf = rx_buf;
	data->current.tx_len = tx_buf_len;
	data->current.tx_buf = tx_buf;

	/* Enable Peripheral */
	spi->cr1.bit.spe = 1;

#ifdef CONFIG_SPI_STM32LX_INTERRUPT
	if (rx_buf_len) {
		spi->cr2.bit.rxneie = 1;
	}

	if (tx_buf_len) {
		spi->cr2.bit.txeie = 1;
	}

	k_sem_take(&data->device_sync_sem, K_FOREVER);
#else
	do {
		if (spi->sr.bit.txe && data->current.tx_len) {
			*dr8 = *data->current.tx_buf;
			data->current.tx_buf++;
			data->current.tx_len--;
		}

		if (spi->sr.bit.rxne && data->current.rx_len) {
			*data->current.rx_buf = *dr8;
			data->current.rx_buf++;
			data->current.rx_len--;
		}

	} while (data->current.tx_len || data->current.rx_len);
#endif

	/* Empty the RX FIFO */
	while (spi->sr.bit.frlvl) {
		(void)*dr8;
	}

	if (!data->current.is_slave) {
		while (spi->sr.bit.bsy) {
			/* NOP */
		}

		/* Disable Peripheral */
		spi->cr1.bit.spe = 0;
	}

	return 0;
}

static const struct spi_driver_api api_funcs = {
	.configure = spi_stm32lx_configure,
	.slave_select = spi_stm32lx_slave_select,
	.transceive = spi_stm32lx_transceive,
};

static inline void spi_stm32lx_get_clock(struct device *dev)
{
	struct spi_stm32lx_data *data = DEV_DATA(dev);
	struct device *clk =
		device_get_binding(STM32_CLOCK_CONTROL_NAME);

	__ASSERT_NO_MSG(clk);

	data->clock = clk;
}

static int spi_stm32lx_init(struct device *dev)
{
	volatile struct spi_stm32lx *spi = SPI_STRUCT(dev);
	struct spi_stm32lx_data *data = DEV_DATA(dev);
	const struct spi_stm32lx_config *cfg = DEV_CFG(dev);

	k_sem_init(&data->device_sync_sem, 0, UINT_MAX);

	spi_stm32lx_get_clock(dev);

	/* enable clock */
	clock_control_on(data->clock,
		(clock_control_subsys_t *)&cfg->pclken);

	/* Reset config */
	spi->cr1.val = 0;
	spi->cr2.val = 0;
	spi->sr.val = 0;

#ifdef CONFIG_SPI_STM32LX_INTERRUPT
	cfg->irq_config_func(dev);
#endif

	return 0;
}

#ifdef CONFIG_SPI_1

#ifdef CONFIG_SPI_STM32LX_INTERRUPT
static void spi_stm32lx_irq_config_func_1(struct device *port);
#endif

static const struct spi_stm32lx_config spi_stm32lx_cfg_1 = {
	.base = (u8_t *)SPI1_BASE,
	.pclken = { .bus = STM32_CLOCK_BUS_APB2,
		    .enr = LL_APB2_GRP1_PERIPH_SPI1 },
#ifdef CONFIG_SPI_STM32LX_INTERRUPT
	.irq_config_func = spi_stm32lx_irq_config_func_1,
#endif
};

static struct spi_stm32lx_data spi_stm32lx_dev_data_1 = {
};

DEVICE_AND_API_INIT(spi_stm32lx_1, CONFIG_SPI_1_NAME, &spi_stm32lx_init,
		    &spi_stm32lx_dev_data_1, &spi_stm32lx_cfg_1,
		    PRE_KERNEL_2, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &api_funcs);

#ifdef CONFIG_SPI_STM32LX_INTERRUPT
static void spi_stm32lx_irq_config_func_1(struct device *dev)
{
#ifdef CONFIG_SOC_SERIES_STM32L4X
#define PORT_1_SPI_IRQ STM32L4_IRQ_SPI1
#endif
	IRQ_CONNECT(PORT_1_SPI_IRQ, CONFIG_SPI_1_IRQ_PRI,
		spi_stm32lx_isr, DEVICE_GET(spi_stm32lx_1), 0);
	irq_enable(PORT_1_SPI_IRQ);
}
#endif

#endif /* CONFIG_SPI_1 */

#ifdef CONFIG_SPI_2

#ifdef CONFIG_SPI_STM32LX_INTERRUPT
static void spi_stm32lx_irq_config_func_2(struct device *port);
#endif

static const struct spi_stm32lx_config spi_stm32lx_cfg_2 = {
	.base = (u8_t *)SPI2_BASE,
	.pclken = { .bus = STM32_CLOCK_BUS_APB1,
		    .enr = LL_APB1_GRP1_PERIPH_SPI2 },
#ifdef CONFIG_SPI_STM32LX_INTERRUPT
	.irq_config_func = spi_stm32lx_irq_config_func_2,
#endif
};

static struct spi_stm32lx_data spi_stm32lx_dev_data_2 = {
};

DEVICE_AND_API_INIT(spi_stm32lx_2, CONFIG_SPI_2_NAME, &spi_stm32lx_init,
		    &spi_stm32lx_dev_data_2, &spi_stm32lx_cfg_2,
		    PRE_KERNEL_2, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &api_funcs);

#ifdef CONFIG_SPI_STM32LX_INTERRUPT
static void spi_stm32lx_irq_config_func_2(struct device *dev)
{
#ifdef CONFIG_SOC_SERIES_STM32L4X
#define PORT_2_SPI_IRQ STM32L4_IRQ_SPI2
#endif
	IRQ_CONNECT(PORT_2_SPI_IRQ, CONFIG_SPI_2_IRQ_PRI,
		spi_stm32lx_isr, DEVICE_GET(spi_stm32lx_2), 0);
	irq_enable(PORT_2_SPI_IRQ);
}
#endif

#endif /* CONFIG_SPI_2 */

#ifdef CONFIG_SPI_3

#ifdef CONFIG_SPI_STM32LX_INTERRUPT
static void spi_stm32lx_irq_config_func_3(struct device *port);
#endif

static const  struct spi_stm32lx_config spi_stm32lx_cfg_3 = {
	.base = (u8_t *)SPI3_BASE,
	.pclken = { .bus = STM32_CLOCK_BUS_APB1,
		    .enr = LL_APB1_GRP1_PERIPH_SPI3 },
#ifdef CONFIG_SPI_STM32LX_INTERRUPT
	.irq_config_func = spi_stm32lx_irq_config_func_3,
#endif
};

static struct spi_stm32lx_data spi_stm32lx_dev_data_3 = {
};

DEVICE_AND_API_INIT(spi_stm32lx_3, CONFIG_SPI_3_NAME, &spi_stm32lx_init,
		    &spi_stm32lx_dev_data_3, &spi_stm32lx_cfg_3,
		    PRE_KERNEL_2, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &api_funcs);

#ifdef CONFIG_SPI_STM32LX_INTERRUPT
static void spi_stm32lx_irq_config_func_3(struct device *dev)
{
#ifdef CONFIG_SOC_SERIES_STM32L4X
#define PORT_3_SPI_IRQ STM32L4_IRQ_SPI3
#endif
	IRQ_CONNECT(PORT_3_SPI_IRQ, CONFIG_SPI_3_IRQ_PRI,
		spi_stm32lx_isr, DEVICE_GET(spi_stm32lx_3), 0);
	irq_enable(PORT_3_SPI_IRQ);
}
#endif

#endif /* CONFIG_SPI_3 */
