/*
 * Copyright (c) 2026 David Paul-Beier
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * SDHC driver for the Xilinx Zynq-7000 PS SD/SDIO controller (Arasan-derived
 * SDHCI IP, UG585 ch. 13). Polling / PIO only -- no DMA, no UHS/tuning:
 * the controller instance verified on real hardware (TE0726 ZynqBerry,
 * xc7z010) never exercised anything beyond default/high-speed timing, and
 * shipping ADMA2 or 1.8V-signalling support unverified on real Zynq-7000
 * silicon would be worse than not shipping it. Both are natural follow-ups
 * once host_caps confirms real hardware support (see get_host_props()
 * below) and someone re-verifies against a scope/logic analyzer.
 *
 * Structurally this mirrors drivers/sdhc/xlnx_sdhc.c (the Xilinx Versal
 * SDHC driver) -- same subsys/sd integration shape -- but does not share
 * code with it: Zynq-7000's controller generation has no PHY/tap-delay
 * block, no Command Queue Engine, and (unlike Versal's driver) no
 * clock_control provider exists for this SoC in Zephyr, so the maximum
 * SD clock is read from the Capabilities register instead, the same way
 * every other Zynq-7000 driver in this tree gets board timing constants.
 */

#define DT_DRV_COMPAT xlnx_zynq_8_9a

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sdhc.h>
#include <zephyr/sd/sd_spec.h>
#include <zephyr/sd/sd.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include "xlnx_zynq_sdhc.h"

LOG_MODULE_REGISTER(xlnx_zynq_sdhc, CONFIG_SD_LOG_LEVEL);

/* Pin the SDHCI register offsets: struct zynq_sdhc_reg_base is a plain (non-
 * packed) struct that must land byte-for-byte on the standard SDHCI map. If a
 * field ever drifts (padding, a wrong type), these fail the build.
 */
BUILD_ASSERT(offsetof(struct zynq_sdhc_reg_base, block_size) == 0x04);
BUILD_ASSERT(offsetof(struct zynq_sdhc_reg_base, argument) == 0x08);
BUILD_ASSERT(offsetof(struct zynq_sdhc_reg_base, present_state) == 0x24);
BUILD_ASSERT(offsetof(struct zynq_sdhc_reg_base, clock_ctrl) == 0x2C);
BUILD_ASSERT(offsetof(struct zynq_sdhc_reg_base, normal_int_stat) == 0x30);
BUILD_ASSERT(offsetof(struct zynq_sdhc_reg_base, normal_int_stat_en) == 0x34);
BUILD_ASSERT(offsetof(struct zynq_sdhc_reg_base, capabilities) == 0x40);
BUILD_ASSERT(offsetof(struct zynq_sdhc_reg_base, adma_sys_addr) == 0x58);
BUILD_ASSERT(offsetof(struct zynq_sdhc_reg_base, host_cntrl_version) == 0xFE);
BUILD_ASSERT(sizeof(struct zynq_sdhc_reg_base) == 0x100);

#define XLNX_ZYNQ_SDHC_CHECK_BITS(b) ((uint64_t)1 << (b))
#define XLNX_ZYNQ_SDHC_GET_HOST_PROP_BIT(cap, b)                                                   \
	((uint8_t)((cap & (XLNX_ZYNQ_SDHC_CHECK_BITS(b))) >> b))

/** @brief Holds device private data. */
struct xlnx_zynq_sdhc_data {
	DEVICE_MMIO_RAM;
	/**< Current I/O settings of SDHC */
	struct sdhc_io host_io;
	/**< Supported properties of SDHC */
	struct sdhc_host_props props;
	/**< Transfer mode of the in-flight request */
	uint16_t transfermode;
	/**< SD clock input, read from Capabilities at reset (see .h fallback note) */
	uint32_t maxclock;
};

/** @brief Holds SDHC configuration data. */
struct xlnx_zynq_sdhc_config {
	/* MMIO mapping information for SDHC register base address */
	DEVICE_MMIO_ROM;
	/**< Card detection pin not usable -- always report present */
	bool broken_cd;
	/**< Delay given to card to power up or down fully */
	uint16_t powerdelay;
};

