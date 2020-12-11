/*
 * Copyright (c) 2019 Brett Witherspoon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_cc13xx_cc26xx_spi

#define LOG_LEVEL CONFIG_SPI_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(spi_cc13xx_cc26xx);

#include <drivers/spi.h>
#include <power/power.h>

#include <driverlib/prcm.h>
#include <driverlib/ssi.h>
#include <driverlib/ioc.h>

#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC26X2.h>

#include "spi_context.h"

struct spi_cc13xx_cc26xx_config {
	uint32_t base;
	uint32_t sck_pin;
	uint32_t mosi_pin;
	uint32_t miso_pin;
	uint32_t cs_pin;
};

struct spi_cc13xx_cc26xx_data {
	struct spi_context ctx;
#ifdef CONFIG_PM_DEVICE
	uint32_t pm_state;
#endif
};

#define CPU_FREQ DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency)

static inline struct spi_cc13xx_cc26xx_data *get_dev_data(const struct device *dev)
{
	return dev->data;
}

static inline const struct spi_cc13xx_cc26xx_config *
get_dev_config(const struct device *dev)
{
	return dev->config;
}

static int spi_cc13xx_cc26xx_configure(const struct device *dev,
				       const struct spi_config *config)
{
	const struct spi_cc13xx_cc26xx_config *cfg = get_dev_config(dev);
	struct spi_context *ctx = &get_dev_data(dev)->ctx;
	uint32_t prot;

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

	if (2 * config->frequency > CPU_FREQ) {
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
	SSIConfigSetExpClk(cfg->base, CPU_FREQ, prot,
			   SSI_MODE_MASTER, config->frequency, 8);

	if (SPI_MODE_GET(config->operation) & SPI_MODE_LOOP) {
		sys_set_bit(cfg->base + SSI_O_CR1, 0);
	}

	/* Re-enable SSI after making configuration changes */
	SSIEnable(cfg->base);

	return 0;
}

static int spi_cc13xx_cc26xx_transceive(const struct device *dev,
					const struct spi_config *config,
					const struct spi_buf_set *tx_bufs,
					const struct spi_buf_set *rx_bufs)
{
	const struct spi_cc13xx_cc26xx_config *cfg = get_dev_config(dev);
	struct spi_context *ctx = &get_dev_data(dev)->ctx;
	uint32_t txd, rxd;
	int err;

	spi_context_lock(ctx, false, NULL, config);

#if defined(CONFIG_PM) && \
	defined(CONFIG_PM_SLEEP_STATES)
	pm_ctrl_disable_state(POWER_STATE_SLEEP_2);
#endif

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
#if defined(CONFIG_PM) && \
	defined(CONFIG_PM_SLEEP_STATES)
	pm_ctrl_enable_state(POWER_STATE_SLEEP_2);
#endif
	spi_context_release(ctx, err);
	return err;
}

