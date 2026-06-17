/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_unicomm_spi

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_ti_unicomm, CONFIG_SPI_LOG_LEVEL);

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/mspm0_clock_control.h>
#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/sys/util.h>

#include "spi_context.h"

/* UNICOMM Register Offsets */
#define UNICOMM_SPI_CLKDIV 0x0
#define UNICOMM_SPI_CLKSEL 0x8

/* CPU_INT interrupt sub-block register offsets */
#define UNICOMM_SPI_IIDX  0x020
#define UNICOMM_SPI_IMASK 0x028
#define UNICOMM_SPI_RIS   0x030
#define UNICOMM_SPI_MIS   0x038
#define UNICOMM_SPI_ISET  0x040
#define UNICOMM_SPI_ICLR  0x048

#define UNICOMM_SPI_CTL0   0x100
#define UNICOMM_SPI_STAT   0x108
#define UNICOMM_SPI_IFLS   0x10C
#define UNICOMM_SPI_CTL1   0x14C
#define UNICOMM_SPI_CLKCTL 0x110

#define UNICOMM_SPI_TXDATA 0x120
#define UNICOMM_SPI_RXDATA 0x124

/* CTL0 bits */
#define UNICOMMSPI_CTL0_DSS_MASK   GENMASK(4, 0)
#define UNICOMMSPI_CTL0_FRF_MASK   GENMASK(6, 5)
#define UNICOMMSPI_CTL0_SPO_MASK   BIT(8)
#define UNICOMMSPI_CTL0_SPH_MASK   BIT(9)
#define UNICOMMSPI_CTL0_CSCLR_MASK BIT(14)

/* CTL1 bits */
#define UNICOMMSPI_CTL1_ENABLE_MASK  BIT(0)
#define UNICOMMSPI_CTL1_LBM_MASK     BIT(1)
#define UNICOMMSPI_CTL1_CP_MASK      BIT(2)
#define UNICOMMSPI_CTL1_MSB_MASK     BIT(4)
#define UNICOMMSPI_CTL1_PREN_MASK    BIT(5)
#define UNICOMMSPI_CTL1_PES_MASK     BIT(6)
#define UNICOMMSPI_CTL1_PTEN_MASK    BIT(8)
#define UNICOMMSPI_CTL1_SUSPEND_MASK BIT(9)

/* CLKCTL bits */
#define UNICOMMSPI_CLKCTL_SCR_MASK GENMASK(9, 0)

/* STAT bits */
#define UNICOMMSPI_STAT_RXFE_MASK  BIT(2)
#define UNICOMMSPI_STAT_RXFF_MASK  BIT(3)
#define UNICOMMSPI_STAT_RXCLR_MASK BIT(4)
#define UNICOMMSPI_STAT_TXFE_MASK  BIT(5)
#define UNICOMMSPI_STAT_TXFF_MASK  BIT(6)
#define UNICOMMSPI_STAT_TXCLR_MASK BIT(7)
#define UNICOMMSPI_STAT_BUSY_MASK  BIT(8)

/* CPU_INT interrupt mask bits (same for IMASK, RIS, MIS, ISET, ICLR) */
#define UNICOMMSPI_INT_PER_MASK        BIT(0) /* Parity error */
#define UNICOMMSPI_INT_RXFIFO_OVF_MASK BIT(1) /* RX FIFO overflow */
#define UNICOMMSPI_INT_RX_MASK         BIT(2) /* RX FIFO threshold reached */
#define UNICOMMSPI_INT_RXFULL_MASK     BIT(3) /* RX FIFO full */
#define UNICOMMSPI_INT_TXFIFO_UNF_MASK BIT(4) /* TX FIFO underflow */
#define UNICOMMSPI_INT_TX_MASK         BIT(5) /* TX FIFO threshold reached */
#define UNICOMMSPI_INT_TXEMPTY_MASK    BIT(6) /* TX FIFO empty */
#define UNICOMMSPI_INT_IDLE_MASK       BIT(8) /* SPI IDLE (BUSY went low) */
#define UNICOMMSPI_INT_RTOUT_MASK      BIT(9) /* RX timeout */