/** @brief Polled wait for selected 32-bit events */
static int8_t xlnx_zynq_sdhc_waitl_events(const void *base, int32_t timeout_ms, uint32_t events,
					  uint32_t value)
{
	for (int32_t retry = 0; retry < timeout_ms; retry++) {
		if ((*((volatile uint32_t *)base) & events) == value) {
			return 0;
		}
		k_msleep(1);
	}
	return -EAGAIN;
}

/** @brief Polled wait for selected 8-bit events */
static int8_t xlnx_zynq_sdhc_waitb_events(const void *base, int32_t timeout_ms, uint32_t events,
					  uint32_t value)
{
	for (int32_t retry = 0; retry < timeout_ms; retry++) {
		if ((*((volatile uint8_t *)base) & events) == value) {
			return 0;
		}
		k_msleep(1);
	}
	return -EAGAIN;
}

/** @brief Polled wait for any one of the given normal-interrupt-status events */
static int8_t xlnx_zynq_sdhc_wait_for_events(const void *base, int32_t timeout_ms, uint32_t events)
{
	for (int32_t retry = 0; retry < timeout_ms; retry++) {
		if ((*((volatile uint16_t *)base) & events) != 0U) {
			return 0;
		}
		k_msleep(1);
	}
	return -EAGAIN;
}

static int xlnx_zynq_sdhc_card_detect(const struct device *dev)
{
	const volatile struct zynq_sdhc_reg_base *reg =
		(struct zynq_sdhc_reg_base *)DEVICE_MMIO_GET(dev);
	const struct xlnx_zynq_sdhc_config *config = dev->config;

	if ((reg->present_state & XLNX_ZYNQ_SDHC_PSR_CARD_INSRT_MASK) != 0U) {
		return 1;
	}
	return config->broken_cd ? 1 : 0;
}

static void xlnx_zynq_sdhc_clear_intr(volatile struct zynq_sdhc_reg_base *reg)
{
	reg->normal_int_stat = XLNX_ZYNQ_SDHC_NORM_INTR_ALL;
	reg->err_int_stat = XLNX_ZYNQ_SDHC_ERROR_INTR_ALL;
}

static uint16_t xlnx_zynq_sdhc_cmd_frame(struct sdhc_command *cmd, bool data)
{
	uint16_t command = (cmd->opcode << XLNX_ZYNQ_SDHC_OPCODE_SHIFT);

	switch (cmd->response_type & XLNX_ZYNQ_SDHC_RESP) {
	case SD_RSP_TYPE_NONE:
		command |= XLNX_ZYNQ_SDHC_RESP_NONE;
		break;
	case SD_RSP_TYPE_R1:
	case SD_RSP_TYPE_R5:
	case SD_RSP_TYPE_R7:
		command |= XLNX_ZYNQ_SDHC_RESP_R1;
		break;
	case SD_RSP_TYPE_R1b:
		command |= XLNX_ZYNQ_SDHC_RESP_R1B;
		break;
	case SD_RSP_TYPE_R2:
		command |= XLNX_ZYNQ_SDHC_RESP_R2;
		break;
	case SD_RSP_TYPE_R3:
	case SD_RSP_TYPE_R4:
		command |= XLNX_ZYNQ_SDHC_RESP_R3;
		break;
	case SD_RSP_TYPE_R6:
		command |= XLNX_ZYNQ_SDHC_RESP_R6;
		break;
	default:
		LOG_DBG("Invalid response type");
		return XLNX_ZYNQ_SDHC_CMD_RESP_INVAL;
	}

	if (data) {
		command |= XLNX_ZYNQ_SDHC_DAT_PRESENT_SEL_MASK;
	}

	return command;
}

