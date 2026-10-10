/*
 * Copyright (c) 2026 Jonathan Elliot Peace <jep@alphabetiq.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Broadcom BCM283x legacy SD Host Controller.
 *
 * Second SD controller on the BCM283x SoCs, distinct from the Arasan
 * SDHCI block. Drives the external microSD slot via GPIO 48..53 at
 * ALT0. Register layout is proprietary Broadcom (SDCMD/SDARG/SDHSTS/
 * SDEDM/SDDATA/SDCDIV at offsets 0x00..0x50), not SDHCI-spec.
 *
 * Polled implementation of the sdhc.h device API: command path
 * supports R1/R1b/R2/R3/R6/R7 (busy responses wait on DAT0 via the
 * controller's SDCMD_BUSYWAIT); data path uses a CPU-paced FIFO with
 * no IRQ or DMA.
 */

#define DT_DRV_COMPAT brcm_bcm2835_sdhost

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/sdhc.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sd/sd_spec.h>

LOG_MODULE_REGISTER(sdhc_bcm2835_sdhost, CONFIG_SDHC_LOG_LEVEL);

/* ===== Register offsets =========================================== */
#define SDCMD	0x00	/* command index, flags, NEW/FAIL status */
#define SDARG	0x04	/* command argument */
#define SDTOUT	0x08	/* data timeout */
#define SDCDIV	0x0c	/* bus clock divider */
#define SDRSP0	0x10	/* response bits 31:0 */
#define SDRSP1	0x14	/* response bits 63:32 (R2 only) */
#define SDRSP2	0x18	/* response bits 95:64 (R2 only) */
#define SDRSP3	0x1c	/* response bits 127:96 (R2 only) */
#define SDHSTS	0x20	/* host status, write 1 to clear */
#define SDVDD	0x30	/* card power */
#define SDEDM	0x34	/* data FSM state, FIFO fill level and thresholds */
#define SDHCFG	0x38	/* host configuration */
#define SDHBCT	0x3c	/* bytes per block */
#define SDDATA	0x40	/* data FIFO */
#define SDHBLC	0x50	/* block count */

/* ===== SDCMD bits (low 16) ======================================== */
#define SDCMD_NEW_FLAG		0x8000
#define SDCMD_FAIL_FLAG		0x4000
#define SDCMD_BUSYWAIT		0x800
#define SDCMD_NO_RESPONSE	0x400
#define SDCMD_LONG_RESPONSE	0x200
#define SDCMD_WRITE_CMD		0x80
#define SDCMD_READ_CMD		0x40
#define SDCMD_CMD_MASK		0x3f

/* ===== SDCDIV ===================================================== */
#define SDCDIV_MAX_CDIV		0x7ff

/* ===== SDHSTS bits ================================================ */
#define SDHSTS_BUSY_IRPT	0x400
#define SDHSTS_BLOCK_IRPT	0x200
#define SDHSTS_SDIO_IRPT	0x100
#define SDHSTS_REW_TIME_OUT	0x80
#define SDHSTS_CMD_TIME_OUT	0x40
#define SDHSTS_CRC16_ERROR	0x20
#define SDHSTS_CRC7_ERROR	0x10
#define SDHSTS_FIFO_ERROR	0x08
#define SDHSTS_DATA_FLAG	0x01
#define SDHSTS_W1C_ALL		0x7f8
#define SDHSTS_ERROR_MASK	(SDHSTS_CMD_TIME_OUT | SDHSTS_REW_TIME_OUT | \
				 SDHSTS_CRC7_ERROR | SDHSTS_CRC16_ERROR | \
				 SDHSTS_FIFO_ERROR)

/* ===== SDVDD ====================================================== */
#define SDVDD_POWER_OFF		0
#define SDVDD_POWER_ON		1

