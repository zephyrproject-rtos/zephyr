/*
 * Copyright (c) 2019 Brett Witherspoon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_LEVEL CONFIG_SPI_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(spi_cc13xx_cc26xx);

#include <drivers/spi.h>
#include <sys_clock.h>

#include <driverlib/prcm.h>
#include <driverlib/ssi.h>
#include <driverlib/ioc.h>

#include "spi_context.h"

struct spi_cc13xx_cc26xx_config {
	u32_t base;
	u32_t sck_pin;
	u32_t mosi_pin;
	u32_t miso_pin;
	u32_t cs_pin;
};

struct spi_cc13xx_cc26xx_data {
	struct spi_context ctx;
};

static inline struct spi_cc13xx_cc26xx_data *get_dev_data(struct device *dev)
{
	return dev->driver_data;
}

static inline const struct spi_cc13xx_cc26xx_config *
get_dev_config(struct device *dev)
{
	return dev->config->config_info;
}

static int spi_cc13xx_cc26xx_configure(struct device *dev,
				       const struct spi_config *config)
{
	const struct spi_cc13xx_cc26xx_config *cfg = get_dev_config(dev);
	struct spi_context *ctx = &get_dev_data(dev)->ctx;
	u32_t prot;

	if (spi_context_configured(ctx, config)) {
		return 0;
	}

	/* Slave mode has not been implemented */
	if (SPI_OP_MODE_GET(config->operation) != SPI_OP_MODE_MASTER) {
		LOG_ERR("Slave mode is not supported");
		return -ENOTSUP;
	}

	/* Word sizes other than 8 bits has not been implemented */
	if (SPI_WORD_SIZE_GET(config->operation) != 8) {
		LOG_ERR("Word sizes other than 8 bits are not supported");
		return -ENOTSUP;
	}

	if (config->operation & SPI_TRANSFER_LSB) {
		LOG_ERR("Transfer LSB first mode is not supported");
		return -EINVAL;
	}

	if ((config->operation & SPI_LINES_MASK) != SPI_LINES_SINGLE) {
		LOG_ERR("Multiple lines are not supported");
		return -EINVAL;
	}

	if (config->operation & SPI_CS_ACTIVE_HIGH && !config->cs) {
		LOG_ERR("Active high CS requires emulation through a GPIO line.");
		return -EINVAL;
	}

	if (config->frequency < 2000000) {
		LOG_ERR("Frequencies lower than 2 MHz are not supported");
		return -EINVAL;
	}

	if (2 * config->frequency > sys_clock_hw_cycles_per_sec()) {
		LOG_ERR("Frequency greater than supported in master mode");
		return -EINVAL;
	}

	if (SPI_MODE_GET(config->operation) & SPI_MODE_CPOL) {
		if (SPI_MODE_GET(config->operation) & SPI_MODE_CPHA) {
			prot = SSI_FRF_MOTO_MODE_3;
		} else {
			prot = SSI_FRF_MOTO_MODE_2;
		}
	} else {
		if (SPI_MODE_GET(config->operation) & SPI_MODE_CPHA) {
			prot = SSI_FRF_MOTO_MODE_1;
		} else {
			prot = SSI_FRF_MOTO_MODE_0;
		}
	}

	IOCPinTypeSsiMaster(cfg->base, cfg->miso_pin, cfg->mosi_pin,
			    cfg->cs_pin, cfg->sck_pin);

	ctx->config = config;
	/* This will reconfigure the CS pin as GPIO if same as cfg->cs_pin. */
	spi_context_cs_configure(ctx);

	/* Disable SSI before making configuration changes */
	SSIDisable(cfg->base);

	/* Configure SSI */
	SSIConfigSetExpClk(cfg->base, sys_clock_hw_cycles_per_sec(), prot,
			   SSI_MODE_MASTER, config->frequency, 8);

	if (SPI_MODE_GET(config->operation) & SPI_MODE_LOOP) {
		sys_set_bit(cfg->base + SSI_O_CR1, 0);
	}

	/* Re-enable SSI after making configuration changes */
	SSIEnable(cfg->base);

	return 0;
}

