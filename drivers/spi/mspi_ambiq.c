/*
 * Copyright (c) 2023 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ambiq_mspi

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mspi_ambiq);

#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>

#include "spi_context.h"
#include <am_mcu_apollo.h>

#define SPI_WORD_SIZE 8
#if defined(CONFIG_SOC_SERIES_APOLLO3X)
#define MSPI_MAX_FREQ 48000000
#else
#define MSPI_MAX_FREQ 96000000
#endif
#define MSPI_DEFAULT_FREQ 24000000

#define MSPI_TIMEOUT_US     1000000
#define PWRCTRL_MAX_WAIT_US 5
#define MSPI_BUSY           BIT(2)
/* Apollo MSPI in zephyr only support CE0. */
#define MSPI_CE_NUMBER      0

typedef int (*ambiq_mspi_pwr_func_t)(void);

struct mspi_ambiq_config {
	uint32_t base;
	int size;
	uint8_t mspi_ce;
	struct gpio_dt_spec *cs_gpio;
	uint32_t clock_freq;
	const struct pinctrl_dev_config *pcfg;
	ambiq_mspi_pwr_func_t pwr_func;
#if defined(CONFIG_MSPI_AMBIQ_DMA)
	void (*irq_config_func)(void);
#endif
};

struct mspi_ambiq_data {
	struct spi_context ctx;
	void *mspiHandle;
	am_hal_mspi_dev_config_t mspicfg;
	int inst_idx;
};

#if defined(CONFIG_MSPI_AMBIQ_DMA)
static __aligned(32) struct {
	__aligned(32) uint32_t buf[CONFIG_MSPI_DMA_TCB_BUFFER_SIZE];
} mspi_dma_tcb_buf[DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT)] __attribute__((__section__(".nocache")));
#endif

am_hal_mspi_dev_config_t MspiCfgDefault = {
	.ui8TurnAround = 0,
	.eAddrCfg = 0,
	.eInstrCfg = 0,
#if defined(AM_PART_APOLLO4_API)
	.ui16ReadInstr = 0,
	.ui16WriteInstr = 0,
#else
	.ui8ReadInstr = 0,
	.ui8WriteInstr = 0,
#endif
	.eDeviceConfig = AM_HAL_MSPI_FLASH_SERIAL_CE0,
	.eSpiMode = AM_HAL_MSPI_SPI_MODE_0,
	.eClockFreq = AM_HAL_MSPI_CLK_24MHZ,
	.bSendAddr = false,
	.bSendInstr = false,
	.bTurnaround = false,
#if defined(AM_PART_APOLLO3P)
	.ui8WriteLatency = 0,
	.bEnWriteLatency = false,
	.bEmulateDDR = false,
	.ui16DMATimeLimit = 80,
	.eDMABoundary = AM_HAL_MSPI_BOUNDARY_BREAK1K,
#elif defined(AM_PART_APOLLO4_API)
	.ui8WriteLatency = 0,
	.bEnWriteLatency = false,
	.bEmulateDDR = false,
	.ui16DMATimeLimit = 80,
	.eDMABoundary = AM_HAL_MSPI_BOUNDARY_BREAK1K,
#if defined(AM_PART_APOLL4)
	.eDeviceNum = AM_HAL_MSPI_DEVICE0,
#endif
#else
	.ui32TCBSize = 0,
	.pTCB = NULL,
	.scramblingStartAddr = 0,
	.scramblingEndAddr = 0,
#endif
};