/* ===== SDEDM bits ================================================= */
#define SDEDM_FSM_MASK			0xf
#define SDEDM_FSM_IDENTMODE		0x0
#define SDEDM_FSM_DATAMODE		0x1
#define SDEDM_FSM_READDATA		0x2
#define SDEDM_FSM_WRITEDATA		0x3
#define SDEDM_FSM_READWAIT		0x4
#define SDEDM_FSM_READCRC		0x5
#define SDEDM_FSM_WRITECRC		0x6
#define SDEDM_FSM_WRITEWAIT1		0x7
#define SDEDM_FSM_POWERDOWN		0x8
#define SDEDM_FSM_POWERUP		0x9
#define SDEDM_FSM_WRITESTART1		0xa
#define SDEDM_FSM_WRITESTART2		0xb
#define SDEDM_FSM_GENPULSES		0xc
#define SDEDM_FSM_WRITEWAIT2		0xd
#define SDEDM_FSM_STARTPOWDOWN		0xf
#define SDEDM_FORCE_DATA_MODE		BIT(19)
#define SDEDM_CLOCK_PULSE		BIT(20)
#define SDEDM_BYPASS			BIT(21)
#define SDEDM_WRITE_THRESHOLD_SHIFT	9
#define SDEDM_READ_THRESHOLD_SHIFT	14
#define SDEDM_THRESHOLD_MASK		0x1f

/* ===== SDHCFG bits ================================================ */
#define SDHCFG_BUSY_IRPT_EN	BIT(10)
#define SDHCFG_BLOCK_IRPT_EN	BIT(8)
#define SDHCFG_SDIO_IRPT_EN	BIT(5)
#define SDHCFG_DATA_IRPT_EN	BIT(4)
#define SDHCFG_SLOW_CARD	BIT(3)
#define SDHCFG_WIDE_EXT_BUS	BIT(2)
#define SDHCFG_WIDE_INT_BUS	BIT(1)
#define SDHCFG_REL_CMD_LINE	BIT(0)

/* ===== FIFO / timing constants ==================================== */
#define SDDATA_FIFO_WORDS	16
#define FIFO_READ_THRESHOLD	4
#define FIFO_WRITE_THRESHOLD	4
#define SDTOUT_RESET_VALUE	0x00f00000
#define SDHOST_POWER_SETTLE_MS	20

/* Caps reported via get_host_props, the same limits Linux's
 * bcm2835_add_host() uses: f_max defaults to clk_in (the VPU
 * core_freq), f_min is clk_in / SDCDIV_MAX_CDIV, and max_blk_size is
 * 1024 (encoded as 1 in the Zephyr SDHC API). We cap f_max at 50 MHz
 * since the HS SD spec runs at 50 MHz and we haven't verified anything
 * faster.
 */
#define SDHOST_F_MAX_HZ		50000000

/* Default command-completion poll timeout. CMDs at the slow 400 kHz
 * card-ID clock can take a few ms; 100 ms is comfortably enough for
 * any non-busy command. Callers can override via cmd->timeout_ms.
 */
#define SDHOST_CMD_TIMEOUT_MS	100

struct sdhc_bcm2835_sdhost_config {
	DEVICE_MMIO_ROM;
	const struct pinctrl_dev_config *pincfg;
	uint32_t clock_freq;
	uint8_t bus_width;
};

struct sdhc_bcm2835_sdhost_data {
	DEVICE_MMIO_RAM;
	uint32_t hcfg;
	uint32_t cdiv;
};

/* ===== Soft reset ================================================== */

/* Idle value of each command/data register, written in this order. */
static const struct {
	uint8_t reg;
	uint32_t val;
} sdhost_idle_regs[] = {
	{ SDCMD, 0 },
	{ SDARG, 0 },
	{ SDTOUT, SDTOUT_RESET_VALUE },
	{ SDCDIV, 0 },
	{ SDHSTS, SDHSTS_W1C_ALL },
	{ SDHCFG, 0 },
	{ SDHBCT, 0 },
	{ SDHBLC, 0 },
};

#define SDEDM_READ_THRESHOLD	(SDEDM_THRESHOLD_MASK << SDEDM_READ_THRESHOLD_SHIFT)
#define SDEDM_WRITE_THRESHOLD	(SDEDM_THRESHOLD_MASK << SDEDM_WRITE_THRESHOLD_SHIFT)

/* Power the card down, return the command and data registers to idle
 * and program the FIFO thresholds, then power the card back up. The
 * controller is only reliable with low FIFO thresholds (Linux's bcm2835
 * driver uses the same values).
 */
