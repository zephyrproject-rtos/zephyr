/*
 * Copyright (c) 2026 Jonathan Elliot Peace <jep@alphabetiq.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Broadcom BCM283x Arasan SDHCI host controller. Standard SDHCI
 * register layout with three silicon quirks:
 *
 *   - All register accesses must be 32-bit aligned and 32-bit wide;
 *     sub-word accesses corrupt or hang the controller. SDHCI 16-bit
 *     fields (BLKSIZECNT, CMDTM, CONTROL0, CONTROL1) are composed in
 *     32-bit values rather than via an iproc-style shadow layer.
 *
 *   - The CAPABILITIES register at 0x40 reads as zero on this
 *     silicon. get_host_props() returns hardcoded values mirroring
 *     Linux's sdhci-iproc.c::bcm2835_data.
 *
 *   - SDHCI-spec quirks apply: BROKEN_CARD_DETECTION (no CD line),
 *     NO_HISPD_BIT (high speed from clock divider only),
 *     DATA_TIMEOUT_USES_SDCLK (DATA_TOUT counts SD clocks), and
 *     PRESET_VALUE_BROKEN (configure clock/voltage explicitly).
 *
 * IRQ-driven completion: CMD_COMPLETE / DATA_END are SIGNAL_ENABLE'd
 * and demuxed in the ISR; the request thread sleeps on cmd_done /
 * data_done sems. The Arasan's built-in DMA is not usable
 * (CAPABILITIES reads zero) so data bytes move through the BUFFER
 * FIFO via the external BCM2835 DMA engine, DREQ-handshaked. R1b/R5b
 * responses are not supported.
 */

#define DT_DRV_COMPAT brcm_bcm2835_sdhci

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/sdhc.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/cache.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/arch/arm64/arm_mem.h>
#include <zephyr/sd/sd_spec.h>
#include <string.h>

LOG_MODULE_REGISTER(sdhc_bcm2835, CONFIG_SDHC_LOG_LEVEL);

/* Standard SDHCI register offsets we use. Names mirror Linux's
 * include/linux/mmc/sdhci.h so cross-references read 1:1. The BCM
 * datasheet packages some of these as 32-bit aggregates (e.g. CONTROL1
 * = CLOCK_CONTROL + TIMEOUT + SOFTWARE_RESET); we always touch the
 * containing 32-bit word.
 */
#define SDHCI_BLKSIZECNT		0x04	/* BLOCK_SIZE | BLOCK_COUNT<<16 */
#define SDHCI_ARG1			0x08	/* command argument (32-bit) */
#define SDHCI_BUFFER			0x20	/* PIO buffer data port (32-bit) */
#define SDHCI_CMDTM			0x0C	/* TRANSFER_MODE | COMMAND<<16 */
#define SDHCI_RESPONSE			0x10	/* RESP0..RESP3 (4x 32-bit) */
#define SDHCI_PRESENT_STATE		0x24	/* "STATUS" in BCM doc */
#define SDHCI_HOST_CONTROL		0x28	/* CONTROL0 word: HCTL+POWER+... */
#define SDHCI_CLOCK_CONTROL		0x2C	/* CONTROL1 word: CLK+TOUT+RESET */
#define SDHCI_INT_STATUS		0x30	/* INTERRUPT (W1C) */
#define SDHCI_INT_ENABLE		0x34	/* IRPT_MASK */
#define SDHCI_SIGNAL_ENABLE		0x38	/* IRPT_EN */
#define SDHCI_SLOT_INT_STATUS_VERSION	0xFC

/* PRESENT_STATE bits */
#define SDHCI_PSTATE_CMD_INHIBIT		BIT(0)	/* CMD line busy */
#define SDHCI_PSTATE_DATA_INHIBIT		BIT(1)	/* DAT lines busy */
#define SDHCI_PSTATE_BUFFER_WRITE_ENABLE	BIT(10)	/* FIFO has space for write */
#define SDHCI_PSTATE_BUFFER_READ_ENABLE		BIT(11)	/* FIFO has data to read */

/* CMDTM (32-bit at 0x0C). The low 16 bits are TRANSFER_MODE, high 16 bits
 * are COMMAND. Per spec, writing the COMMAND half (= writing the 32-bit
 * word) is what fires the command on the CMD line.
 */
#define SDHCI_CMDTM_TM_BLKCNT_EN	BIT(1)	/* enable block count for data */
#define SDHCI_CMDTM_TM_AUTO_CMD12	BIT(2)
#define SDHCI_CMDTM_TM_AUTO_CMD23	BIT(3)
#define SDHCI_CMDTM_TM_DAT_DIR_READ	BIT(4)	/* 1 = card->host */
#define SDHCI_CMDTM_TM_MULTI_BLOCK	BIT(5)
#define SDHCI_CMDTM_RSP_NONE		(0 << 16)
#define SDHCI_CMDTM_RSP_136		(1 << 16)	/* long, R2 */
#define SDHCI_CMDTM_RSP_48		(2 << 16)	/* short, R1/R3/R5/R6/R7 */
#define SDHCI_CMDTM_RSP_48_BUSY		(3 << 16)	/* short with busy, R1b */
#define SDHCI_CMDTM_CRC_CHECK		BIT(16 + 3)	/* check response CRC */
#define SDHCI_CMDTM_INDEX_CHECK		BIT(16 + 4)	/* check response index */
#define SDHCI_CMDTM_DATA_PRESENT	BIT(16 + 5)	/* command has data phase */
#define SDHCI_CMDTM_TYPE_ABORT		(3 << (16 + 6))
#define SDHCI_CMDTM_INDEX_SHIFT		(16 + 8)

/* INTERRUPT (32-bit at 0x30): low 16 = NORMAL, high 16 = ERROR. W1C. */
#define SDHCI_INT_CMD_COMPLETE		BIT(0)
#define SDHCI_INT_DATA_END		BIT(1)
#define SDHCI_INT_BUF_WRITE_READY	BIT(4)
#define SDHCI_INT_BUF_READ_READY	BIT(5)
#define SDHCI_INT_CARD_INSERT		BIT(6)
#define SDHCI_INT_CARD_REMOVE		BIT(7)
#define SDHCI_INT_CARD_INT		BIT(8)	/* SDIO async event */
#define SDHCI_INT_ERROR			BIT(15)
#define SDHCI_INT_CMD_TIMEOUT		BIT(16)
#define SDHCI_INT_CMD_CRC		BIT(17)
#define SDHCI_INT_CMD_END_BIT		BIT(18)
#define SDHCI_INT_CMD_INDEX		BIT(19)
#define SDHCI_INT_DATA_TIMEOUT		BIT(20)
#define SDHCI_INT_DATA_CRC		BIT(21)
#define SDHCI_INT_DATA_END_BIT		BIT(22)

