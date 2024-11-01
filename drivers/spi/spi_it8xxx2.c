/*
 * Copyright (c) 2024 ITE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ite_it8xxx2_spi

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_it8xxx2, CONFIG_SPI_LOG_LEVEL);

#include <zephyr/irq.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/pm/policy.h>
#include <soc.h>

#include "spi_context.h"

#define BYTE_0(x) (uint8_t)(((x) >> 0) & 0xFF)
#define BYTE_1(x) (uint8_t)(((x) >> 8) & 0xFF)
#define BYTE_2(x) (uint8_t)(((x) >> 16) & 0xFF)

#define SRAM_BASE_ADDR DT_REG_ADDR(DT_NODELABEL(sram0))

#define SPI_CHIP_SELECT_COUNT   2
#define SPI_CMDQ_WR_CMD_LEN_MAX 16
#define SPI_CMDQ_DATA_LEN_MAX   0xFFFF

/* IT8xxx2 SSPI Registers Definition */
#define SPI01_CTRL1    0x01
#define CLOCK_POLARTY  BIT(6)
#define SSCK_FREQ_MASK (BIT(2) | BIT(3) | BIT(4))
#define INTERRUPT_EN   BIT(1)

#define SPI04_CTRL3 0x04
#define AUTO_MODE   BIT(5)

#define SPI05_CH0_CMD_ADDR_LB     0x05
#define SPI06_CH0_CMD_ADDR_HB     0x06
#define SPI0C_INT_STS             0x0C
#define SPI_CMDQ_BUS_END_INT_MASK BIT(4)
#define SPI_DMA_RBUF_1_FULL       BIT(2)
#define SPI_DMA_RBUF_0_FULL       BIT(1)
#define SPI_CMDQ_BUS_END          BIT(0)

#define SPI0D_CTRL5       0x0D
#define CH1_SEL_CMDQ      BIT(5)
#define CH0_SEL_CMDQ      BIT(4)
#define SCK_FREQ_DIV_1_EN BIT(1)
#define CMDQ_MODE_EN      BIT(0)

#define SPI0E_CH0_WR_MEM_ADDR_LB  0x0E
#define SPI0F_CH0_WR_MEM_ADDR_HB  0x0F
#define SPI12_CH1_CMD_ADDR_LB     0x12
#define SPI13_CH1_CMD_ADDR_HB     0x13
#define SPI14_CH1_WR_MEM_ADDR_LB  0x14
#define SPI15_CH1_WR_MEM_ADDR_HB  0x15
#define SPI21_CH0_CMD_ADDR_HB2    0x21
#define SPI23_CH0_WR_MEM_ADDR_HB2 0x23
#define SPI25_CH1_CMD_ADDR_HB2    0x25
#define SPI27_CH1_WR_MEM_ADDR_HB2 0x27

struct spi_it8xxx2_cmdq_data {
	uint8_t spi_write_cmd_length;

	union {
		uint8_t value;
		struct {
			uint8_t cmd_end: 1;
			uint8_t read_write: 1;
			uint8_t auto_check_sts: 1;
			uint8_t cs_active: 1;
			uint8_t reserved: 1;
			uint8_t cmd_mode: 2;
			uint8_t dtr: 1;
		} __packed fields;
	} __packed command;

	uint8_t data_length_lb;
	uint8_t data_length_hb;
	uint8_t data_addr_lb;
	uint8_t data_addr_hb;
	uint8_t check_bit_mask;
	uint8_t check_bit_value;

	uint8_t write_data[SPI_CMDQ_WR_CMD_LEN_MAX];
};

struct spi_it8xxx2_config {
	mm_reg_t base;
	const struct pinctrl_dev_config *pcfg;
	uint8_t spi_irq;
};

struct spi_it8xxx2_data {
	struct spi_context ctx;
	struct spi_it8xxx2_cmdq_data cmdq_data;
	size_t transfer_len;
	size_t receive_len;
};