#if defined(CONFIG_SOC_SERIES_APOLLO3X)
static am_hal_mspi_device_e mspi_set_line(const struct mspi_ambiq_config *cfg,
					  spi_operation_t operation)
{
	am_hal_mspi_device_e line_mode;

	if (IS_ENABLED(CONFIG_SPI_EXTENDED_MODES)) {
		switch (operation & SPI_LINES_MASK) {
		case SPI_LINES_SINGLE:
			line_mode = cfg->mspi_ce ? AM_HAL_MSPI_FLASH_SERIAL_CE1
						 : AM_HAL_MSPI_FLASH_SERIAL_CE0;
			break;
		case SPI_LINES_DUAL:
			line_mode = cfg->mspi_ce ? AM_HAL_MSPI_FLASH_DUAL_CE1
						 : AM_HAL_MSPI_FLASH_DUAL_CE0;
			break;
		case SPI_LINES_QUAD:
			line_mode = cfg->mspi_ce ? AM_HAL_MSPI_FLASH_QUAD_CE1
						 : AM_HAL_MSPI_FLASH_QUAD_CE0;
			break;
		case SPI_LINES_OCTAL:
			line_mode = cfg->mspi_ce ? AM_HAL_MSPI_FLASH_OCTAL_CE0
						 : AM_HAL_MSPI_FLASH_OCTAL_CE0;
			break;
		default:
			line_mode = AM_HAL_MSPI_FLASH_MAX;
			LOG_ERR("SPI Lines On not supported");
			break;
		}
	}
	return line_mode;
}
#endif /*CONFIG_SOC_SERIES_APOLLO3X*/

static int mspi_set_freq(uint32_t freq)
{
	uint32_t d = MSPI_MAX_FREQ / freq;

	switch (d) {
#if !defined(CONFIG_SOC_SERIES_APOLLO3X)
	case AM_HAL_MSPI_CLK_96MHZ:
	case AM_HAL_MSPI_CLK_32MHZ:
#endif
	case AM_HAL_MSPI_CLK_48MHZ:
	case AM_HAL_MSPI_CLK_24MHZ:
	case AM_HAL_MSPI_CLK_16MHZ:
	case AM_HAL_MSPI_CLK_12MHZ:
	case AM_HAL_MSPI_CLK_8MHZ:
	case AM_HAL_MSPI_CLK_6MHZ:
	case AM_HAL_MSPI_CLK_4MHZ:
	case AM_HAL_MSPI_CLK_3MHZ:
		break;
	default:
		LOG_ERR("Frequency not supported!");
#if !defined(CONFIG_SOC_SERIES_APOLLO3X)
		d = AM_HAL_MSPI_CLK_INVALID;
#else
		d = 0;
#endif
		break;
	}

	return d;
}

static int mspi_config(const struct device *dev, const struct spi_config *config)
{
	struct mspi_ambiq_data *data = dev->data;
	int ret;
	am_hal_mspi_dev_config_t mspicfg = MspiCfgDefault;

	if (config->operation & SPI_FULL_DUPLEX) {
		LOG_ERR("Full-duplex not supported");
		return -ENOTSUP;
	}

	if (SPI_WORD_SIZE_GET(config->operation) != SPI_WORD_SIZE) {
		LOG_ERR("Word size must be %d", SPI_WORD_SIZE);
		return -ENOTSUP;
	}
#if defined(CONFIG_SOC_SERIES_APOLLO4X)
	if ((config->operation & SPI_LINES_MASK) != SPI_LINES_SINGLE) {
		LOG_ERR("Only single mode is currently supported");
		return -ENOTSUP;
	}
#endif /*CONFIG_SOC_SERIES_APOLLO4X*/

	if (config->operation & SPI_LOCK_ON) {
		LOG_ERR("Lock On not supported");
		return -ENOTSUP;
	}

	if (config->operation & SPI_TRANSFER_LSB) {
		LOG_ERR("LSB first not supported");
		return -ENOTSUP;
	}

	if (config->operation & (SPI_MODE_CPOL | SPI_MODE_CPHA)) {
		if (config->operation & (SPI_MODE_CPOL && SPI_MODE_CPHA)) {
			mspicfg.eSpiMode = AM_HAL_MSPI_SPI_MODE_3;
		} else if (config->operation & SPI_MODE_CPOL) {
			mspicfg.eSpiMode = AM_HAL_MSPI_SPI_MODE_2;
		} else if (config->operation & SPI_MODE_CPHA) {
			mspicfg.eSpiMode = AM_HAL_MSPI_SPI_MODE_1;
		} else {
			mspicfg.eSpiMode = AM_HAL_MSPI_SPI_MODE_0;
		}
	}
#if defined(CONFIG_SOC_SERIES_APOLLO3X)
	const struct mspi_ambiq_config *cfg = dev->config;

	mspicfg.eClockFreq = mspi_set_freq(config->frequency);
	if (mspicfg.eClockFreq == 0) {
		return -ENOTSUP;
	}
	mspicfg.eDeviceConfig = mspi_set_line(cfg, config->operation);
	if (mspicfg.eDeviceConfig == AM_HAL_MSPI_FLASH_MAX) {
		return -ENOTSUP;
	}
#ifdef CONFIG_MSPI_AMBIQ_DMA
	mspicfg.pTCB = mspi_dma_tcb_buf[data->inst_idx].buf;
	mspicfg.ui32TCBSize = CONFIG_MSPI_DMA_TCB_BUFFER_SIZE;
#endif /*CONFIG_MSPI_AMBIQ_DMA*/

#else /* CONFIG_SOC_SERIES_APOLLO3X */
	mspicfg.eClockFreq = mspi_set_freq(config->frequency);
	if (mspicfg.eClockFreq == AM_HAL_MSPI_CLK_INVALID) {
		return -ENOTSUP;
	}

	mspicfg.eDeviceConfig = AM_HAL_MSPI_FLASH_SERIAL_CE0;

#endif /* CONFIG_SOC_SERIES_APOLLO3X */

	ret = am_hal_mspi_disable(data->mspiHandle);
	if (ret) {
		return ret;
	}

	ret = am_hal_mspi_device_configure(data->mspiHandle, &mspicfg);
	if (ret) {
		return ret;
	}

	ret = am_hal_mspi_enable(data->mspiHandle);

	return ret;
}
#if defined(CONFIG_SOC_SERIES_APOLLO4X)

