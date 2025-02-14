/*
 * Copyright (C) Atmosic 2021-2024
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmosic_atm_spi

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_atm, CONFIG_SPI_LOG_LEVEL);

#include <zephyr/sys/sys_io.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/math_extras.h>
#ifdef CONFIG_PM
#include <zephyr/pm/pm.h>
#include <zephyr/pm/policy.h>
#endif
#include <soc.h>
#include <stdbool.h>
#include "at_clkrstgen.h"

#include "spi_context.h"
#include "arch.h"
#include "at_wrpr.h"
#include "at_pinmux.h"
#include "at_apb_spi_regs_core_macro.h"

#define SPI_WORD_SIZE 8


#define SPI_CLK at_clkrstgen_get_bp()
#define SPI_CLK_DIV(freq) ((DIV_ROUND_UP(SPI_CLK, freq) >> 1) - 1)
#define SPI_CLK_MIN (SPI_CLK / (1 << (SPI_TRANSACTION_SETUP__CLKDIV__WIDTH + 1)))
#define SPI_CLK_MAX (SPI_CLK >> 1)
#define DEV_CFG(dev) ((struct spi_atm_config *)(dev)->config)
#define DEV_DATA(dev) ((struct spi_atm_data *)(dev)->data)
#define SPI_OPCODE(d) ((d)->io[0].tx)
#define SPI_DATA_SIZE(d) ((d)->num_bytes)
#define SPI_DATA_LOWER(d)                                                                          \
	(((d)->io[1].tx << 0) | ((d)->io[2].tx << 8) | ((d)->io[3].tx << 16) |                     \
	 ((d)->io[4].tx << 24))
#define SPI_DATA_UPPER(d)                                                                          \
	(((d)->io[5].tx << 0) | ((d)->io[6].tx << 8) | ((d)->io[7].tx << 16) |                     \
	 ((d)->io[8].tx << 24))
#define SPI_DATA_LOWER_WIDTH                                                                       \
	((SPI_DATA_BYTES_LOWER__BYTE0__WIDTH + SPI_DATA_BYTES_LOWER__BYTE1__WIDTH +                \
	  SPI_DATA_BYTES_LOWER__BYTE2__WIDTH + SPI_DATA_BYTES_LOWER__BYTE3__WIDTH) >>              \
	 3)
#define SPI_DATA_UPPER_WIDTH                                                                       \
	((SPI_DATA_BYTES_UPPER__BYTE4__WIDTH + SPI_DATA_BYTES_UPPER__BYTE5__WIDTH +                \
	  SPI_DATA_BYTES_UPPER__BYTE6__WIDTH + SPI_DATA_BYTES_UPPER__BYTE7__WIDTH) >>              \
	 3)
#define SPI_OPCODE_WIDTH (SPI_TRANSACTION_SETUP__OPCODE__WIDTH >> 3)
#define SPI_DATA_WIDTH (SPI_DATA_LOWER_WIDTH + SPI_DATA_UPPER_WIDTH)
#define SPI_PAYLOAD_WIDTH (SPI_OPCODE_WIDTH + SPI_DATA_WIDTH)

struct spi_atm_data {
	struct spi_context ctx;
	struct shift {
		uint8_t tx;
		uint8_t *rx;
	} io[SPI_PAYLOAD_WIDTH];
	uint32_t num_bytes;
	bool read;
};

typedef void (*set_callback_t)(void);

struct spi_atm_config {
	int dummy_cycles;
	CMSDK_AT_APB_SPI_TypeDef *base;
	set_callback_t config_pins;
};

static int spi_atm_process_data(struct device const *dev)
{
	int ret = 0;
	struct spi_atm_data *data = DEV_DATA(dev);
	struct spi_atm_config const *config = DEV_CFG(dev);

	for (int i = 0; i < SPI_PAYLOAD_WIDTH; i++) {
		if (data->io[i].rx) {
			uint8_t val = 0;
			switch (i) {
			case 0:
				val = SPI_TRANSACTION_STATUS__OPCODE_STATUS__READ(
					config->base->TRANSACTION_STATUS);
				break;

			case 1:
				val = SPI_DATA_BYTES_LOWER__BYTE0__READ(
					config->base->DATA_BYTES_LOWER);
				break;

			case 2:
				val = SPI_DATA_BYTES_LOWER__BYTE1__READ(
					config->base->DATA_BYTES_LOWER);
				break;

			case 3:
				val = SPI_DATA_BYTES_LOWER__BYTE2__READ(
					config->base->DATA_BYTES_LOWER);
				break;

			case 4:
				val = SPI_DATA_BYTES_LOWER__BYTE3__READ(
					config->base->DATA_BYTES_LOWER);
				break;

			case 5:
				val = SPI_DATA_BYTES_UPPER__BYTE4__READ(
					config->base->DATA_BYTES_UPPER);
				break;

			case 6:
				val = SPI_DATA_BYTES_UPPER__BYTE5__READ(
					config->base->DATA_BYTES_UPPER);
				break;

			case 7:
				val = SPI_DATA_BYTES_UPPER__BYTE6__READ(
					config->base->DATA_BYTES_UPPER);
				break;

			case 8:
				val = SPI_DATA_BYTES_UPPER__BYTE7__READ(
					config->base->DATA_BYTES_UPPER);
				break;

			default:
				LOG_ERR("Invalid receive index. Received: %d", i);
				ret = -EINVAL;
			}

			if (!ret) {
				UNALIGNED_PUT(val, data->io[i].rx);
			}
		}
	}

	return ret;
}

static int spi_atm_execute_transaction(struct device const *dev, uint16_t clkdiv, uint8_t dcycles,
				       bool csn, bool loopback)
{
	struct spi_atm_data *data = DEV_DATA(dev);
	LOG_DBG("%s(%d): %#x %#x %#x %d %d %d %d %d", __func__, __LINE__, SPI_OPCODE(data),
		SPI_DATA_LOWER(data), SPI_DATA_UPPER(data), SPI_DATA_SIZE(data), clkdiv, dcycles,
		csn, loopback);
	uint32_t transaction = SPI_TRANSACTION_SETUP__DUMMY_CYCLES__WRITE(dcycles) |
			       SPI_TRANSACTION_SETUP__CSN_STAYS_LOW__WRITE(csn) |
			       SPI_TRANSACTION_SETUP__OPCODE__WRITE(SPI_OPCODE(data)) |
			       SPI_TRANSACTION_SETUP__CLKDIV__WRITE(clkdiv) |
			       SPI_TRANSACTION_SETUP__RWB__MASK |
			       SPI_TRANSACTION_SETUP__NUM_DATA_BYTES__WRITE(SPI_DATA_SIZE(data));
	if (loopback) {
		transaction |= SPI_TRANSACTION_SETUP__LOOPBACK__WRITE(1);
	}

	struct spi_atm_config const *config = DEV_CFG(dev);
	config->base->DATA_BYTES_LOWER = SPI_DATA_LOWER(data);
	config->base->DATA_BYTES_UPPER = SPI_DATA_UPPER(data);
	config->base->TRANSACTION_SETUP = transaction;
	config->base->TRANSACTION_SETUP |= SPI_TRANSACTION_SETUP__START__MASK;

	int timeout = CONFIG_SPI_ATM_TIMEOUT;
	while (config->base->TRANSACTION_STATUS & SPI_TRANSACTION_STATUS__RUNNING__MASK) {
		if (--timeout) {
			k_busy_wait(1);
		} else {
			config->base->TRANSACTION_SETUP = 0;
			LOG_ERR("SPI communication timed out: %#x",
				config->base->TRANSACTION_STATUS);
			return -EIO;
		}
	}

	if (data->read) {
		return spi_atm_process_data(dev);
	}

	return 0;
}

static int spi_atm_transfer(struct device const *dev, struct spi_config const *config)
{
	struct spi_atm_data *data = DEV_DATA(dev);
	int ret = 0;
	bool last = false;

	spi_context_lock(&data->ctx, 0, NULL, NULL, config);

	do {
		data->read = false;
		data->num_bytes = SPI_DATA_WIDTH;
		for (int i = 0; i < SPI_PAYLOAD_WIDTH; i++) {
			data->io[i].tx = 0;
			data->io[i].rx = NULL;
			if (last) {
				continue;
			}

			if (spi_context_tx_buf_on(&data->ctx)) {
				data->io[i].tx = UNALIGNED_GET(data->ctx.tx_buf);
			}
			spi_context_update_tx(&data->ctx, 1, 1);

			if (spi_context_rx_buf_on(&data->ctx)) {
				data->io[i].rx = data->ctx.rx_buf;
				data->read = true;
			}
			spi_context_update_rx(&data->ctx, 1, 1);

			if (!(spi_context_tx_on(&data->ctx) || spi_context_rx_on(&data->ctx))) {
				last = true;
				data->num_bytes = i;
			}
		}

		struct spi_atm_config const *aconfig = DEV_CFG(dev);
		uint8_t dcycles = (data->read) ? aconfig->dummy_cycles : 0;
		bool loopback = (config->operation & SPI_MODE_LOOP) ? true : false;
		ret = spi_atm_execute_transaction(dev, SPI_CLK_DIV(config->frequency), dcycles,
						  !last, loopback);
	} while (!ret && (spi_context_tx_on(&data->ctx) || spi_context_rx_on(&data->ctx)));

	spi_context_complete(&data->ctx, dev, 0);
	spi_context_unlock_unconditionally(&data->ctx);

	return ret;
}

static int spi_atm_transceive(struct device const *dev, struct spi_config const *config,
			      struct spi_buf_set const *tx_bufs, struct spi_buf_set const *rx_bufs)
{
	if (SPI_WORD_SIZE_GET(config->operation) != SPI_WORD_SIZE) {
		LOG_ERR("Invalid word size. Received: %d", SPI_WORD_SIZE_GET(config->operation));
		return -ENOTSUP;
	}

	if (config->operation & SPI_CS_ACTIVE_HIGH) {
		LOG_ERR("Active high CS not supported");
		return -ENOTSUP;
	}

	if ((config->operation & SPI_LINES_MASK) != SPI_LINES_SINGLE) {
		LOG_ERR("MISO lines not supported. Received: %d",
			(config->operation & SPI_LINES_MASK));
		return -ENOTSUP;
	}

	if (config->operation & SPI_OP_MODE_SLAVE) {
		LOG_ERR("Slave mode not supported");
		return -ENOTSUP;
	}

	if ((config->frequency < SPI_CLK_MIN) || (config->frequency > SPI_CLK_MAX)) {
		LOG_ERR("Frequency not supported. Received: %d", config->frequency);
		return -ENOTSUP;
	}

	struct spi_atm_data *data = DEV_DATA(dev);
	data->ctx.config = config;
	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, 1);
	return spi_atm_transfer(dev, config);
}

#ifdef CONFIG_SPI_ASYNC
static int spi_atm_transceive_async(struct device const *dev,
				    struct spi_config const *config,
				    struct spi_buf_set const *tx_bufs,
				    struct spi_buf_set const *rx_bufs,
				    spi_callback_t cb, void *userdata)
{
	return -ENOTSUP;
}
#endif /* CONFIG_SPI_ASYNC */