#define SDHCI_INT_CMD_ERROR_MASK	(SDHCI_INT_CMD_TIMEOUT | \
					 SDHCI_INT_CMD_CRC     | \
					 SDHCI_INT_CMD_END_BIT | \
					 SDHCI_INT_CMD_INDEX)
#define SDHCI_INT_DATA_ERROR_MASK	(SDHCI_INT_DATA_TIMEOUT | \
					 SDHCI_INT_DATA_CRC     | \
					 SDHCI_INT_DATA_END_BIT)
#define SDHCI_INT_ALL_NORMAL		0x0000FFFFU
#define SDHCI_INT_ALL_ERROR		0xFFFF0000U
#define SDHCI_INT_ALL_W1C		(SDHCI_INT_ALL_NORMAL | SDHCI_INT_ALL_ERROR)

/* CONTROL0 (32-bit at 0x28) -- HCTL[7:0] + POWER[15:8] + BLOCK_GAP[23:16]
 * + WAKEUP[31:24]. We touch HCTL bits and the POWER subregister.
 */
#define SDHCI_CTRL0_HCTL_DWIDTH		BIT(1)	/* 4-bit data bus */
#define SDHCI_CTRL0_HCTL_HS		BIT(2)	/* high-speed mode (NO_HISPD_BIT
						 * quirk: silicon ignores this --
						 * speed is set via clock divider)
						 */
#define SDHCI_CTRL0_HCTL_8BIT		BIT(5)	/* not supported on this silicon */
#define SDHCI_CTRL0_POWER_ON		BIT(8)	/* SD bus power enable */
#define SDHCI_CTRL0_VOLT_SHIFT		9
#define SDHCI_CTRL0_VOLT_MASK		(0x7 << SDHCI_CTRL0_VOLT_SHIFT)
#define SDHCI_CTRL0_VOLT_330		(0x7 << SDHCI_CTRL0_VOLT_SHIFT)

/* CONTROL1 (32-bit at 0x2C) -- standard SDHCI clock + timeout + reset */
#define SDHCI_CTRL1_CLK_INTLEN		BIT(0)	/* internal clock enable */
#define SDHCI_CTRL1_CLK_STABLE		BIT(1)	/* internal clock stable (RO) */
#define SDHCI_CTRL1_CLK_EN		BIT(2)	/* SD bus clock enable */
#define SDHCI_CTRL1_CLK_GENSEL		BIT(5)	/* 0 = divided clock mode */
#define SDHCI_CTRL1_CLK_FREQ_MS_SHIFT	6	/* upper 2 bits of 10-bit div */
#define SDHCI_CTRL1_CLK_FREQ_MS_MASK	(0x3 << SDHCI_CTRL1_CLK_FREQ_MS_SHIFT)
#define SDHCI_CTRL1_CLK_FREQ_LO_SHIFT	8	/* lower 8 bits of 10-bit div */
#define SDHCI_CTRL1_CLK_FREQ_LO_MASK	(0xFF << SDHCI_CTRL1_CLK_FREQ_LO_SHIFT)
#define SDHCI_CTRL1_CLK_FREQ_MASK	(SDHCI_CTRL1_CLK_FREQ_MS_MASK | \
					 SDHCI_CTRL1_CLK_FREQ_LO_MASK)
#define SDHCI_CTRL1_DATA_TOUT_SHIFT	16	/* timeout exponent */
#define SDHCI_CTRL1_DATA_TOUT_MASK	(0xF << SDHCI_CTRL1_DATA_TOUT_SHIFT)
#define SDHCI_CTRL1_RESET_ALL		BIT(24)	/* SRST_HC: reset whole HC */
#define SDHCI_CTRL1_RESET_CMD		BIT(25)	/* SRST_CMD: cmd line reset */
#define SDHCI_CTRL1_RESET_DATA		BIT(26)	/* SRST_DATA: data line reset */

/* Software reset timeout. Linux's sdhci.c defaults to 100 ms here and
 * that's plenty for any sane controller -- the bit self-clears in
 * microseconds on healthy silicon. Same ceiling for clock-stable.
 */
#define BCM2835_SDHCI_RESET_TIMEOUT_MS	100
#define BCM2835_SDHCI_CLK_STABLE_MS	100

/* Hardcoded host capabilities -- see sdhci-iproc.c::bcm2835_data.
 * The silicon's CAPABILITIES register at 0x40 reads as zero on this
 * controller, so we expose these constants from get_host_props().
 */
#define BCM2835_MAX_BLOCK_BYTES		1024	/* internal FIFO size */
#define BCM2835_F_MIN_HZ		400000	/* card identification */
#define BCM2835_F_MAX_HZ		50000000 /* SDR25 / high speed */

struct sdhc_bcm2835_config {
	DEVICE_MMIO_ROM;
	const struct pinctrl_dev_config *pincfg;
	const struct device *dma_dev;
	uint32_t clock_freq;
	uint32_t fifo_phys;	/* ARM-phys addr of SDHCI BUFFER (base + 0x20) */
	uint32_t dma_dreq;	/* DREQ slot programmed into the DMA channel */
	uint8_t bus_width;
};

struct sdhc_bcm2835_data {
	DEVICE_MMIO_RAM;
	struct sdhc_io host_io;
	/* External DMA engine state. One channel acquired at init and
	 * reconfigured per-transfer for direction; only one transfer is
	 * ever in flight on this controller so no per-direction split.
	 */
	uint32_t dma_channel;
	struct k_sem dma_done;
	int dma_status;		/* set by callback; 0 = success */
	struct dma_block_config dma_block;
	struct dma_config dma_cfg;

	/* IRQ-driven CMD/DATA completion. The ISR captures INT_STATUS
	 * bits matching cmd_mask / data_mask, ACKs them by W1C, and
	 * gives the sem. Only one command/transfer is ever in flight, so
	 * a single state slot is enough.
	 */
	struct k_sem cmd_done;
	struct k_sem data_done;
	uint32_t cmd_status;	/* captured INT_STATUS bits (success + errors) */
	uint32_t data_status;

	/* SDIO in-band interrupt callback (set via .enable_interrupt).
	 * Mirrors Linux's sdhci card-irq path. The chip asserts DAT1
	 * (CARD_INT) to wake the SDIO consumer.
	 */
	sdhc_interrupt_cb_t sdhc_cb;
	void *sdhc_cb_user_data;
};

/* Forward decl: error recovery in request() needs to reset the cmd line. */
static int sdhc_bcm2835_soft_reset(const struct device *dev, uint32_t mask);

/* Forward decl: IRQ_CONNECT in init() references it; body is below
 * (alongside the .enable_interrupt / .disable_interrupt implementations).
 */
static void sdhc_bcm2835_isr(const struct device *dev);

