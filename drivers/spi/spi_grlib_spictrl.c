/*
 * Copyright (c) 2025 Frontgrade Gaisler AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT gaisler_spictrl

#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/spi/rtio.h>

#define LOG_LEVEL CONFIG_SPI_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_spictrl);
#include "spi_context.h"

struct spictrl_regs {
	uint32_t capability;    /* 0x00 */
	uint32_t resv0[7];      /* 0x04-0x1c */
	uint32_t mode;          /* 0x20 */
	uint32_t event;         /* 0x24 */
	uint32_t mask;          /* 0x28 */
	uint32_t command;       /* 0x2c */
	uint32_t tx;            /* 0x30 */
	uint32_t rx;            /* 0x34 */
	uint32_t slvsel;        /* 0x38 */
	uint32_t aslvsel;       /* 0x3c */
};

/* Capability register */
#define SPICTRL_CAPABILITY_SSSZ_BIT     24
#define SPICTRL_CAPABILITY_ASELA_BIT    17
#define SPICTRL_CAPABILITY_SSEN_BIT     16
#define SPICTRL_CAPABILITY_FDEPTH_BIT   8

#define SPICTRL_CAPABILITY_SSSZ         (0xff << SPICTRL_CAPABILITY_SSSZ_BIT)
#define SPICTRL_CAPABILITY_ASELA        (1 << SPICTRL_CAPABILITY_ASELA_BIT)
#define SPICTRL_CAPABILITY_SSEN         (1 << SPICTRL_CAPABILITY_SSEN_BIT)
#define SPICTRL_CAPABILITY_FDEPTH       (0xff << SPICTRL_CAPABILITY_FDEPTH_BIT)

/* Mode register */
#define SPICTRL_MODE_LOOP_BIT   30
#define SPICTRL_MODE_CPOL_BIT   29
#define SPICTRL_MODE_CPHA_BIT   28
#define SPICTRL_MODE_DIV16_BIT  27
#define SPICTRL_MODE_REV_BIT    26
#define SPICTRL_MODE_MS_BIT     25
#define SPICTRL_MODE_EN_BIT     24
#define SPICTRL_MODE_LEN_BIT    20
#define SPICTRL_MODE_PM_BIT     16
#define SPICTRL_MODE_ASEL_BIT   14
#define SPICTRL_MODE_FACT_BIT   13
#define SPICTRL_MODE_CG_BIT      7
#define SPICTRL_MODE_ASELDEL_BIT 5
#define SPICTRL_MODE_TAC_BIT     4
#define SPICTRL_MODE_IGSEL_BIT   2

#define SPICTRL_MODE_LOOP       (1 << SPICTRL_MODE_LOOP_BIT)
#define SPICTRL_MODE_CPOL       (1 << SPICTRL_MODE_CPOL_BIT)
#define SPICTRL_MODE_CPHA       (1 << SPICTRL_MODE_CPHA_BIT)
#define SPICTRL_MODE_DIV16      (1 << SPICTRL_MODE_DIV16_BIT)
#define SPICTRL_MODE_REV        (1 << SPICTRL_MODE_REV_BIT)
#define SPICTRL_MODE_MS         (1 << SPICTRL_MODE_MS_BIT)
#define SPICTRL_MODE_EN         (1 << SPICTRL_MODE_EN_BIT)
#define SPICTRL_MODE_LEN        (0xf << SPICTRL_MODE_LEN_BIT)
#define SPICTRL_MODE_PM         (0xf << SPICTRL_MODE_PM_BIT)
#define SPICTRL_MODE_ASEL       (1 << SPICTRL_MODE_ASEL_BIT)
#define SPICTRL_MODE_FACT       (1 << SPICTRL_MODE_FACT_BIT)
#define SPICTRL_MODE_CG         (0x1f << SPICTRL_MODE_CG_BIT)
#define SPICTRL_MODE_ASELDEL    (0x3 << SPICTRL_MODE_ASELDEL_BIT)
#define SPICTRL_MODE_TAC        (1 << SPICTRL_MODE_TAC_BIT)
#define SPICTRL_MODE_IGSEL      (1 << SPICTRL_MODE_IGSEL_BIT)