static void sdhost_soft_reset(const struct device *dev)
{
	struct sdhc_bcm2835_sdhost_data *data = dev->data;
	uintptr_t base = DEVICE_MMIO_GET(dev);
	uint32_t thresholds = FIELD_PREP(SDEDM_READ_THRESHOLD, FIFO_READ_THRESHOLD) |
			      FIELD_PREP(SDEDM_WRITE_THRESHOLD, FIFO_WRITE_THRESHOLD);

	sys_write32(SDVDD_POWER_OFF, base + SDVDD);
	ARRAY_FOR_EACH(sdhost_idle_regs, i) {
		sys_write32(sdhost_idle_regs[i].val, base + sdhost_idle_regs[i].reg);
	}
	sys_write32((sys_read32(base + SDEDM) & ~(SDEDM_READ_THRESHOLD | SDEDM_WRITE_THRESHOLD)) |
		    thresholds, base + SDEDM);
	k_msleep(SDHOST_POWER_SETTLE_MS);

	sys_write32(SDVDD_POWER_ON, base + SDVDD);
	k_msleep(SDHOST_POWER_SETTLE_MS);

	data->hcfg = 0;
	data->cdiv = SDCDIV_MAX_CDIV;
	sys_write32(data->hcfg, base + SDHCFG);
	sys_write32(data->cdiv, base + SDCDIV);
}

/* ===== Command-completion poll ===================================== */
static int sdhost_wait_cmd_done(const struct device *dev, int timeout_ms)
{
	uintptr_t base = DEVICE_MMIO_GET(dev);

	if (!WAIT_FOR(!(sys_read32(base + SDCMD) & SDCMD_NEW_FLAG),
		      timeout_ms * 1000, k_busy_wait(10))) {
		return -ETIMEDOUT;
	}
	return 0;
}

/* ===== Direction inference =========================================
 * sdhc.h's struct sdhc_data has no direction field. For commands that
 * carry a data phase, the direction is baked into the opcode (or, for
 * SDIO CMD53, into arg bit 31). Mirror the Arasan driver's table;
 * extend as new opcodes need data-phase support.
 */
static int sdhost_data_direction(const struct sdhc_command *cmd, bool *is_read)
{
	switch (cmd->opcode) {
	case SD_READ_SINGLE_BLOCK:		/* CMD17 -- 1 block */
	case SD_READ_MULTIPLE_BLOCK:		/* CMD18 -- N blocks */
	case SD_SWITCH:				/* CMD6  -- 64-byte switch status
						 *          (NB same opcode as
						 *          ACMD6 SET_BUS_WIDTH
						 *          which is non-data;
						 *          this case only fires
						 *          when data != NULL, so
						 *          unambiguous)
						 */
	case SD_APP_SEND_SCR:			/* ACMD51 -- 8-byte SCR */
	case SD_APP_SEND_NUM_WRITTEN_BLK:	/* ACMD22 -- 4-byte count */
		*is_read = true;
		return 0;
	case SD_WRITE_SINGLE_BLOCK:		/* CMD24 -- 1 block */
	case SD_WRITE_MULTIPLE_BLOCK:		/* CMD25 -- N blocks */
		*is_read = false;
		return 0;
	case SDIO_RW_EXTENDED:			/* CMD53 -- SDIO multi-byte */
		*is_read = !(cmd->arg & BIT(SDIO_CMD_ARG_RW_SHIFT));
		return 0;
	default:
		/* Unrecognised data-phase opcode -- refuse rather than
		 * guess a direction (a wrong direction on a write would
		 * leave the card holding bytes we never sent).
		 */
		return -EINVAL;
	}
}

/* True once the data FSM has left the transfer states. After a read it
 * can park in READWAIT, and after a write in WRITESTART1, instead of
 * returning to idle; FORCE_DATA_MODE moves it on from there.
 */
static bool sdhost_data_fsm_settled(uintptr_t base, bool is_read)
{
	uint32_t edm = sys_read32(base + SDEDM);
	uint32_t parked = is_read ? SDEDM_FSM_READWAIT : SDEDM_FSM_WRITESTART1;

	switch (edm & SDEDM_FSM_MASK) {
	case SDEDM_FSM_IDENTMODE:
	case SDEDM_FSM_DATAMODE:
		return true;
	default:
		if ((edm & SDEDM_FSM_MASK) != parked) {
			return false;
		}
		sys_write32(edm | SDEDM_FORCE_DATA_MODE, base + SDEDM);
		return true;
	}
}