/* Default command timeout when the caller leaves cmd->timeout_ms zero.
 * Linux uses 10s for non-data commands. On BCM2835 the hardware
 * CMD_TIMEOUT is governed by CONTROL1.DATA_TOUNIT (extension to
 * SDHCI spec on this Arasan integration), which we set to 0x6
 * (~1.3 s at 400 kHz SDCLK). 5 s here gives the hardware enough
 * margin to fire its bit before we give up.
 */
#define BCM2835_SDHCI_CMD_TIMEOUT_MS	5000

/* Build the full 32-bit CMDTM word -- TRANSFER_MODE in the low 16 bits,
 * COMMAND in the high 16. Writing this 32-bit word fires the command;
 * the BCM 32-bit-only access requirement makes the natural composition
 * here (write the whole word in one go) the right thing.
 *
 * We deliberately don't support R1b yet: the controller waits for DAT0
 * to clear after a busy response, which can hang indefinitely if the
 * card never deasserts busy. SDIO data traffic (CMD53) uses R5, not
 * R5b -- so R1b isn't on the hot path.
 */
static int sdhc_bcm2835_build_cmd(const struct sdhc_command *cmd,
				  const struct sdhc_data *data, uint32_t *cmdtm)
{
	uint32_t flags = (uint32_t)cmd->opcode << SDHCI_CMDTM_INDEX_SHIFT;
	uint32_t rsp = cmd->response_type & SDHC_NATIVE_RESPONSE_MASK;

	if (data != NULL) {
		flags |= SDHCI_CMDTM_DATA_PRESENT;
		/* TRANSFER_MODE bits (low 16): block-count enable, direction,
		 * multi-block if blocks > 1. AUTO_CMD12 / AUTO_CMD23 are not
		 * used -- SDIO CMD53 carries its own byte/block count and the
		 * card terminates without needing CMD12.
		 *
		 * Direction inference: Zephyr's sdhc_data has no explicit
		 * direction field. For canonical SD opcodes the direction is
		 * baked into the opcode; for SDIO CMD53 it's in arg bit 31
		 * per the SDIO spec (1=write, 0=read).
		 */
		bool is_read;

		switch (cmd->opcode) {
		case SDIO_RW_EXTENDED:
			is_read = !(cmd->arg & BIT(SDIO_CMD_ARG_RW_SHIFT));
			break;
		case SD_READ_SINGLE_BLOCK:
		case SD_READ_MULTIPLE_BLOCK:
		case SD_APP_SEND_SCR:
			is_read = true;
			break;
		case SD_WRITE_SINGLE_BLOCK:
		case SD_WRITE_MULTIPLE_BLOCK:
			is_read = false;
			break;
		default:
			/* Other opcodes shouldn't carry a data phase. Refuse
			 * rather than silently picking a direction.
			 */
			return -EINVAL;
		}

		flags |= SDHCI_CMDTM_TM_BLKCNT_EN;
		if (is_read) {
			flags |= SDHCI_CMDTM_TM_DAT_DIR_READ;
		}
		if (data->blocks > 1) {
			flags |= SDHCI_CMDTM_TM_MULTI_BLOCK;
		}
	}

	switch (rsp) {
	case SD_RSP_TYPE_NONE:
		flags |= SDHCI_CMDTM_RSP_NONE;
		break;
	case SD_RSP_TYPE_R2:
		flags |= SDHCI_CMDTM_RSP_136 | SDHCI_CMDTM_CRC_CHECK;
		break;
	case SD_RSP_TYPE_R3:
	case SD_RSP_TYPE_R4:
		/* OCR / IO_SEND_OP_COND -- no CRC check, no index check */
		flags |= SDHCI_CMDTM_RSP_48;
		break;
	case SD_RSP_TYPE_R1:
	case SD_RSP_TYPE_R5:
	case SD_RSP_TYPE_R6:
	case SD_RSP_TYPE_R7:
		flags |= SDHCI_CMDTM_RSP_48 | SDHCI_CMDTM_CRC_CHECK |
			 SDHCI_CMDTM_INDEX_CHECK;
		break;
	case SD_RSP_TYPE_R1b:
	case SD_RSP_TYPE_R5b:
		/* TODO: R1b/R5b need busy-wait on DAT0; not handled yet. */
		return -ENOTSUP;
	default:
		return -EINVAL;
	}

	*cmdtm = flags;
	return 0;
}

/* External DMA completion. The Zephyr DMA core invokes this from an ISR
 * context after the BCM2835 DMA engine raises its per-channel done bit.
 * Just stash status + wake the request thread; the SDHCI side has its
 * own DATA_END to wait on separately.
 */
static void sdhc_bcm2835_dma_cb(const struct device *dma_dev, void *user_data,
				uint32_t channel, int status)
{
	struct sdhc_bcm2835_data *drvdata = user_data;

	ARG_UNUSED(dma_dev);
	ARG_UNUSED(channel);
	drvdata->dma_status = status;
	k_sem_give(&drvdata->dma_done);
}

/* Arm the external DMA engine to drain (RX) or fill (TX) the SDHCI
 * BUFFER FIFO. Must be called BEFORE the command is fired -- otherwise
 * the controller's FIFO can fill on a multi-KB read while DMA is still
 * being configured, stalling the data phase or worse losing data.
 *
 * The controller itself is NOT told it's doing DMA (TRANSFER_MODE.DMA_EN
 * stays zero); from its perspective the FIFO is accessed via the BUFFER
 * port as in PIO. The external engine just steals every FIFO access via
 * the DREQ handshake, freeing the CPU to do other work.
 *
 * Cache management on the user buffer:
 *   TX: flush -- write back any CPU-dirty cache lines so the engine
 *       reads the latest data from DRAM.
 *   RX: invalidate -- drop any speculatively-loaded stale lines so the
 *       engine's DRAM writes are not masked when the CPU re-reads.
 * A second invalidate-after on RX lives in transfer_data().
 *
 * Requires CONFIG_CACHE_MANAGEMENT=y (Kconfig.bcm2835 selects it).
 */
static int sdhc_bcm2835_dma_prepare(const struct device *dev,
				    struct sdhc_data *data, bool is_read)
{
	const struct sdhc_bcm2835_config *cfg = dev->config;
	struct sdhc_bcm2835_data *drvdata = dev->data;
	size_t total = (size_t)data->blocks * data->block_size;
	int ret;

	if (is_read) {
		sys_cache_data_invd_range(data->data, total);
	} else {
		sys_cache_data_flush_range(data->data, total);
	}

	memset(&drvdata->dma_block, 0, sizeof(drvdata->dma_block));
	memset(&drvdata->dma_cfg, 0, sizeof(drvdata->dma_cfg));