/* Event register */
#define SPICTRL_EVENT_TIP_BIT   31
#define SPICTRL_EVENT_AT_BIT    15
#define SPICTRL_EVENT_LT_BIT    14
#define SPICTRL_EVENT_OV_BIT    12
#define SPICTRL_EVENT_UN_BIT    11
#define SPICTRL_EVENT_MME_BIT   10
#define SPICTRL_EVENT_NE_BIT    9
#define SPICTRL_EVENT_NF_BIT    8

#define SPICTRL_EVENT_TIP       (1 << SPICTRL_EVENT_TIP_BIT)
#define SPICTRL_EVENT_AT        (1 << SPICTRL_EVENT_AT_BIT)
#define SPICTRL_EVENT_LT        (1 << SPICTRL_EVENT_LT_BIT)
#define SPICTRL_EVENT_OV        (1 << SPICTRL_EVENT_OV_BIT)
#define SPICTRL_EVENT_UN        (1 << SPICTRL_EVENT_UN_BIT)
#define SPICTRL_EVENT_MME       (1 << SPICTRL_EVENT_MME_BIT)
#define SPICTRL_EVENT_NE        (1 << SPICTRL_EVENT_NE_BIT)
#define SPICTRL_EVENT_NF        (1 << SPICTRL_EVENT_NF_BIT)

/* Mask register */
#define SPICTRL_MASK_TIPE_BIT   31
#define SPICTRL_MASK_LTE_BIT    14
#define SPICTRL_MASK_OVE_BIT    12
#define SPICTRL_MASK_UNE_BIT    11
#define SPICTRL_MASK_MMEE_BIT   10
#define SPICTRL_MASK_NEE_BIT    9
#define SPICTRL_MASK_NFE_BIT    8

#define SPICTRL_MASK_TIPE       (1 << SPICTRL_MASK_TIPE_BIT)
#define SPICTRL_MASK_LTE        (1 << SPICTRL_MASK_LTE_BIT)
#define SPICTRL_MASK_OVE        (1 << SPICTRL_MASK_OVE_BIT)
#define SPICTRL_MASK_UNE        (1 << SPICTRL_MASK_UNE_BIT)
#define SPICTRL_MASK_MMEE       (1 << SPICTRL_MASK_MMEE_BIT)
#define SPICTRL_MASK_NEE        (1 << SPICTRL_MASK_NEE_BIT)
#define SPICTRL_MASK_NFE        (1 << SPICTRL_MASK_NFE_BIT)

#define SPICTRL_EVENT_CLEAR ( \
	SPICTRL_EVENT_LT | \
	SPICTRL_EVENT_OV | \
	SPICTRL_EVENT_UN | \
	SPICTRL_EVENT_MME \
)

#define SPI_DATA(dev) ((struct data *) ((dev)->data))

struct cfg {
	volatile struct spictrl_regs *regs;
	int interrupt;
};

struct data {
	struct spi_context ctx;
	struct k_sem rxavail;
	int fifo_depth;
	int cs_output;
};

/* return 0 if and only if freq Hz or lower is supported by hardware. */
static int validate_freq(unsigned int sysfreq, unsigned int freq)
{
	unsigned int lowest_freq_possible;

	/* Lowest possible when DIV16 is set and PM is 0xf */
	lowest_freq_possible = sysfreq / (16 * 4 * (0xf + 1));

	if (freq < lowest_freq_possible) {
		return 1;
	}
	return 0;
}

/*
 * Calculate mode word for as high frequency of SCK as possible but not higher
 * than requested frequency (freq).
 */
static uint32_t get_clkmagic(unsigned int sysfreq, unsigned int freq)
{
	uint32_t magic;
	uint32_t q;
	uint32_t div16;
	uint32_t pm;
	uint32_t fact;

	q = ((sysfreq / 2) + (freq - 1)) / freq;

	if (q > 16) {
		q = (q + (16 - 1)) / 16;
		div16 = 1;
	} else {
		div16 = 0;
	}

	if (q > 0xf) {
		/* FACT adds a factor /2 */
		fact = 0;
		q = (q + (2 - 1)) / 2;
	} else {
		fact = 1;
	}

	pm = q - 1;

	magic =
	    (pm << SPICTRL_MODE_PM_BIT) |
	    (div16 << SPICTRL_MODE_DIV16_BIT) | (fact << SPICTRL_MODE_FACT_BIT);

	return magic;
}