static int8_t xlnx_zynq_sdhc_cmd_response(const struct device *dev, struct sdhc_command *cmd)
{
	volatile struct zynq_sdhc_reg_base *reg = (struct zynq_sdhc_reg_base *)DEVICE_MMIO_GET(dev);
	uint32_t mask = XLNX_ZYNQ_SDHC_INTR_ERR_MASK | XLNX_ZYNQ_SDHC_INTR_CC_MASK;
	int8_t ret;

	ret = xlnx_zynq_sdhc_wait_for_events((void *)&reg->normal_int_stat, cmd->timeout_ms, mask);
	if (ret != 0) {
		LOG_ERR("No response from card (opcode %u)", cmd->opcode);
		LOG_ERR("DIAG present=0x%08x norm_int=0x%04x err_int=0x%04x "
			"clock_ctrl=0x%04x power_ctrl=0x%02x host_ctrl1=0x%02x "
			"cmd_reg=0x%04x arg=0x%08x xfer=0x%04x",
			reg->present_state, reg->normal_int_stat, reg->err_int_stat,
			reg->clock_ctrl, reg->power_ctrl, reg->host_ctrl1, reg->cmd, reg->argument,
			reg->transfer_mode);
		return ret;
	}

	if ((reg->normal_int_stat & XLNX_ZYNQ_SDHC_INTR_ERR_MASK) != 0U) {
		LOG_ERR("Error response from card (opcode %u, err_int_stat 0x%04x)", cmd->opcode,
			reg->err_int_stat);
		reg->err_int_stat = XLNX_ZYNQ_SDHC_ERROR_INTR_ALL;
		return -EINVAL;
	}
	reg->normal_int_stat = XLNX_ZYNQ_SDHC_INTR_CC_MASK;
	return 0;
}

static void xlnx_zynq_sdhc_update_response(const volatile struct zynq_sdhc_reg_base *reg,
					   struct sdhc_command *cmd)
{
	if (cmd->response_type == SD_RSP_TYPE_NONE) {
		return;
	}

	if (cmd->response_type == SD_RSP_TYPE_R2) {
		cmd->response[0] = reg->resp_0;
		cmd->response[1] = reg->resp_1;
		cmd->response[2] = reg->resp_2;
		cmd->response[3] = reg->resp_3;

		/* CRC is stripped from the response; shift to restore CSD/CID bit layout. */
		for (uint8_t i = 3; i != 0; i--) {
			cmd->response[i] <<= XLNX_ZYNQ_SDHC_CRC_LEFT_SHIFT;
			cmd->response[i] |= cmd->response[i - 1] >> XLNX_ZYNQ_SDHC_CRC_RIGHT_SHIFT;
		}
		cmd->response[0] <<= XLNX_ZYNQ_SDHC_CRC_LEFT_SHIFT;
	} else {
		cmd->response[0] = reg->resp_0;
	}
}

/** @brief Issue one command, wait for Command Complete, capture the response */
static int8_t xlnx_zynq_sdhc_cmd(const struct device *dev, struct sdhc_command *cmd, bool data)
{
	volatile struct zynq_sdhc_reg_base *reg = (struct zynq_sdhc_reg_base *)DEVICE_MMIO_GET(dev);
	struct xlnx_zynq_sdhc_data *dev_data = dev->data;
	uint16_t command;
	int8_t ret;

	if (xlnx_zynq_sdhc_waitl_events((void *)&reg->present_state, 100,
					XLNX_ZYNQ_SDHC_PSR_CMD_INHIBIT_MASK, 0) != 0) {
		LOG_ERR("Command line stuck busy");
		return -EBUSY;
	}

	reg->argument = cmd->arg;
	xlnx_zynq_sdhc_clear_intr(reg);

	command = xlnx_zynq_sdhc_cmd_frame(cmd, data);
	if (command == XLNX_ZYNQ_SDHC_CMD_RESP_INVAL) {
		return -EINVAL;
	}

	if (data && (reg->present_state & XLNX_ZYNQ_SDHC_PSR_INHIBIT_DAT_MASK) != 0U) {
		LOG_ERR("Card data lines busy");
		return -EBUSY;
	}

	reg->transfer_mode = dev_data->transfermode;
	reg->cmd = command;

	ret = xlnx_zynq_sdhc_cmd_response(dev, cmd);
	if (ret != 0) {
		return ret;
	}

	xlnx_zynq_sdhc_update_response(reg, cmd);
	return 0;
}