	if (is_read) {
		drvdata->dma_block.source_address = cfg->fifo_phys;
		drvdata->dma_block.dest_address   = (uint32_t)(uintptr_t)data->data;
		drvdata->dma_block.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
		drvdata->dma_block.dest_addr_adj   = DMA_ADDR_ADJ_INCREMENT;
		drvdata->dma_cfg.channel_direction = PERIPHERAL_TO_MEMORY;
	} else {
		drvdata->dma_block.source_address = (uint32_t)(uintptr_t)data->data;
		drvdata->dma_block.dest_address   = cfg->fifo_phys;
		drvdata->dma_block.source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
		drvdata->dma_block.dest_addr_adj   = DMA_ADDR_ADJ_NO_CHANGE;
		drvdata->dma_cfg.channel_direction = MEMORY_TO_PERIPHERAL;
	}
	drvdata->dma_block.block_size = total;

	drvdata->dma_cfg.dma_slot             = cfg->dma_dreq;
	drvdata->dma_cfg.complete_callback_en = 1;
	drvdata->dma_cfg.block_count          = 1;
	drvdata->dma_cfg.head_block           = &drvdata->dma_block;
	drvdata->dma_cfg.dma_callback         = sdhc_bcm2835_dma_cb;
	drvdata->dma_cfg.user_data            = drvdata;
	/* One 32-bit word per DREQ — matches the SDHCI BUFFER access width. */
	drvdata->dma_cfg.source_data_size     = 4;
	drvdata->dma_cfg.dest_data_size       = 4;
	drvdata->dma_cfg.source_burst_length  = 4;
	drvdata->dma_cfg.dest_burst_length    = 4;

	k_sem_reset(&drvdata->dma_done);
	drvdata->dma_status = 0;

	ret = dma_config(cfg->dma_dev, drvdata->dma_channel, &drvdata->dma_cfg);
	if (ret != 0) {
		LOG_ERR("dma_config failed: %d", ret);
		return ret;
	}
	ret = dma_start(cfg->dma_dev, drvdata->dma_channel);
	if (ret != 0) {
		LOG_ERR("dma_start failed: %d", ret);
		return ret;
	}
	return 0;
}

/* Wait for the data phase to finish on both sides: the SDHCI controller
 * raises DATA_END once it has moved every byte across the FIFO; the DMA
 * engine raises its completion callback once it has flushed the last
 * burst into DRAM (for RX) or read the last burst from DRAM (for TX).
 * Both must happen; either-order works in practice but we wait DATA_END
 * first because it carries the controller-side error bits.
 *
 * Post-DMA invalidate on RX guards against any speculative line-fill
 * the CPU may have done into cache while DMA was in flight.
 */
static int sdhc_bcm2835_transfer_data(const struct device *dev,
				      struct sdhc_data *data, bool is_read,
				      int timeout_ms)
{
	const struct sdhc_bcm2835_config *cfg = dev->config;
	struct sdhc_bcm2835_data *drvdata = dev->data;
	uintptr_t base = DEVICE_MMIO_GET(dev);
	size_t total = (size_t)data->blocks * data->block_size;
	uint32_t int_status;

	if (k_sem_take(&drvdata->data_done, K_MSEC(timeout_ms)) != 0) {
		LOG_ERR("data %s: DATA_END timeout (pstate=0x%08x)",
			is_read ? "read" : "write",
			sys_read32(base + SDHCI_PRESENT_STATE));
		dma_stop(cfg->dma_dev, drvdata->dma_channel);
		return -ETIMEDOUT;
	}
	int_status = drvdata->data_status;
	if (int_status & SDHCI_INT_DATA_ERROR_MASK) {
		LOG_ERR("data %s end: int_status=0x%08x (pstate=0x%08x)",
			is_read ? "read" : "write", int_status,
			sys_read32(base + SDHCI_PRESENT_STATE));
		(void)sdhc_bcm2835_soft_reset(dev, SDHCI_CTRL1_RESET_DATA);
		dma_stop(cfg->dma_dev, drvdata->dma_channel);
		return -EIO;
	}

	/* DMA-side completion. DATA_END fires when the SDHCI moves the last
	 * byte across the FIFO; the DMA engine has at most one burst still
	 * to flush. 100 ms is several orders of magnitude over worst-case.
	 */
	if (k_sem_take(&drvdata->dma_done, K_MSEC(100)) != 0) {
		LOG_ERR("data %s: DMA callback timeout after DATA_END",
			is_read ? "read" : "write");
		dma_stop(cfg->dma_dev, drvdata->dma_channel);
		return -EIO;
	}
	if (drvdata->dma_status != 0) {
		LOG_ERR("data %s: DMA callback status %d",
			is_read ? "read" : "write", drvdata->dma_status);
		return -EIO;
	}

	if (is_read) {
		sys_cache_data_invd_range(data->data, total);
	}

	data->bytes_xfered = total;
	return 0;
}

/* Copy the response register(s) into cmd->response[]. R2 (long form,
 * 136-bit CID/CSD) populates RESP0..RESP3 with the CRC/start bits
 * stripped; everyone else uses RESP0 alone.
 */
static void sdhc_bcm2835_read_response(const struct device *dev,
				       struct sdhc_command *cmd)
{
	uintptr_t base = DEVICE_MMIO_GET(dev);
	uint32_t rsp = cmd->response_type & SDHC_NATIVE_RESPONSE_MASK;

	cmd->response[0] = sys_read32(base + SDHCI_RESPONSE + 0);
	if (rsp == SD_RSP_TYPE_R2) {
		cmd->response[1] = sys_read32(base + SDHCI_RESPONSE + 4);
		cmd->response[2] = sys_read32(base + SDHCI_RESPONSE + 8);
		cmd->response[3] = sys_read32(base + SDHCI_RESPONSE + 12);
	}
}

static int sdhc_bcm2835_request(const struct device *dev,
				struct sdhc_command *cmd,
				struct sdhc_data *data)
{
	uintptr_t base = DEVICE_MMIO_GET(dev);
	struct sdhc_bcm2835_data *drvdata = dev->data;
	uint32_t cmdtm;
	uint32_t int_status;
	int cmd_timeout_ms;
	int data_timeout_ms;
	bool is_read;
	int ret;

	if (cmd == NULL) {
		return -EINVAL;
	}
	if (data != NULL) {
		if (data->data == NULL || data->blocks == 0 ||
		    data->block_size == 0 ||
		    data->block_size > BCM2835_MAX_BLOCK_BYTES) {
			return -EINVAL;
		}
	}

	ret = sdhc_bcm2835_build_cmd(cmd, data, &cmdtm);
	if (ret != 0) {
		return ret;
	}

	is_read = (cmdtm & SDHCI_CMDTM_TM_DAT_DIR_READ) != 0;
	cmd_timeout_ms = cmd->timeout_ms ? cmd->timeout_ms
					 : BCM2835_SDHCI_CMD_TIMEOUT_MS;
	data_timeout_ms = (data && data->timeout_ms) ? data->timeout_ms
						     : cmd_timeout_ms;

