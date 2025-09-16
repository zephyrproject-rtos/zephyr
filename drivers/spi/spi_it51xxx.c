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

#define SPI04_CTRL3        0x04
#define SPI_INT_LEVEL_MODE BIT(6)
#define BYTE_DONE_INT_STS  BIT(4)

#define SPI05_CHAIN_CTRL           0x05
#define PLL_CLOCK_SOURCE_SELECTION BIT(6)

#define SPI06_PAGE_SIZE        0x06
#define SPI09_FIFO_BASE_ADDR_1 0x09
#define BIG_ENDIAN_EN          BIT(7)
#define FIFO_BASE_ADDR_LB(x)   FIELD_GET(GENMASK(14, 8), x)

#define SPI0B_FIFO_CTRL      0x0B
#define GSCLK_INT_STS        BIT(3)
#define FIFO_TX_RX_TERMINATE BIT(1)
#define FIFO_TX_RX_START     BIT(0)

#define SPI0E_CTRL_4      0x0E
#define FIFO_FULL_INT_EN  BIT(5)
#define FIFO_FULL_INT_STS BIT(4)

#define SPI0F_FIFO_BASE_ADDR_2 0x0F
#define FIFO_BASE_ADDR_HB(x)   FIELD_GET(GENMASK(17, 15), x)

#define SPI24_GSCLK_MID_POINT_HOOK_METHOD 0x24
#define GSCLK_END_POINT_INT_EN            BIT(7)
#define GSCLK_END_POINT_HOOK_METHOD_EN    FIELD_PREP(GENMASK(6, 4), 4)

#define SPI82_GROUP_PAGE_SIZE_1 0x82
#define PAGE_SIZE_LB(x)         FIELD_GET(GENMASK(7, 0), x)

#define SPI83_GROUP_PAGE_SIZE_2  0x83
#define PAGE_CONTEXT_SIZE_LSB_EN BIT(7)
#define PAGE_SIZE_HB(x)          FIELD_GET(GENMASK(10, 8), x)

#ifdef CONFIG_SPI_ITE_IT51XXX_FIFO_MODE
/* Based on the hardware design, the shared FIFO mode can be used
 * for byte counts divisible by 8, and group FIFO mode for those
 * divisible by 2.
 */
#define IS_SHARED_FIFO_MODE(x) ((x >= 8) && (x <= CONFIG_SPI_ITE_IT51XXX_FIFO_SIZE) && (x % 8 == 0))
#define IS_GROUP_FIFO_MODE(x)  ((x >= 2) && (x <= CONFIG_SPI_ITE_IT51XXX_FIFO_SIZE) && (x % 2 == 0))

enum spi_it51xxx_xfer_mode {
	XFER_TX_ONLY = 0,
	XFER_RX_ONLY,
	XFER_TX_RX,
};

enum spi_it51xxx_ctrl_mode {
	PIO_MODE = 0,
	SHARED_FIFO_MODE,
	GROUP_FIFO_MODE,
};

#define GET_MODE_STRING(x)                                                                         \
	((x == PIO_MODE)           ? "pio mode"                                                    \
	 : (x == SHARED_FIFO_MODE) ? "share fifo mode"                                             \
				   : "group fifo mode")
#endif /* CONFIG_SPI_ITE_IT51XXX_FIFO_MODE */

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

#ifdef CONFIG_SPI_ITE_IT51XXX_FIFO_MODE
	uint8_t xfer_mode;

	struct {
		uint8_t tx;
		uint8_t rx;
	} ctrl_mode;

	__aligned(256) uint8_t fifo_data[CONFIG_SPI_ITE_IT51XXX_FIFO_SIZE];

	size_t transfer_len;
	size_t receive_len;

	bool direction_turnaround;
#endif /* CONFIG_SPI_ITE_IT51XXX_FIFO_MODE */
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