static int spi_config(
	const struct spi_config *config,
	volatile struct spictrl_regs *const regs,
	struct spi_context *ctx
)
{
	uint32_t mode = 0;

	if (spi_context_configured(ctx, config)) {
		return 0;
	}

	if (config->slave != 0) {
		LOG_ERR("More slaves than supported");
		return -ENOTSUP;
	}

	if (SPI_WORD_SIZE_GET(config->operation) != 8) {
		LOG_ERR("Word size must be 8");
		return -ENOTSUP;
	}

	if (config->operation & SPI_CS_ACTIVE_HIGH) {
		LOG_ERR("CS active high not supported");
		return -ENOTSUP;
	}

	if ((config->operation & SPI_LINES_MASK) != SPI_LINES_SINGLE) {
		LOG_ERR("Only supports single mode");
		return -ENOTSUP;
	}

	if (config->operation & SPI_TRANSFER_LSB) {
		LOG_ERR("LSB first not supported");
		return -ENOTSUP;
	}

	if (SPI_MODE_GET(config->operation) & SPI_MODE_CPOL) {
		mode |= SPICTRL_MODE_CPOL;
	}

	if (SPI_MODE_GET(config->operation) & SPI_MODE_CPHA) {
		mode |= SPICTRL_MODE_CPHA;
	}

	if (config->operation & SPI_OP_MODE_SLAVE) {
		LOG_ERR("Slave mode not supported");
		return -ENOTSUP;
	}

	if (SPI_MODE_GET(config->operation) & SPI_MODE_LOOP) {
		mode |= SPICTRL_MODE_LOOP;
	}

	if (validate_freq(sys_clock_hw_cycles_per_sec(), config->frequency)) {
		LOG_ERR("Frequency lower than supported");
		return -EINVAL;
	}

	mode |= SPICTRL_MODE_REV;
	mode |= SPICTRL_MODE_MS;
	mode |= SPICTRL_MODE_EN;
	mode |= 0x7 << SPICTRL_MODE_LEN_BIT;
	mode |= get_clkmagic(sys_clock_hw_cycles_per_sec(), config->frequency);
	regs->mode = mode;

	ctx->config = config;

	return 0;
}

static int transceive(const struct device *dev,
		      const struct spi_config *config,
		      const struct spi_buf_set *tx_bufs,
		      const struct spi_buf_set *rx_bufs)
{
	const struct cfg *const cfg = dev->config;
	volatile struct spictrl_regs *const regs = cfg->regs;
	struct data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	int rc;

	spi_context_lock(ctx, false, NULL, NULL, config);

	rc = spi_config(config, regs, ctx);
	if (rc) {
		LOG_ERR("%s: config", __func__);
		spi_context_release(ctx, rc);
		return rc;
	}

	spi_context_buffers_setup(ctx, tx_bufs, rx_bufs, 1);

	regs->slvsel &= ~(1U << data->cs_output);
	while (spi_context_tx_buf_on(ctx) || spi_context_rx_buf_on(ctx)) {
		size_t length;
		int outdex = 0;
		int index = 0;
		uint32_t event;
		uint32_t td;

		length = MAX(spi_context_total_tx_len(ctx), spi_context_total_rx_len(ctx));
		/* Initial fast fill */
		regs->mask |= SPICTRL_MASK_NEE;
		for (outdex = 0; (outdex <= data->fifo_depth) && (outdex < length); outdex++) {
			if (spi_context_tx_buf_on(ctx)) {
				td = *ctx->tx_buf;
				spi_context_update_tx(ctx, 1, 1);
			} else {
				td = 0;
			}
			regs->tx = td << 24;
		}

		while (1) {
			event = regs->event;
			while (event & SPICTRL_EVENT_NE) {
				uint32_t rd = regs->rx >> 16;

				if (spi_context_rx_on(ctx)) {
					*ctx->rx_buf = rd;
					spi_context_update_rx(ctx, 1, 1);
				}
				event = regs->event;
				index++;
			}

			while (
				(regs->event & SPICTRL_EVENT_NF) &&
				(outdex < length) &&
				(outdex < (index + data->fifo_depth))
			) {
				if (spi_context_tx_buf_on(ctx)) {
					td = *ctx->tx_buf;
					spi_context_update_tx(ctx, 1, 1);
				} else {
					td = 0;
				}
				regs->tx = td << 24;
				outdex++;
				event = regs->event;
			}

			if (index < length) {
				k_sem_take(&data->rxavail, K_FOREVER);
			} else {
				break;
			}
		}
	}
	regs->mask = 0;
	regs->slvsel |= 1U << data->cs_output;

	spi_context_complete(ctx, dev, 0);
	rc = spi_context_wait_for_completion(ctx);

	spi_context_release(ctx, rc);

	return 0;
}

