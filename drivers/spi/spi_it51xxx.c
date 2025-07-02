/*
 * Copyright (c) 2025 ITE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ite_it51xxx_spi

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_it51xxx, CONFIG_SPI_LOG_LEVEL);

#include <zephyr/irq.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/interrupt-controller/ite-it51xxx-intc.h>
#include <zephyr/pm/policy.h>
#include <soc.h>

#include "spi_context.h"

#define SPI_CHIP_SELECT_COUNT 2

/* IT51xxx SSPI Registers Definition */
#define SPI00_DATA     0x00
#define SPI01_CTRL1    0x01
#define CLOCK_POLARTY  BIT(6)
#define CLOCK_PHASE    BIT(5)
#define SSCK_FREQ_MASK GENMASK(4, 2)
#define INTERRUPT_EN   BIT(1)

#define SPI02_CTRL2        0x02
#define READ_CYCLE         BIT(2)
#define BLOCKING_SELECTION BIT(1)

#define SPI03_STATUS         0x03
#define DEVICE_BUSY          BIT(6)
#define SPI_TRANSMISSION_END BIT(5)
#define CH0_START            BIT(4)
#define CH1_START            BIT(3)
#define TRANSFER_IN_PROGRESS BIT(2)
#define TRANSFER_END         BIT(1)
#define SPI_BUS_BUSY         BIT(0)

#define SPI04_CTRL3       0x04
#define BYTE_DONE_INT_STS BIT(4)

#define SPI05_CHAIN_CTRL           0x05
#define PLL_CLOCK_SOURCE_SELECTION BIT(6)

struct spi_it51xxx_config {
	mm_reg_t base;
	const struct pinctrl_dev_config *pcfg;

	const struct device *clk_dev;
	struct ite_clk_cfg clk_cfg;

	void (*irq_config_func)(void);

	int irq_flags;
	uint8_t irq_no;
};

struct spi_it51xxx_data {
	struct spi_context ctx;
};

static inline int spi_it51xxx_set_freq(const struct device *dev, const uint32_t frequency)
{
	const struct spi_it51xxx_config *cfg = dev->config;
	const uint8_t freq_pll_div[8] = {2, 4, 6, 8, 10, 12, 14, 1};
	const uint8_t freq_ec_div[8] = {2, 4, 6, 8, 10, 12, 14, 16};
	uint32_t clk_pll;
	uint8_t reg_val;
	uint8_t divisor;
	int ret;

	ret = clock_control_get_rate(cfg->clk_dev, (clock_control_subsys_t)&cfg->clk_cfg, &clk_pll);
	if (ret) {
		LOG_WRN("failed to get pll frequency %d", ret);
		return ret;
	}

	for (uint8_t i = 0; i < ARRAY_SIZE(freq_pll_div); i++) {
		if (frequency == (clk_pll / freq_pll_div[i])) {
			/* select pll frequency as clock source */
			sys_write8(sys_read8(cfg->base + SPI05_CHAIN_CTRL) |
					   PLL_CLOCK_SOURCE_SELECTION,
				   cfg->base + SPI05_CHAIN_CTRL);
			divisor = i;
			LOG_DBG("freq: pll %dHz, ssck %dHz", clk_pll, frequency);
			goto out;
		}
	}

	for (uint8_t i = 0; i < ARRAY_SIZE(freq_ec_div); i++) {
		if (frequency == (IT51XXX_EC_FREQ / freq_ec_div[i])) {
			/* select ec frequency as clock source */
			sys_write8(sys_read8(cfg->base + SPI05_CHAIN_CTRL) &
					   ~PLL_CLOCK_SOURCE_SELECTION,
				   cfg->base + SPI05_CHAIN_CTRL);
			divisor = i;
			LOG_DBG("freq: ec %dHz, ssck %dHz", IT51XXX_EC_FREQ, frequency);
			goto out;
		}
	}

	LOG_ERR("unknown frequency %dHz, pll %dHz, ec %dHz", frequency, clk_pll, IT51XXX_EC_FREQ);
	return -ENOTSUP;

out:
	reg_val = sys_read8(cfg->base + SPI01_CTRL1) & ~SSCK_FREQ_MASK;
	reg_val |= FIELD_PREP(SSCK_FREQ_MASK, divisor);
	sys_write8(reg_val, cfg->base + SPI01_CTRL1);

	return 0;
}