/**
 * @brief PIO block transfer -- one buffer-ready wait per 512-byte block.
 *
 * The controller signals Buffer Read/Write Ready once per block boundary
 * during a multi-block PIO transfer (SDHCI spec, not a Zynq-specific
 * behaviour); AUTO_CMD12 (set by the caller's transfer_mode) closes out a
 * multi-block command without a separate CMD12. No DMA / cache maintenance
 * is needed since the CPU itself is the only ADMA-equivalent here.
 */
static int8_t xlnx_zynq_sdhc_pio_xfr(const struct device *dev, struct sdhc_data *data, bool read)
{
	volatile struct zynq_sdhc_reg_base *reg = (struct zynq_sdhc_reg_base *)DEVICE_MMIO_GET(dev);
	uint8_t *buf = data->data;
	uint32_t ready_mask = read ? XLNX_ZYNQ_SDHC_INTR_BRR_MASK : XLNX_ZYNQ_SDHC_INTR_BWR_MASK;
	uint32_t err_mask = XLNX_ZYNQ_SDHC_INTR_ERR_MASK;

	for (uint32_t blk = 0; blk < data->blocks; blk++) {
		if (xlnx_zynq_sdhc_wait_for_events((void *)&reg->normal_int_stat, data->timeout_ms,
						   ready_mask | err_mask) != 0) {
			LOG_ERR("Block %u/%u buffer-ready timeout", blk, data->blocks);
			return -ETIMEDOUT;
		}
		if ((reg->normal_int_stat & err_mask) != 0U) {
			LOG_ERR("Error during block %u/%u (err_int_stat 0x%04x)", blk, data->blocks,
				reg->err_int_stat);
			reg->err_int_stat = XLNX_ZYNQ_SDHC_ERROR_INTR_ALL;
			return -EIO;
		}
		reg->normal_int_stat = ready_mask;

		for (uint32_t i = 0; i < data->block_size / 4U; i++) {
			if (read) {
				uint32_t w = reg->data_port;

				memcpy(buf + blk * data->block_size + i * 4U, &w, 4U);
			} else {
				uint32_t w;

				memcpy(&w, buf + blk * data->block_size + i * 4U, 4U);
				reg->data_port = w;
			}
		}
	}

	if (xlnx_zynq_sdhc_wait_for_events((void *)&reg->normal_int_stat, data->timeout_ms,
					   XLNX_ZYNQ_SDHC_INTR_TC_MASK | err_mask) != 0) {
		LOG_ERR("Transfer-complete timeout");
		return -ETIMEDOUT;
	}
	if ((reg->normal_int_stat & err_mask) != 0U) {
		LOG_ERR("Error at transfer complete (err_int_stat 0x%04x)", reg->err_int_stat);
		reg->err_int_stat = XLNX_ZYNQ_SDHC_ERROR_INTR_ALL;
		return -EIO;
	}
	reg->normal_int_stat = XLNX_ZYNQ_SDHC_INTR_TC_MASK;
	return 0;
}

static int xlnx_zynq_sdhc_transfer(const struct device *dev, struct sdhc_command *cmd,
				   struct sdhc_data *data)
{
	struct xlnx_zynq_sdhc_data *dev_data = dev->data;
	bool read;
	int ret;

	if (data == NULL) {
		return xlnx_zynq_sdhc_cmd(dev, cmd, false);
	}

	volatile struct zynq_sdhc_reg_base *reg = (struct zynq_sdhc_reg_base *)DEVICE_MMIO_GET(dev);

	reg->block_size = data->block_size;
	reg->block_count = data->blocks;
	read = (dev_data->transfermode & XLNX_ZYNQ_SDHC_TM_DAT_DIR_SEL_MASK) != 0U;

	ret = xlnx_zynq_sdhc_cmd(dev, cmd, true);
	if (ret != 0) {
		return ret;
	}

	return xlnx_zynq_sdhc_pio_xfr(dev, data, read);
}

