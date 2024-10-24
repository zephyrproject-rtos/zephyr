/*
 * Copyright (c) 2024 Ambiq <www.ambiq.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ambiq_spid

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_ambiq_spid);

#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>
#include <zephyr/pm/device_runtime.h>

#include <stdlib.h>
#include <errno.h>
#include "spi_context.h"
#include <am_mcu_apollo.h>

#define AMBIQ_SPID_PWRCTRL_MAX_WAIT_US 5

typedef int (*ambiq_spi_pwr_func_t)(void);

struct spi_ambiq_config {
	const struct gpio_dt_spec int_gpios;
	uint32_t base;
	int size;
	const struct pinctrl_dev_config *pcfg;
	ambiq_spi_pwr_func_t pwr_func;
	void (*irq_config_func)(void);
};

struct spi_ambiq_data {
	struct spi_context ctx;
	am_hal_ios_config_t ios_cfg;
	void *ios_handler;
	int inst_idx;
	struct k_sem spim_wrcmp_sem;
};

#define AMBIQ_SPID_TX_BUFSIZE_MAX 1023
uint8_t ambiq_spid_sram_buffer[AMBIQ_SPID_TX_BUFSIZE_MAX];

#define AMBIQ_SPID_DUMMY_BYTE   0
#define AMBIQ_SPID_DUMMY_LENGTH 16
static uint8_t ambiq_spid_dummy_buffer[2][AMBIQ_SPID_DUMMY_LENGTH];

#define AMBIQ_SPID_WORD_SIZE 8

#define AMBIQ_SPID_FIFO_BASE   0x78
#define AMBIQ_SPID_FIFO_END    0x100
#define AMBIQ_SPID_FIFO_LENGTH (AMBIQ_SPID_FIFO_END - AMBIQ_SPID_FIFO_BASE)

#define AMBIQ_SPID_INT_ERR (AM_HAL_IOS_INT_FOVFL | AM_HAL_IOS_INT_FUNDFL | AM_HAL_IOS_INT_FRDERR)

#define AMBIQ_SPID_XCMP_INT                                                                        \
	(AM_HAL_IOS_INT_XCMPWR | AM_HAL_IOS_INT_XCMPWF | AM_HAL_IOS_INT_XCMPRR |                   \
	 AM_HAL_IOS_INT_XCMPRF)

static void spi_ambiq_reset(const struct device *dev)
{
	struct spi_ambiq_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;

	/* cancel timed out transaction */
	am_hal_ios_disable(data->ios_handler);
	/* NULL config to trigger reconfigure on next xfer */
	ctx->config = NULL;
	/* signal any thread waiting on sync semaphore */
	spi_context_complete(ctx, dev, -ETIMEDOUT);
}

static void spi_ambiq_inform(const struct device *dev)
{
	const struct spi_ambiq_config *cfg = dev->config;
	/* Inform the controller */
	gpio_pin_set_dt(&cfg->int_gpios, 1);
	gpio_pin_set_dt(&cfg->int_gpios, 0);
}

static void spi_ambiq_isr(const struct device *dev)
{
	uint32_t ui32Status;
	struct spi_ambiq_data *data = dev->data;

	am_hal_ios_interrupt_status_get(data->ios_handler, false, &ui32Status);
	am_hal_ios_interrupt_clear(data->ios_handler, ui32Status);
	if (ui32Status & AM_HAL_IOS_INT_XCMPWR) {
		k_sem_give(&data->spim_wrcmp_sem);
	}
}