static int mspi_ambiq_xfer(const struct device *dev, const struct spi_config *config)
{
	struct mspi_ambiq_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	int ret;

	am_hal_mspi_pio_transfer_t trans = {0};

	trans.bSendAddr = true;
	trans.bSendInstr = true;
	trans.ui16DeviceInstr = *ctx->tx_buf;
	spi_context_update_tx(ctx, 1, 1);
	trans.ui32DeviceAddr = *ctx->tx_buf;

	if (ctx->rx_buf != NULL) {
		spi_context_update_rx(ctx, 1, ctx->rx_len);
		trans.eDirection = AM_HAL_MSPI_RX;
		trans.pui32Buffer = (uint32_t *)ctx->rx_buf;
		trans.ui32NumBytes = ctx->rx_len;

	} else if (ctx->tx_buf != NULL) {
		spi_context_update_tx(ctx, 1, 1);
		trans.eDirection = AM_HAL_MSPI_TX;
		trans.pui32Buffer = (uint32_t *)ctx->tx_buf;
		trans.ui32NumBytes = ctx->tx_len;
	}
	ret = am_hal_mspi_blocking_transfer(data->mspiHandle, &trans, MSPI_TIMEOUT_US);
	spi_context_complete(ctx, dev, 0);

	return ret;
}
#elif defined(CONFIG_SOC_SERIES_APOLLO3X)

#if defined(CONFIG_MSPI_AMBIQ_DMA)

static void pfnMSPI_Callback(void *pCallbackCtxt, uint32_t status)
{
	const struct device *dev = pCallbackCtxt;
	struct mspi_ambiq_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;

	spi_context_complete(ctx, dev, 0);
}

static void mspi_ambiq_isr(const struct device *dev)
{
	uint32_t ui32Status;
	struct mspi_ambiq_data *data = dev->data;

	am_hal_mspi_interrupt_status_get(data->mspiHandle, &ui32Status, false);
	am_hal_mspi_interrupt_clear(data->mspiHandle, ui32Status);
	am_hal_mspi_interrupt_service(data->mspiHandle, ui32Status);
}
#endif /*CONFIG_MSPI_AMBIQ_DMA*/