	/* Reset the completion sems before firing so a stale give from the
	 * previous request (e.g. interleaved with a DAT0 busy ISR) can't
	 * race ahead of the new k_sem_take. PSTATE inhibit bits are
	 * guaranteed clear by IRQ ordering -- previous request returned
	 * only after CMD_COMPLETE (+ DATA_END if applicable) acked, which
	 * is also when the controller clears CMD_INHIBIT / DATA_INHIBIT.
	 * No defensive poll: if inhibit is somehow stuck, the new CMD53
	 * will time out and surface the error rather than mask it with a
	 * silent wait_inhibit fallback.
	 */
	k_sem_reset(&drvdata->cmd_done);
	if (data != NULL) {
		k_sem_reset(&drvdata->data_done);
	}

	if (data != NULL) {
		uint32_t blksizecnt = (data->block_size & 0x3FF) |
				      ((uint32_t)data->blocks << 16);
		sys_write32(blksizecnt, base + SDHCI_BLKSIZECNT);

		/* Arm the external DMA channel BEFORE the command fires; the
		 * controller starts moving bytes through the BUFFER FIFO as
		 * soon as it processes the data phase, and the FIFO can fill
		 * in microseconds on a multi-KB read.
		 */
		ret = sdhc_bcm2835_dma_prepare(dev, data, is_read);
		if (ret != 0) {
			return ret;
		}
	}

	sys_write32(cmd->arg, base + SDHCI_ARG1);
	sys_write32(cmdtm, base + SDHCI_CMDTM);	/* fires command */

	if (k_sem_take(&drvdata->cmd_done, K_MSEC(cmd_timeout_ms)) != 0) {
		LOG_ERR("CMD%u arg=0x%08x: ISR timeout pstate=0x%08x int_status=0x%08x",
			cmd->opcode, cmd->arg,
			sys_read32(base + SDHCI_PRESENT_STATE),
			sys_read32(base + SDHCI_INT_STATUS));
		(void)sdhc_bcm2835_soft_reset(dev, SDHCI_CTRL1_RESET_CMD);
		if (data != NULL) {
			(void)sdhc_bcm2835_soft_reset(dev,
						      SDHCI_CTRL1_RESET_DATA);
		}
		return -ETIMEDOUT;
	}
	int_status = drvdata->cmd_status;

	if (int_status & SDHCI_INT_CMD_ERROR_MASK) {
		uint32_t cmd_err = int_status & SDHCI_INT_CMD_ERROR_MASK;

		/* Plain CMD_TIMEOUT (no CRC / end-bit / index error alongside)
		 * is expected for some probe commands -- e.g. CMD8 against
		 * legacy SDIO cards that do not implement it -- and the SD
		 * subsystem retries before falling back. Demote timeout-only
		 * to LOG_DBG; keep CRC / end-bit / index at LOG_ERR.
		 */
		if (cmd_err == SDHCI_INT_CMD_TIMEOUT) {
			LOG_DBG("CMD%u arg=0x%08x: timeout (pstate=0x%08x)",
				cmd->opcode, cmd->arg,
				sys_read32(base + SDHCI_PRESENT_STATE));
		} else {
			LOG_ERR("CMD%u arg=0x%08x: int_status=0x%08x pstate=0x%08x",
				cmd->opcode, cmd->arg, int_status,
				sys_read32(base + SDHCI_PRESENT_STATE));
		}
		/* Reset the CMD state machine to recover from the error.
		 * Only reset the DATA state machine if this command had a
		 * data phase; CMD-only commands (CMD0/3/5/7/8/52) don't
		 * touch the DAT lines and pulsing RESET_DATA for them is
		 * unnecessary -- and on this silicon, the repeated
		 * RESET_DATA pulses during e.g. the 11 CMD8 retries of
		 * sd_init's SD-2.0 probe destabilise the DAT state machine
		 * for subsequent CMD53s.
		 */
		(void)sdhc_bcm2835_soft_reset(dev, SDHCI_CTRL1_RESET_CMD);
		if (data != NULL) {
			(void)sdhc_bcm2835_soft_reset(dev,
						      SDHCI_CTRL1_RESET_DATA);
		}
		if (int_status & SDHCI_INT_CMD_TIMEOUT) {
			return -ETIMEDOUT;
		}
		return -EIO;
	}

	sdhc_bcm2835_read_response(dev, cmd);

	if (data != NULL) {
		ret = sdhc_bcm2835_transfer_data(dev, data, is_read,
						 data_timeout_ms);
		if (ret != 0) {
			return ret;
		}
	}

	return 0;
}

/* Compute the 10-bit "divided clock mode" divider that yields the
 * largest SD bus clock <= target_hz, given clk_emmc as the input. The
 * SDHCI v3 spec defines the divisor as 2 * V where V is the 10-bit
 * value programmed into CLK_FREQ; V=0 means 1:1 (max base clock).
 *
 * V = ceil(clk_emmc / (2 * target_hz))
 *
 * Linux's sdhci.c does the same calculation (see __sdhci_calc_clock).
 * We ceil-divide so we never run the bus *above* target_hz.
 */
static uint32_t sdhc_bcm2835_calc_clk_div(uint32_t clk_in, uint32_t target_hz)
{
	uint32_t div;

	if (target_hz == 0 || target_hz >= clk_in) {
		return 0;	/* V=0 => bus = clk_in */
	}

	div = (clk_in + (target_hz * 2) - 1) / (target_hz * 2);
	if (div > 0x3FF) {
		div = 0x3FF;	/* clamp to 10-bit */
	}
	return div;
}