static int spi_config(const struct device *dev, const struct spi_config *config)
{
	struct spi_ambiq_data *data = dev->data;
	struct spi_context *ctx = &(data->ctx);
	int ret = 0;

	data->ios_cfg.ui32InterfaceSelect = AM_HAL_IOS_USE_SPI;

	if (spi_context_configured(ctx, config)) {
		/* Already configured. No need to do it again. */
		return 0;
	}

	if (SPI_WORD_SIZE_GET(config->operation) != AMBIQ_SPID_WORD_SIZE) {
		LOG_ERR("Word size must be %d", AMBIQ_SPID_WORD_SIZE);
		return -ENOTSUP;
	}

	if ((config->operation & SPI_LINES_MASK) != SPI_LINES_SINGLE) {
		LOG_ERR("Only supports single mode");
		return -ENOTSUP;
	}

	if (config->operation & SPI_LOCK_ON) {
		LOG_ERR("Lock On not supported");
		return -ENOTSUP;
	}

	if (config->operation & SPI_TRANSFER_LSB) {
		LOG_ERR("LSB first not supported");
		return -ENOTSUP;
	}

	if (config->operation & SPI_MODE_CPOL) {
		if (config->operation & SPI_MODE_CPHA) {
			data->ios_cfg.ui32InterfaceSelect |= AM_HAL_IOS_SPIMODE_3;
		} else {
			data->ios_cfg.ui32InterfaceSelect |= AM_HAL_IOS_SPIMODE_2;
		}
	} else {
		if (config->operation & SPI_MODE_CPHA) {
			data->ios_cfg.ui32InterfaceSelect |= AM_HAL_IOS_SPIMODE_1;
		} else {
			data->ios_cfg.ui32InterfaceSelect |= AM_HAL_IOS_SPIMODE_0;
		}
	}

	if (config->operation & SPI_OP_MODE_MASTER) {
		LOG_ERR("Controller mode not supported");
		return -ENOTSUP;
	}

	if (config->operation & SPI_MODE_LOOP) {
		LOG_ERR("Loopback mode not supported");
		return -ENOTSUP;
	}

	if (spi_cs_is_gpio(config)) {
		LOG_ERR("CS control via GPIO is not supported");
		return -EINVAL;
	}

	/* Eliminate the "read-only" section, so an external controller can use the
	 * entire "direct write" section.
	 */
	data->ios_cfg.ui32ROBase = AMBIQ_SPID_FIFO_BASE;
	/* Making the "FIFO" section as big as possible. */
	data->ios_cfg.ui32FIFOBase = AMBIQ_SPID_FIFO_BASE;
	/* We don't need any RAM space, so extend the FIFO all the way to the end
	 * of the LRAM.
	 */
	data->ios_cfg.ui32RAMBase = AMBIQ_SPID_FIFO_END;
	/* FIFO Threshold - set to half the size */
	data->ios_cfg.ui32FIFOThreshold = AMBIQ_SPID_FIFO_LENGTH >> 1;

	data->ios_cfg.pui8SRAMBuffer = ambiq_spid_sram_buffer,
	data->ios_cfg.ui32SRAMBufferCap = AMBIQ_SPID_TX_BUFSIZE_MAX,

	ctx->config = config;

	ret = am_hal_ios_configure(data->ios_handler, &data->ios_cfg);

	return ret;
}