static int xlnx_zynq_sdhc_request(const struct device *dev, struct sdhc_command *cmd,
				  struct sdhc_data *data)
{
	struct xlnx_zynq_sdhc_data *dev_data = dev->data;
	int ret;

	if (data == NULL) {
		/* No data phase -- Transfer Mode is meaningless to the
		 * controller here (and per SDHCI, undefined bits in it should
		 * not be asserted alongside a Command Present bit of 0).
		 */
		dev_data->transfermode = 0;
		return xlnx_zynq_sdhc_transfer(dev, cmd, data);
	}

	/* PIO: TM_DMA_EN (bit 0) is deliberately never set. */
	dev_data->transfermode =
		XLNX_ZYNQ_SDHC_TM_BLK_CNT_EN_MASK | XLNX_ZYNQ_SDHC_TM_DAT_DIR_SEL_MASK;

	switch (cmd->opcode) {
	case SD_READ_MULTIPLE_BLOCK:
		dev_data->transfermode |= XLNX_ZYNQ_SDHC_TM_AUTO_CMD12_EN_MASK |
					  XLNX_ZYNQ_SDHC_TM_MUL_SIN_BLK_SEL_MASK;
		break;
	case SD_WRITE_MULTIPLE_BLOCK:
		dev_data->transfermode |= XLNX_ZYNQ_SDHC_TM_AUTO_CMD12_EN_MASK |
					  XLNX_ZYNQ_SDHC_TM_MUL_SIN_BLK_SEL_MASK;
		dev_data->transfermode &= ~XLNX_ZYNQ_SDHC_TM_DAT_DIR_SEL_MASK;
		break;
	case SD_WRITE_SINGLE_BLOCK:
		dev_data->transfermode &= ~XLNX_ZYNQ_SDHC_TM_DAT_DIR_SEL_MASK;
		break;
	default:
		break;
	}

	ret = xlnx_zynq_sdhc_transfer(dev, cmd, data);
	dev_data->transfermode = 0;
	return ret;
}

static int xlnx_zynq_sdhc_host_props(const struct device *dev, struct sdhc_host_props *props)
{
	const volatile struct zynq_sdhc_reg_base *reg =
		(struct zynq_sdhc_reg_base *)DEVICE_MMIO_GET(dev);
	const struct xlnx_zynq_sdhc_config *config = dev->config;
	struct xlnx_zynq_sdhc_data *dev_data = dev->data;
	const uint64_t cap = reg->capabilities;
	const uint64_t current = reg->max_current_cap;

	memset(props, 0, sizeof(*props));

	/* High Speed (50 MHz, no tuning) is the fastest mode this driver
	 * implements -- see set_timing(). UHS is left to a follow-up once
	 * SDR50/SDR104/DDR50 capability bits are confirmed non-zero on real
	 * hardware; this driver never advertises them regardless of what the
	 * register reports, so subsys/sd cannot select an untested timing.
	 */
	props->f_max = SD_CLOCK_50MHZ;
	props->f_min = SDMMC_CLOCK_400KHZ;
	props->power_delay = config->powerdelay;

	props->host_caps.vol_180_support =
		XLNX_ZYNQ_SDHC_GET_HOST_PROP_BIT(cap, XLNX_ZYNQ_SDHC_1P8_VOL_SUPPORT);
	props->host_caps.vol_300_support =
		XLNX_ZYNQ_SDHC_GET_HOST_PROP_BIT(cap, XLNX_ZYNQ_SDHC_3P0_VOL_SUPPORT);
	props->host_caps.vol_330_support =
		XLNX_ZYNQ_SDHC_GET_HOST_PROP_BIT(cap, XLNX_ZYNQ_SDHC_3P3_VOL_SUPPORT);
	props->max_current_330 = (uint8_t)(current & XLNX_ZYNQ_SDHC_CURRENT_BYTE);
	props->max_current_300 = (uint8_t)((current >> XLNX_ZYNQ_SDHC_3P0_CURRENT_SUPPORT_SHIFT) &
					   XLNX_ZYNQ_SDHC_CURRENT_BYTE);
	props->max_current_180 = (uint8_t)((current >> XLNX_ZYNQ_SDHC_1P8_CURRENT_SUPPORT_SHIFT) &
					   XLNX_ZYNQ_SDHC_CURRENT_BYTE);
	props->host_caps.sdma_support =
		XLNX_ZYNQ_SDHC_GET_HOST_PROP_BIT(cap, XLNX_ZYNQ_SDHC_SDMA_SUPPORT);
	props->host_caps.high_spd_support =
		XLNX_ZYNQ_SDHC_GET_HOST_PROP_BIT(cap, XLNX_ZYNQ_SDHC_HIGH_SPEED_SUPPORT);
	/* ADMA2 capability is read and reported for diagnostics only -- this
	 * driver's transfer path is PIO-only regardless (see the file header).
	 */
	props->host_caps.adma_2_support =
		XLNX_ZYNQ_SDHC_GET_HOST_PROP_BIT(cap, XLNX_ZYNQ_SDHC_ADMA2_SUPPORT);
	props->host_caps.max_blk_len =
		(uint8_t)((cap >> XLNX_ZYNQ_SDHC_MAX_BLK_LEN_SHIFT) & XLNX_ZYNQ_SDHC_MAX_BLK_LEN);
	props->bus_4_bit_support =
		XLNX_ZYNQ_SDHC_GET_HOST_PROP_BIT(cap, XLNX_ZYNQ_SDHC_4BIT_SUPPORT);

	dev_data->maxclock =
		(uint32_t)((cap >> XLNX_ZYNQ_SDHC_BASE_CLK_SHIFT) & XLNX_ZYNQ_SDHC_BASE_CLK_MASK) *
		1000000U;
	if (dev_data->maxclock == 0U) {
		LOG_WRN("Capabilities base clock reads 0 -- assuming %u Hz (see driver header)",
			XLNX_ZYNQ_SDHC_FALLBACK_BASE_CLK_HZ);
		dev_data->maxclock = XLNX_ZYNQ_SDHC_FALLBACK_BASE_CLK_HZ;
	}

	dev_data->props = *props;
	return 0;
}