static int spi_it51xxx_configure(const struct device *dev, const struct spi_config *spi_cfg)
{
	const struct spi_it51xxx_config *cfg = dev->config;
	struct spi_it51xxx_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	int ret;
	uint8_t reg_val;

	if (spi_cfg->slave > (SPI_CHIP_SELECT_COUNT - 1)) {
		LOG_ERR("slave %d is greater than %d", spi_cfg->slave, SPI_CHIP_SELECT_COUNT - 1);
		return -EINVAL;
	}

	LOG_DBG("chip select: %d, operation: 0x%x", spi_cfg->slave, spi_cfg->operation);

	if (SPI_OP_MODE_GET(spi_cfg->operation) == SPI_OP_MODE_SLAVE) {
		LOG_ERR("unsupported spi slave mode");
		return -ENOTSUP;
	}

	if (SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_LOOP) {
		LOG_ERR("unsupported loopback mode");
		return -ENOTSUP;
	}

	reg_val = sys_read8(cfg->base + SPI01_CTRL1);
	if (SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_CPHA) {
		reg_val |= CLOCK_PHASE;
	} else {
		reg_val &= ~CLOCK_PHASE;
	}

	if (SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_CPOL) {
		reg_val |= CLOCK_POLARTY;
	} else {
		reg_val &= ~CLOCK_POLARTY;
	}
	sys_write8(reg_val, cfg->base + SPI01_CTRL1);

	if (SPI_WORD_SIZE_GET(spi_cfg->operation) != 8) {
		return -ENOTSUP;
	}

	if (IS_ENABLED(CONFIG_SPI_EXTENDED_MODES) &&
	    (spi_cfg->operation & SPI_LINES_MASK) != SPI_LINES_SINGLE) {
		LOG_ERR("only single line mode is supported");
		return -EINVAL;
	}

	ret = spi_it51xxx_set_freq(dev, spi_cfg->frequency);
	if (ret) {
		return ret;
	}

	/* select non-blocking mode */
	sys_write8(sys_read8(cfg->base + SPI02_CTRL2) & ~BLOCKING_SELECTION,
		   cfg->base + SPI02_CTRL2);

	ctx->config = spi_cfg;

	return 0;
}

static inline void spi_it51xxx_tx(const struct device *dev)
{
	const struct spi_it51xxx_config *cfg = dev->config;
	struct spi_it51xxx_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;

	sys_write8(sys_read8(cfg->base + SPI02_CTRL2) & ~READ_CYCLE, cfg->base + SPI02_CTRL2);

	sys_write8(ctx->tx_buf[0], cfg->base + SPI00_DATA);
	sys_write8(ctx->config->slave ? CH1_START : CH0_START, cfg->base + SPI03_STATUS);
}

static inline void spi_it51xxx_rx(const struct device *dev)
{
	const struct spi_it51xxx_config *cfg = dev->config;
	struct spi_it51xxx_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;

	sys_write8(sys_read8(cfg->base + SPI02_CTRL2) | READ_CYCLE, cfg->base + SPI02_CTRL2);
	sys_write8(ctx->config->slave ? CH1_START : CH0_START, cfg->base + SPI03_STATUS);
}

static inline bool spi_it51xxx_xfer_done(struct spi_context *ctx)
{
	return !spi_context_tx_buf_on(ctx) && !spi_context_rx_buf_on(ctx);
}

static void spi_it51xxx_next_xfer(const struct device *dev)
{
	const struct spi_it51xxx_config *cfg = dev->config;
	struct spi_it51xxx_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;

	if (spi_it51xxx_xfer_done(ctx)) {
		sys_write8(SPI_TRANSMISSION_END, cfg->base + SPI03_STATUS);
		return;
	}

	if (!spi_context_tx_on(ctx)) {
		spi_it51xxx_rx(dev);
	} else {
		spi_it51xxx_tx(dev);
	}
}

static int transceive(const struct device *dev, const struct spi_config *config,
		      const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs,
		      bool asynchronous, spi_callback_t cb, void *userdata)
{
	struct spi_it51xxx_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	int ret;

	spi_context_lock(ctx, asynchronous, cb, userdata, config);

	/* configure spi */
	ret = spi_it51xxx_configure(dev, config);
	if (ret) {
		goto out;
	}

	spi_context_buffers_setup(ctx, tx_bufs, rx_bufs, 1);
	pm_policy_state_lock_get(PM_STATE_STANDBY, PM_ALL_SUBSTATES);

	LOG_DBG("tx/rx: %d/%d", spi_context_total_tx_len(ctx), spi_context_total_rx_len(ctx));

	spi_it51xxx_next_xfer(dev);
	ret = spi_context_wait_for_completion(ctx);

	pm_policy_state_lock_put(PM_STATE_STANDBY, PM_ALL_SUBSTATES);

out:
	spi_context_release(ctx, ret);

	return ret;
}

static int it51xxx_transceive(const struct device *dev, const struct spi_config *config,
			      const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs)
{
	return transceive(dev, config, tx_bufs, rx_bufs, false, NULL, NULL);
}

#ifdef CONFIG_SPI_ASYNC
static int it51xxx_transceive_async(const struct device *dev, const struct spi_config *config,
				    const struct spi_buf_set *tx_bufs,
				    const struct spi_buf_set *rx_bufs, spi_callback_t cb,
				    void *userdata)
{
	return transceive(dev, config, tx_bufs, rx_bufs, true, cb, userdata);
}
#endif /* CONFIG_SPI_ASYNC */