static int spi_ambiq_xfer(const struct device *dev, const struct spi_config *config)
{
	struct spi_ambiq_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	int ret = 0;
	uint32_t chunk, num_written, num_read, used_space = 0;

	while (1) {
		/* First send out all data */
		if (spi_context_tx_on(ctx)) {
			spi_ambiq_inform(dev);
			chunk = (ctx->tx_len > AMBIQ_SPID_TX_BUFSIZE_MAX)
					? AMBIQ_SPID_TX_BUFSIZE_MAX
					: ctx->tx_len;
			am_hal_ios_fifo_space_used(data->ios_handler, &used_space);
			/* Controller done reading the last block signalled
			 * Check if any more data available
			 */
			if (!used_space) {
				if (ctx->tx_buf) {
					/*  Copy data into FIFO */
					ret = am_hal_ios_fifo_write(data->ios_handler,
								    (uint8_t *)ctx->tx_buf, chunk,
								    &num_written);
				} else {
					uint32_t dummy_written = 0;

					num_written = 0;
					/*  Copy dummy into FIFO */
					while (chunk) {
						uint32_t size = (chunk > AMBIQ_SPID_DUMMY_LENGTH)
									? AMBIQ_SPID_DUMMY_LENGTH
									: chunk;
						ret = am_hal_ios_fifo_write(
							data->ios_handler,
							ambiq_spid_dummy_buffer[0], size,
							&dummy_written);
						num_written += dummy_written;
						chunk -= dummy_written;
					}
				}
				if (ret != 0) {
					LOG_ERR("SPID write error: %d", ret);
					goto end;
				}
				spi_context_update_tx(ctx, 1, num_written);
			}
		} else if (spi_context_rx_on(ctx)) {
			/* Wait for controller write complete interrupt */
			(void)k_sem_take(&data->spim_wrcmp_sem, K_FOREVER);
			/* Read out the first byte as packet length */
			num_read = am_hal_ios_pui8LRAM[0];
			uint32_t size, offset = 0;

			while (spi_context_rx_on(ctx)) {
				/* There's no data in the LRAM any more */
				if (!num_read) {
					spi_ambiq_inform(dev);
					goto end;
				}
				if (ctx->rx_buf) {
					size = MIN(num_read, ctx->rx_len);
					/* Read data from LRAM */
					memcpy(ctx->rx_buf,
					       (uint8_t *)&am_hal_ios_pui8LRAM[1 + offset], size);
				} else {
					size = MIN(num_read, AMBIQ_SPID_DUMMY_LENGTH);
					/*  Consume data from LRAM */
					memcpy(ambiq_spid_dummy_buffer[1],
					       (uint8_t *)&am_hal_ios_pui8LRAM[1 + offset], size);
				}
				num_read -= size;
				offset += size;
				spi_context_update_rx(ctx, 1, size);
			}
		} else {
			break;
		}
	}

end:
	if (ret != 0) {
		spi_ambiq_reset(dev);
	} else {
		spi_context_complete(ctx, dev, ret);
	}

	return ret;
}

static int spi_ambiq_transceive(const struct device *dev, const struct spi_config *config,
				const struct spi_buf_set *tx_bufs,
				const struct spi_buf_set *rx_bufs)
{
	struct spi_ambiq_data *data = dev->data;
	int ret;

	if (!tx_bufs && !rx_bufs) {
		return 0;
	}

	ret = pm_device_runtime_get(dev);
	if (ret < 0) {
		LOG_ERR("pm_device_runtime_get failed: %d", ret);
	}

	/* context setup */
	spi_context_lock(&data->ctx, false, NULL, NULL, config);

	ret = spi_config(dev, config);
	if (ret) {
		spi_context_release(&data->ctx, ret);
		return ret;
	}

	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, 1);

	ret = spi_ambiq_xfer(dev, config);

	spi_context_release(&data->ctx, ret);

	/* Use async put to avoid useless device suspension/resumption
	 * when doing consecutive transmission.
	 */
	ret = pm_device_runtime_put_async(dev, K_MSEC(2));
	if (ret < 0) {
		LOG_ERR("pm_device_runtime_put failed: %d", ret);
	}

	return ret;
}