/*
 * Enabled interrupts during a controller-mode transfer:
 *  - RX: RX FIFO reached the configured threshold
 *  - RXFULL: RX FIFO filled completely (prevents overflow if the RX threshold is set to a high
 * level)
 *  - TX: TX FIFO reached the configured threshold; used to refill the FIFO
 *  - TXEMPTY: TX FIFO fully drained to the shift register, used as transfer completion trigger
 */
#define UNICOMMSPI_TRANSFER_IMASK                                                                  \
	(UNICOMMSPI_INT_RX_MASK | UNICOMMSPI_INT_RXFULL_MASK | UNICOMMSPI_INT_TX_MASK |            \
	 UNICOMMSPI_INT_TXEMPTY_MASK)

/*
 * Enabled interrupts during a peripheral-mode transfer:
 *  - RX: RX FIFO reached the configured threshold
 *  - RXFULL: RX FIFO filled completely (prevents overflow if the RX threshold is set to a high
 * level)
 *  - TX: TX FIFO reached the configured threshold
 *  - IDLE: fires when the controller deasserts CS, used as the transfer completion trigger
 */
#define UNICOMMSPI_PERIPHERAL_IMASK                                                                \
	(UNICOMMSPI_INT_RX_MASK | UNICOMMSPI_INT_RXFULL_MASK | UNICOMMSPI_INT_TX_MASK |            \
	 UNICOMMSPI_INT_IDLE_MASK)

/* IFLS register bits for FIFO threshold selection */
#define UNICOMMSPI_IFLS_TXIFLSEL_MASK GENMASK(2, 0)
#define UNICOMMSPI_IFLS_TXCLR_MASK    BIT(3)
#define UNICOMMSPI_IFLS_RXIFLSEL_MASK GENMASK(6, 4)
#define UNICOMMSPI_IFLS_RXCLR_MASK    BIT(7)

/* Configuration Values */
#define SPI_CLKDIV_DIVIDE_BY_1   0
#define SPI_CLKSEL_BUSCLK_ENABLE BIT(3)

#define SPI_CTL1_ENABLE  FIELD_PREP(UNICOMMSPI_CTL1_ENABLE_MASK, 1)
#define SPI_CTL1_DISABLE FIELD_PREP(UNICOMMSPI_CTL1_ENABLE_MASK, 0)

#define SPI_CTL0_FRF_MOTOROLA_3WIRE FIELD_PREP(UNICOMMSPI_CTL0_FRF_MASK, 0)
#define SPI_CTL0_FRF_MOTOROLA_4WIRE FIELD_PREP(UNICOMMSPI_CTL0_FRF_MASK, 1)
#define SPI_CTL0_FRF_TI             FIELD_PREP(UNICOMMSPI_CTL0_FRF_MASK, 2)
#define SPI_CTL0_SPO_CPOL           FIELD_PREP(UNICOMMSPI_CTL0_SPO_MASK, 1)
#define SPI_CTL0_SPH_CPHA           FIELD_PREP(UNICOMMSPI_CTL0_SPH_MASK, 1)
#define SPI_CTL0_DSS_8BIT           FIELD_PREP(UNICOMMSPI_CTL0_DSS_MASK, 8)

/* FIFO depth of the UNICOMM SPI peripheral */
#define UNICOMMSPI_FIFO_DEPTH 16U

/*
 * UNICOMMSPI Registers are offset by 0x1000 from UNICOMMSPI base on MSPM33C.
 * If such cases arise for other platforms, updating the node's address in Devicetree
 * (thus diverging from the TRM addresses, even though there's functionally no difference)
 * can be considered.
 */
#define MSPM33C_UCSPI_OFFSET 0x1000U

/* Helper macros */
#define UPDATE_REG(reg_offset, value, mask)                                                        \
	{                                                                                          \
		uint32_t tmp = sys_read32(reg_offset);                                             \
		tmp = tmp & ~(mask);                                                               \
		sys_write32(tmp | ((value) & (mask)), reg_offset);                                 \
	}

/* SCR range: 0-1023 */
#define UNICOMMSPI_SCR_MIN 0
#define UNICOMMSPI_SCR_MAX 1023

struct spi_ti_unicomm_config {
	const struct pinctrl_dev_config *pcfg;