static uint16_t xlnx_zynq_sdhc_cal_clock(uint32_t maxclock, enum sdhc_clock_speed speed)
{
	uint16_t divisor = 0U;

	if (maxclock > speed) {
		for (uint16_t divcnt = 2U; divcnt <= XLNX_ZYNQ_SDHC_CC_EXT_MAX_DIV_CNT;
		     divcnt += 2U) {
			if ((maxclock / divcnt) <= speed) {
				divisor = divcnt >> 1;
				break;
			}
		}
	}

	return (divisor & XLNX_ZYNQ_SDHC_CC_SDCLK_FREQ_SEL) << XLNX_ZYNQ_SDHC_CC_DIV_SHIFT;
}

static int xlnx_zynq_sdhc_set_clock(const struct device *dev, enum sdhc_clock_speed speed)
{
	volatile struct zynq_sdhc_reg_base *reg = (struct zynq_sdhc_reg_base *)DEVICE_MMIO_GET(dev);
	struct xlnx_zynq_sdhc_data *dev_data = dev->data;
	uint16_t value;
	int ret;

	reg->clock_ctrl = 0;
	if (speed == 0U) {
		return 0;
	}

	value = xlnx_zynq_sdhc_cal_clock(dev_data->maxclock, speed);
	value |= XLNX_ZYNQ_SDHC_CC_INT_CLK_EN_MASK;
	reg->clock_ctrl = value;

	ret = xlnx_zynq_sdhc_waitb_events((void *)&reg->clock_ctrl, 150,
					  XLNX_ZYNQ_SDHC_CC_INT_CLK_STABLE_MASK,
					  XLNX_ZYNQ_SDHC_CC_INT_CLK_STABLE_MASK);
	if (ret != 0) {
		LOG_ERR("Internal clock not stable");
		return ret;
	}

	reg->clock_ctrl |= XLNX_ZYNQ_SDHC_CC_SD_CLK_EN_MASK;
	return 0;
}