static int spi_ambiq_release(const struct device *dev, const struct spi_config *config)
{
	struct spi_ambiq_data *data = dev->data;

	if (!spi_context_configured(&data->ctx, config)) {
		return -EINVAL;
	}

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static const struct spi_driver_api spi_ambiq_driver_api = {
	.transceive = spi_ambiq_transceive,
	.release = spi_ambiq_release,
};

static int spi_ambiq_init(const struct device *dev)
{
	struct spi_ambiq_data *data = dev->data;
	const struct spi_ambiq_config *cfg = dev->config;
	int ret = 0;

	if (AM_HAL_STATUS_SUCCESS !=
	    am_hal_ios_initialize((cfg->base - IOSLAVE_BASE) / cfg->size, &data->ios_handler)) {
		LOG_ERR("Fail to initialize SPID\n");
		return -ENXIO;
	}

	ret = cfg->pwr_func();

	ret |= pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Fail to config SPID pins\n");
		goto end;
	}

	memset(ambiq_spid_dummy_buffer[0], AMBIQ_SPID_DUMMY_BYTE, AMBIQ_SPID_DUMMY_LENGTH);

	am_hal_ios_interrupt_clear(data->ios_handler, AM_HAL_IOS_INT_ALL);
	am_hal_ios_interrupt_enable(data->ios_handler, AMBIQ_SPID_INT_ERR | AM_HAL_IOS_INT_IOINTW |
							       AMBIQ_SPID_XCMP_INT);
	cfg->irq_config_func();

end:
	if (ret < 0) {
		am_hal_ios_uninitialize(data->ios_handler);
	} else {
		spi_context_unlock_unconditionally(&data->ctx);
	}
	return ret;
}

#ifdef CONFIG_PM_DEVICE
static int spi_ambiq_pm_action(const struct device *dev, enum pm_device_action action)
{
	struct spi_ambiq_data *data = dev->data;
	uint32_t ret;
	am_hal_sysctrl_power_state_e status;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		status = AM_HAL_SYSCTRL_WAKE;
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		status = AM_HAL_SYSCTRL_DEEPSLEEP;
		break;
	default:
		return -ENOTSUP;
	}

	ret = am_hal_ios_power_ctrl(data->ios_handler, status, true);
	if (ret != AM_HAL_STATUS_SUCCESS) {
		LOG_ERR("am_hal_ios_power_ctrl failed: %d", ret);
		return -EPERM;
	} else {
		return 0;
	}
}
#endif /* CONFIG_PM_DEVICE */

#define AMBIQ_SPID_INIT(n)                                                                         \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static int pwr_on_ambiq_spi_##n(void)                                                      \
	{                                                                                          \
		uint32_t addr = DT_REG_ADDR(DT_INST_PHANDLE(n, ambiq_pwrcfg)) +                    \
				DT_INST_PHA(n, ambiq_pwrcfg, offset);                              \
		sys_write32((sys_read32(addr) | DT_INST_PHA(n, ambiq_pwrcfg, mask)), addr);        \
		k_busy_wait(AMBIQ_SPID_PWRCTRL_MAX_WAIT_US);                                       \
		return 0;                                                                          \
	}                                                                                          \
	static void spi_irq_config_func_##n(void)                                                  \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), spi_ambiq_isr,              \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	};                                                                                         \
	static struct spi_ambiq_data spi_ambiq_data##n = {                                         \
		SPI_CONTEXT_INIT_LOCK(spi_ambiq_data##n, ctx),                                     \
		SPI_CONTEXT_INIT_SYNC(spi_ambiq_data##n, ctx),                                     \
		.spim_wrcmp_sem = Z_SEM_INITIALIZER(spi_ambiq_data##n.spim_wrcmp_sem, 0, 1),       \
		.inst_idx = n};                                                                    \
	static const struct spi_ambiq_config spi_ambiq_config##n = {                               \
		.int_gpios = GPIO_DT_SPEC_INST_GET(n, int_gpios),                                  \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.size = DT_INST_REG_SIZE(n),                                                       \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.irq_config_func = spi_irq_config_func_##n,                                        \
		.pwr_func = pwr_on_ambiq_spi_##n};                                                 \
	PM_DEVICE_DT_INST_DEFINE(n, spi_ambiq_pm_action);                                          \
	DEVICE_DT_INST_DEFINE(n, spi_ambiq_init, PM_DEVICE_DT_INST_GET(n), &spi_ambiq_data##n,     \
			      &spi_ambiq_config##n, POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,         \
			      &spi_ambiq_driver_api);

DT_INST_FOREACH_STATUS_OKAY(AMBIQ_SPID_INIT)