static int spi_atm_release(struct device const *dev, struct spi_config const *config)
{
	struct spi_atm_data *data = DEV_DATA(dev);

	if (!spi_context_configured(&data->ctx, config)) {
		return -EINVAL;
	}

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static struct spi_driver_api const spi_atm_driver_api = {
	.transceive = spi_atm_transceive,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_atm_transceive_async,
#endif /* CONFIG_SPI_ASYNC */
	.release = spi_atm_release,
};

#ifdef PSEQ_CTRL0__SPI_LATCH_OPEN__MASK
static void spi_pseq_latch_close(void)
{
	WRPR_CTRL_PUSH(CMSDK_PSEQ, WRPR_CTRL__CLK_ENABLE)
	{
		PSEQ_CTRL0__SPI_LATCH_OPEN__CLR(CMSDK_PSEQ->CTRL0);
	}
	WRPR_CTRL_POP();
}

#ifdef CONFIG_PM
static void notify_pm_state_exit(enum pm_state state)
{
	if (state == PM_STATE_SUSPEND_TO_RAM) {
		spi_pseq_latch_close();
	}
}

static struct pm_notifier notifier = {
	.state_entry = NULL,
	.state_exit = notify_pm_state_exit,
};
#endif
#endif

static int spi_atm_init(struct device const *dev)
{
	int err;
	struct spi_atm_config const *config = DEV_CFG(dev);
	struct spi_atm_data *data = DEV_DATA(dev);

	config->config_pins();
	err = spi_context_cs_configure_all(&data->ctx);
	if (err < 0) {
		return err;
	}
	spi_context_unlock_unconditionally(&data->ctx);

#ifdef PSEQ_CTRL0__SPI_LATCH_OPEN__MASK
	spi_pseq_latch_close();

#ifdef CONFIG_PM
	pm_notifier_register(&notifier);
#endif
#endif

	return 0;
}

#define SPI_SIG(n, sig) CONCAT(CONCAT(SPI, DT_INST_PROP(n, instance)), _##sig)
#define SPI_BASE(n) CONCAT(CMSDK_SPI, DT_INST_PROP(n, instance))
#define SPI_DEVICE_INIT(n)                                                                         \
	static void spi_atm_config_pins_##n(void)                                                  \
	{                                                                                          \
		/* Configure pinmux for the given intance */                                       \
		PIN_SELECT(DT_INST_PROP(n, cs_pin), SPI_SIG(n, CS));                               \
		PIN_SELECT(DT_INST_PROP(n, clk_pin), SPI_SIG(n, CLK));                             \
		IF_ENABLED(DT_INST_NODE_HAS_PROP(n, mosi_pin),					   \
			PIN_SELECT(DT_INST_PROP(n, mosi_pin), SPI_SIG(n, MOSI)));          	   \
		IF_ENABLED(DT_INST_NODE_HAS_PROP(n, miso_pin),					   \
			PIN_SELECT(DT_INST_PROP(n, miso_pin), SPI_SIG(n, MISO)));          	   \
		WRPR_CTRL_SET(SPI_BASE(n), WRPR_CTRL__CLK_ENABLE);                                 \
	}                                                                                          \
	static struct spi_atm_config const spi_atm_config_##n = {                                  \
		.base = SPI_BASE(n),                                                               \
		.config_pins = spi_atm_config_pins_##n,                                            \
		.dummy_cycles = DT_INST_PROP(n, dummy_cycles),                                     \
	};                                                                                         \
	static struct spi_atm_data spi_atm_data_##n = {						   \
		SPI_CONTEXT_INIT_LOCK(spi_atm_data_##n, ctx),	 				   \
		SPI_CONTEXT_INIT_SYNC(spi_atm_data_##n, ctx),					   \
	};											   \
	DEVICE_DT_INST_DEFINE(n, &spi_atm_init, NULL, &spi_atm_data_##n, &spi_atm_config_##n,      \
			      POST_KERNEL, CONFIG_SPI_INIT_PRIORITY, &spi_atm_driver_api);         \
	BUILD_ASSERT(SPI_BASE(n) == (CMSDK_AT_APB_SPI_TypeDef *)DT_REG_ADDR(DT_NODELABEL(          \
					    CONCAT(spi, DT_INST_PROP(n, instance)))));

DT_INST_FOREACH_STATUS_OKAY(SPI_DEVICE_INIT)
