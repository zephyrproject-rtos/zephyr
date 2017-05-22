/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <spi.h>
#include <soc.h>
#include <fsl_dspi.h>
#include <fsl_clock.h>

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_SPI_LEVEL
#include <logging/sys_log.h>

struct spi_mcux_config {
	SPI_Type *base;
	clock_name_t clock_source;
	void (*irq_config_func)(struct device *dev);
	struct spi_config default_cfg;
};

struct spi_mcux_data {
	dspi_master_handle_t handle;
	struct k_sem sync;
	status_t callback_status;
	u32_t slave;
};

static void spi_mcux_master_transfer_callback(SPI_Type *base,
		dspi_master_handle_t *handle, status_t status, void *userData)
{
	struct device *dev = userData;
	struct spi_mcux_data *data = dev->driver_data;

	data->callback_status = status;
	k_sem_give(&data->sync);
}

static int spi_mcux_configure(struct device *dev, struct spi_config *spi_config)
{
	const struct spi_mcux_config *config = dev->config->config_info;
	struct spi_mcux_data *data = dev->driver_data;
	SPI_Type *base = config->base;
	dspi_master_config_t master_config;
	u32_t flags = spi_config->config;
	u32_t clock_freq;
	u32_t word_size;

	DSPI_MasterGetDefaultConfig(&master_config);

	word_size = SPI_WORD_SIZE_GET(flags);
	if (word_size > FSL_FEATURE_DSPI_MAX_DATA_WIDTH) {
		SYS_LOG_ERR("Word size %d is greater than %d",
			    word_size, FSL_FEATURE_DSPI_MAX_DATA_WIDTH);
		return -EINVAL;
	}

	master_config.ctarConfig.bitsPerFrame = word_size;

	master_config.ctarConfig.cpol = (flags & SPI_MODE_CPOL)
					? kDSPI_ClockPolarityActiveLow
					: kDSPI_ClockPolarityActiveHigh;

	master_config.ctarConfig.cpha = (flags & SPI_MODE_CPHA)
					? kDSPI_ClockPhaseSecondEdge
					: kDSPI_ClockPhaseFirstEdge;

	master_config.ctarConfig.direction = (flags & SPI_TRANSFER_LSB)
					     ? kDSPI_LsbFirst
					     : kDSPI_MsbFirst;

	master_config.ctarConfig.baudRate = spi_config->max_sys_freq;

	SYS_LOG_DBG("word size = %d, baud rate = %d",
		    word_size, spi_config->max_sys_freq);

	clock_freq = CLOCK_GetFreq(config->clock_source);
	if (!clock_freq) {
		SYS_LOG_ERR("Got frequency of 0");
		return -EINVAL;
	}
	DSPI_MasterInit(base, &master_config, clock_freq);

	DSPI_MasterTransferCreateHandle(base, &data->handle,
			spi_mcux_master_transfer_callback, dev);

	return 0;
}

static int spi_mcux_slave_select(struct device *dev, u32_t slave)
{
	struct spi_mcux_data *data = dev->driver_data;

	if (slave > FSL_FEATURE_DSPI_CHIP_SELECT_COUNT) {
		SYS_LOG_ERR("Slave %d is greater than %d",
			    slave, FSL_FEATURE_DSPI_CHIP_SELECT_COUNT);
		return -EINVAL;
	}

	data->slave = slave;

	return 0;
}