static int spi_cc13xx_cc26xx_transceive(struct device *dev,
					const struct spi_config *config,
					const struct spi_buf_set *tx_bufs,
					const struct spi_buf_set *rx_bufs)
{
	const struct spi_cc13xx_cc26xx_config *cfg = get_dev_config(dev);
	struct spi_context *ctx = &get_dev_data(dev)->ctx;
	u32_t txd, rxd;
	int err;

	spi_context_lock(ctx, false, NULL);

	err = spi_cc13xx_cc26xx_configure(dev, config);
	if (err) {
		goto done;
	}

	spi_context_buffers_setup(ctx, tx_bufs, rx_bufs, 1);

	spi_context_cs_control(ctx, true);

	do {
		if (spi_context_tx_buf_on(ctx)) {
			txd = *ctx->tx_buf;
		} else {
			txd = 0U;
		}

		SSIDataPut(cfg->base, txd);

		spi_context_update_tx(ctx, 1, 1);

		SSIDataGet(cfg->base, &rxd);

		if (spi_context_rx_buf_on(ctx)) {
			*ctx->rx_buf = rxd;
		}

		spi_context_update_rx(ctx, 1, 1);
	} while (spi_context_tx_on(ctx) || spi_context_rx_on(ctx));

	spi_context_cs_control(ctx, false);

done:
	spi_context_release(ctx, err);
	return err;
}

static int spi_cc13xx_cc26xx_release(struct device *dev,
				     const struct spi_config *config)
{
	struct spi_context *ctx = &get_dev_data(dev)->ctx;

	if (!spi_context_configured(ctx, config)) {
		return -EINVAL;
	}

	if (SSIBusy(get_dev_config(dev)->base)) {
		return -EBUSY;
	}

	spi_context_unlock_unconditionally(ctx);

	return 0;
}

#if defined(CONFIG_SPI_0) || defined(CONFIG_SPI_1)
static const struct spi_driver_api spi_cc13xx_cc26xx_driver_api = {
	.transceive = spi_cc13xx_cc26xx_transceive,
	.release = spi_cc13xx_cc26xx_release,
};
#else
#warning "No SPI port configured"
#endif

#ifdef CONFIG_SPI_0
static int spi_cc13xx_cc26xx_init_0(struct device *dev)
{
	/* Enable SSI0 power domain */
	PRCMPowerDomainOn(PRCM_DOMAIN_SERIAL);

	/* Enable SSI0 peripherals */
	PRCMPeripheralRunEnable(PRCM_PERIPH_SSI0);
	/* Enable in sleep mode until proper power management is added */
	PRCMPeripheralSleepEnable(PRCM_PERIPH_SSI0);
	PRCMPeripheralDeepSleepEnable(PRCM_PERIPH_SSI0);

	/* Load PRCM settings */
	PRCMLoadSet();
	while (!PRCMLoadGet()) {
		continue;
	}

	/* SSI should not be accessed until power domain is on. */
	while (PRCMPowerDomainStatus(PRCM_DOMAIN_SERIAL) !=
	       PRCM_DOMAIN_POWER_ON) {
		continue;
	}

	spi_context_unlock_unconditionally(&get_dev_data(dev)->ctx);

	return 0;
}

static const struct spi_cc13xx_cc26xx_config spi_cc13xx_cc26xx_config_0 = {
	.base = DT_TI_CC13XX_CC26XX_SPI_40000000_BASE_ADDRESS,
	.sck_pin = DT_TI_CC13XX_CC26XX_SPI_40000000_SCK_PIN,
	.mosi_pin = DT_TI_CC13XX_CC26XX_SPI_40000000_MOSI_PIN,
	.miso_pin = DT_TI_CC13XX_CC26XX_SPI_40000000_MISO_PIN,
#ifdef DT_TI_CC13XX_CC26XX_SPI_40000000_CS_PIN
	.cs_pin = DT_TI_CC13XX_CC26XX_SPI_40000000_CS_PIN,
#else
	.cs_pin = IOID_UNUSED,
#endif /* DT_INST_0_TI_CC13XX_CC26XX_SPI_CS_PIN */
};