#ifdef CONFIG_SPI_ITE_IT51XXX_FIFO_MODE
static void spi_it51xxx_ctrl_mode_selection(const struct device *dev)
{
	const struct spi_it51xxx_config *cfg = dev->config;
	struct spi_it51xxx_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	size_t total_tx_len = spi_context_total_tx_len(ctx);
	size_t total_rx_len = spi_context_total_rx_len(ctx);

	/* rx buffer includes reserved space for tx data pointer and length,
	 * with the tx pointer set to null
	 */
	if (!ctx->rx_buf) {
		total_rx_len -= ctx->rx_len;
	}

	/* spi cs1 only supports pio mode */
	if (ctx->config->slave != 0) {
		data->ctrl_mode.tx = PIO_MODE;
		data->ctrl_mode.rx = PIO_MODE;
		goto out;
	}

	/* the shared/group fifo mode is supported only under spi mode 0 */
	if (sys_read8(cfg->base + SPI01_CTRL1) & (CLOCK_POLARTY | CLOCK_PHASE)) {
		data->ctrl_mode.tx = PIO_MODE;
		data->ctrl_mode.rx = PIO_MODE;
		goto out;
	}

	if (IS_SHARED_FIFO_MODE(total_rx_len)) {
		data->ctrl_mode.rx = SHARED_FIFO_MODE;
	} else if (IS_GROUP_FIFO_MODE(total_rx_len)) {
		/* group fifo mode only operates with the pll frequency clock source */
		if (sys_read8(cfg->base + SPI05_CHAIN_CTRL) & PLL_CLOCK_SOURCE_SELECTION) {
			data->ctrl_mode.rx = GROUP_FIFO_MODE;
		} else {
			data->ctrl_mode.rx = PIO_MODE;
		}
	} else {
		data->ctrl_mode.rx = PIO_MODE;
	}

	if (data->ctrl_mode.rx == PIO_MODE && total_rx_len != 0) {
		/* pio mode is used for tx if the rx transaction (tx-then-rx)
		 * is in pio mode.
		 */
		data->ctrl_mode.tx = PIO_MODE;
		goto out;
	}

	if (IS_SHARED_FIFO_MODE(total_tx_len)) {
		data->ctrl_mode.tx = SHARED_FIFO_MODE;
	} else if (IS_GROUP_FIFO_MODE(total_tx_len)) {
		/* group fifo mode only operates with the pll frequency clock source */
		if (sys_read8(cfg->base + SPI05_CHAIN_CTRL) & PLL_CLOCK_SOURCE_SELECTION) {
			data->ctrl_mode.tx = GROUP_FIFO_MODE;
		} else {
			data->ctrl_mode.tx = PIO_MODE;
		}
	} else {
		data->ctrl_mode.tx = PIO_MODE;
	}

out:
	LOG_DBG("mode selection: tx/rx: %s/%s",
		total_tx_len ? GET_MODE_STRING(data->ctrl_mode.tx) : "-",
		total_rx_len ? GET_MODE_STRING(data->ctrl_mode.rx) : "-");
}
#endif /* CONFIG_SPI_ITE_IT51XXX_FIFO_MODE */

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

#ifdef CONFIG_SPI_ITE_IT51XXX_FIFO_MODE
static inline bool rx_fifo_mode_is_enabled(const struct device *dev)
{
	struct spi_it51xxx_data *data = dev->data;

	return data->ctrl_mode.rx != PIO_MODE;
}

static inline bool tx_fifo_mode_is_enabled(const struct device *dev)
{
	struct spi_it51xxx_data *data = dev->data;

	return data->ctrl_mode.tx != PIO_MODE;
}

static inline void spi_it51xxx_set_fifo_len(const struct device *dev, const uint8_t mode,
					    const size_t length)
{
	const struct spi_it51xxx_config *cfg = dev->config;

	if (mode == GROUP_FIFO_MODE) {
		if (!IS_GROUP_FIFO_MODE(length)) {
			LOG_WRN("length (%d) is incompatible with group fifo mode", length);
		}
		sys_write8(PAGE_CONTEXT_SIZE_LSB_EN, cfg->base + SPI83_GROUP_PAGE_SIZE_2);
		sys_write8(PAGE_SIZE_LB(length - 1), cfg->base + SPI82_GROUP_PAGE_SIZE_1);
		sys_write8(sys_read8(cfg->base + SPI83_GROUP_PAGE_SIZE_2) |
				   PAGE_SIZE_HB(length - 1),
			   cfg->base + SPI83_GROUP_PAGE_SIZE_2);
	} else {
		if (!IS_SHARED_FIFO_MODE(length)) {
			LOG_WRN("length (%d) is incompatible with shared fifo mode", length);
		}
		sys_write8(sys_read8(cfg->base + SPI83_GROUP_PAGE_SIZE_2) &
				   ~PAGE_CONTEXT_SIZE_LSB_EN,
			   cfg->base + SPI83_GROUP_PAGE_SIZE_2);
		sys_write8(length / 8 - 1, cfg->base + SPI06_PAGE_SIZE);
	}
}

