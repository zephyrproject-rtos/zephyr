/*
 * Copyright (c) 2019 Brett Witherspoon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_cc13xx_cc26xx_spi

#define LOG_LEVEL CONFIG_SPI_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_cc13xx_cc26xx);

#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>

#include <driverlib/prcm.h>
#include <driverlib/ssi.h>

#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC26X2.h>

#include "spi_context.h"

struct spi_cc13xx_cc26xx_config {
	uint32_t base;
	const struct pinctrl_dev_config *pcfg;
};

struct spi_cc13xx_cc26xx_data {
	struct spi_context ctx;
};

#define CPU_FREQ DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency)

static int spi_cc13xx_cc26xx_configure(const struct device *dev,
				       const struct spi_config *config)
{
	const struct spi_cc13xx_cc26xx_config *cfg = dev->config;
	struct spi_cc13xx_cc26xx_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	uint32_t prot;
	int ret;

	if (spi_context_configured(ctx, config)) {
		return 0;
	}

	if (config->operation & SPI_HALF_DUPLEX) {
		LOG_ERR("Half-duplex not supported");
		return -ENOTSUP;
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

	if (IS_ENABLED(CONFIG_SPI_EXTENDED_MODES) &&
	    (config->operation & SPI_LINES_MASK) != SPI_LINES_SINGLE) {
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

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("applying SPI pinctrl state failed");
		return ret;
	}

	ctx->config = config;

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
	const struct spi_cc13xx_cc26xx_config *cfg = dev->config;
	struct spi_cc13xx_cc26xx_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	uint32_t txd, rxd;
	int err;

	spi_context_lock(ctx, false, NULL, NULL, config);
	pm_policy_state_lock_get(PM_STATE_STANDBY, PM_ALL_SUBSTATES);

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
	pm_policy_state_lock_put(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
	spi_context_release(ctx, err);
	return err;
}

static int spi_cc13xx_cc26xx_release(const struct device *dev,
				     const struct spi_config *config)
{
	const struct spi_cc13xx_cc26xx_config *cfg = dev->config;
	struct spi_cc13xx_cc26xx_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;

	if (!spi_context_configured(ctx, config)) {
		return -EINVAL;
	}

	if (SSIBusy(cfg->base)) {
		return -EBUSY;
	}

	spi_context_unlock_unconditionally(ctx);

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int spi_cc13xx_cc26xx_pm_action(const struct device *dev,
				       enum pm_device_action action)
{
	const struct spi_cc13xx_cc26xx_config *config = dev->config;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		if (config->base == DT_INST_REG_ADDR(0)) {
			Power_setDependency(PowerCC26XX_PERIPH_SSI0);
		} else {
			Power_setDependency(PowerCC26XX_PERIPH_SSI1);
		}
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		SSIDisable(config->base);
		/*
		 * Release power dependency
		 */
		if (config->base == DT_INST_REG_ADDR(0)) {
			Power_releaseDependency(PowerCC26XX_PERIPH_SSI0);
		} else {
			Power_releaseDependency(PowerCC26XX_PERIPH_SSI1);
		}
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
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
	} while (false)
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
		while (PRCMPowerDomainsAllOn(domain) !=			  \
			PRCM_DOMAIN_POWER_ON) {				  \
			continue;					  \
		}							  \
	} while (false)
#endif

#define SPI_CC13XX_CC26XX_DEVICE_INIT(n)				    \
	PM_DEVICE_DT_INST_DEFINE(n, spi_cc13xx_cc26xx_pm_action);	    \
									    \
	DEVICE_DT_INST_DEFINE(n,					    \
		spi_cc13xx_cc26xx_init_##n,				    \
		PM_DEVICE_DT_INST_GET(n),				    \
		&spi_cc13xx_cc26xx_data_##n, &spi_cc13xx_cc26xx_config_##n, \
		POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,			    \
		&spi_cc13xx_cc26xx_driver_api)

#define SPI_CC13XX_CC26XX_INIT_FUNC(n)						\
	static int spi_cc13xx_cc26xx_init_##n(const struct device *dev)		\
	{									\
		struct spi_cc13xx_cc26xx_data *data = dev->data;		\
		int err;							\
		SPI_CC13XX_CC26XX_POWER_SPI(n);					\
										\
		err = spi_context_cs_configure_all(&data->ctx);			\
		if (err < 0) {							\
			return err;						\
		}								\
										\
		spi_context_unlock_unconditionally(&data->ctx);			\
										\
		return 0;							\
	}

#define SPI_CC13XX_CC26XX_INIT(n)					\
	PINCTRL_DT_INST_DEFINE(n);	\
	SPI_CC13XX_CC26XX_INIT_FUNC(n)					\
									\
	static const struct spi_cc13xx_cc26xx_config			\
		spi_cc13xx_cc26xx_config_##n = {			\
		.base = DT_INST_REG_ADDR(n),				\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n) \
	};								\
									\
	static struct spi_cc13xx_cc26xx_data				\
		spi_cc13xx_cc26xx_data_##n = {				\
		SPI_CONTEXT_INIT_LOCK(spi_cc13xx_cc26xx_data_##n, ctx),	\
		SPI_CONTEXT_INIT_SYNC(spi_cc13xx_cc26xx_data_##n, ctx),	\
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(n), ctx)	\
	};								\
									\
	SPI_CC13XX_CC26XX_DEVICE_INIT(n);

DT_INST_FOREACH_STATUS_OKAY(SPI_CC13XX_CC26XX_INIT)