static int mspi_ambiq_xfer(const struct device *dev, const struct spi_config *config)
{
	const struct mspi_ambiq_config *cfg = dev->config;
	struct mspi_ambiq_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	int ret;

#ifdef CONFIG_MSPI_AMBIQ_DMA
	am_hal_mspi_dma_transfer_t Transaction;

	Transaction.ui8Priority = 1;
	Transaction.ui32PauseCondition = 0;
	Transaction.ui32StatusSetClr = 0;

	if (ctx->tx_len == 0) {
		/* rx only, nothing to tx */
		/* Set the transfer direction to RX (Read) */
		Transaction.eDirection = AM_HAL_MSPI_RX;
		Transaction.ui32SRAMAddress = (uint32_t)ctx->rx_buf;
		Transaction.ui32TransferCount = ctx->rx_len;
		am_hal_mspi_nonblocking_transfer(data->mspiHandle, &Transaction,
						 AM_HAL_MSPI_TRANS_DMA, pfnMSPI_Callback,
						 (void *)dev);
		ret = spi_context_wait_for_completion(ctx);
	} else if (ctx->rx_len == 0) {
		/* tx only, nothing to rx */
		/* Set the transfer direction to TX (Write) */
		Transaction.eDirection = AM_HAL_MSPI_TX;
		Transaction.ui32SRAMAddress = (uint32_t)ctx->tx_buf;
		Transaction.ui32TransferCount = ctx->tx_len;
		am_hal_mspi_nonblocking_transfer(data->mspiHandle, &Transaction,
						 AM_HAL_MSPI_TRANS_DMA, pfnMSPI_Callback,
						 (void *)dev);
		ret = spi_context_wait_for_completion(ctx);
	} else {
		/* Breaks the data into two transfers, keeps the chip select active between the two
		 * transfers, and reconfigures the pin to the CS function when the transfer is
		 * complete.
		 */
		gpio_pin_configure_dt(cfg->cs_gpio, GPIO_OUTPUT_LOW);

		/* Set the transfer direction to TX (Write) */
		Transaction.eDirection = AM_HAL_MSPI_TX;
		Transaction.ui32SRAMAddress = (uint32_t)ctx->tx_buf;
		Transaction.ui32TransferCount = ctx->tx_len;
		ret = am_hal_mspi_nonblocking_transfer(data->mspiHandle, &Transaction,
						       AM_HAL_MSPI_TRANS_DMA, NULL, NULL);

		/* Set the transfer direction to RX (Read) */
		Transaction.eDirection = AM_HAL_MSPI_RX;
		Transaction.ui32SRAMAddress = (uint32_t)ctx->rx_buf;
		Transaction.ui32TransferCount = ctx->rx_len;
		ret = am_hal_mspi_nonblocking_transfer(data->mspiHandle, &Transaction,
						       AM_HAL_MSPI_TRANS_DMA, pfnMSPI_Callback,
						       (void *)dev);
		ret = spi_context_wait_for_completion(ctx);
		gpio_pin_configure_dt(cfg->cs_gpio, GPIO_OUTPUT_HIGH);
		ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	}
#else /*CONFIG_MSPI_AMBIQ_DMA*/
	am_hal_mspi_pio_transfer_t trans = {0};

	trans.bSendAddr = false;
	trans.bSendInstr = false;

	if (ctx->tx_len == 0) {
		/* rx only, nothing to tx */
		/* Set the transfer direction to RX (Read) */
		trans.eDirection = AM_HAL_MSPI_RX;
		trans.pui32Buffer = (uint32_t *)ctx->rx_buf;
		trans.ui32NumBytes = ctx->rx_len;
		ret = am_hal_mspi_blocking_transfer(data->mspiHandle, &trans, MSPI_TIMEOUT_US);

	} else if (ctx->rx_len == 0) {
		/* tx only, nothing to rx */
		/* Set the transfer direction to TX (Write) */
		trans.eDirection = AM_HAL_MSPI_TX;
		trans.pui32Buffer = (uint32_t *)ctx->tx_buf;
		trans.ui32NumBytes = ctx->tx_len;
		ret = am_hal_mspi_blocking_transfer(data->mspiHandle, &trans, MSPI_TIMEOUT_US);

	} else {
		trans.bSendAddr = false;
		trans.bSendInstr = false;
		/* Breaks the data into two transfers, keeps the chip select active between the two
		 * transfers, and reconfigures the pin to the CS function when the transfer is
		 * complete.
		 */
		gpio_pin_configure_dt(cfg->cs_gpio, GPIO_OUTPUT_LOW);
		/* Set the transfer direction to TX (Write) */
		trans.eDirection = AM_HAL_MSPI_TX;
		trans.pui32Buffer = (uint32_t *)ctx->tx_buf;
		trans.ui32NumBytes = ctx->tx_len;
		ret = am_hal_mspi_blocking_transfer(data->mspiHandle, &trans, MSPI_TIMEOUT_US);
		/* Set the transfer direction to RX (Read) */
		trans.eDirection = AM_HAL_MSPI_RX;
		trans.pui32Buffer = (uint32_t *)ctx->rx_buf;
		trans.ui32NumBytes = ctx->rx_len;
		ret = am_hal_mspi_blocking_transfer(data->mspiHandle, &trans, MSPI_TIMEOUT_US);

		gpio_pin_configure_dt(cfg->cs_gpio, GPIO_OUTPUT_HIGH);
		ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	}
	spi_context_complete(ctx, dev, 0);

#endif /*CONFIG_MSPI_AMBIQ_DMA*/

	return ret;
}