static int xlnx_zynq_sdhc_set_buswidth(volatile struct zynq_sdhc_reg_base *reg,
				       enum sdhc_bus_width width)
{
	switch (width) {
	case SDHC_BUS_WIDTH1BIT:
		reg->host_ctrl1 &= ~XLNX_ZYNQ_SDHC_DAT_WIDTH4_MASK;
		break;
	case SDHC_BUS_WIDTH4BIT:
		reg->host_ctrl1 |= XLNX_ZYNQ_SDHC_DAT_WIDTH4_MASK;
		break;
	default:
		/* No eMMC slot on any known Zynq-7000 carrier -- no 8-bit mode. */
		return -ENOTSUP;
	}
	return 0;
}

static void xlnx_zynq_sdhc_set_power(const struct device *dev, enum sdhc_power power)
{
	volatile struct zynq_sdhc_reg_base *reg = (struct zynq_sdhc_reg_base *)DEVICE_MMIO_GET(dev);

	if (power == SDHC_POWER_ON) {
		reg->power_ctrl |= XLNX_ZYNQ_SDHC_PC_BUS_PWR_MASK;
	} else {
		reg->power_ctrl &= ~XLNX_ZYNQ_SDHC_PC_BUS_PWR_MASK;
	}
}

static int xlnx_zynq_sdhc_set_voltage(volatile struct zynq_sdhc_reg_base *reg,
				      enum sd_voltage voltage)
{
	switch (voltage) {
	case SD_VOL_3_3_V:
		reg->power_ctrl = XLNX_ZYNQ_SDHC_PC_BUS_VSEL_3V3;
		reg->host_ctrl2 &= ~XLNX_ZYNQ_SDHC_HC2_1V8_EN_MASK;
		break;
	case SD_VOL_3_0_V:
		reg->power_ctrl = XLNX_ZYNQ_SDHC_PC_BUS_VSEL_3V0;
		reg->host_ctrl2 &= ~XLNX_ZYNQ_SDHC_HC2_1V8_EN_MASK;
		break;
	default:
		/* 1.8V UHS signalling: not implemented (see get_host_props()) --
		 * unreachable in practice since vol_180_support is never advertised.
		 */
		return -ENOTSUP;
	}
	return 0;
}