static inline int spi_it8xxx2_set_freq(const struct device *dev, const uint32_t frequency)
{
	const struct spi_it8xxx2_config *cfg = dev->config;
	uint8_t freq_div[8] = {2, 4, 6, 8, 10, 12, 14, 16};
	uint32_t clk_pll, clk_sspi;
	uint8_t reg_val;

	clk_pll = chip_get_pll_freq();
	clk_sspi = clk_pll / (((IT8XXX2_ECPM_SCDCR2 & 0xF0) >> 4) + 1U);
	if (frequency < (clk_sspi / 16) || frequency > clk_sspi) {
		LOG_ERR("Unsupported frequency %d", frequency);
		return -ENOTSUP;
	}

	if (frequency == clk_sspi) {
		sys_write8(sys_read8(cfg->base + SPI0D_CTRL5) | SCK_FREQ_DIV_1_EN,
			   cfg->base + SPI0D_CTRL5);
	} else {
		for (int i = 0; i <= ARRAY_SIZE(freq_div); i++) {
			if (i == ARRAY_SIZE(freq_div)) {
				LOG_ERR("Unknown frequency %d", frequency);
				return -ENOTSUP;
			}
			if (frequency == (clk_sspi / freq_div[i])) {
				sys_write8(sys_read8(cfg->base + SPI0D_CTRL5) & ~SCK_FREQ_DIV_1_EN,
					   cfg->base + SPI0D_CTRL5);
				reg_val = sys_read8(cfg->base + SPI01_CTRL1);
				reg_val = (reg_val & (~SSCK_FREQ_MASK)) | (i << 2);
				sys_write8(reg_val, cfg->base + SPI01_CTRL1);
				break;
			}
		}
	}

	LOG_DBG("freq: pll %dHz, sspi %dHz, ssck %dHz", clk_pll, clk_sspi, frequency);
	return 0;
}

static int spi_it8xxx2_configure(const struct device *dev, const struct spi_config *spi_cfg)
{
	const struct spi_it8xxx2_config *cfg = dev->config;
	struct spi_it8xxx2_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	int ret;
	uint8_t reg_val;

	if (spi_cfg->slave > (SPI_CHIP_SELECT_COUNT - 1)) {
		LOG_ERR("Slave %d is greater than %d", spi_cfg->slave, SPI_CHIP_SELECT_COUNT - 1);
		return -EINVAL;
	}

	LOG_DBG("chip select: %d, operation: 0x%x", spi_cfg->slave, spi_cfg->operation);

	if (SPI_OP_MODE_GET(spi_cfg->operation) == SPI_OP_MODE_SLAVE) {
		LOG_ERR("Unsupported SPI slave mode");
		return -ENOTSUP;
	}

	if (SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_LOOP) {
		LOG_ERR("Unsupported loopback mode");
		return -ENOTSUP;
	}

	if (SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_CPHA) {
		LOG_ERR("Unsupported cpha mode");
		return -ENOTSUP;
	}

	reg_val = sys_read8(cfg->base + SPI01_CTRL1);
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
		LOG_ERR("Only single line mode is supported");
		return -EINVAL;
	}

	ret = spi_it8xxx2_set_freq(dev, spi_cfg->frequency);
	if (ret) {
		return ret;
	}

	reg_val = sys_read8(cfg->base + SPI0C_INT_STS);
	reg_val = (reg_val & (~SPI_CMDQ_BUS_END_INT_MASK));
	sys_write8(reg_val, cfg->base + SPI0C_INT_STS);

	ctx->config = spi_cfg;
	return 0;
}

static inline bool spi_it8xxx2_transfer_done(struct spi_context *ctx)
{
	return !spi_context_tx_buf_on(ctx) && !spi_context_rx_buf_on(ctx);
}