static int spi_mcux_transceive(struct device *dev,
		const void *tx_buf, u32_t tx_buf_len,
		void *rx_buf, u32_t rx_buf_len)
{
	const struct spi_mcux_config *config = dev->config->config_info;
	struct spi_mcux_data *data = dev->driver_data;
	SPI_Type *base = config->base;
	u8_t buf[CONFIG_SPI_MCUX_BUF_SIZE];
	dspi_transfer_t transfer;
	status_t status;

	/* Initialize the transfer descriptor */

	SYS_LOG_DBG("tx_buf_len = %d, rx_buf_len = %d, local buf len = %d",
		    tx_buf_len, rx_buf_len, sizeof(buf));

	if (tx_buf_len == rx_buf_len) {
		/* The tx and rx buffers are the same length, so we can use
		 * them directly.
		 */
		transfer.txData = (u8_t *)tx_buf;
		transfer.rxData = rx_buf;
		transfer.dataSize = rx_buf_len;
		SYS_LOG_DBG("Using tx and rx buffers directly");
	} else if (tx_buf_len == 0) {
		/* The tx buffer length is zero, so this is a one-way spi read
		 * operation.
		 */
		transfer.txData = NULL;
		transfer.rxData = rx_buf;
		transfer.dataSize = rx_buf_len;
		SYS_LOG_DBG("Using rx buffer directly, tx buffer is null");
	} else if (rx_buf_len == 0) {
		/* The rx buffer length is zero, so this is a one-way spi write
		 * operation.
		 */
		transfer.txData = (u8_t *)tx_buf;
		transfer.rxData = NULL;
		transfer.dataSize = tx_buf_len;
		SYS_LOG_DBG("Using tx buffer directly, rx buffer is null");
	} else if ((tx_buf_len < rx_buf_len) && (rx_buf_len <= sizeof(buf))) {
		/* The tx buffer is shorter than the rx buffer, so copy the tx
		 * buffer to the longer local buffer.
		 */
		transfer.txData = buf;
		transfer.rxData = rx_buf;
		transfer.dataSize = rx_buf_len;
		memcpy(buf, tx_buf, tx_buf_len);
		memset(&buf[tx_buf_len], CONFIG_SPI_MCUX_DUMMY_CHAR,
		       rx_buf_len - tx_buf_len);
		SYS_LOG_DBG("Using local buffer for tx");
	} else if ((rx_buf_len < tx_buf_len) && (tx_buf_len <= sizeof(buf))) {
		/* The rx buffer is shorter than the tx buffer, so use the
		 * longer local buffer for rx. After the transfer is complete,
		 * we'll need to copy the local buffer back to the rx buffer.
		 */
		transfer.txData = (u8_t *)tx_buf;
		transfer.rxData = buf;
		transfer.dataSize = tx_buf_len;
		SYS_LOG_DBG("Using local buffer for rx");
	} else {
		SYS_LOG_ERR("Local buffer too small for transfer");
		return -EINVAL;
	}

	transfer.configFlags = kDSPI_MasterCtar0 | kDSPI_MasterPcsContinuous |
			       (data->slave << DSPI_MASTER_PCS_SHIFT);

	/* Start the transfer */
	status = DSPI_MasterTransferNonBlocking(base, &data->handle, &transfer);

	/* Return an error if the transfer didn't start successfully e.g., if
	 * the bus was busy
	 */
	if (status != kStatus_Success) {
		SYS_LOG_ERR("Transfer could not start");
		return -EIO;
	}

	/* Wait for the transfer to complete */
	k_sem_take(&data->sync, K_FOREVER);

	/* Return an error if the transfer didn't complete successfully. */
	if (data->callback_status != kStatus_Success) {
		SYS_LOG_ERR("Transfer could not complete");
		return -EIO;
	}

	/* Copy the local buffer back to the rx buffer. */
	if ((rx_buf_len < tx_buf_len) && (tx_buf_len <= sizeof(buf))) {
		memcpy(rx_buf, buf, rx_buf_len);
	}

	return 0;
}

static void spi_mcux_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	const struct spi_mcux_config *config = dev->config->config_info;
	struct spi_mcux_data *data = dev->driver_data;
	SPI_Type *base = config->base;

	DSPI_MasterTransferHandleIRQ(base, &data->handle);
}