static struct spi_cc13xx_cc26xx_data spi_cc13xx_cc26xx_data_0 = {
	SPI_CONTEXT_INIT_LOCK(spi_cc13xx_cc26xx_data_0, ctx),
	SPI_CONTEXT_INIT_SYNC(spi_cc13xx_cc26xx_data_0, ctx),
};

DEVICE_AND_API_INIT(spi_cc13xx_cc26xx_0, DT_TI_CC13XX_CC26XX_SPI_40000000_LABEL,
		    spi_cc13xx_cc26xx_init_0, &spi_cc13xx_cc26xx_data_0,
		    &spi_cc13xx_cc26xx_config_0, POST_KERNEL,
		    CONFIG_SPI_INIT_PRIORITY, &spi_cc13xx_cc26xx_driver_api);
#endif /* CONFIG_SPI_0 */

#ifdef CONFIG_SPI_1
static int spi_cc13xx_cc26xx_init_1(struct device *dev)
{
	/* Enable SSI1 power domain */
	PRCMPowerDomainOn(PRCM_DOMAIN_PERIPH);

	/* Enable SSI1 peripherals */
	PRCMPeripheralRunEnable(PRCM_PERIPH_SSI1);
	/* Enable in sleep mode until proper power management is added */
	PRCMPeripheralSleepEnable(PRCM_PERIPH_SSI1);
	PRCMPeripheralDeepSleepEnable(PRCM_PERIPH_SSI1);

	/* Load PRCM settings */
	PRCMLoadSet();
	while (!PRCMLoadGet()) {
		continue;
	}

	/* SSI should not be accessed until power domain is on. */
	while (PRCMPowerDomainStatus(PRCM_DOMAIN_PERIPH) !=
	       PRCM_DOMAIN_POWER_ON) {
		continue;
	}

	spi_context_unlock_unconditionally(&get_dev_data(dev)->ctx);

	return 0;
}

static const struct spi_cc13xx_cc26xx_config spi_cc13xx_cc26xx_config_1 = {
	.base = DT_TI_CC13XX_CC26XX_SPI_40008000_BASE_ADDRESS,
	.sck_pin = DT_TI_CC13XX_CC26XX_SPI_40008000_SCK_PIN,
	.mosi_pin = DT_TI_CC13XX_CC26XX_SPI_40008000_MOSI_PIN,
	.miso_pin = DT_TI_CC13XX_CC26XX_SPI_40008000_MISO_PIN,
#ifdef DT_TI_CC13XX_CC26XX_SPI_40008000_CS_PIN
	.cs_pin = DT_TI_CC13XX_CC26XX_SPI_40008000_CS_PIN,
#else
	.cs_pin = IOID_UNUSED,
#endif /* DT_TI_CC13XX_CC26XX_SPI_1_CS_PIN */
};

static struct spi_cc13xx_cc26xx_data spi_cc13xx_cc26xx_data_1 = {
	SPI_CONTEXT_INIT_LOCK(spi_cc13xx_cc26xx_data_1, ctx),
	SPI_CONTEXT_INIT_SYNC(spi_cc13xx_cc26xx_data_1, ctx),
};

DEVICE_AND_API_INIT(spi_cc13xx_cc26xx_1, DT_TI_CC13XX_CC26XX_SPI_40008000_LABEL,
		    spi_cc13xx_cc26xx_init_1, &spi_cc13xx_cc26xx_data_1,
		    &spi_cc13xx_cc26xx_config_1, POST_KERNEL,
		    CONFIG_SPI_INIT_PRIORITY, &spi_cc13xx_cc26xx_driver_api);
#endif /* CONFIG_SPI_1 */