/* The last FIFO word is not the end of a transfer: the controller still
 * clocks the CRC and, on a write, waits for the card to finish
 * programming. A command issued before the data FSM settles fails (the
 * CMD13 status poll after a write then reports "card did not return to
 * ready state"), so wait for it here.
 *
 * Poll without a delay. During an open-ended multi-block read the card
 * starts sending the next block soon after the FSM reaches READWAIT; a
 * slower poll can miss that window, and the FSM then runs on into the
 * next block and stays in READDATA with the FIFO full.
 */
static int sdhost_wait_xfer_complete(const struct device *dev, bool is_read,
				     int timeout_ms)
{
	uintptr_t base = DEVICE_MMIO_GET(dev);

	if (!WAIT_FOR(sdhost_data_fsm_settled(base, is_read),
		      timeout_ms * 1000, NULL)) {
		LOG_ERR("data FSM still busy after %s, edm 0x%08x",
			is_read ? "read" : "write", sys_read32(base + SDEDM));
		return -ETIMEDOUT;
	}
	return 0;
}

/* Polled PIO transfer: drains (read) or fills (write) the SDDATA
 * FIFO one block at a time. SDEDM bits 8:4 give the fill level; when
 * the FIFO is not ready we check FSM is still in an active data
 * state and either surface a SDHSTS error or hit the outer deadline.
 */
static int sdhost_pio_xfer(const struct device *dev, struct sdhc_data *data,
			   bool is_read)
{
	uintptr_t base = DEVICE_MMIO_GET(dev);
	uint8_t *buf = (uint8_t *)data->data;
	uint32_t blocks_left = data->blocks;
	uint32_t words_per_block = data->block_size / 4;
	int64_t deadline = k_uptime_get() +
			   (data->timeout_ms ? data->timeout_ms : 500);
	uint32_t sdhsts;

	if (data->block_size % 4 != 0) {
		/* Word-paced FIFO; partial-word blocks would need a
		 * scratch-word read with byte-shift unpacking. SD spec
		 * blocks are always multiples of 4 so refuse rather than
		 * carry dead code.
		 */
		return -EINVAL;
	}

	while (blocks_left > 0) {
		uint32_t words_left = words_per_block;

		while (words_left > 0) {
			uint32_t edm = sys_read32(base + SDEDM);
			uint32_t fifo_used = (edm >> 4) & 0x1f;
			uint32_t fifo_ready = is_read ? fifo_used
						      : (SDDATA_FIFO_WORDS -
							 fifo_used);

			if (fifo_ready == 0) {
				uint32_t fsm = edm & SDEDM_FSM_MASK;
				bool fsm_active = is_read
					? (fsm == SDEDM_FSM_READDATA ||
					   fsm == SDEDM_FSM_READWAIT ||
					   fsm == SDEDM_FSM_READCRC)
					: (fsm == SDEDM_FSM_WRITEDATA ||
					   fsm == SDEDM_FSM_WRITESTART1 ||
					   fsm == SDEDM_FSM_WRITESTART2);

				if (!fsm_active) {
					sdhsts = sys_read32(base + SDHSTS);
					if (sdhsts & SDHSTS_ERROR_MASK) {
						LOG_ERR("PIO %c: sdhsts=0x%08x fsm=0x%x b=%u w=%u",
							is_read ? 'r' : 'w',
							sdhsts, fsm,
							blocks_left, words_left);
						sys_write32(sdhsts & SDHSTS_W1C_ALL,
							    base + SDHSTS);
						return (sdhsts &
							(SDHSTS_CMD_TIME_OUT |
							 SDHSTS_REW_TIME_OUT))
							       ? -ETIMEDOUT
							       : -EIO;
					}
				}

				if (k_uptime_get() > deadline) {
					LOG_ERR("PIO %s: timeout edm=0x%08x blocks=%u words=%u",
						is_read ? "read" : "write",
						edm, blocks_left, words_left);
					return -ETIMEDOUT;
				}
				continue;
			}

			uint32_t do_words = fifo_ready < words_left
						    ? fifo_ready
						    : words_left;

			for (uint32_t i = 0; i < do_words; i++) {
				if (is_read) {
					uint32_t w = sys_read32(base + SDDATA);

					buf[0] = w & 0xff;
					buf[1] = (w >> 8) & 0xff;
					buf[2] = (w >> 16) & 0xff;
					buf[3] = (w >> 24) & 0xff;
				} else {
					uint32_t w =
						(uint32_t)buf[0] |
						((uint32_t)buf[1] << 8) |
						((uint32_t)buf[2] << 16) |
						((uint32_t)buf[3] << 24);

					sys_write32(w, base + SDDATA);
				}
				buf += 4;
				words_left--;
			}
		}
		blocks_left--;
	}

	int ret = sdhost_wait_xfer_complete(dev, is_read,
			data->timeout_ms ? data->timeout_ms : 500);
	if (ret != 0) {
		return ret;
	}

	sdhsts = sys_read32(base + SDHSTS);
	if (sdhsts & (SDHSTS_CRC16_ERROR | SDHSTS_CRC7_ERROR |
		      SDHSTS_FIFO_ERROR)) {
		LOG_ERR("PIO %s post-xfer error sdhsts=0x%08x",
			is_read ? "read" : "write", sdhsts);
		sys_write32(sdhsts & SDHSTS_W1C_ALL, base + SDHSTS);
		return -EILSEQ;
	}
	if (sdhsts & (SDHSTS_CMD_TIME_OUT | SDHSTS_REW_TIME_OUT)) {
		LOG_ERR("PIO %s post-xfer timeout sdhsts=0x%08x",
			is_read ? "read" : "write", sdhsts);
		sys_write32(sdhsts & SDHSTS_W1C_ALL, base + SDHSTS);
		return -ETIMEDOUT;
	}

	data->bytes_xfered = (size_t)data->blocks * data->block_size;
	return 0;
}