static int spi_mcux_init(struct device *dev)
{
	const struct spi_mcux_config *config = dev->config->config_info;
	struct spi_mcux_data *data = dev->driver_data;
	struct spi_config *spi_config;
	int error;

	k_sem_init(&data->sync, 0, UINT_MAX);

	spi_config = (struct spi_config *)&config->default_cfg;
	error = spi_mcux_configure(dev, spi_config);
	if (error) {
		SYS_LOG_ERR("Could not configure");
		return error;
	}

	config->irq_config_func(dev);

	return 0;
}

static const struct spi_driver_api spi_mcux_driver_api = {
	.configure = spi_mcux_configure,
	.slave_select = spi_mcux_slave_select,
	.transceive = spi_mcux_transceive,
};

#ifdef CONFIG_SPI_0
static void spi_mcux_config_func_0(struct device *dev);

static const struct spi_mcux_config spi_mcux_config_0 = {
	.base = DSPI0,
	.clock_source = DSPI0_CLK_SRC,
	.irq_config_func = spi_mcux_config_func_0,
	.default_cfg = {
		.config = CONFIG_SPI_0_DEFAULT_CFG,
		.max_sys_freq = CONFIG_SPI_0_DEFAULT_BAUD_RATE,
	}
};

static struct spi_mcux_data spi_mcux_data_0;

DEVICE_AND_API_INIT(spi_mcux_0, CONFIG_SPI_0_NAME, &spi_mcux_init,
		    &spi_mcux_data_0, &spi_mcux_config_0,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &spi_mcux_driver_api);

static void spi_mcux_config_func_0(struct device *dev)
{
	IRQ_CONNECT(IRQ_SPI0, CONFIG_SPI_0_IRQ_PRI,
		    spi_mcux_isr, DEVICE_GET(spi_mcux_0), 0);

	irq_enable(IRQ_SPI0);
}
#endif /* CONFIG_SPI_0 */

#ifdef CONFIG_SPI_1
static void spi_mcux_config_func_1(struct device *dev);

static const struct spi_mcux_config spi_mcux_config_1 = {
	.base = DSPI1,
	.clock_source = DSPI1_CLK_SRC,
	.irq_config_func = spi_mcux_config_func_1,
	.default_cfg = {
		.config = CONFIG_SPI_1_DEFAULT_CFG,
		.max_sys_freq = CONFIG_SPI_1_DEFAULT_BAUD_RATE,
	}
};

static struct spi_mcux_data spi_mcux_data_1;

DEVICE_AND_API_INIT(spi_mcux_1, CONFIG_SPI_1_NAME, &spi_mcux_init,
		    &spi_mcux_data_1, &spi_mcux_config_1,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &spi_mcux_driver_api);

static void spi_mcux_config_func_1(struct device *dev)
{
	IRQ_CONNECT(IRQ_SPI1, CONFIG_SPI_1_IRQ_PRI,
		    spi_mcux_isr, DEVICE_GET(spi_mcux_1), 0);

	irq_enable(IRQ_SPI1);
}
#endif /* CONFIG_SPI_1 */

#ifdef CONFIG_SPI_2
static void spi_mcux_config_func_2(struct device *dev);

static const struct spi_mcux_config spi_mcux_config_2 = {
	.base = DSPI2,
	.clock_source = DSPI2_CLK_SRC,
	.irq_config_func = spi_mcux_config_func_2,
	.default_cfg = {
		.config = CONFIG_SPI_2_DEFAULT_CFG,
		.max_sys_freq = CONFIG_SPI_2_DEFAULT_BAUD_RATE,
	}
};

static struct spi_mcux_data spi_mcux_data_2;

DEVICE_AND_API_INIT(spi_mcux_2, CONFIG_SPI_2_NAME, &spi_mcux_init,
		    &spi_mcux_data_2, &spi_mcux_config_2,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &spi_mcux_driver_api);

static void spi_mcux_config_func_2(struct device *dev)
{
	IRQ_CONNECT(IRQ_SPI2, CONFIG_SPI_2_IRQ_PRI,
		    spi_mcux_isr, DEVICE_GET(spi_mcux_2), 0);

	irq_enable(IRQ_SPI2);
}
#endif /* CONFIG_SPI_2 */