	uint32_t unicomm_spi_base;

	const struct device *clk_dev;
	struct mspm0_sys_clock clk_subsys;

	void (*irq_config_func)(const struct device *dev);

	uint8_t tx_fifo_threshold;
	uint8_t rx_fifo_threshold;
};

struct spi_ti_unicomm_data {
	struct spi_context ctx;
	/* Error status captured in ISR context */
	int isr_error;
	/* Content copy of the last applied spi_config */
	struct spi_config last_config;
	/*
	 * Set to true when a transfer is in progress (between tx_fill/IMASK-enable
	 * and spi_context_complete). Guards the ISR completion check against
	 * spurious TXEMPTY interrupts that fire when IMASK is re-enabled for a
	 * new transfer but the TX FIFO is already empty from the previous one.
	 */
	bool transfer_active;
};

/*
 * Helper: push up to FIFO_DEPTH bytes from spi_context TX into the TX FIFO.
 * Returns the number of bytes pushed.
 */
static uint32_t spi_ti_unicomm_tx_fill(uint32_t base, struct spi_context *ctx)
{
	uint32_t count = 0;

	while (count < UNICOMMSPI_FIFO_DEPTH &&
	       (spi_context_tx_on(ctx) || spi_context_rx_on(ctx))) {
		if (sys_read32(base + UNICOMM_SPI_STAT) & UNICOMMSPI_STAT_TXFF_MASK) {
			break; /* TX FIFO full */
		}

		uint8_t byte = spi_context_tx_buf_on(ctx) ? *(const uint8_t *)ctx->tx_buf : 0U;

		sys_write32(byte, base + UNICOMM_SPI_TXDATA);
		spi_context_update_tx(ctx, 1, 1);
		count++;
	}
	return count;
}

/*
 * Helper: drain all available bytes from the RX FIFO into spi_context RX buffer.
 */
static void spi_ti_unicomm_rx_drain(uint32_t base, struct spi_context *ctx)
{
	while (!(sys_read32(base + UNICOMM_SPI_STAT) & UNICOMMSPI_STAT_RXFE_MASK)) {
		uint8_t byte = (uint8_t)sys_read32(base + UNICOMM_SPI_RXDATA);

		if (spi_context_rx_buf_on(ctx)) {
			*(uint8_t *)ctx->rx_buf = byte;
		}
		spi_context_update_rx(ctx, 1, 1);
	}
}

/*
 * Peripheral-mode TX refill: push up to FIFO_DEPTH bytes from spi_context TX
 * into the TX FIFO.
 */
static uint32_t spi_ti_unicomm_peripheral_tx_fill(uint32_t base, struct spi_context *ctx)
{
	uint32_t count = 0;

	while (count < UNICOMMSPI_FIFO_DEPTH && spi_context_tx_on(ctx)) {
		if (sys_read32(base + UNICOMM_SPI_STAT) & UNICOMMSPI_STAT_TXFF_MASK) {
			break; /* TX FIFO full */
		}

		uint8_t byte = spi_context_tx_buf_on(ctx) ? *(const uint8_t *)ctx->tx_buf : 0U;

		sys_write32(byte, base + UNICOMM_SPI_TXDATA);
		spi_context_update_tx(ctx, 1, 1);
		count++;
	}
	return count;
}

/*
 * Peripheral-mode RX drain: read out every byte currently in the RX FIFO.
 */
static void spi_ti_unicomm_peripheral_rx_drain(uint32_t base, struct spi_context *ctx)
{
	while (!(sys_read32(base + UNICOMM_SPI_STAT) & UNICOMMSPI_STAT_RXFE_MASK)) {
		uint8_t byte = (uint8_t)sys_read32(base + UNICOMM_SPI_RXDATA);

		if (spi_context_rx_buf_on(ctx)) {
			*(uint8_t *)ctx->rx_buf = byte;
			spi_context_update_rx(ctx, 1, 1);
		}
	}
}