static inline bool direction_turnaround_check_workaround(const struct device *dev)
{
	struct spi_it51xxx_data *data = dev->data;

	/* Hardware limitation: When the direction is switched from tx to rx, the rx
	 * fifo done(FIFO_FULL_INT_STS) interrupt is triggered unexpectedly. This
	 * abnormal situation only occurs when using the shared/group fifo mode for
	 * tx transaction.
	 */
	return data->xfer_mode == XFER_TX_RX && data->ctrl_mode.tx != PIO_MODE;
}

static inline void spi_it51xxx_fifo_rx(const struct device *dev)
{
	const struct spi_it51xxx_config *cfg = dev->config;
	struct spi_it51xxx_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;

	if (direction_turnaround_check_workaround(dev) && !data->direction_turnaround) {
		data->direction_turnaround = true;
		sys_write8(sys_read8(cfg->base + SPI02_CTRL2) | READ_CYCLE,
			   cfg->base + SPI02_CTRL2);
		return;
	}

	sys_write8(sys_read8(cfg->base + SPI02_CTRL2) | READ_CYCLE, cfg->base + SPI02_CTRL2);

	data->direction_turnaround = false;

	if (data->xfer_mode == XFER_RX_ONLY) {
		data->receive_len = spi_context_total_rx_len(ctx);
	} else {
		data->receive_len = ctx->rx_len;
	}
	spi_it51xxx_set_fifo_len(dev, data->ctrl_mode.rx, data->receive_len);

	sys_write8(sys_read8(cfg->base + SPI0B_FIFO_CTRL) | FIFO_TX_RX_START,
		   cfg->base + SPI0B_FIFO_CTRL);
}

static inline void spi_it51xxx_fifo_tx(const struct device *dev)
{
	const struct spi_it51xxx_config *cfg = dev->config;
	struct spi_it51xxx_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;

	sys_write8(sys_read8(cfg->base + SPI02_CTRL2) & ~READ_CYCLE, cfg->base + SPI02_CTRL2);
	sys_write8(sys_read8(cfg->base + SPI09_FIFO_BASE_ADDR_1) | BIG_ENDIAN_EN,
		   cfg->base + SPI09_FIFO_BASE_ADDR_1);

	if (data->xfer_mode == XFER_TX_ONLY) {
		for (int i = 0; i < ctx->tx_count; i++) {
			const struct spi_buf spi_buf = ctx->current_tx[i];

			memcpy(data->fifo_data + data->transfer_len, spi_buf.buf, spi_buf.len);
			data->transfer_len += spi_buf.len;
		}
	} else {
		memcpy(data->fifo_data, ctx->tx_buf, ctx->tx_len);
		data->transfer_len = ctx->tx_len;
	}
	spi_it51xxx_set_fifo_len(dev, data->ctrl_mode.tx, data->transfer_len);

	LOG_HEXDUMP_DBG(data->fifo_data, data->transfer_len, "fifo: tx:");
	sys_write8(sys_read8(cfg->base + SPI0B_FIFO_CTRL) | FIFO_TX_RX_START,
		   cfg->base + SPI0B_FIFO_CTRL);
}
#endif /* CONFIG_SPI_ITE_IT51XXX_FIFO_MODE */

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
#ifdef CONFIG_SPI_ITE_IT51XXX_FIFO_MODE
		sys_write8(FIFO_TX_RX_TERMINATE, cfg->base + SPI0B_FIFO_CTRL);
		spi_context_complete(ctx, dev, 0);
#else
		sys_write8(SPI_TRANSMISSION_END, cfg->base + SPI03_STATUS);