#ifdef CONFIG_SPI_ASYNC
static int transceive_async(const struct device *dev,
			    const struct spi_config *config,
			    const struct spi_buf_set *tx_bufs,
			    const struct spi_buf_set *rx_bufs,
			    struct k_poll_signal *async)
{
	return -ENOTSUP;
}
#endif /* CONFIG_SPI_ASYNC */

static int release(const struct device *dev, const struct spi_config *config)
{
	struct data *data = dev->data;

	spi_context_unlock_unconditionally(&data->ctx);
	return 0;
}

static void spictrl_isr(const struct device *dev)
{
	const struct cfg *const cfg = dev->config;
	volatile struct spictrl_regs *const regs = cfg->regs;
	struct data *data = dev->data;

	if ((regs->mask & SPICTRL_MASK_NEE) == 0) {
		return;
	}
	k_sem_give(&data->rxavail);
}

static int init(const struct device *dev)
{
	const struct cfg *const cfg = dev->config;
	volatile struct spictrl_regs *const regs = cfg->regs;
	struct data *data = dev->data;

	data->fifo_depth = (
		(regs->capability & SPICTRL_CAPABILITY_FDEPTH) >>
		SPICTRL_CAPABILITY_FDEPTH_BIT
	);
	/* Mask all Interrupts. */
	regs->mask = 0;
	/* Disable core and select master mode. */
	regs->mode = SPICTRL_MODE_MS;
	/* Clear all events. */
	regs->event = SPICTRL_EVENT_CLEAR;
	/* LST bit is not used by driver so force it to zero. */
	regs->command = 0;

	irq_connect_dynamic(
		cfg->interrupt,
		0,
		(void (*)(const void *)) spictrl_isr,
		dev,
		0
	);
	irq_enable(cfg->interrupt);

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static DEVICE_API(spi, api) = {
	.transceive             = transceive,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async       = transceive_async,
#endif /* CONFIG_SPI_ASYNC */
#ifdef CONFIG_SPI_RTIO
	.iodev_submit = spi_rtio_iodev_default_submit,
#endif
	.release                = release,
};

#define SPI_INIT(n)	                                                \
	static const struct cfg cfg_##n = {                             \
		.regs           = (struct spictrl_regs *)               \
				  DT_INST_REG_ADDR(n),                  \
		.interrupt      = DT_INST_IRQN(n),                      \
	};                                                              \
	static struct data data_##n = {                                 \
		SPI_CONTEXT_INIT_LOCK(data_##n, ctx),                   \
		SPI_CONTEXT_INIT_SYNC(data_##n, ctx),                   \
		.rxavail = Z_SEM_INITIALIZER(data_##n.rxavail, 0, 1),   \
		.cs_output = DT_INST_PROP(n, cs_output),                \
	};                                                              \
	SPI_DEVICE_DT_INST_DEFINE(n,                                    \
			init,                                           \
			NULL,                                           \
			&data_##n,                                      \
			&cfg_##n,                                       \
			POST_KERNEL,                                    \
			CONFIG_SPI_INIT_PRIORITY,                       \
			&api);

DT_INST_FOREACH_STATUS_OKAY(SPI_INIT)