static int spi_ti_unicomm_configure(const struct device *dev, const struct spi_config *config)
{
	const struct spi_ti_unicomm_config *cfg = dev->config;
	struct spi_ti_unicomm_data *data = dev->data;

	uint32_t ctl0 = 0;
	uint32_t ctl1 = 0;
	uint32_t clock_rate;
	uint32_t scr;
	int ret;

	/*
	 * Use content comparison instead of spi_context_configured() (which
	 * checks pointer equality). The test framework reuses a fixed set of
	 * spec_copies[] addresses across SLOW and FAST runs, so the pointer
	 * stays the same even though frequency/operation bits have changed.
	 * Comparing the actual fields ensures we always reconfigure when
	 * anything meaningful has changed.
	 */
	if (data->last_config.frequency == config->frequency &&
	    data->last_config.operation == config->operation) {
		/* Nothing to do */
		return 0;
	}

	/* Only single line mode is supported */
	if (IS_ENABLED(CONFIG_SPI_EXTENDED_MODES) &&
	    (config->operation & SPI_LINES_MASK) != SPI_LINES_SINGLE) {
		return -EINVAL;
	}

	/* Set Frame Format*/
	if (config->operation & SPI_FRAME_FORMAT_TI) {
		ctl0 = SPI_CTL0_FRF_TI;

		/* Half duplex mode is not supported by TI Frame Format */
		if (config->operation & SPI_HALF_DUPLEX) {
			return -ENOTSUP;
		}
	} else {
		ctl0 = SPI_CTL0_FRF_MOTOROLA_4WIRE;

		if (config->operation & SPI_HALF_DUPLEX) {
			ctl0 = SPI_CTL0_FRF_MOTOROLA_3WIRE;
		}

		if (config->operation & SPI_MODE_CPOL) {
			ctl0 |= SPI_CTL0_SPO_CPOL;
		}

		if (config->operation & SPI_MODE_CPHA) {
			ctl0 |= SPI_CTL0_SPH_CPHA;
		}
	}

	/* Data size */
	if (SPI_WORD_SIZE_GET(config->operation) != 8) {
		return -ENOTSUP;
	}
	ctl0 |= SPI_CTL0_DSS_8BIT;

	if (!(config->operation & SPI_TRANSFER_MSB)) {
		ctl1 |= UNICOMMSPI_CTL1_MSB_MASK;
	}

	/* Internal loopback */
	if (config->operation & SPI_MODE_LOOP) {
		ctl1 |= UNICOMMSPI_CTL1_LBM_MASK;
	}

	/* Set controller mode */
	if (SPI_OP_MODE_GET(config->operation) == SPI_OP_MODE_MASTER) {
		ctl1 |= UNICOMMSPI_CTL1_CP_MASK;
	}

	/*
	 * Get BUSCLK rate and compute SCR for the requested frequency.
	 * f_SPI = BUSCLK / (2 * (1 + SCR))  =>  SCR = ceil(BUSCLK / (2 * freq)) - 1
	 */
	ret = clock_control_get_rate(cfg->clk_dev, (clock_control_subsys_t)&cfg->clk_subsys,
				     &clock_rate);
	if (ret < 0) {
		return ret;
	}

	if (config->frequency == 0 || config->frequency > clock_rate / 2) {
		return -EINVAL;
	}

	scr = DIV_ROUND_UP(clock_rate, 2 * config->frequency) - 1;
	if (scr > UNICOMMSPI_SCR_MAX) {
		return -EINVAL;
	}

	/* Disable SPI before reconfiguring */
	UPDATE_REG(cfg->unicomm_spi_base + UNICOMM_SPI_CTL1, SPI_CTL1_DISABLE,
		   UNICOMMSPI_CTL1_ENABLE_MASK);

	/* Configure BUSCLK as clock source (CLKDIV=0: divide-by-1) */
	sys_write32(SPI_CLKDIV_DIVIDE_BY_1, cfg->unicomm_spi_base + UNICOMM_SPI_CLKDIV);
	sys_write32(SPI_CLKSEL_BUSCLK_ENABLE, cfg->unicomm_spi_base + UNICOMM_SPI_CLKSEL);

	/* Set CTL0 and CTL1 */
	UPDATE_REG(cfg->unicomm_spi_base + UNICOMM_SPI_CTL0, ctl0,
		   UNICOMMSPI_CTL0_FRF_MASK | UNICOMMSPI_CTL0_SPO_MASK | UNICOMMSPI_CTL0_SPH_MASK |
			   UNICOMMSPI_CTL0_DSS_MASK);

	UPDATE_REG(cfg->unicomm_spi_base + UNICOMM_SPI_CTL1, ctl1,
		   UNICOMMSPI_CTL1_PES_MASK | UNICOMMSPI_CTL1_PREN_MASK |
			   UNICOMMSPI_CTL1_PTEN_MASK | UNICOMMSPI_CTL1_MSB_MASK |
			   UNICOMMSPI_CTL1_CP_MASK | UNICOMMSPI_CTL1_LBM_MASK);

	/* Set SPI bitrate Serial Clock Divider (SCR) */
	UPDATE_REG(cfg->unicomm_spi_base + UNICOMM_SPI_CLKCTL, scr, UNICOMMSPI_CLKCTL_SCR_MASK);

	/* Enable SPI */
	UPDATE_REG(cfg->unicomm_spi_base + UNICOMM_SPI_CTL1, SPI_CTL1_ENABLE,
		   UNICOMMSPI_CTL1_ENABLE_MASK);

	/* Cache SPI config for reuse, required by spi_context owner */
	data->ctx.config = config;
	/* Save a content copy for content-based check*/
	data->last_config = *config;

	return 0;
}