static int sdhc_bcm2835_set_clock(const struct device *dev, uint32_t target_hz)
{
	uintptr_t base = DEVICE_MMIO_GET(dev);
	const struct sdhc_bcm2835_config *cfg = dev->config;
	uint32_t ctrl1;
	uint32_t div;
	int64_t deadline;

	if (target_hz == 0) {
		/* Gate the bus clock by clearing CLK_EN only. */
		ctrl1 = sys_read32(base + SDHCI_CLOCK_CONTROL);
		ctrl1 &= ~SDHCI_CTRL1_CLK_EN;
		sys_write32(ctrl1, base + SDHCI_CLOCK_CONTROL);
		return 0;
	}

	/* Single atomic CTL1 write with divider + DATA_TOUT + CLK_INTLEN +
	 * CLK_EN. The spec-suggested disable / configure / enable dance
	 * disturbs the DAT-side state machine on this silicon, causing the
	 * first CMD53 after sd_init's bus-config ramps to fire spurious
	 * DATA_CRC.
	 *
	 * DATA_TOUNIT = 0xE -- 2^27 SDCLK cycles (~5.4 s at 25 MHz, the
	 * spec-max). NO_HISPD_BIT quirk: we don't touch HCTL_HS in CONTROL0
	 * -- the silicon ignores it; speed comes from the divider alone.
	 */
	div = sdhc_bcm2835_calc_clk_div(cfg->clock_freq, target_hz);
	ctrl1 = (((div >> 8) & 0x3) << SDHCI_CTRL1_CLK_FREQ_MS_SHIFT) |
		((div & 0xFF) << SDHCI_CTRL1_CLK_FREQ_LO_SHIFT) |
		(0xE << SDHCI_CTRL1_DATA_TOUT_SHIFT) |
		SDHCI_CTRL1_CLK_INTLEN |
		SDHCI_CTRL1_CLK_EN;
	sys_write32(ctrl1, base + SDHCI_CLOCK_CONTROL);

	/* Wait for the internal clock to stabilise. */
	deadline = k_uptime_get() + BCM2835_SDHCI_CLK_STABLE_MS;
	while (!(sys_read32(base + SDHCI_CLOCK_CONTROL) & SDHCI_CTRL1_CLK_STABLE)) {
		if (k_uptime_get() > deadline) {
			return -ETIMEDOUT;
		}
		k_busy_wait(10);
	}

	return 0;
}

static void sdhc_bcm2835_set_bus_width(const struct device *dev,
				       enum sdhc_bus_width width)
{
	uintptr_t base = DEVICE_MMIO_GET(dev);
	uint32_t ctrl0 = sys_read32(base + SDHCI_HOST_CONTROL);

	/* 8-bit bus is unreachable on this silicon (caps say so + we don't
	 * advertise it); a future caller asking for it is a Zephyr SD core
	 * bug, not something we'd silently expand to. Treat 1-bit as the
	 * fallback for any non-4-bit value.
	 */
	ctrl0 &= ~(SDHCI_CTRL0_HCTL_DWIDTH | SDHCI_CTRL0_HCTL_8BIT);
	if (width == SDHC_BUS_WIDTH4BIT) {
		ctrl0 |= SDHCI_CTRL0_HCTL_DWIDTH;
	}

	/* Reset the DAT state machine around the CTL0 write. Plain RMW
	 * (Linux sdhci_set_bus_width style) is not enough on this
	 * silicon: the first 4-bit data transfer after a width change
	 * spuriously fires DATA_CRC without it.
	 */
	(void)sdhc_bcm2835_soft_reset(dev, SDHCI_CTRL1_RESET_DATA);
	sys_write32(ctrl0, base + SDHCI_HOST_CONTROL);
}

static void sdhc_bcm2835_set_power(const struct device *dev,
				   enum sdhc_power power_mode)
{
	uintptr_t base = DEVICE_MMIO_GET(dev);
	uint32_t ctrl0 = sys_read32(base + SDHCI_HOST_CONTROL);

	ctrl0 &= ~(SDHCI_CTRL0_POWER_ON | SDHCI_CTRL0_VOLT_MASK);

	if (power_mode == SDHC_POWER_ON) {
		/* This silicon is 3.3V-only; there's no actual voltage
		 * switching to do, but the controller's state machine still
		 * wants the voltage select bits programmed alongside POWER.
		 */
		ctrl0 |= SDHCI_CTRL0_VOLT_330 | SDHCI_CTRL0_POWER_ON;
	}

	sys_write32(ctrl0, base + SDHCI_HOST_CONTROL);
}

static int sdhc_bcm2835_set_io(const struct device *dev, struct sdhc_io *ios)
{
	struct sdhc_bcm2835_data *data = dev->data;
	int ret;

	/* The Zephyr SD core calls set_io for every state change (clock,
	 * width, power, voltage, timing). We always reprogram unconditionally
	 * rather than diffing against the cached host_io -- it's a few extra
	 * register writes during init and saves any state-tracking bug
	 * masking real hardware issues. The cached host_io stays for future
	 * read-only consumers (e.g. timeout calculation in request()).
	 */
	sdhc_bcm2835_set_power(dev, ios->power_mode);
	sdhc_bcm2835_set_bus_width(dev, ios->bus_width);

	ret = sdhc_bcm2835_set_clock(dev, ios->clock);
	if (ret != 0) {
		return ret;
	}

	data->host_io = *ios;
	return 0;
}

static int sdhc_bcm2835_get_card_present(const struct device *dev)
{
	ARG_UNUSED(dev);
	/* The silicon has no card-detect line
	 * (SDHCI_QUIRK_BROKEN_CARD_DETECTION); whatever is wired to the
	 * EMMC port is assumed always present.
	 */
	return 1;
}