/* ===== Clock divider math =========================================
 * SDHost bus clock = clk_in / (SDCDIV + 2). The fastest bus clock not
 * above target_hz uses the smallest divisor d with clk_in / d <=
 * target_hz, i.e. d = ceil(clk_in / target_hz), so SDCDIV = d - 2.
 */
static uint32_t sdhost_calc_clk_div(uint32_t clk_in, uint32_t target_hz)
{
	if (target_hz == 0 || target_hz >= clk_in / 2) {
		return 0;
	}
	return MIN(DIV_ROUND_UP(clk_in, target_hz) - 2, SDCDIV_MAX_CDIV);
}

static int sdhc_bcm2835_sdhost_reset(const struct device *dev)
{
	sdhost_soft_reset(dev);
	return 0;
}

static int sdhc_bcm2835_sdhost_set_io(const struct device *dev,
				      struct sdhc_io *ios)
{
	const struct sdhc_bcm2835_sdhost_config *cfg = dev->config;
	struct sdhc_bcm2835_sdhost_data *data = dev->data;
	uintptr_t base = DEVICE_MMIO_GET(dev);
	uint32_t cdiv;

	/* Reject targets outside the host's claimed range; the SDHC API
	 * caller relies on this to discover bus capability.
	 */
	if (ios->clock != 0) {
		uint32_t f_max = MIN(cfg->clock_freq / 2, SDHOST_F_MAX_HZ);

		if (ios->clock > f_max) {
			return -EINVAL;
		}
	}

	/* Clock divider. SDCDIV gates and re-arms the bus clock; SDHost
	 * has no separate enable bit. Target 0 -> slowest divider.
	 */
	cdiv = (ios->clock == 0) ? SDCDIV_MAX_CDIV
				 : sdhost_calc_clk_div(cfg->clock_freq, ios->clock);
	data->cdiv = cdiv;
	sys_write32(cdiv, base + SDCDIV);

	/* Power: bit 0 of SDVDD. SDHost is 3.3V-only; signal_voltage
	 * field is ignored (the only meaningful value is SD_VOL_3_3_V).
	 */
	sys_write32(ios->power_mode == SDHC_POWER_ON ? SDVDD_POWER_ON
						     : SDVDD_POWER_OFF,
		    base + SDVDD);

	/* WIDE_EXT_BUS selects the 4-bit card bus. WIDE_INT_BUS, the
	 * internal FIFO-side datapath, must be set whatever the card bus
	 * width: without it the FIFO assembles bytes into the wrong
	 * 32-bit words even on a 1-bit read. SLOW_CARD stays set because
	 * the controller's automatic clock switching is not reliable at
	 * this core clock.
	 */
	data->hcfg = (data->hcfg & ~SDHCFG_WIDE_EXT_BUS) |
		     (ios->bus_width == SDHC_BUS_WIDTH4BIT ? SDHCFG_WIDE_EXT_BUS : 0) |
		     SDHCFG_WIDE_INT_BUS | SDHCFG_SLOW_CARD;
	sys_write32(data->hcfg, base + SDHCFG);