static void spi_ti_unicomm_controller_isr(const struct device *dev)
{
	const struct spi_ti_unicomm_config *cfg = dev->config;
	struct spi_ti_unicomm_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	uint32_t base = cfg->unicomm_spi_base;
	uint32_t mis = sys_read32(base + UNICOMM_SPI_MIS);

	/* Always drain RX FIFO to prevent polluting future transfers */
	spi_ti_unicomm_rx_drain(base, ctx);

	/*
	 * Refill TX FIFO on either the TX or the TXEMPTY interrupt.
	 * TXEMPTY is triggered when all bytes have left the FIFO,
	 * which is also the completion signal when there is nothing
	 * more to send or receive.
	 *
	 * Keep feeding dummy bytes as long as RX still needs more clock cycles
	 * (Controller-mode: SCLK only runs while TXDATA is being written).
	 */
	if (mis & (UNICOMMSPI_INT_TX_MASK | UNICOMMSPI_INT_TXEMPTY_MASK)) {
		if (spi_context_tx_on(ctx) || spi_context_rx_on(ctx)) {
			spi_ti_unicomm_tx_fill(base, ctx);
		}
	}

	/* Clear all interrupts we may have serviced */
	sys_write32(mis, base + UNICOMM_SPI_ICLR);

	if ((mis & UNICOMMSPI_INT_TXEMPTY_MASK) && data->transfer_active &&
	    !spi_context_tx_on(ctx) && !spi_context_rx_on(ctx)) {
		/* Mask all UNICOMMSPI interrupts */
		sys_write32(0U, base + UNICOMM_SPI_IMASK);

		/* Wait for shift register to drain (last byte still clocking out). */
		while (sys_read32(base + UNICOMM_SPI_STAT) & UNICOMMSPI_STAT_BUSY_MASK) {
		}

		/*
		 * Any RX bytes received during the busy-wait above have to be dropped to
		 * prevent polluting the next transfer
		 */
		spi_ti_unicomm_rx_drain(base, ctx);

		/*
		 * Clear all pending interrupt sources (especially TXEMPTY which
		 * re-asserts as soon as the FIFO goes empty). This ensures the
		 * next transceive() call starts with a clean RIS and does not see
		 * a stale TXEMPTY the moment it enables IMASK.
		 */
		sys_write32(0xFFFFFFFFU, base + UNICOMM_SPI_ICLR);

		/* Deassert GPIO chip select */
		spi_context_cs_control(ctx, false);

		data->transfer_active = false;
		spi_context_complete(ctx, dev, data->isr_error);
	}
}