#endif

static int mspi_ambiq_transceive(const struct device *dev, const struct spi_config *config,
				 const struct spi_buf_set *tx_bufs,
				 const struct spi_buf_set *rx_bufs)
{
	struct mspi_ambiq_data *data = dev->data;
	int ret;

	if (!tx_bufs && !rx_bufs) {
		return 0;
	}

	spi_context_lock(&data->ctx, false, NULL, NULL, config);

	ret = mspi_config(dev, config);
	if (ret) {
		goto done;
	}

	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, 1);

	ret = mspi_ambiq_xfer(dev, config);

done:
	spi_context_release(&data->ctx, ret);

	return ret;
}

static int mspi_ambiq_release(const struct device *dev, const struct spi_config *config)
{
	const struct mspi_ambiq_config *cfg = dev->config;
	struct mspi_ambiq_data *data = dev->data;

	spi_context_unlock_unconditionally(&data->ctx);
	if (sys_read32(cfg->base) & MSPI_BUSY) {
		return -EBUSY;
	}

	return 0;
}

static const struct spi_driver_api mspi_ambiq_driver_api = {
	.transceive = mspi_ambiq_transceive,
	.release = mspi_ambiq_release,
};

static int mspi_ambiq_init(const struct device *dev)
{
	struct mspi_ambiq_data *data = dev->data;
	const struct mspi_ambiq_config *cfg = dev->config;
	int ret;

	if (AM_HAL_STATUS_SUCCESS !=
	    am_hal_mspi_initialize((cfg->base - REG_MSPI_BASEADDR) / (cfg->size * 4),
				   &data->mspiHandle)) {
		LOG_ERR("Fail to initialize MSPI, code:%d", ret);
		return -ENODEV;
	}
	ret = cfg->pwr_func();
#if defined(CONFIG_SOC_SERIES_APOLLO4X)
	am_hal_mspi_config_t mspiCfg = {0};

	mspiCfg.pTCB = NULL;
	ret = am_hal_mspi_configure(data->mspiHandle, &mspiCfg);
	if (ret) {
		return ret;
	}

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
#elif defined(CONFIG_SOC_SERIES_APOLLO3X)

	/* default SPI Mode 0 signalling */
	struct spi_config spi_cfg = {
		.frequency = MSPI_DEFAULT_FREQ,
		.operation = SPI_WORD_SET(SPI_WORD_SIZE),
	};
	ret = mspi_config(dev, &spi_cfg);
	if (ret) {
		LOG_ERR("Ambiq MSPi init configure failed (%d)", ret);
		return ret;
	}

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0 && ret != -ENOENT) {
		return ret;
	}

#ifdef CONFIG_MSPI_AMBIQ_DMA
	void *buf;

	cfg->irq_config_func();

	if (AM_HAL_STATUS_SUCCESS !=
	    am_hal_mspi_interrupt_clear(data->mspiHandle,
					AM_HAL_MSPI_INT_CQUPD | AM_HAL_MSPI_INT_ERR)) {
		ret = -ENODEV;
		LOG_ERR("Fail to clear interrupt, code:%d", ret);
		return ret;
	}

	if (AM_HAL_STATUS_SUCCESS !=
	    am_hal_mspi_interrupt_enable(data->mspiHandle,
					 AM_HAL_MSPI_INT_CQUPD | AM_HAL_MSPI_INT_ERR)) {
		ret = -ENODEV;
		LOG_ERR("Fail to turn on interrupt, code:%d", ret);
		return ret;
	}