	return 0;
}

/* Auto-CMD12: end an open-ended multi-block transfer with a
 * STOP_TRANSMISSION. The SD subsystem issues CMD18 / CMD25 without a
 * bounding CMD23 and leaves the stop to the host (see the note in
 * subsys/sd/sd_ops.c card_read/card_write); without it the card stays
 * in data mode and never returns to the transfer state. CMD12 carries
 * an R1b busy response, so SDCMD_BUSYWAIT holds the command active
 * until the card releases DAT0, and SDHSTS_BUSY_IRPT is cleared after.
 */
static int sdhost_send_stop(const struct device *dev)
{
	uintptr_t base = DEVICE_MMIO_GET(dev);
	uint32_t sdcmd;
	int ret;

	sys_write32(0, base + SDARG);
	sys_write32((SD_STOP_TRANSMISSION & SDCMD_CMD_MASK) |
		    SDCMD_BUSYWAIT | SDCMD_NEW_FLAG, base + SDCMD);

	ret = sdhost_wait_cmd_done(dev, SDHOST_CMD_TIMEOUT_MS);
	if (ret != 0) {
		LOG_ERR("%s CMD12 stop: NEW_FLAG never cleared", dev->name);
		return ret;
	}

	sdcmd = sys_read32(base + SDCMD);
	if (sdcmd & SDCMD_FAIL_FLAG) {
		uint32_t sdhsts = sys_read32(base + SDHSTS);

		sys_write32(sdhsts & SDHSTS_W1C_ALL, base + SDHSTS);
		LOG_ERR("%s CMD12 stop failed sdhsts=0x%08x sdcmd=0x%08x",
			dev->name, sdhsts, sdcmd);
		return -EIO;
	}

	sys_write32(SDHSTS_BUSY_IRPT, base + SDHSTS);
	return 0;
}

static int sdhc_bcm2835_sdhost_request(const struct device *dev,
				       struct sdhc_command *cmd,
				       struct sdhc_data *data)
{
	uintptr_t base = DEVICE_MMIO_GET(dev);
	uint32_t sdcmd;
	uint32_t sdhsts;
	uint32_t rsp;
	int cmd_timeout_ms;
	int ret;

	bool data_is_read = false;

	if (cmd == NULL) {
		return -EINVAL;
	}
	if (data != NULL) {
		if (data->data == NULL || data->blocks == 0 ||
		    data->block_size == 0) {
			return -EINVAL;
		}
		ret = sdhost_data_direction(cmd, &data_is_read);
		if (ret != 0) {
			return ret;
		}
	}

	cmd_timeout_ms = cmd->timeout_ms ? cmd->timeout_ms
					 : SDHOST_CMD_TIMEOUT_MS;

	/* Make sure any previous command finished arming. The controller
	 * latches SDCMD on NEW_FLAG transitions; a stale NEW_FLAG means
	 * the previous command never completed -- recover by failing
	 * fast rather than overwriting it.
	 */
	ret = sdhost_wait_cmd_done(dev, cmd_timeout_ms);
	if (ret != 0) {
		LOG_ERR("%s CMD%u: previous command never completed",
			dev->name, cmd->opcode);
		return ret;
	}

	/* Clear any stale error bits before issuing the new command, so
	 * the post-completion error check only sees fresh state.
	 */
	sdhsts = sys_read32(base + SDHSTS);
	if (sdhsts & SDHSTS_ERROR_MASK) {
		sys_write32(sdhsts & SDHSTS_W1C_ALL, base + SDHSTS);
	}

	sdcmd = cmd->opcode & SDCMD_CMD_MASK;
	rsp = cmd->response_type & SDHC_NATIVE_RESPONSE_MASK;