static void spi_it8xxx2_complete(const struct device *dev, const int status)
{
	struct spi_it8xxx2_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;

	spi_context_complete(ctx, dev, status);
	if (spi_cs_is_gpio(ctx->config)) {
		spi_context_cs_control(ctx, false);
	}
	/* Permit to enter power policy and idle mode. */
	pm_policy_state_lock_put(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
	chip_permit_idle();
}

static inline void spi_it8xxx2_tx(const struct device *dev)
{
	struct spi_it8xxx2_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	uint32_t mem_address;

	if (ctx->tx_count > 1) {
		data->cmdq_data.command.fields.cs_active = 1;
	} else {
		data->cmdq_data.command.fields.cs_active = 0;
	}
	data->cmdq_data.command.fields.cmd_end = 1;
	data->cmdq_data.command.fields.read_write = 0;
	if (ctx->tx_len <= SPI_CMDQ_WR_CMD_LEN_MAX) {
		data->cmdq_data.spi_write_cmd_length = ctx->tx_len;
		memcpy(data->cmdq_data.write_data, ctx->tx_buf, ctx->tx_len);
		data->cmdq_data.data_length_lb = 0;
		data->cmdq_data.data_length_hb = 0;
		data->cmdq_data.data_addr_lb = 0;
		data->cmdq_data.data_addr_hb = 0;
	} else {
		data->cmdq_data.spi_write_cmd_length = SPI_CMDQ_WR_CMD_LEN_MAX;
		memcpy(data->cmdq_data.write_data, ctx->tx_buf, SPI_CMDQ_WR_CMD_LEN_MAX);
		data->cmdq_data.data_length_lb = BYTE_0(ctx->tx_len - SPI_CMDQ_WR_CMD_LEN_MAX);
		data->cmdq_data.data_length_hb = BYTE_1(ctx->tx_len - SPI_CMDQ_WR_CMD_LEN_MAX);
		mem_address = (uint32_t)(ctx->tx_buf + SPI_CMDQ_WR_CMD_LEN_MAX) - SRAM_BASE_ADDR;
		data->cmdq_data.data_addr_lb = BYTE_0(mem_address);
		data->cmdq_data.data_addr_hb = BYTE_1(mem_address);
		data->cmdq_data.check_bit_mask |= ((BYTE_2(mem_address)) & 0x03);
	}
	data->transfer_len = ctx->tx_len;
}

static inline void spi_it8xxx2_rx(const struct device *dev)
{
	struct spi_it8xxx2_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;

	if (ctx->rx_count > 1) {
		data->cmdq_data.command.fields.cs_active = 1;
	} else {
		data->cmdq_data.command.fields.cs_active = 0;
	}
	data->cmdq_data.command.fields.cmd_end = 1;
	data->cmdq_data.command.fields.read_write = 1;
	data->cmdq_data.spi_write_cmd_length = 0;
	data->cmdq_data.data_length_lb = BYTE_0(ctx->rx_len);
	data->cmdq_data.data_length_hb = BYTE_1(ctx->rx_len);
	data->cmdq_data.data_addr_lb = 0;
	data->cmdq_data.data_addr_hb = 0;
	data->receive_len = ctx->rx_len;
}

static inline void spi_it8xxx2_tx_rx(const struct device *dev)
{
	struct spi_it8xxx2_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	uint32_t mem_address;

	data->cmdq_data.command.fields.cmd_end = 1;
	if (ctx->tx_len <= SPI_CMDQ_WR_CMD_LEN_MAX) {
		data->cmdq_data.command.fields.cs_active = 0;
		data->cmdq_data.command.fields.read_write = 1;
		data->cmdq_data.spi_write_cmd_length = ctx->tx_len;
		memcpy(data->cmdq_data.write_data, ctx->tx_buf, ctx->tx_len);
		if (ctx->rx_buf == ctx->tx_buf) {
			spi_context_update_tx(ctx, 1, ctx->tx_len);
			spi_context_update_rx(ctx, 1, ctx->rx_len);
		}

		data->cmdq_data.data_length_lb = BYTE_0(ctx->rx_len);
		data->cmdq_data.data_length_hb = BYTE_1(ctx->rx_len);
		data->cmdq_data.data_addr_lb = 0;
		data->cmdq_data.data_addr_hb = 0;
		data->transfer_len = ctx->tx_len;
		data->receive_len = ctx->rx_len;
	} else {
		data->cmdq_data.command.fields.cs_active = 1;
		data->cmdq_data.command.fields.read_write = 0;
		data->cmdq_data.spi_write_cmd_length = SPI_CMDQ_WR_CMD_LEN_MAX;
		memcpy(data->cmdq_data.write_data, ctx->tx_buf, SPI_CMDQ_WR_CMD_LEN_MAX);
		data->cmdq_data.data_length_lb = BYTE_0(ctx->tx_len - SPI_CMDQ_WR_CMD_LEN_MAX);
		data->cmdq_data.data_length_hb = BYTE_1(ctx->tx_len - SPI_CMDQ_WR_CMD_LEN_MAX);

		mem_address = (uint32_t)(ctx->tx_buf + SPI_CMDQ_WR_CMD_LEN_MAX) - SRAM_BASE_ADDR;
		data->cmdq_data.data_addr_lb = BYTE_0(mem_address);
		data->cmdq_data.data_addr_hb = BYTE_1(mem_address);
		data->cmdq_data.check_bit_mask |= ((BYTE_2(mem_address)) & 0x03);
		if (ctx->rx_buf == ctx->tx_buf) {
			spi_context_update_tx(ctx, 1, ctx->tx_len);
			spi_context_update_rx(ctx, 1, ctx->rx_len);
		}
		data->transfer_len = ctx->tx_len;
		data->receive_len = 0;
	}
}

static int spi_it8xxx2_next_xfer(const struct device *dev)
{
	const struct spi_it8xxx2_config *cfg = dev->config;
	struct spi_it8xxx2_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	uint8_t reg_val;
	uint32_t cmd_address, mem_address;

	if (spi_it8xxx2_transfer_done(ctx)) {
		spi_it8xxx2_complete(dev, 0);
		return 0;
	}

	if (spi_cs_is_gpio(ctx->config)) {
		spi_context_cs_control(ctx, true);
	}

	if (spi_context_longest_current_buf(ctx) > SPI_CMDQ_DATA_LEN_MAX) {
		return -EINVAL;
	}

	memset(&data->cmdq_data, 0, sizeof(struct spi_it8xxx2_cmdq_data));

	/* Prepare command queue data */
	if (!spi_context_tx_on(ctx)) {
		/* rx only, nothing to tx */
		spi_it8xxx2_rx(dev);
	} else if (!spi_context_rx_on(ctx)) {
		/* tx only, nothing to rx */
		spi_it8xxx2_tx(dev);
	} else {
		spi_it8xxx2_tx_rx(dev);
	}

	cmd_address = (uint32_t)(&data->cmdq_data) - SRAM_BASE_ADDR;
	mem_address = (uint32_t)ctx->rx_buf - SRAM_BASE_ADDR;
	if (ctx->config->slave == 0) {
		sys_write8(BYTE_0(cmd_address), cfg->base + SPI05_CH0_CMD_ADDR_LB);
		sys_write8(BYTE_1(cmd_address), cfg->base + SPI06_CH0_CMD_ADDR_HB);
		sys_write8(BYTE_2(cmd_address), cfg->base + SPI21_CH0_CMD_ADDR_HB2);

		if (spi_context_rx_on(ctx)) {
			sys_write8(BYTE_0(mem_address), cfg->base + SPI0E_CH0_WR_MEM_ADDR_LB);
			sys_write8(BYTE_1(mem_address), cfg->base + SPI0F_CH0_WR_MEM_ADDR_HB);
			sys_write8(BYTE_2(mem_address), cfg->base + SPI23_CH0_WR_MEM_ADDR_HB2);
		}
	} else {
		sys_write8(BYTE_0(cmd_address), cfg->base + SPI12_CH1_CMD_ADDR_LB);
		sys_write8(BYTE_1(cmd_address), cfg->base + SPI13_CH1_CMD_ADDR_HB);
		sys_write8(BYTE_2(cmd_address), cfg->base + SPI25_CH1_CMD_ADDR_HB2);

		if (spi_context_rx_on(ctx)) {
			sys_write8(BYTE_0(mem_address), cfg->base + SPI14_CH1_WR_MEM_ADDR_LB);
			sys_write8(BYTE_1(mem_address), cfg->base + SPI15_CH1_WR_MEM_ADDR_HB);
			sys_write8(BYTE_2(mem_address), cfg->base + SPI27_CH1_WR_MEM_ADDR_HB2);
		}
	}

	sys_write8(sys_read8(cfg->base + SPI01_CTRL1) | INTERRUPT_EN, cfg->base + SPI01_CTRL1);

	reg_val = sys_read8(cfg->base + SPI0D_CTRL5);
	reg_val |= (ctx->config->slave == 0) ? CH0_SEL_CMDQ : CH1_SEL_CMDQ;
	sys_write8(reg_val | CMDQ_MODE_EN, cfg->base + SPI0D_CTRL5);
	return 0;
}

static int transceive(const struct device *dev, const struct spi_config *config,
		      const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs,
		      bool asynchronous, spi_callback_t cb, void *userdata)
{
	struct spi_it8xxx2_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	int ret;

	spi_context_lock(ctx, asynchronous, cb, userdata, config);

	/* Configure spi */
	ret = spi_it8xxx2_configure(dev, config);
	if (ret) {
		spi_context_release(ctx, ret);
		return ret;
	}

	/*
	 * The EC processor(CPU) cannot be in the k_cpu_idle() and power
	 * policy during the transactions with the CQ mode.
	 * Otherwise, the EC processor would be clock gated.
	 */
	chip_block_idle();
	pm_policy_state_lock_get(PM_STATE_STANDBY, PM_ALL_SUBSTATES);

	spi_context_buffers_setup(ctx, tx_bufs, rx_bufs, 1);
	ret = spi_it8xxx2_next_xfer(dev);
	if (!ret) {
		ret = spi_context_wait_for_completion(ctx);
	} else {
		spi_it8xxx2_complete(dev, ret);
	}

	spi_context_release(ctx, ret);
	return ret;
}

static int it8xxx2_transceive(const struct device *dev, const struct spi_config *config,
			      const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs)
{
	return transceive(dev, config, tx_bufs, rx_bufs, false, NULL, NULL);
}

#ifdef CONFIG_SPI_ASYNC
static int it8xxx2_transceive_async(const struct device *dev, const struct spi_config *config,
				    const struct spi_buf_set *tx_bufs,
				    const struct spi_buf_set *rx_bufs, spi_callback_t cb,
				    void *userdata)
{
	return transceive(dev, config, tx_bufs, rx_bufs, true, cb, userdata);
}
#endif /* CONFIG_SPI_ASYNC */

static int it8xxx2_release(const struct device *dev, const struct spi_config *config)
{
	struct spi_it8xxx2_data *data = dev->data;

	spi_context_unlock_unconditionally(&data->ctx);
	return 0;
}

static void it8xxx2_spi_isr(const void *arg)
{
	const struct device *dev = arg;
	const struct spi_it8xxx2_config *cfg = dev->config;
	struct spi_it8xxx2_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	uint8_t reg_val;
	int ret;

	reg_val = sys_read8(cfg->base + SPI0C_INT_STS);
	sys_write8(reg_val, cfg->base + SPI0C_INT_STS);
	if (reg_val & (SPI_DMA_RBUF_0_FULL | SPI_DMA_RBUF_1_FULL)) {
		LOG_INF("Triggered dma ring buffer full interrupt, status: 0x%x", reg_val);
	}

	if (reg_val & SPI_CMDQ_BUS_END) {
		reg_val = sys_read8(cfg->base + SPI0D_CTRL5);
		if (ctx->config->slave == 0) {
			reg_val &= ~CH0_SEL_CMDQ;
		} else {
			reg_val &= ~CH1_SEL_CMDQ;
		}
		sys_write8(reg_val, cfg->base + SPI0D_CTRL5);

		spi_context_update_tx(ctx, 1, data->transfer_len);
		spi_context_update_rx(ctx, 1, data->receive_len);
		ret = spi_it8xxx2_next_xfer(dev);
		if (ret) {
			spi_it8xxx2_complete(dev, ret);
		}
	}
}

static int spi_it8xxx2_init(const struct device *dev)
{
	const struct spi_it8xxx2_config *cfg = dev->config;
	struct spi_it8xxx2_data *data = dev->data;
	int ret;

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret) {
		LOG_ERR("Failed to set default pinctrl");
		return ret;
	}

	/* Enable one-shot mode */
	sys_write8(sys_read8(cfg->base + SPI04_CTRL3) & ~AUTO_MODE, cfg->base + SPI04_CTRL3);

	irq_connect_dynamic(cfg->spi_irq, 0, it8xxx2_spi_isr, dev, 0);
	irq_enable(cfg->spi_irq);

	ret = spi_context_cs_configure_all(&data->ctx);
	if (ret) {
		return ret;
	}

	spi_context_unlock_unconditionally(&data->ctx);
	return 0;
}

static const struct spi_driver_api spi_it8xxx2_driver_api = {
	.transceive = it8xxx2_transceive,
	.release = it8xxx2_release,

#ifdef CONFIG_SPI_ASYNC
	.transceive_async = it8xxx2_transceive_async,
#endif
};

#define SPI_IT8XXX2_INIT(n)                                                                        \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static const struct spi_it8xxx2_config spi_it8xxx2_cfg_##n = {                             \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.spi_irq = DT_INST_IRQ(n, irq),                                                    \
	};                                                                                         \
                                                                                                   \
	static struct spi_it8xxx2_data spi_it8xxx2_data_##n = {                                    \
		SPI_CONTEXT_INIT_LOCK(spi_it8xxx2_data_##n, ctx),                                  \
		SPI_CONTEXT_INIT_SYNC(spi_it8xxx2_data_##n, ctx),                                  \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(n), ctx)};                             \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &spi_it8xxx2_init, NULL, &spi_it8xxx2_data_##n,                   \
			      &spi_it8xxx2_cfg_##n, POST_KERNEL,                                   \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &spi_it8xxx2_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SPI_IT8XXX2_INIT)