static void spi_ti_unicomm_peripheral_isr(const struct device *dev)
{
	const struct spi_ti_unicomm_config *cfg = dev->config;
	struct spi_ti_unicomm_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	uint32_t base = cfg->unicomm_spi_base;
	uint32_t mis = sys_read32(base + UNICOMM_SPI_MIS);

	/* Always drain RX FIFO to prevent polluting future transfers */
	spi_ti_unicomm_peripheral_rx_drain(base, ctx);

	/* Refill the TX FIFO when the controller's clock has consumed entries. */
	if (mis & UNICOMMSPI_INT_TX_MASK) {
		spi_ti_unicomm_peripheral_tx_fill(base, ctx);
	}

	if ((mis & UNICOMMSPI_INT_IDLE_MASK) && data->transfer_active) {
		spi_ti_unicomm_peripheral_rx_drain(base, ctx);

		/* Mask all UNICOMMSPI interrupts */
		sys_write32(0U, base + UNICOMM_SPI_IMASK);

		/* Clear all pending interrupt sources (incl. the RX byte that
		 * might have just latched), so the next transceive() starts
		 * with a clean RIS. */
		sys_write32(0xFFFFFFFFU, base + UNICOMM_SPI_ICLR);

		data->transfer_active = false;
		spi_context_complete(ctx, dev, data->isr_error);
		return;
	}

	/*
	 * Keep the TX threshold interrupt armed only while there is still data
	 * to send. The TX interrupt is level-based (asserted while the FIFO is
	 * at or below the threshold): on an exhausted TX FIFO it would re-fire
	 * forever after ICLR, so it is masked here while RX and IDLE remain
	 * covered until the controller ends the transaction.
	 */
	sys_write32(UNICOMMSPI_INT_RX_MASK | UNICOMMSPI_INT_RXFULL_MASK | UNICOMMSPI_INT_IDLE_MASK |
			    (spi_context_tx_on(ctx) ? UNICOMMSPI_INT_TX_MASK : 0U),
		    base + UNICOMM_SPI_IMASK);

	/* Clear serviced interrupts */
	sys_write32(mis, base + UNICOMM_SPI_ICLR);
}

static void spi_ti_unicomm_isr(const struct device *dev)
{
	const struct spi_ti_unicomm_config *cfg = dev->config;
	uint32_t base = cfg->unicomm_spi_base;

	uint32_t ctl1 = sys_read32(base + UNICOMM_SPI_CTL1);
	bool is_controller = FIELD_GET(UNICOMMSPI_CTL1_CP_MASK, ctl1);

	if (is_controller) {
		spi_ti_unicomm_controller_isr(dev);
	} else {
		spi_ti_unicomm_peripheral_isr(dev);
	}
}