	switch (rsp) {
	case SD_RSP_TYPE_NONE:
		sdcmd |= SDCMD_NO_RESPONSE;
		break;
	case SD_RSP_TYPE_R2:
		sdcmd |= SDCMD_LONG_RESPONSE;
		break;
	case SD_RSP_TYPE_R1:
	case SD_RSP_TYPE_R3:
	case SD_RSP_TYPE_R4:
	case SD_RSP_TYPE_R5:
	case SD_RSP_TYPE_R6:
	case SD_RSP_TYPE_R7:
		/* Short 48-bit response. SDHost handles CRC + index check
		 * decisions automatically based on the opcode; we don't
		 * need explicit toggles like SDHCI's CMDTM_CRC_CHECK /
		 * INDEX_CHECK.
		 */
		break;
	case SD_RSP_TYPE_R1b:
	case SD_RSP_TYPE_R5b:
		/* Busy response, e.g. CMD38 erase or CMD12 stop. SDCMD_BUSYWAIT
		 * keeps the command active -- NEW_FLAG stays set -- until the
		 * card releases DAT0 busy, so the NEW_FLAG poll below waits out
		 * the busy period, bounded by the command's timeout_ms. The
		 * response itself is the short 48-bit form, like R1.
		 */
		sdcmd |= SDCMD_BUSYWAIT;
		break;
	default:
		return -EINVAL;
	}

	/* If this command has a data phase, set the FIFO direction bit
	 * and program SDHBCT (byte count per block) + SDHBLC (block
	 * count) before arming the command. The controller uses SDHBLC
	 * to know when the data transfer is done; without it set, the
	 * FSM never leaves the DATA state.
	 */
	if (data != NULL) {
		sdcmd |= data_is_read ? SDCMD_READ_CMD : SDCMD_WRITE_CMD;
		sys_write32(data->block_size, base + SDHBCT);
		sys_write32(data->blocks, base + SDHBLC);
	}

	sys_write32(cmd->arg, base + SDARG);
	sys_write32(sdcmd | SDCMD_NEW_FLAG, base + SDCMD);

	ret = sdhost_wait_cmd_done(dev, cmd_timeout_ms);
	if (ret != 0) {
		LOG_ERR("%s CMD%u arg=0x%08x: NEW_FLAG never cleared",
			dev->name, cmd->opcode, cmd->arg);
		return ret;
	}

	/* Re-read SDCMD; FAIL_FLAG signals the controller hit a CRC /
	 * timeout / etc. error during the command.
	 */
	sdcmd = sys_read32(base + SDCMD);
	if (sdcmd & SDCMD_FAIL_FLAG) {
		sdhsts = sys_read32(base + SDHSTS);
		sys_write32(sdhsts & SDHSTS_W1C_ALL, base + SDHSTS);

		if (sdhsts & SDHSTS_CMD_TIME_OUT) {
			/* CMD8 on legacy SD 1.x cards times out by spec;
			 * the sd subsystem retries with the legacy path.
			 * Don't spam ERR for the expected case.
			 */
			LOG_DBG("%s CMD%u arg=0x%08x: timeout (sdhsts=0x%08x)",
				dev->name, cmd->opcode, cmd->arg, sdhsts);
			return -ETIMEDOUT;
		}
		LOG_ERR("%s CMD%u arg=0x%08x: failed sdhsts=0x%08x sdcmd=0x%08x",
			dev->name, cmd->opcode, cmd->arg, sdhsts, sdcmd);
		return -EIO;
	}

	/* A busy command completed via SDCMD_BUSYWAIT sets SDHSTS_BUSY_IRPT.
	 * Clear it (W1C) so it can't bleed into the next command's
	 * completion check.
	 */
	if (rsp == SD_RSP_TYPE_R1b || rsp == SD_RSP_TYPE_R5b) {
		sys_write32(SDHSTS_BUSY_IRPT, base + SDHSTS);
	}

	/* Read the response in straight order: response[0] = SDRSP0 =
	 * card's [31:0]. Matches the Arasan driver's convention; opposite
	 * of Linux (which swaps for the MMC layer's high-bits-first
	 * response[] array).
	 */
	if (rsp != SD_RSP_TYPE_NONE) {
		cmd->response[0] = sys_read32(base + SDRSP0);
		if (rsp == SD_RSP_TYPE_R2) {
			cmd->response[1] = sys_read32(base + SDRSP1);
			cmd->response[2] = sys_read32(base + SDRSP2);
			cmd->response[3] = sys_read32(base + SDRSP3);
		}
	}

	if (data != NULL) {
		ret = sdhost_pio_xfer(dev, data, data_is_read);
		if (ret != 0) {
			return ret;
		}

		if (cmd->opcode == SD_READ_MULTIPLE_BLOCK ||
		    cmd->opcode == SD_WRITE_MULTIPLE_BLOCK) {
			ret = sdhost_send_stop(dev);
			if (ret != 0) {
				return ret;
			}
		}
	}

	return 0;
}