static int sdhc_bcm2835_card_busy(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

static int sdhc_bcm2835_get_host_props(const struct device *dev,
				       struct sdhc_host_props *props)
{
	const struct sdhc_bcm2835_config *cfg = dev->config;

	/* These mirror the hardcoded bcm2835 caps in Linux's
	 * sdhci-iproc.c::bcm2835_data (caps + caps1). The silicon's
	 * CAPABILITIES register reads as zero on this controller.
	 */
	memset(props, 0, sizeof(*props));
	props->f_min = BCM2835_F_MIN_HZ;
	props->f_max = BCM2835_F_MAX_HZ;
	props->host_caps.max_blk_len = 1;	/* 1 = 1024 bytes (FIFO size) */
	props->host_caps.high_spd_support = true;
	props->host_caps.vol_330_support = true;
	props->host_caps.drv_type_a_support = true;
	props->host_caps.drv_type_c_support = true;
	props->bus_4_bit_support = (cfg->bus_width == 4);
	props->is_spi = false;
	return 0;
}

/* Software reset of one or more controller domains. mask is any
 * combination of SDHCI_CTRL1_RESET_{ALL,CMD,DATA}. Writes the bit(s)
 * into CONTROL1 and polls until the silicon clears them. The reset
 * bit is self-clearing per spec; we time out in case the silicon
 * disagrees rather than spinning forever.
 *
 * We always RMW the full 32-bit CONTROL1 word -- per the BCM datasheet
 * the EMMC accepts only 32-bit aligned 32-bit accesses, and CONTROL1
 * has clock-control fields below the reset bits that we mustn't
 * disturb.
 */
static int sdhc_bcm2835_soft_reset(const struct device *dev, uint32_t mask)
{
	uintptr_t base = DEVICE_MMIO_GET(dev);
	uint32_t ctrl1;
	int64_t deadline = k_uptime_get() + BCM2835_SDHCI_RESET_TIMEOUT_MS;

	ctrl1 = sys_read32(base + SDHCI_CLOCK_CONTROL);
	sys_write32(ctrl1 | mask, base + SDHCI_CLOCK_CONTROL);

	while (sys_read32(base + SDHCI_CLOCK_CONTROL) & mask) {
		if (k_uptime_get() > deadline) {
			return -ETIMEDOUT;
		}
		k_busy_wait(10);
	}

	return 0;
}

static int sdhc_bcm2835_reset(const struct device *dev)
{
	return sdhc_bcm2835_soft_reset(dev, SDHCI_CTRL1_RESET_ALL);
}


static int sdhc_bcm2835_init(const struct device *dev)
{
	const struct sdhc_bcm2835_config *cfg = dev->config;
	struct sdhc_bcm2835_data *drvdata = dev->data;
	int ret;

	if (!device_is_ready(cfg->dma_dev)) {
		LOG_ERR("%s: DMA controller %s not ready", dev->name,
			cfg->dma_dev->name);
		return -ENODEV;
	}
	k_sem_init(&drvdata->dma_done, 0, 1);
	k_sem_init(&drvdata->cmd_done, 0, 1);
	k_sem_init(&drvdata->data_done, 0, 1);
	ret = dma_request_channel(cfg->dma_dev, NULL);
	if (ret < 0) {
		LOG_ERR("%s: dma_request_channel failed: %d", dev->name, ret);
		return ret;
	}
	drvdata->dma_channel = (uint32_t)ret;

	/* MMIO mapping uses Device-nGnRE (early-write-ack permitted).
	 * Zephyr's default K_MEM_CACHE_NONE maps as Device-nGnRnE; the
	 * BCM283x peripheral region is documented as Device-nGnRE.
	 */
	DEVICE_MMIO_MAP(dev, K_MEM_ARM_DEVICE_nGnRE);

	/* The Arasan controller is routable to two pin groups via ALT3
	 * (GPIO 34..39 and GPIO 48..53). The VPU firmware boots with the
	 * second group at ALT3; if pinctrl then puts the first group at
	 * ALT3 while the second is still set, the controller's RX mux
	 * stays tied to the unused pins and DAT1..3 read their pull state
	 * (DATA_CRC on the first transfer). Force GPIO 48..53 to ALT0
	 * before pinctrl runs so the new group is the only ALT3 set.
	 *
	 * 3 bits per pin in GPFSEL4/5, ALT0 = 0b100 = 4.
	 */
	{
		volatile uint32_t *gpfsel4 = (volatile uint32_t *)0x3F200010;
		volatile uint32_t *gpfsel5 = (volatile uint32_t *)0x3F200014;
		uint32_t f4 = *gpfsel4;
		uint32_t f5 = *gpfsel5;
		uint32_t mask5 = 0, set5 = 0;

		f4 = (f4 & ~((7u << 24) | (7u << 27))) |
		     ((4u << 24) | (4u << 27));
		for (int n = 50; n <= 53; n++) {
			int s = (n - 50) * 3;

			mask5 |= 7u << s;
			set5 |= 4u << s;
		}
		f5 = (f5 & ~mask5) | set5;

		*gpfsel4 = f4;
		*gpfsel5 = f5;
	}

	ret = pinctrl_apply_state(cfg->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		LOG_ERR("%s pinctrl apply failed: %d", dev->name, ret);
		return ret;
	}

	ret = sdhc_bcm2835_soft_reset(dev, SDHCI_CTRL1_RESET_ALL);
	if (ret != 0) {
		LOG_ERR("%s reset timeout", dev->name);
		return ret;
	}

	/* INT_ENABLE controls which bits get LATCHED into INT_STATUS (per
	 * SDHCI spec, independent of IRQ generation). SIGNAL_ENABLE
	 * controls which bits PROPAGATE to the ARM IRQ line.
	 *
	 * CMD_COMPLETE / DATA_END plus their error masks are both LATCHED
	 * and SIGNALED -- the request thread sleeps on cmd_done / data_done
	 * sems and the ISR captures status + gives the sem.
	 *
	 * CARD_INT is the SDIO async event from the chip; SDIO consumers
	 * opt in dynamically via sdhc_enable_interrupt(SDHC_INT_SDIO).
	 * Off at init so it does not fire before any consumer is ready.
	 *
	 * BUF_READ_READY / BUF_WRITE_READY are latched (in case a future
	 * PIO fallback wants to consume them) but not signaled -- data
	 * transfers go through external DMA + DATA_END.
	 */
	uintptr_t base = DEVICE_MMIO_GET(dev);
	uint32_t int_en = SDHCI_INT_CMD_COMPLETE | SDHCI_INT_DATA_END |
			  SDHCI_INT_BUF_READ_READY | SDHCI_INT_BUF_WRITE_READY |
			  SDHCI_INT_CMD_TIMEOUT | SDHCI_INT_CMD_CRC |
			  SDHCI_INT_CMD_END_BIT | SDHCI_INT_CMD_INDEX |
			  SDHCI_INT_DATA_TIMEOUT | SDHCI_INT_DATA_CRC |
			  SDHCI_INT_DATA_END_BIT |
			  BIT(23);	/* BUS_POWER_ERR */
	uint32_t sig_en = SDHCI_INT_CMD_COMPLETE | SDHCI_INT_DATA_END |
			  SDHCI_INT_CMD_ERROR_MASK |
			  SDHCI_INT_DATA_ERROR_MASK;
	sys_write32(int_en, base + SDHCI_INT_ENABLE);
	sys_write32(sig_en, base + SDHCI_SIGNAL_ENABLE);

	struct sdhc_io ios = {
		.clock = SDMMC_CLOCK_400KHZ,
		.bus_width = SDHC_BUS_WIDTH1BIT,
		.power_mode = SDHC_POWER_ON,
		.signal_voltage = SD_VOL_3_3_V,
	};
	ret = sdhc_bcm2835_set_io(dev, &ios);
	if (ret != 0) {
		LOG_ERR("%s set_io failed: %d", dev->name, ret);
		return ret;
	}


	/* Wire the Arasan IRQ line. CARD_INT is left disabled at boot;
	 * the SDIO consumer opts in via .enable_interrupt(SDHC_INT_SDIO)
	 * once it is ready to handle wakes. CMD_COMPLETE / DATA_END
	 * (+ error masks) drive the request-thread completion path.
	 */
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority),
		    sdhc_bcm2835_isr, DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));

	return 0;
}

/* SDHCI interrupt path --------------------------------------------------- */