static int spi_ti_unicomm_init(const struct device *dev)
{
	const struct spi_ti_unicomm_config *cfg = dev->config;
	struct spi_ti_unicomm_data *data = dev->data;
	int ret = 0;

	if (!device_is_ready(cfg->clk_dev)) {
		LOG_ERR("Clock device not ready");
		return -ENODEV;
	}

	/* Select BUSCLK as clock source (CLKDIV=0: divide-by-1) */
	sys_write32(SPI_CLKDIV_DIVIDE_BY_1, cfg->unicomm_spi_base + UNICOMM_SPI_CLKDIV);
	sys_write32(SPI_CLKSEL_BUSCLK_ENABLE, cfg->unicomm_spi_base + UNICOMM_SPI_CLKSEL);

	/*
	 * Reset defaults:
	 * CTL0: Motorola 4-wire, CPHA=1, 8-bit
	 * CTL1: controller mode, MSB first, no parity, SCR computed per-transfer
	 */
	UPDATE_REG(cfg->unicomm_spi_base + UNICOMM_SPI_CTL0,
		   SPI_CTL0_FRF_MOTOROLA_4WIRE | SPI_CTL0_SPH_CPHA | SPI_CTL0_DSS_8BIT,
		   UNICOMMSPI_CTL0_FRF_MASK | UNICOMMSPI_CTL0_SPO_MASK | UNICOMMSPI_CTL0_SPH_MASK |
			   UNICOMMSPI_CTL0_DSS_MASK);

	UPDATE_REG(cfg->unicomm_spi_base + UNICOMM_SPI_CTL1, UNICOMMSPI_CTL1_CP_MASK,
		   UNICOMMSPI_CTL1_PES_MASK | UNICOMMSPI_CTL1_PREN_MASK |
			   UNICOMMSPI_CTL1_PTEN_MASK | UNICOMMSPI_CTL1_MSB_MASK |
			   UNICOMMSPI_CTL1_CP_MASK | UNICOMMSPI_CTL1_LBM_MASK);

	/* IFLS: set TX and RX FIFO thresholds from devicetree configuration */
	UPDATE_REG(cfg->unicomm_spi_base + UNICOMM_SPI_IFLS,
		   (cfg->tx_fifo_threshold & UNICOMMSPI_IFLS_TXIFLSEL_MASK) |
			   ((cfg->rx_fifo_threshold << 4) & UNICOMMSPI_IFLS_RXIFLSEL_MASK),
		   UNICOMMSPI_IFLS_TXIFLSEL_MASK | UNICOMMSPI_IFLS_RXIFLSEL_MASK);

	/* All interrupts masked until a transfer starts */
	sys_write32(0U, cfg->unicomm_spi_base + UNICOMM_SPI_IMASK);

	/* Enable SPI */
	UPDATE_REG(cfg->unicomm_spi_base + UNICOMM_SPI_CTL1, SPI_CTL1_ENABLE,
		   UNICOMMSPI_CTL1_ENABLE_MASK);

	/* Apply pinctrl config */
	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	/* Configure any GPIO-based chip-select pins as outputs */
	ret = spi_context_cs_configure_all(&data->ctx);
	if (ret < 0) {
		return ret;
	}

	/* Connect and enable the peripheral IRQ */
	cfg->irq_config_func(dev);

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static int spi_ti_unicomm_transceive_call(const struct device *dev, const struct spi_config *config,
					  const struct spi_buf_set *tx_bufs,
					  const struct spi_buf_set *rx_bufs, bool asynchronous,
					  spi_callback_t cb, void *userdata)
{
	const struct spi_ti_unicomm_config *cfg = dev->config;
	struct spi_ti_unicomm_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	uint32_t base = cfg->unicomm_spi_base;
	bool is_controller;
	int ret = 0;

	spi_context_lock(ctx, asynchronous, cb, userdata, config);

	ret = spi_ti_unicomm_configure(dev, config);
	if (ret != 0) {
		spi_context_release(&data->ctx, ret);
		return ret;
	}

	is_controller = SPI_OP_MODE_GET(config->operation) == SPI_OP_MODE_MASTER;

	data->isr_error = 0;
	spi_context_buffers_setup(ctx, tx_bufs, rx_bufs, 1);

	/* Nothing to transmit or receive; complete immediately */
	if (!spi_context_tx_on(ctx) && !spi_context_rx_on(ctx)) {
		spi_context_release(ctx, 0);
		return 0;
	}

	if (is_controller) {
		spi_context_cs_control(ctx, true);

		spi_ti_unicomm_tx_fill(base, ctx);

		/*
		 * Mark the transfer as active before enabling interrupts. The
		 * ISR's completion check is gated on this flag, preventing it
		 * from signalling completion for a spurious TXEMPTY that fires
		 * the moment IMASK is enabled while the TX FIFO is still empty
		 * from the previous transfer.
		 */
		data->transfer_active = true;

		sys_write32(UNICOMMSPI_TRANSFER_IMASK, base + UNICOMM_SPI_IMASK);
	} else {
		/*
		 * Peripheral mode: pre-load the TX FIFO with every byte the
		 * controller is about to clock out. Unlike controller mode, no
		 * dummy bytes are ever written (SCLK is generated by the
		 * controller), so the TX FIFO sits ready until the transaction
		 * begins and is refilled on demand by the ISR.
		 */
		spi_ti_unicomm_peripheral_tx_fill(base, ctx);

		data->transfer_active = true;

		/* Flush any stale RIS/TXEMPTY state from a prior transfer
		 * before arming this one. */
		sys_write32(0xFFFFFFFFU, base + UNICOMM_SPI_ICLR);

		/*
		 * Enable RX, RXFULL, TX and IDLE interrupts. RX fires as soon
		 * as any byte is available (threshold "not empty"), TX refills
		 * the FIFO as the controller consumes it, and IDLE completes
		 * the transfer when the controller deasserts CS.
		 */
		sys_write32(UNICOMMSPI_PERIPHERAL_IMASK, base + UNICOMM_SPI_IMASK);
	}

	ret = spi_context_wait_for_completion(ctx);

	spi_context_release(ctx, ret);
	return ret;
}

static int spi_ti_unicomm_transceive(const struct device *dev, const struct spi_config *config,
				     const struct spi_buf_set *tx_bufs,
				     const struct spi_buf_set *rx_bufs)
{
	return spi_ti_unicomm_transceive_call(dev, config, tx_bufs, rx_bufs, false, NULL, NULL);
}

#ifdef CONFIG_SPI_ASYNC
static int spi_ti_unicomm_transceive_async(const struct device *dev,
					   const struct spi_config *config,
					   const struct spi_buf_set *tx_bufs,
					   const struct spi_buf_set *rx_bufs, spi_callback_t cb,
					   void *userdata)
{
	return spi_ti_unicomm_transceive_call(dev, config, tx_bufs, rx_bufs, true, cb, userdata);
}
#endif /* CONFIG_SPI_ASYNC */

static int spi_ti_unicomm_release(const struct device *dev, const struct spi_config *config)
{
	struct spi_ti_unicomm_data *data = dev->data;
	const struct spi_ti_unicomm_config *cfg = dev->config;

	if (!spi_context_configured(&data->ctx, config)) {
		return -EINVAL;
	}

	if (sys_read32(cfg->unicomm_spi_base + UNICOMM_SPI_STAT) & UNICOMMSPI_STAT_BUSY_MASK) {
		return -EBUSY;
	}

	spi_context_unlock_unconditionally(&data->ctx);
	return 0;
}

static DEVICE_API(spi, spi_ti_unicomm_api) = {
	.transceive = spi_ti_unicomm_transceive,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_ti_unicomm_transceive_async,
#endif
	.release = spi_ti_unicomm_release,
};

#define SPI_TI_UNICOMM_IRQ_FUNC(index)                                                             \
	static void spi_ti_unicomm_irq_config_##index(const struct device *dev)                    \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(index), DT_INST_IRQ(index, priority), spi_ti_unicomm_isr, \
			    DEVICE_DT_INST_GET(index), 0);                                         \
		irq_enable(DT_INST_IRQN(index));                                                   \
	}