static int sdhc_bcm2835_sdhost_get_card_present(const struct device *dev)
{
	ARG_UNUSED(dev);
	/* Pi Zero 2W's microSD slot has no card-detect line wired to a
	 * GPIO. Linux's bcm2835.c does the same -- relies on CMD0
	 * timeout (or CMD8/ACMD41 timeout) to signal an empty slot.
	 */
	return 1;
}

static int sdhc_bcm2835_sdhost_card_busy(const struct device *dev)
{
	ARG_UNUSED(dev);
	/* Busy responses wait on DAT0 inside the command via
	 * SDCMD_BUSYWAIT, so a command never returns while the card is
	 * still busy and there is nothing to poll between commands.
	 */
	return 0;
}

static int sdhc_bcm2835_sdhost_get_host_props(const struct device *dev,
					      struct sdhc_host_props *props)
{
	const struct sdhc_bcm2835_sdhost_config *cfg = dev->config;

	memset(props, 0, sizeof(*props));
	props->f_min = cfg->clock_freq / SDCDIV_MAX_CDIV;
	props->f_max = MIN(cfg->clock_freq / 2, SDHOST_F_MAX_HZ);
	props->host_caps.max_blk_len = 1;	/* 1 = 1024-byte blocks */
	props->host_caps.high_spd_support = true;
	props->host_caps.vol_330_support = true;
	props->bus_4_bit_support = (cfg->bus_width == 4);
	return 0;
}

static int sdhc_bcm2835_sdhost_init(const struct device *dev)
{
	const struct sdhc_bcm2835_sdhost_config *cfg = dev->config;
	uintptr_t base;
	uint32_t edm;
	int ret;

	/* K_MEM_CACHE_NONE to match the pinctrl and PL011 co-tenants of
	 * the same 64KB page; arm64's MMU rejects re-maps with
	 * mismatched attributes.
	 */
	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	ret = pinctrl_apply_state(cfg->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		LOG_ERR("%s pinctrl apply failed: %d", dev->name, ret);
		return ret;
	}

	sdhost_soft_reset(dev);

	base = DEVICE_MMIO_GET(dev);
	edm = sys_read32(base + SDEDM);
	LOG_INF("%s init ok (clk_in=%u Hz, bus=%u-bit, "
		"SDEDM=0x%08x [FSM=0x%x rd_thr=%u wr_thr=%u])",
		dev->name, cfg->clock_freq, cfg->bus_width, edm,
		edm & SDEDM_FSM_MASK,
		(edm >> SDEDM_READ_THRESHOLD_SHIFT) & SDEDM_THRESHOLD_MASK,
		(edm >> SDEDM_WRITE_THRESHOLD_SHIFT) & SDEDM_THRESHOLD_MASK);

	return 0;
}

static DEVICE_API(sdhc, sdhc_bcm2835_sdhost_api) = {
	.reset = sdhc_bcm2835_sdhost_reset,
	.request = sdhc_bcm2835_sdhost_request,
	.set_io = sdhc_bcm2835_sdhost_set_io,
	.get_card_present = sdhc_bcm2835_sdhost_get_card_present,
	.card_busy = sdhc_bcm2835_sdhost_card_busy,
	.get_host_props = sdhc_bcm2835_sdhost_get_host_props,
};

#define SDHC_BCM2835_SDHOST_INIT(inst)						\
	PINCTRL_DT_INST_DEFINE(inst);						\
	static const struct sdhc_bcm2835_sdhost_config				\
		sdhc_bcm2835_sdhost_cfg_##inst = {				\
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(inst)),			\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),			\
		.clock_freq = DT_INST_PROP(inst, clock_frequency),		\
		.bus_width = DT_INST_PROP(inst, bus_width),			\
	};									\
	static struct sdhc_bcm2835_sdhost_data sdhc_bcm2835_sdhost_data_##inst;	\
	DEVICE_DT_INST_DEFINE(inst,						\
			      sdhc_bcm2835_sdhost_init,				\
			      NULL,						\
			      &sdhc_bcm2835_sdhost_data_##inst,			\
			      &sdhc_bcm2835_sdhost_cfg_##inst,			\
			      POST_KERNEL,					\
			      CONFIG_SDHC_INIT_PRIORITY,			\
			      &sdhc_bcm2835_sdhost_api);

DT_INST_FOREACH_STATUS_OKAY(SDHC_BCM2835_SDHOST_INIT)