/* ISR demuxes INT_STATUS into three concerns:
 *
 *   (a) CMD-completion bits (CMD_COMPLETE + CMD_ERROR_MASK). W1C-acked
 *       here, status captured in drvdata->cmd_status, cmd_done sem given;
 *       the request thread reads response + checks the captured status.
 *
 *   (b) DATA-completion bits (DATA_END + DATA_ERROR_MASK). Same pattern
 *       via data_status / data_done. Mirrors Linux's split between
 *       sdhci_cmd_irq and sdhci_data_irq in drivers/mmc/host/sdhci.c.
 *
 *   (c) SDIO CARD_INT (DAT1-low, level-sensitive). Special masking
 *       dance because INT_STATUS[CARD_INTERRUPT] continuously tracks
 *       DAT1 and cannot be W1C-acked. We mask BOTH INT_ENABLE and
 *       SIGNAL_ENABLE -- with INT_ENABLE off the STATUS bit stops
 *       tracking, so when the callback later calls
 *       sdhc_enable_interrupt() the controller does one fresh DAT1
 *       sample: IRQ re-fires only if the chip is *currently* driving
 *       DAT1 low. Masking SIGNAL_ENABLE alone is not enough -- a
 *       transient DAT1 low between cycles (e.g. CCCR-IENx-driven from
 *       F2 with no chip-side SDPCMD_INTSTATUS bit set) gates the STATUS
 *       bit through the next sdhc_enable_interrupt() call and storms
 *       the ISR. Mirrors Linux's sdhci_enable_sdio_irq_nolock(host,
 *       false).
 *
 * Edge-triggered W1C bits (CMD/DATA) are acked BEFORE the sem_give so
 * the IRQ can't re-fire on the same condition while the request thread
 * runs. CARD_INT keeps its level-sensitive dance.
 */
static void sdhc_bcm2835_isr(const struct device *dev)
{
	struct sdhc_bcm2835_data *drvdata = dev->data;
	uintptr_t base = DEVICE_MMIO_GET(dev);
	uint32_t status = sys_read32(base + SDHCI_INT_STATUS);
	uint32_t cmd_bits  = status & (SDHCI_INT_CMD_COMPLETE  |
				       SDHCI_INT_CMD_ERROR_MASK);
	uint32_t data_bits = status & (SDHCI_INT_DATA_END      |
				       SDHCI_INT_DATA_ERROR_MASK);

	if (cmd_bits != 0) {
		sys_write32(cmd_bits, base + SDHCI_INT_STATUS);
		drvdata->cmd_status = cmd_bits;
		k_sem_give(&drvdata->cmd_done);
	}

	if (data_bits != 0) {
		sys_write32(data_bits, base + SDHCI_INT_STATUS);
		drvdata->data_status = data_bits;
		k_sem_give(&drvdata->data_done);
	}

	if (status & SDHCI_INT_CARD_INT) {
		uint32_t int_en = sys_read32(base + SDHCI_INT_ENABLE);
		uint32_t sig_en = sys_read32(base + SDHCI_SIGNAL_ENABLE);

		sys_write32(int_en & ~SDHCI_INT_CARD_INT,
			    base + SDHCI_INT_ENABLE);
		sys_write32(sig_en & ~SDHCI_INT_CARD_INT,
			    base + SDHCI_SIGNAL_ENABLE);
		if (drvdata->sdhc_cb != NULL) {
			drvdata->sdhc_cb(dev, SDHC_INT_SDIO,
					 drvdata->sdhc_cb_user_data);
		}
	}
}

static int sdhc_bcm2835_enable_interrupt(const struct device *dev,
					 sdhc_interrupt_cb_t callback,
					 int sources, void *user_data)
{
	struct sdhc_bcm2835_data *drvdata = dev->data;
	uintptr_t base = DEVICE_MMIO_GET(dev);

	if (sources & ~SDHC_INT_SDIO) {
		/* Only CARD_INT plumbing today; card insert/remove would need
		 * extra wiring (the Pi has no detect line for the SDIO slot).
		 */
		return -ENOTSUP;
	}

	drvdata->sdhc_cb = callback;
	drvdata->sdhc_cb_user_data = user_data;

	if (sources & SDHC_INT_SDIO) {
		uint32_t int_en = sys_read32(base + SDHCI_INT_ENABLE);
		uint32_t sig_en = sys_read32(base + SDHCI_SIGNAL_ENABLE);

		sys_write32(int_en | SDHCI_INT_CARD_INT,
			    base + SDHCI_INT_ENABLE);
		sys_write32(sig_en | SDHCI_INT_CARD_INT,
			    base + SDHCI_SIGNAL_ENABLE);
	}
	return 0;
}

static int sdhc_bcm2835_disable_interrupt(const struct device *dev, int sources)
{
	uintptr_t base = DEVICE_MMIO_GET(dev);

	if (sources & SDHC_INT_SDIO) {
		uint32_t int_en = sys_read32(base + SDHCI_INT_ENABLE);
		uint32_t sig_en = sys_read32(base + SDHCI_SIGNAL_ENABLE);

		sys_write32(int_en & ~SDHCI_INT_CARD_INT,
			    base + SDHCI_INT_ENABLE);
		sys_write32(sig_en & ~SDHCI_INT_CARD_INT,
			    base + SDHCI_SIGNAL_ENABLE);
	}
	return 0;
}

static DEVICE_API(sdhc, sdhc_bcm2835_api) = {
	.request = sdhc_bcm2835_request,
	.set_io = sdhc_bcm2835_set_io,
	.get_host_props = sdhc_bcm2835_get_host_props,
	.get_card_present = sdhc_bcm2835_get_card_present,
	.reset = sdhc_bcm2835_reset,
	.card_busy = sdhc_bcm2835_card_busy,
	.enable_interrupt = sdhc_bcm2835_enable_interrupt,
	.disable_interrupt = sdhc_bcm2835_disable_interrupt,
};

#define SDHC_BCM2835_INIT(inst)							\
	PINCTRL_DT_INST_DEFINE(inst);						\
	static const struct sdhc_bcm2835_config sdhc_bcm2835_cfg_##inst = {	\
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(inst)),			\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),			\
		.dma_dev    = DEVICE_DT_GET(					\
				DT_INST_DMAS_CTLR_BY_NAME(inst, rx_tx)),	\
		.fifo_phys  = DT_INST_REG_ADDR(inst) + SDHCI_BUFFER,		\
		.dma_dreq   = DT_INST_DMAS_CELL_BY_NAME(inst, rx_tx, dreq),	\
		.clock_freq = DT_INST_PROP(inst, clock_frequency),		\
		.bus_width  = DT_INST_PROP(inst, bus_width),			\
	};									\
	static struct sdhc_bcm2835_data sdhc_bcm2835_data_##inst;		\
	DEVICE_DT_INST_DEFINE(inst,						\
			      sdhc_bcm2835_init,				\
			      NULL,						\
			      &sdhc_bcm2835_data_##inst,			\
			      &sdhc_bcm2835_cfg_##inst,				\
			      POST_KERNEL,					\
			      CONFIG_SDHC_INIT_PRIORITY,			\
			      &sdhc_bcm2835_api);

DT_INST_FOREACH_STATUS_OKAY(SDHC_BCM2835_INIT)