#endif /* CONFIG_SPI_ITE_IT51XXX_FIFO_MODE */
		return;
	}

	if (!spi_context_tx_on(ctx)) {
#ifdef CONFIG_SPI_ITE_IT51XXX_FIFO_MODE
		if (rx_fifo_mode_is_enabled(dev)) {
			spi_it51xxx_fifo_rx(dev);
		} else {
#endif /* CONFIG_SPI_ITE_IT51XXX_FIFO_MODE */
			spi_it51xxx_rx(dev);
#ifdef CONFIG_SPI_ITE_IT51XXX_FIFO_MODE
		}
#endif /* CONFIG_SPI_ITE_IT51XXX_FIFO_MODE */
	} else {
#ifdef CONFIG_SPI_ITE_IT51XXX_FIFO_MODE
		if (tx_fifo_mode_is_enabled(dev)) {
			spi_it51xxx_fifo_tx(dev);
		} else {
#endif /* CONFIG_SPI_ITE_IT51XXX_FIFO_MODE */
			spi_it51xxx_tx(dev);
#ifdef CONFIG_SPI_ITE_IT51XXX_FIFO_MODE
		}
#endif /* CONFIG_SPI_ITE_IT51XXX_FIFO_MODE */
	}
}

static int transceive(const struct device *dev, const struct spi_config *config,
		      const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs,
		      bool asynchronous, spi_callback_t cb, void *userdata)
{
	const struct spi_it51xxx_config *cfg = dev->config;
	struct spi_it51xxx_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	int ret;

	spi_context_lock(ctx, asynchronous, cb, userdata, config);

	/* configure spi */
	ret = spi_it51xxx_configure(dev, config);
	if (ret) {
		goto out;
	}

	ret = clock_control_on(cfg->clk_dev, (clock_control_subsys_t *)&cfg->clk_cfg);
	if (ret) {
		LOG_ERR("failed to turn on spi clock %d", ret);
		goto out;
	}

	spi_context_buffers_setup(ctx, tx_bufs, rx_bufs, 1);

#ifdef CONFIG_SPI_ITE_IT51XXX_FIFO_MODE
	spi_it51xxx_ctrl_mode_selection(dev);

	if (!spi_context_tx_on(ctx)) {
		data->xfer_mode = XFER_RX_ONLY;
	} else if (!spi_context_rx_on(ctx)) {
		data->xfer_mode = XFER_TX_ONLY;
	} else {
		data->xfer_mode = XFER_TX_RX;
	}

	chip_block_idle();
#endif /* CONFIG_SPI_ITE_IT51XXX_FIFO_MODE */
	pm_policy_state_lock_get(PM_STATE_STANDBY, PM_ALL_SUBSTATES);

	LOG_DBG("tx/rx: %d/%d", spi_context_total_tx_len(ctx), spi_context_total_rx_len(ctx));

	spi_it51xxx_next_xfer(dev);
	ret = spi_context_wait_for_completion(ctx);

#ifdef CONFIG_SPI_ITE_IT51XXX_FIFO_MODE
	chip_permit_idle();

	data->direction_turnaround = false;
#endif /* CONFIG_SPI_ITE_IT51XXX_FIFO_MODE */
	pm_policy_state_lock_put(PM_STATE_STANDBY, PM_ALL_SUBSTATES);

	ret = clock_control_off(cfg->clk_dev, (clock_control_subsys_t *)&cfg->clk_cfg);
	if (ret) {
		LOG_ERR("failed to turn off spi clock %d", ret);
	}
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

#ifdef CONFIG_SPI_ITE_IT51XXX_FIFO_MODE
static void it51xxx_spi_fifo_done_handle(const struct device *dev, const bool is_tx_done)
{
	struct spi_it51xxx_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;

	if (is_tx_done) {
		do {
			size_t curr_tx_len = ctx->tx_len;

			spi_context_update_tx(ctx, 1, curr_tx_len);
			data->transfer_len -= curr_tx_len;
		} while (data->transfer_len > 0);
	} else {
		if (!data->direction_turnaround) {
			memcpy(ctx->rx_buf, data->fifo_data, data->receive_len);
			LOG_HEXDUMP_DBG(ctx->rx_buf, data->receive_len, "fifo: rx:");
			do {
				size_t curr_rx_len = ctx->rx_len;

				spi_context_update_rx(ctx, 1, curr_rx_len);
				data->receive_len -= curr_rx_len;
			} while (data->receive_len > 0);
		}
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
#endif /* CONFIG_SPI_ITE_IT51XXX_FIFO_MODE */

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
#ifdef CONFIG_SPI_ITE_IT51XXX_FIFO_MODE
		return;
#else
		struct spi_it51xxx_data *data = dev->data;

		spi_context_complete(&data->ctx, dev, 0);
#endif /* CONFIG_SPI_ITE_IT51XXX_FIFO_MODE */
	}

#ifdef CONFIG_SPI_ITE_IT51XXX_FIFO_MODE
	uint8_t fifo_sts, gsclk_sts;

	fifo_sts = sys_read8(cfg->base + SPI0E_CTRL_4);
	if (fifo_sts & FIFO_FULL_INT_STS) {
		LOG_DBG("isr: fifo full is asserted");
		sys_write8(fifo_sts, cfg->base + SPI0E_CTRL_4);
		it51xxx_spi_fifo_done_handle(dev, false);
	}

	gsclk_sts = sys_read8(cfg->base + SPI0B_FIFO_CTRL);
	if (gsclk_sts & GSCLK_INT_STS) {
		LOG_DBG("isr: gsclk is asserted");
		sys_write8(gsclk_sts, cfg->base + SPI0B_FIFO_CTRL);
		it51xxx_spi_fifo_done_handle(dev, true);
	}
#endif /* CONFIG_SPI_ITE_IT51XXX_FIFO_MODE */

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

#ifdef CONFIG_SPI_ITE_IT51XXX_FIFO_MODE
	/* set fifo base address */
	LOG_INF("fifo base address 0x%x", (uint32_t)&data->fifo_data);
	sys_write8(FIFO_BASE_ADDR_LB((uint32_t)&data->fifo_data),
		   cfg->base + SPI09_FIFO_BASE_ADDR_1);
	sys_write8(FIFO_BASE_ADDR_HB((uint32_t)&data->fifo_data),
		   cfg->base + SPI0F_FIFO_BASE_ADDR_2);

	/* enable gsclk middle point method */
	sys_write8(GSCLK_INT_STS, cfg->base + SPI0B_FIFO_CTRL);
	sys_write8(GSCLK_END_POINT_INT_EN | GSCLK_END_POINT_HOOK_METHOD_EN,
		   cfg->base + SPI24_GSCLK_MID_POINT_HOOK_METHOD);

	/* set fifo full interrupt */
	sys_write8(FIFO_FULL_INT_STS, cfg->base + SPI0E_CTRL_4);
	sys_write8(FIFO_FULL_INT_EN, cfg->base + SPI0E_CTRL_4);
#endif /* CONFIG_SPI_ITE_IT51XXX_FIFO_MODE */

	ite_intc_irq_polarity_set(cfg->irq_no, cfg->irq_flags);

	/* write 1 clear interrupt status and enable interrupt */
	sys_write8(sys_read8(cfg->base + SPI03_STATUS), cfg->base + SPI03_STATUS);
#ifdef CONFIG_SPI_ITE_IT51XXX_FIFO_MODE
	sys_write8(SPI_INT_LEVEL_MODE | BYTE_DONE_INT_STS, cfg->base + SPI04_CTRL3);
#else
	sys_write8(BYTE_DONE_INT_STS, cfg->base + SPI04_CTRL3);
#endif /* CONFIG_SPI_ITE_IT51XXX_FIFO_MODE */
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

/* according to the it51xxx spi hardware design, high-level triggering
 * is supported in fifo mode, while pio mode supports rising-edge
 * triggering.
 */
#define SUPPORTED_INTERRUPT_FLAG                                                                   \
	(IS_ENABLED(CONFIG_SPI_ITE_IT51XXX_FIFO_MODE) ? IRQ_TYPE_LEVEL_HIGH : IRQ_TYPE_EDGE_RISING)

#define SPI_IT51XXX_INIT(n)                                                                        \
	BUILD_ASSERT(DT_INST_IRQ(n, flags) == SUPPORTED_INTERRUPT_FLAG,                            \
		     "unsupported interrupt flag");                                                \
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