/** @brief Legacy (default speed) or High Speed (50 MHz) only -- no UHS/tuning */
static int xlnx_zynq_sdhc_set_timing(const struct device *dev, enum sdhc_timing_mode timing)
{
	volatile struct zynq_sdhc_reg_base *reg = (struct zynq_sdhc_reg_base *)DEVICE_MMIO_GET(dev);

	switch (timing) {
	case SDHC_TIMING_LEGACY:
		reg->host_ctrl1 &= ~XLNX_ZYNQ_SDHC_HS_SPEED_MODE_EN_MASK;
		return 0;
	case SDHC_TIMING_HS:
	case SDHC_TIMING_SDR25:
		reg->host_ctrl1 |= XLNX_ZYNQ_SDHC_HS_SPEED_MODE_EN_MASK;
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int xlnx_zynq_sdhc_set_io(const struct device *dev, struct sdhc_io *ios)
{
	struct xlnx_zynq_sdhc_data *dev_data = dev->data;
	struct sdhc_io *host_io = &dev_data->host_io;
	volatile struct zynq_sdhc_reg_base *reg = (struct zynq_sdhc_reg_base *)DEVICE_MMIO_GET(dev);
	int ret;

	if ((ios->clock != 0) &&
	    ((ios->clock > dev_data->props.f_max) || (ios->clock < dev_data->props.f_min))) {
		LOG_ERR("Invalid clock value %u", ios->clock);
		return -EINVAL;
	}

	if (ios->power_mode != host_io->power_mode) {
		xlnx_zynq_sdhc_set_power(dev, ios->power_mode);
		host_io->power_mode = ios->power_mode;
	}

	if (ios->signal_voltage != host_io->signal_voltage) {
		ret = xlnx_zynq_sdhc_set_voltage(reg, ios->signal_voltage);
		if (ret != 0) {
			return ret;
		}
		host_io->signal_voltage = ios->signal_voltage;
	}

	if (ios->timing != host_io->timing) {
		ret = xlnx_zynq_sdhc_set_timing(dev, ios->timing);
		if (ret != 0) {
			return ret;
		}
		host_io->timing = ios->timing;
	}

	if (ios->clock != host_io->clock) {
		ret = xlnx_zynq_sdhc_set_clock(dev, ios->clock);
		if (ret != 0) {
			return ret;
		}
		host_io->clock = ios->clock;
	}

	if (ios->bus_width != host_io->bus_width) {
		ret = xlnx_zynq_sdhc_set_buswidth(reg, ios->bus_width);
		if (ret != 0) {
			return ret;
		}
		host_io->bus_width = ios->bus_width;
	}

	return 0;
}

static int xlnx_zynq_sdhc_host_reset(const struct device *dev)
{
	volatile struct zynq_sdhc_reg_base *reg = (struct zynq_sdhc_reg_base *)DEVICE_MMIO_GET(dev);
	int ret;

	reg->sw_reset = XLNX_ZYNQ_SDHC_SWRST_ALL_MASK;
	ret = xlnx_zynq_sdhc_waitb_events((void *)&reg->sw_reset, 100,
					  XLNX_ZYNQ_SDHC_SWRST_ALL_MASK, 0);
	if (ret != 0) {
		LOG_ERR("Software reset timed out -- device busy");
		return -EBUSY;
	}

	/* Status latching only -- signal_en stays 0, this driver is polling-only. */
	reg->normal_int_stat_en = XLNX_ZYNQ_SDHC_NORM_INTR_ALL;
	reg->err_int_stat_en = XLNX_ZYNQ_SDHC_ERROR_INTR_ALL;
	reg->normal_int_signal_en = 0;
	reg->err_int_signal_en = 0;

	reg->timeout_ctrl = XLNX_ZYNQ_SDHC_DAT_LINE_TIMEOUT;
	reg->block_size = XLNX_ZYNQ_SDHC_BLK_SIZE_512;

	xlnx_zynq_sdhc_clear_intr(reg);
	return 0;
}

static int xlnx_zynq_sdhc_card_busy(const struct device *dev)
{
	const volatile struct zynq_sdhc_reg_base *reg =
		(struct zynq_sdhc_reg_base *)DEVICE_MMIO_GET(dev);

	return ((reg->present_state & XLNX_ZYNQ_SDHC_PSR_INHIBIT_DAT_MASK) != 0U) ? 1 : 0;
}

static int xlnx_zynq_sdhc_init(const struct device *dev)
{
	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);
	return xlnx_zynq_sdhc_host_reset(dev);
}

static DEVICE_API(sdhc, xlnx_zynq_sdhc_api) = {
	.reset = xlnx_zynq_sdhc_host_reset,
	.request = xlnx_zynq_sdhc_request,
	.set_io = xlnx_zynq_sdhc_set_io,
	.get_card_present = xlnx_zynq_sdhc_card_detect,
	.card_busy = xlnx_zynq_sdhc_card_busy,
	.get_host_props = xlnx_zynq_sdhc_host_props,
	/* .execute_tuning intentionally omitted: no UHS support in this driver. */
};

#define XLNX_ZYNQ_SDHC_INIT(n)                                                                     \
	const static struct xlnx_zynq_sdhc_config xlnx_zynq_sdhc_inst_##n = {                      \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),                                              \
		.broken_cd = DT_INST_PROP_OR(n, broken_cd, 0),                                     \
		.powerdelay = DT_INST_PROP(n, power_delay_ms),                                     \
	};                                                                                         \
	static struct xlnx_zynq_sdhc_data xlnx_zynq_sdhc_data_##n;                                 \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, xlnx_zynq_sdhc_init, NULL, &xlnx_zynq_sdhc_data_##n,              \
			      &xlnx_zynq_sdhc_inst_##n, POST_KERNEL,                               \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &xlnx_zynq_sdhc_api);

DT_INST_FOREACH_STATUS_OKAY(XLNX_ZYNQ_SDHC_INIT)