#endif /*CONFIG_MSPI_AMBIQ_DMA*/

#endif

	spi_context_unlock_unconditionally(&data->ctx);
	return 0;
}

#if defined(CONFIG_SOC_SERIES_APOLLO4X)
#define AMBIQ_MSPI_DEFINE(n)                                                                       \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static int pwr_on_ambiq_mspi_##n(void)                                                     \
	{                                                                                          \
		uint32_t addr = DT_REG_ADDR(DT_INST_PHANDLE(n, ambiq_pwrcfg)) +                    \
				DT_INST_PHA(n, ambiq_pwrcfg, offset);                              \
		sys_write32((sys_read32(addr) | DT_INST_PHA(n, ambiq_pwrcfg, mask)), addr);        \
		k_busy_wait(PWRCTRL_MAX_WAIT_US);                                                  \
		return 0;                                                                          \
	}                                                                                          \
	static struct mspi_ambiq_data mspi_ambiq_data##n = {                                       \
		SPI_CONTEXT_INIT_SYNC(mspi_ambiq_data##n, ctx)};                                   \
	static const struct mspi_ambiq_config mspi_ambiq_config##n = {                             \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.size = DT_INST_REG_SIZE(n),                                                       \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.pwr_func = pwr_on_ambiq_mspi_##n,                                                 \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, mspi_ambiq_init, NULL, &mspi_ambiq_data##n,                       \
			      &mspi_ambiq_config##n, POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,        \
			      &mspi_ambiq_driver_api);

DT_INST_FOREACH_STATUS_OKAY(AMBIQ_MSPI_DEFINE)

#elif defined(CONFIG_SOC_SERIES_APOLLO3X)

#define AMBIQ_DMA_CONFIGURE(n)                                                                     \
	static void mspi_irq_config_func_##n(void)                                                 \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), mspi_ambiq_isr,             \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}

#define AMBIQ_MSPI_DEFINE(n)                                                                       \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	IF_ENABLED(CONFIG_MSPI_AMBIQ_DMA, (AMBIQ_DMA_CONFIGURE(n)));                               \
	static int pwr_on_ambiq_mspi_##n(void)                                                     \
	{                                                                                          \
		uint32_t addr = DT_REG_ADDR(DT_INST_PHANDLE(n, ambiq_pwrcfg)) +                    \
				DT_INST_PHA(n, ambiq_pwrcfg, offset);                              \
		sys_write32((sys_read32(addr) | DT_INST_PHA(n, ambiq_pwrcfg, mask)), addr);        \
		k_busy_wait(PWRCTRL_MAX_WAIT_US);                                                  \
		return 0;                                                                          \
	};                                                                                         \
	static struct gpio_dt_spec ce_gpios##n[] = {COND_CODE_1(                                   \
		DT_NODE_HAS_PROP(DT_DRV_INST(n), cs_gpios),                                        \
		(DT_INST_FOREACH_PROP_ELEM_SEP(n, cs_gpios, GPIO_DT_SPEC_GET_BY_IDX, (,))), ())}; \
	static struct mspi_ambiq_data mspi_ambiq_data##n = {                                       \
		SPI_CONTEXT_INIT_LOCK(mspi_ambiq_data##n, ctx),                                    \
		SPI_CONTEXT_INIT_SYNC(mspi_ambiq_data##n, ctx), .inst_idx = n};                \
	static const struct mspi_ambiq_config mspi_ambiq_config##n = {                             \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.size = DT_INST_REG_SIZE(n),                                                       \
		.mspi_ce = DT_INST_PROP_OR(i, mspi_ce, MSPI_CE_NUMBER),                            \
		.cs_gpio = (struct gpio_dt_spec *)ce_gpios##n,                                     \
		IF_ENABLED(CONFIG_MSPI_AMBIQ_DMA, (.irq_config_func = mspi_irq_config_func_##n,)) \
			.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                 \
		.pwr_func = pwr_on_ambiq_mspi_##n,                                                 \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, mspi_ambiq_init, NULL, &mspi_ambiq_data##n,                       \
			      &mspi_ambiq_config##n, POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,        \
			      &mspi_ambiq_driver_api);

DT_INST_FOREACH_STATUS_OKAY(AMBIQ_MSPI_DEFINE)

#endif