#define SPI_TI_UNICOMM_INIT(index)                                                                 \
	PINCTRL_DT_INST_DEFINE(index);                                                             \
                                                                                                   \
	SPI_TI_UNICOMM_IRQ_FUNC(index);                                                            \
                                                                                                   \
	static const struct spi_ti_unicomm_config spi_config_##index = {                           \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                                     \
		.unicomm_spi_base =                                                                \
			(uint32_t)(DT_INST_REG_ADDR(index)) +                                      \
			COND_CODE_1(CONFIG_SOC_SERIES_MSPM33C,               \
                                      (MSPM33C_UCSPI_OFFSET), (0U)),                                  \
				     .clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(index)),         \
				     .clk_subsys = MSPM0_CLOCK_SUBSYS_FN(index),                   \
				     .irq_config_func = spi_ti_unicomm_irq_config_##index,         \
				     .tx_fifo_threshold = DT_INST_PROP(index, tx_fifo_threshold),  \
				     .rx_fifo_threshold = DT_INST_PROP(index, rx_fifo_threshold),  \
	};                                                                                         \
                                                                                                   \
	static struct spi_ti_unicomm_data spi_data_##index = {                                     \
		SPI_CONTEXT_INIT_LOCK(spi_data_##index, ctx),                                      \
		SPI_CONTEXT_INIT_SYNC(spi_data_##index, ctx),                                      \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(index), ctx)};                         \
                                                                                                   \
	SPI_DEVICE_DT_INST_DEFINE(index, spi_ti_unicomm_init, NULL, &spi_data_##index,             \
				  &spi_config_##index, POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,      \
				  &spi_ti_unicomm_api);

DT_INST_FOREACH_STATUS_OKAY(SPI_TI_UNICOMM_INIT)