static int it51xxx_release(const struct device *dev, const struct spi_config *config)
{
	ARG_UNUSED(config);

	struct spi_it51xxx_data *data = dev->data;

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static inline bool is_read_cycle(const struct device *dev)
{
	const struct spi_it51xxx_config *cfg = dev->config;

	return sys_read8(cfg->base + SPI02_CTRL2) & READ_CYCLE;
}

static void it51xxx_spi_byte_done_handle(const struct device *dev)
{
	const struct spi_it51xxx_config *cfg = dev->config;
	struct spi_it51xxx_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;

	if (is_read_cycle(dev)) {
		*ctx->rx_buf = sys_read8(cfg->base + SPI00_DATA);
		spi_context_update_rx(ctx, 1, 1);
	} else {
		spi_context_update_tx(ctx, 1, 1);
	}

	/* it51xxx spi driver accommodates two scenarios: when spi rx buffer
	 * either exclude tx data pointer and length, or include them but with
	 * null pointer.
	 */
	if (!ctx->rx_buf) {
		spi_context_update_rx(ctx, 1, ctx->rx_len);
	}

	spi_it51xxx_next_xfer(dev);
}

static void it51xxx_spi_isr(const void *arg)
{
	const struct device *dev = arg;
	const struct spi_it51xxx_config *cfg = dev->config;
	struct spi_it51xxx_data *data = dev->data;
	uint8_t int_sts;

	int_sts = sys_read8(cfg->base + SPI03_STATUS);
	LOG_DBG("isr: status 0x%x", int_sts);

	if (int_sts & DEVICE_BUSY) {
		LOG_DBG("isr: device is busy");
	}

	if (int_sts & TRANSFER_IN_PROGRESS) {
		LOG_DBG("isr: transfer is in progress");
	}

	if (int_sts & SPI_BUS_BUSY) {
		LOG_DBG("isr: spi bus is busy");
	}

	if (int_sts & TRANSFER_END) {
		LOG_DBG("isr: transaction finished");
		sys_write8(TRANSFER_END, cfg->base + SPI03_STATUS);
		spi_context_complete(&data->ctx, dev, 0);
	}

	int_sts = sys_read8(cfg->base + SPI04_CTRL3);
	if (int_sts & BYTE_DONE_INT_STS) {
		LOG_DBG("isr: byte transfer is done");
		sys_write8(int_sts, cfg->base + SPI04_CTRL3);
		it51xxx_spi_byte_done_handle(dev);
	}
}

static int spi_it51xxx_init(const struct device *dev)
{
	const struct spi_it51xxx_config *cfg = dev->config;
	struct spi_it51xxx_data *data = dev->data;
	int ret;

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret) {
		LOG_ERR("failed to set default pinctrl, ret %d", ret);
		return ret;
	}

	ret = clock_control_on(cfg->clk_dev, (clock_control_subsys_t *)&cfg->clk_cfg);
	if (ret) {
		LOG_ERR("failed to turn on spi clock %d", ret);
		return ret;
	}

	if (cfg->irq_flags != IRQ_TYPE_EDGE_RISING) {
		LOG_ERR("only supports rising-edge-triggered");
		return -ENOTSUP;
	}
	ite_intc_irq_polarity_set(cfg->irq_no, cfg->irq_flags);

	/* write 1 clear interrupt status and enable interrupt */
	sys_write8(sys_read8(cfg->base + SPI03_STATUS), cfg->base + SPI03_STATUS);
	sys_write8(BYTE_DONE_INT_STS, cfg->base + SPI04_CTRL3);
	sys_write8(sys_read8(cfg->base + SPI01_CTRL1) | INTERRUPT_EN, cfg->base + SPI01_CTRL1);

	cfg->irq_config_func();

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static DEVICE_API(spi, spi_it51xxx_driver_api) = {
	.transceive = it51xxx_transceive,
	.release = it51xxx_release,

#ifdef CONFIG_SPI_ASYNC
	.transceive_async = it51xxx_transceive_async,
#endif
};

#define SPI_IT51XXX_INIT(n)                                                                        \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static void it51xxx_spi_config_func_##n(void)                                              \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), 0, it51xxx_spi_isr, DEVICE_DT_INST_GET(n), 0);        \
		irq_enable(DT_INST_IRQN(n));                                                       \
	};                                                                                         \
	static const struct spi_it51xxx_config spi_it51xxx_cfg_##n = {                             \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.irq_config_func = it51xxx_spi_config_func_##n,                                    \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.clk_dev = DEVICE_DT_GET(DT_INST_PHANDLE(n, clocks)),                              \
		.clk_cfg =                                                                         \
			{                                                                          \
				.ctrl = DT_INST_CLOCKS_CELL(n, ctrl),                              \
				.bits = DT_INST_CLOCKS_CELL(n, bits),                              \
			},                                                                         \
		.irq_no = DT_INST_IRQ(n, irq),                                                     \
		.irq_flags = DT_INST_IRQ(n, flags),                                                \
	};                                                                                         \
                                                                                                   \
	static struct spi_it51xxx_data spi_it51xxx_data_##n = {                                    \
		SPI_CONTEXT_INIT_LOCK(spi_it51xxx_data_##n, ctx),                                  \
		SPI_CONTEXT_INIT_SYNC(spi_it51xxx_data_##n, ctx),                                  \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &spi_it51xxx_init, NULL, &spi_it51xxx_data_##n,                   \
			      &spi_it51xxx_cfg_##n, POST_KERNEL,                                   \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &spi_it51xxx_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SPI_IT51XXX_INIT)