static int spi_cc13xx_cc26xx_release(const struct device *dev,
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

#ifdef CONFIG_PM_DEVICE
static int spi_cc13xx_cc26xx_set_power_state(const struct device *dev,
					     uint32_t new_state)
{
	int ret = 0;

	if ((new_state == DEVICE_PM_ACTIVE_STATE) &&
		(new_state != get_dev_data(dev)->pm_state)) {
		if (get_dev_config(dev)->base ==
			DT_INST_REG_ADDR(0)) {
			Power_setDependency(PowerCC26XX_PERIPH_SSI0);
		} else {
			Power_setDependency(PowerCC26XX_PERIPH_SSI1);
		}
		get_dev_data(dev)->pm_state = new_state;
	} else {
		__ASSERT_NO_MSG(new_state == DEVICE_PM_LOW_POWER_STATE ||
			new_state == DEVICE_PM_SUSPEND_STATE ||
			new_state == DEVICE_PM_OFF_STATE);

		if (get_dev_data(dev)->pm_state == DEVICE_PM_ACTIVE_STATE) {
			SSIDisable(get_dev_config(dev)->base);
			/*
			 * Release power dependency
			 */
			if (get_dev_config(dev)->base ==
				DT_INST_REG_ADDR(0)) {
				Power_releaseDependency(
					PowerCC26XX_PERIPH_SSI0);
			} else {
				Power_releaseDependency(
					PowerCC26XX_PERIPH_SSI1);
			}
			get_dev_data(dev)->pm_state = new_state;
		}
	}

	return ret;
}

static int spi_cc13xx_cc26xx_pm_control(const struct device *dev,
					uint32_t ctrl_command,
					void *context, device_pm_cb cb,
					void *arg)
{
	int ret = 0;

	if (ctrl_command == DEVICE_PM_SET_POWER_STATE) {
		uint32_t new_state = *((const uint32_t *)context);

		if (new_state != get_dev_data(dev)->pm_state) {
			ret = spi_cc13xx_cc26xx_set_power_state(dev,
				new_state);
		}
	} else {
		__ASSERT_NO_MSG(ctrl_command == DEVICE_PM_GET_POWER_STATE);
		*((uint32_t *)context) = get_dev_data(dev)->pm_state;
	}

	if (cb) {
		cb(dev, ret, context, arg);
	}

	return ret;
}
#endif /* CONFIG_PM_DEVICE */


static const struct spi_driver_api spi_cc13xx_cc26xx_driver_api = {
	.transceive = spi_cc13xx_cc26xx_transceive,
	.release = spi_cc13xx_cc26xx_release,
};

#ifdef CONFIG_PM
#define SPI_CC13XX_CC26XX_POWER_SPI(n)					  \
	do {								  \
		/* Set Power dependencies & constraints */		  \
		if (DT_INST_REG_ADDR(n) == 0x40000000) {		  \
			Power_setDependency(PowerCC26XX_PERIPH_SSI0);	  \
		} else {						  \
			Power_setDependency(PowerCC26XX_PERIPH_SSI1);	  \
		}							  \
	} while (0)
#else
#define SPI_CC13XX_CC26XX_POWER_SPI(n)					  \
	do {								  \
		uint32_t domain, periph;				  \
									  \
		/* Enable UART power domain */				  \
		if (DT_INST_REG_ADDR(n) == 0x40000000) {		  \
			domain = PRCM_DOMAIN_SERIAL;			  \
			periph = PRCM_PERIPH_SSI0;			  \
		} else {						  \
			domain = PRCM_DOMAIN_PERIPH;			  \
			periph = PRCM_PERIPH_SSI1;			  \
		}							  \
		/* Enable SSI##n power domain */			  \
		PRCMPowerDomainOn(domain);				  \
									  \
		/* Enable SSI##n peripherals */				  \
		PRCMPeripheralRunEnable(periph);			  \
		PRCMPeripheralSleepEnable(periph);			  \
		PRCMPeripheralDeepSleepEnable(periph);			  \
									  \
		/* Load PRCM settings */				  \
		PRCMLoadSet();						  \
		while (!PRCMLoadGet()) {				  \
			continue;					  \
		}							  \
									  \
		/* SSI should not be accessed until power domain is on. */\
		while (PRCMPowerDomainStatus(domain) !=			  \
			PRCM_DOMAIN_POWER_ON) {				  \
			continue;					  \
		}							  \
	} while (0)
#endif

#define SPI_CC13XX_CC26XX_DEVICE_INIT(n)				    \
	DEVICE_DT_INST_DEFINE(n,						    \
		spi_cc13xx_cc26xx_init_##n,				    \
		spi_cc13xx_cc26xx_pm_control,				    \
		&spi_cc13xx_cc26xx_data_##n, &spi_cc13xx_cc26xx_config_##n, \
		POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,			    \
		&spi_cc13xx_cc26xx_driver_api)

#ifdef CONFIG_PM_DEVICE
#define SPI_CC13XX_CC26XX_INIT_PM_STATE					    \
	do {								    \
		get_dev_data(dev)->pm_state = DEVICE_PM_ACTIVE_STATE;	    \
	} while (0)
#else
#define SPI_CC13XX_CC26XX_INIT_PM_STATE
#endif

#define SPI_CC13XX_CC26XX_INIT_FUNC(n)					    \
	static int spi_cc13xx_cc26xx_init_##n(const struct device *dev)	    \
	{								    \
		SPI_CC13XX_CC26XX_INIT_PM_STATE;			    \
									    \
		SPI_CC13XX_CC26XX_POWER_SPI(n);				    \
									    \
		spi_context_unlock_unconditionally(&get_dev_data(dev)->ctx);\
									    \
		return 0;						    \
	}

#define SPI_CC13XX_CC26XX_INIT(n)					\
	SPI_CC13XX_CC26XX_INIT_FUNC(n)					\
									\
	static const struct spi_cc13xx_cc26xx_config			\
		spi_cc13xx_cc26xx_config_##n = {			\
		.base = DT_INST_REG_ADDR(n),				\
		.sck_pin = DT_INST_PROP(n, sck_pin),			\
		.mosi_pin = DT_INST_PROP(n, mosi_pin),			\
		.miso_pin = DT_INST_PROP(n, miso_pin),			\
		.cs_pin = COND_CODE_1(DT_INST_NODE_HAS_PROP(n, cs_pin),	\
			(DT_INST_PROP(n, cs_pin)), (IOID_UNUSED))	\
	};								\
									\
	static struct spi_cc13xx_cc26xx_data				\
		spi_cc13xx_cc26xx_data_##n = {				\
		SPI_CONTEXT_INIT_LOCK(spi_cc13xx_cc26xx_data_##n, ctx),	  \
		SPI_CONTEXT_INIT_SYNC(spi_cc13xx_cc26xx_data_##n, ctx),	  \
	};								  \
									  \
	SPI_CC13XX_CC26XX_DEVICE_INIT(n);

DT_INST_FOREACH_STATUS_OKAY(SPI_CC13XX_CC26XX_INIT)
