/*
 * Copyright (c) 2025 Advanced Micro Devices, Inc. (AMD)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT xlnx_versal_8_9a

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sdhc.h>
#include <zephyr/sd/sd_spec.h>
#include <zephyr/sd/sd.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/clock_control.h>
#include "xlnx_sdhc.h"

LOG_MODULE_REGISTER(xlnx_sdhc, CONFIG_SD_LOG_LEVEL);

#define CHECK_BITS(b) ((uint64_t)1 << (b))

#define XLNX_SDHC_SLOT_TYPE(dev) \
	((((struct sd_data *)dev->data)->props.host_caps.slot_type != 0) ? \
	 XLNX_SDHC_EMMC_SLOT : XLNX_SDHC_SD_SLOT)

#define XLNX_SDHC_GET_HOST_PROP_BIT(cap, b) ((uint8_t)((cap & (CHECK_BITS(b))) >> b))

/**
 * @brief ADMA2 descriptor table structure.
 */
typedef struct {
	/**< Attributes of descriptor */
	uint16_t attribute;
	/**< Length of current dma transfer max 64kb */
	uint16_t length;
	/**< source/destination address for current dma transfer */
	uint64_t address;
} __packed adma2_descriptor;

/**
 * @brief Holds device private data.
 */
struct sd_data {
	DEVICE_MMIO_RAM;
	/**< Current I/O settings of SDHC */
	struct sdhc_io host_io;
	/**< Supported properties of SDHC */
	struct sdhc_host_props props;
	/**< SDHC IRQ events */
	struct k_event irq_event;
	/**< Used to identify HC internal phy register */
	bool has_phy;
	/**< transfer mode and data direction */
	uint16_t transfermode;
	/**< Maximum input clock supported by HC */
	uint32_t maxclock;
	/**< ADMA descriptor table */
	adma2_descriptor adma2_descrtbl[MAX(1, CONFIG_HOST_ADMA2_DESC_SIZE)];
};

/**
 * @brief Holds SDHC configuration data.
 */
struct xlnx_sdhc_config {
	/* MMIO mapping information for SDHC register base address */
	DEVICE_MMIO_ROM;
	/**< Pointer to the device structure representing the clock bus */
	const struct device *clock_dev;
	/**< Callback to the device interrupt configuration api */
	void (*irq_config_func)(const struct device *dev);
	/**< Card detection pin available or not */
	bool broken_cd;
	/**< Support hs200 mode. */
	bool hs200_mode;
	/**< Support hs400 mode */
	bool hs400_mode;
	/**< delay given to card to power up or down fully */
	uint16_t powerdelay;
};

/**
 * @brief
 * polled wait for selected number of 32 bit events
 */
static int8_t xlnx_sdhc_waitl_events(const void *base, int32_t timeout_ms, uint32_t events,
		uint32_t value)
{
	int8_t ret = -EAGAIN;

	for (uint32_t retry = 0; retry < timeout_ms; retry++) {
		if ((*((volatile uint32_t *)base) & events) == value) {
			ret = 0;
			break;
		}
		k_msleep(1);
	}

	return ret;
}

/**
 * @brief
 * polled wait for selected number of 8 bit events
 */
static int8_t xlnx_sdhc_waitb_events(const void *base, int32_t timeout_ms, uint32_t events,
		uint32_t value)
{
	int8_t ret = -EAGAIN;

	for (uint32_t retry = 0; retry < timeout_ms; retry++) {
		if ((*((volatile uint8_t *)base) & events) == value) {
			ret = 0;
			break;
		}
		k_msleep(1);
	}

	return ret;
}

/**
 * @brief
 * Polled wait for any one of the given event
 */
static int8_t xlnx_sdhc_wait_for_events(const void *base, int32_t timeout_ms,
		uint32_t events)
{
	int8_t ret = -EAGAIN;

	for (uint32_t retry = 0; retry < timeout_ms; retry++) {
		if ((*((volatile uint32_t *)base) & events) != 0U) {
			ret = 0;
			break;
		}
		k_msleep(1);
	}

	return ret;
}

/**
 * @brief
 * Check card is detected by host
 */
static int xlnx_sdhc_card_detect(const struct device *dev)
{
	const volatile struct reg_base *reg = (struct reg_base *)DEVICE_MMIO_GET(dev);
	const struct xlnx_sdhc_config *config = dev->config;

	if ((reg->present_state & XLNX_SDHC_PSR_CARD_INSRT_MASK) != 0U) {
		return 1;
	}

	/* In case of polling always treat card is detected */
	if (config->broken_cd == true) {
		return 1;
	}

	return 0;
}

/**
 * @brief
 * Clear the controller status registers
 */
static void xlnx_sdhc_clear_intr(volatile struct reg_base *reg)
{
	reg->normal_int_stat = XLNX_SDHC_NORM_INTR_ALL;
	reg->err_int_stat = XLNX_SDHC_ERROR_INTR_ALL;
}

/**
 * @brief
 * Setup ADMA2 discriptor table for data transfer
 */
static int xlnx_sdhc_setup_adma(const struct device *dev, const struct sdhc_data *data)
{
	volatile struct reg_base *reg = (struct reg_base *)DEVICE_MMIO_GET(dev);
	struct sd_data *dev_data = dev->data;
	uint32_t adma_table;
	uint32_t descnum;
	const uint8_t *buff = data->data;
	int ret = 0;

	if ((data->block_size * data->blocks) < XLNX_SDHC_DESC_MAX_LENGTH) {
		adma_table = 1U;
	} else {
		adma_table = ((data->block_size * data->blocks) /
				XLNX_SDHC_DESC_MAX_LENGTH);
		if (((data->block_size * data->blocks) % XLNX_SDHC_DESC_MAX_LENGTH) != 0U) {
			adma_table += 1U;
		}
	}

	if (adma_table > CONFIG_HOST_ADMA2_DESC_SIZE) {
		LOG_ERR("Descriptor size is too big");
		return -ENOTSUP;
	}

	for (descnum = 0U; descnum < (adma_table - 1U); descnum++) {
		dev_data->adma2_descrtbl[descnum].address =
			((uintptr_t)buff + (descnum * XLNX_SDHC_DESC_MAX_LENGTH));
		dev_data->adma2_descrtbl[descnum].attribute =
			XLNX_SDHC_DESC_TRAN | XLNX_SDHC_DESC_VALID;
		dev_data->adma2_descrtbl[descnum].length = 0U;
	}

	dev_data->adma2_descrtbl[adma_table - 1U].address =
		((uintptr_t)buff + (descnum * XLNX_SDHC_DESC_MAX_LENGTH));
	dev_data->adma2_descrtbl[adma_table - 1U].attribute = XLNX_SDHC_DESC_TRAN |
		XLNX_SDHC_DESC_END | XLNX_SDHC_DESC_VALID;
	dev_data->adma2_descrtbl[adma_table - 1U].length =
		((data->blocks * data->block_size) - (descnum * XLNX_SDHC_DESC_MAX_LENGTH));

	reg->adma_sys_addr = ((uintptr_t)&(dev_data->adma2_descrtbl[0]) & ~(uintptr_t)0x0);

	return ret;
}

/**
 * @brief
 * Frame the command
 */
static uint16_t xlnx_sdhc_cmd_frame(struct sdhc_command *cmd, bool data, uint8_t slottype)
{
	uint16_t command = (cmd->opcode << XLNX_SDHC_OPCODE_SHIFT);

	switch (cmd->response_type & XLNX_SDHC_RESP) {
	case SD_RSP_TYPE_NONE:
		command |= RESP_NONE;
		break;

	case SD_RSP_TYPE_R1:
		command |= RESP_R1;
		break;

	case SD_RSP_TYPE_R1b:
		command |= RESP_R1B;
		break;

	case SD_RSP_TYPE_R2:
		command |= RESP_R2;
		break;

	case SD_RSP_TYPE_R3:
		command |= RESP_R3;
		break;

	case SD_RSP_TYPE_R6:
		command |= RESP_R6;
		break;

	case SD_RSP_TYPE_R7:
		/* As per spec, EMMC does not support R7 */
		if (slottype == XLNX_SDHC_EMMC_SLOT) {
			return XLNX_SDHC_CMD_RESP_INVAL;
		}
		command |= RESP_R1;
		break;

	default:
		LOG_DBG("Invalid response type");
		return XLNX_SDHC_CMD_RESP_INVAL;
	}

	/* EMMC does not support APP command */
	if ((cmd->opcode == SD_APP_CMD) && (slottype == XLNX_SDHC_EMMC_SLOT)) {
		LOG_DBG("Invalid response type");
		return XLNX_SDHC_CMD_RESP_INVAL;
	}

	if (data) {
		command |= XLNX_SDHC_DAT_PRESENT_SEL_MASK;
	}

	return command;
}

/**
 * @brief
 * Check command response is success or failed also clears status registers
 */
static int8_t xlnx_sdhc_cmd_response(const struct device *dev, struct sdhc_command *cmd)
{
	const struct xlnx_sdhc_config *config = dev->config;
	volatile struct reg_base *reg = (struct reg_base *)DEVICE_MMIO_GET(dev);
	struct sd_data *dev_data = dev->data;
	uint32_t mask;
	uint32_t events;
	int8_t ret;
	k_timeout_t timeout;

	mask = XLNX_SDHC_INTR_ERR_MASK | XLNX_SDHC_INTR_CC_MASK;
	if ((cmd->opcode == SD_SEND_TUNING_BLOCK) || (cmd->opcode == MMC_SEND_TUNING_BLOCK)) {
		mask |= XLNX_SDHC_INTR_BRR_MASK;
	}

	if (config->irq_config_func == NULL) {
		ret = xlnx_sdhc_wait_for_events((void *)&reg->normal_int_stat,
				cmd->timeout_ms, mask);
		if (ret != 0) {
			LOG_ERR("No response from card");
			return ret;
		}

		if ((reg->normal_int_stat & XLNX_SDHC_INTR_ERR_MASK) != 0U) {
			LOG_ERR("Error response from card");
			reg->err_int_stat = XLNX_SDHC_ERROR_INTR_ALL;
			return -EINVAL;
		}
		reg->normal_int_stat = XLNX_SDHC_INTR_CC_MASK;
	} else {
		timeout = K_MSEC(cmd->timeout_ms);

		events = k_event_wait(&dev_data->irq_event, mask, false, timeout);

		if ((events & XLNX_SDHC_INTR_ERR_MASK) != 0U) {
			LOG_ERR("Error response from card");
			ret = -EINVAL;
		} else if ((events & XLNX_SDHC_INTR_CC_MASK) ||
				(events & XLNX_SDHC_INTR_BRR_MASK)) {
			ret = 0;
		} else {
			LOG_ERR("No response from card");
			ret = -EAGAIN;
		}
	}
	return ret;
}

/**
 * @brief
 * Update response member of command structure which is used by subsystem
 */
static void xlnx_sdhc_update_response(const volatile struct reg_base *reg,
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

		/* CRC is striped from the response performing shifting to update response */
		for (uint8_t i = 3; i != 0; i--) {
			cmd->response[i] <<= XLNX_SDHC_CRC_LEFT_SHIFT;
			cmd->response[i] |= cmd->response[i-1] >> XLNX_SDHC_CRC_RIGHT_SHIFT;
		}
		cmd->response[0] <<= XLNX_SDHC_CRC_LEFT_SHIFT;
	} else {
		cmd->response[0] = reg->resp_0;
	}
}

/**
 * @brief
 * Setup and send the command and also check for response
 */
static int8_t xlnx_sdhc_cmd(const struct device *dev, struct sdhc_command *cmd, bool data)
{
	const struct xlnx_sdhc_config *config = dev->config;
	volatile struct reg_base *reg = (struct reg_base *)DEVICE_MMIO_GET(dev);
	struct sd_data *dev_data = dev->data;
	uint16_t command;
	uint8_t slottype = XLNX_SDHC_SLOT_TYPE(dev);
	int8_t ret;

	reg->argument = cmd->arg;

	xlnx_sdhc_clear_intr(reg);

	/* Frame command */
	command = xlnx_sdhc_cmd_frame(cmd, data, slottype);
	if (command == XLNX_SDHC_CMD_RESP_INVAL) {
		return -EINVAL;
	}

	if ((cmd->opcode != SD_SEND_TUNING_BLOCK) && (cmd->opcode != MMC_SEND_TUNING_BLOCK)) {
		if (((reg->present_state & XLNX_SDHC_PSR_INHIBIT_DAT_MASK) != 0U) &&
				((command & XLNX_SDHC_DAT_PRESENT_SEL_MASK) != 0U)) {
			LOG_ERR("Card data lines busy");
			return -EBUSY;
		}
	}

	if (config->irq_config_func != NULL) {
		k_event_clear(&dev_data->irq_event, XLNX_SDHC_TXFR_INTR_EN_MASK);
	}

	reg->transfer_mode = dev_data->transfermode;
	reg->cmd = command;

	/* Check for response */
	ret = xlnx_sdhc_cmd_response(dev, cmd);
	if (ret != 0) {
		return ret;
	}

	xlnx_sdhc_update_response(reg, cmd);

	return 0;
}

/**
 * @brief
 * Check for data transfer completion
 */
static int8_t xlnx_sdhc_xfr(const struct device *dev, struct sdhc_data *data)
{
	const struct xlnx_sdhc_config *config = dev->config;
	volatile struct reg_base *reg = (struct reg_base *)DEVICE_MMIO_GET(dev);
	struct sd_data *dev_data = dev->data;
	uint32_t events;
	uint32_t mask;
	int8_t ret;
	k_timeout_t timeout;

	mask = XLNX_SDHC_INTR_ERR_MASK | XLNX_SDHC_INTR_TC_MASK;
	if (config->irq_config_func == NULL) {
		ret = xlnx_sdhc_wait_for_events((void *)&reg->normal_int_stat,
				data->timeout_ms, mask);
		if (ret != 0) {
			LOG_ERR("Data transfer timeout");
			return ret;
		}

		if ((reg->normal_int_stat & XLNX_SDHC_INTR_ERR_MASK) != 0U) {
			reg->err_int_stat = XLNX_SDHC_ERROR_INTR_ALL;
			LOG_ERR("Error at data transfer");
			return -EINVAL;
		}

		reg->normal_int_stat = XLNX_SDHC_INTR_TC_MASK;
	} else {
		timeout = K_MSEC(data->timeout_ms);

		events = k_event_wait(&dev_data->irq_event, mask, false, timeout);

		if ((events & XLNX_SDHC_INTR_ERR_MASK) != 0U) {
			LOG_ERR("Error at data transfer");
			ret = -EINVAL;
		} else if ((events & XLNX_SDHC_INTR_TC_MASK) != 0U) {
			ret = 0;
		} else {
			LOG_ERR("Data transfer timeout");
			ret = -EAGAIN;
		}
	}
	return ret;
}

/**
 * @brief
 * Performs data and command transfer and check for transfer complete
 */
static int8_t xlnx_sdhc_transfer(const struct device *dev, struct sdhc_command *cmd,
		struct sdhc_data *data)
{
	volatile struct reg_base *reg = (struct reg_base *)DEVICE_MMIO_GET(dev);
	int8_t ret = -EINVAL;

	/* Check command line is in use */
	if ((reg->present_state & 1U) != 0U) {
		LOG_ERR("Command lines are busy");
		return -EBUSY;
	}

	if (data != NULL) {
		reg->block_size = data->block_size;
		reg->block_count = data->blocks;

		/* Setup ADMA2 if data is present */
		ret = xlnx_sdhc_setup_adma(dev, data);
		if (ret != 0) {
			return ret;
		}

		/* Send command and check for command complete */
		ret = xlnx_sdhc_cmd(dev, cmd, true);
		if (ret != 0) {
			return ret;
		}

		/* Check for data transfer complete */
		ret = xlnx_sdhc_xfr(dev, data);
		if (ret != 0) {
			return ret;
		}
	} else {
		/* Send command and check for command complete */
		ret = xlnx_sdhc_cmd(dev, cmd, false);
		return ret;
	}

	return ret;
}

/**
 * @brief
 * Configure transfer mode and transfer command and data
 */
static int xlnx_sdhc_request(const struct device *dev, struct sdhc_command *cmd,
		struct sdhc_data *data)
{
	struct sd_data *dev_data = dev->data;
	int ret;

	if (dev_data->transfermode == 0U) {
		dev_data->transfermode = XLNX_SDHC_TM_DMA_EN_MASK |
			XLNX_SDHC_TM_BLK_CNT_EN_MASK |
			XLNX_SDHC_TM_DAT_DIR_SEL_MASK;
	}

	switch (cmd->opcode) {
	case SD_READ_MULTIPLE_BLOCK:
		dev_data->transfermode |= XLNX_SDHC_TM_AUTO_CMD12_EN_MASK |
			XLNX_SDHC_TM_MUL_SIN_BLK_SEL_MASK;
		ret = xlnx_sdhc_transfer(dev, cmd, data);
		break;

	case SD_WRITE_MULTIPLE_BLOCK:
		dev_data->transfermode |= XLNX_SDHC_TM_AUTO_CMD12_EN_MASK |
			XLNX_SDHC_TM_MUL_SIN_BLK_SEL_MASK;
		dev_data->transfermode &= ~XLNX_SDHC_TM_DAT_DIR_SEL_MASK;
		ret = xlnx_sdhc_transfer(dev, cmd, data);
		break;

	case SD_WRITE_SINGLE_BLOCK:
		dev_data->transfermode &= ~XLNX_SDHC_TM_DAT_DIR_SEL_MASK;
		ret = xlnx_sdhc_transfer(dev, cmd, data);
		break;

	default:
		ret = xlnx_sdhc_transfer(dev, cmd, data);
	}
	dev_data->transfermode = 0;

	return ret;
}

/**
 * @brief
 * Populate sdhc_host_props structure with all sd host controller property
 */
static int xlnx_sdhc_host_props(const struct device *dev, struct sdhc_host_props *props)
{
	const struct xlnx_sdhc_config *config = dev->config;
	const volatile struct reg_base *reg = (struct reg_base *)DEVICE_MMIO_GET(dev);
	struct sd_data *dev_data = dev->data;
	const uint64_t cap = reg->capabilities;
	const uint64_t current = reg->max_current_cap;

	props->f_max = SD_CLOCK_208MHZ;
	props->f_min = SDMMC_CLOCK_400KHZ;

	props->power_delay = config->powerdelay;

	props->host_caps.vol_180_support = XLNX_SDHC_GET_HOST_PROP_BIT(cap,
			XLNX_SDHC_1P8_VOL_SUPPORT);
	props->host_caps.vol_300_support = XLNX_SDHC_GET_HOST_PROP_BIT(cap,
			XLNX_SDHC_3P0_VOL_SUPPORT);
	props->host_caps.vol_330_support = XLNX_SDHC_GET_HOST_PROP_BIT(cap,
			XLNX_SDHC_3P3_VOL_SUPPORT);
	props->max_current_330 = (uint8_t)(current & XLNX_SDHC_CURRENT_BYTE);
	props->max_current_300 = (uint8_t)((current >> XLNX_SDHC_3P0_CURRENT_SUPPORT_SHIFT) &
			XLNX_SDHC_CURRENT_BYTE);
	props->max_current_180 = (uint8_t)((current >> XLNX_SDHC_1P8_CURRENT_SUPPORT_SHIFT) &
			XLNX_SDHC_CURRENT_BYTE);
	props->host_caps.sdma_support = XLNX_SDHC_GET_HOST_PROP_BIT(cap, XLNX_SDHC_SDMA_SUPPORT);
	props->host_caps.high_spd_support = XLNX_SDHC_GET_HOST_PROP_BIT(cap,
			XLNX_SDHC_HIGH_SPEED_SUPPORT);
	props->host_caps.adma_2_support = XLNX_SDHC_GET_HOST_PROP_BIT(cap,
			XLNX_SDHC_ADMA2_SUPPORT);
	props->host_caps.max_blk_len = (uint8_t)((cap >> XLNX_SDHC_MAX_BLK_LEN_SHIFT) &
			XLNX_SDHC_MAX_BLK_LEN);
	props->host_caps.ddr50_support = XLNX_SDHC_GET_HOST_PROP_BIT(cap, XLNX_SDHC_DDR50_SUPPORT);
	props->host_caps.sdr104_support = XLNX_SDHC_GET_HOST_PROP_BIT(cap,
			XLNX_SDHC_SDR104_SUPPORT);
	props->host_caps.sdr50_support = XLNX_SDHC_GET_HOST_PROP_BIT(cap, XLNX_SDHC_SDR50_SUPPORT);
	props->host_caps.slot_type = (uint8_t)((cap >> XLNX_SDHC_SLOT_TYPE_SHIFT) &
			XLNX_SDHC_SLOT_TYPE_GET);
	props->host_caps.bus_8_bit_support = XLNX_SDHC_GET_HOST_PROP_BIT(cap,
			XLNX_SDHC_8BIT_SUPPORT);
	props->host_caps.bus_4_bit_support = XLNX_SDHC_GET_HOST_PROP_BIT(cap,
			XLNX_SDHC_4BIT_SUPPORT);

	if ((cap & CHECK_BITS(XLNX_SDHC_SDR400_SUPPORT)) != 0U) {
		props->host_caps.hs400_support = (uint8_t)config->hs400_mode;
		dev_data->has_phy = true;
	}
	props->host_caps.hs200_support = (uint8_t)config->hs200_mode;

	dev_data->props = *props;

	return 0;
}

/**
 * @brief
 * Calculate clock value based on the selected speed
 */
static uint16_t xlnx_sdhc_cal_clock(uint32_t maxclock, enum sdhc_clock_speed speed)
{
	uint16_t divcnt;
	uint16_t divisor = 0U, clockval = 0U;

	if (maxclock <= speed) {
		divisor = 0U;
	} else {
		for (divcnt = 2U; divcnt <= XLNX_SDHC_CC_EXT_MAX_DIV_CNT; divcnt += 2U) {
			if ((maxclock / divcnt) <= speed) {
				divisor = divcnt >> XLNX_SDHC_CLOCK_CNT_SHIFT;
				break;
			}
		}
	}

	clockval |= (divisor & XLNX_SDHC_CC_SDCLK_FREQ_SEL) << XLNX_SDHC_CC_DIV_SHIFT;
	clockval |= ((divisor >> XLNX_SDHC_CC_DIV_SHIFT) & XLNX_SDHC_CC_SDCLK_FREQ_SEL_EXT) <<
		XLNX_SDHC_CC_EXT_DIV_SHIFT;

	return clockval;
}

/**
 * @brief
 * Select frequency window for dll clock
 */
static void xlnx_sdhc_select_dll_feq(volatile struct reg_base *reg,
		enum sdhc_clock_speed speed)
{
	uint32_t freq;
	uint8_t selfreq;

	freq = speed / XLNX_SDHC_KHZ_TO_MHZ;
	if ((freq <= XLNX_SDHC_200_FREQ) && (freq > XLNX_SDHC_170_FREQ)) {
		selfreq = XLNX_SDHC_FREQSEL_200M_170M;
	} else if ((freq <= XLNX_SDHC_170_FREQ) && (freq > XLNX_SDHC_140_FREQ)) {
		selfreq = XLNX_SDHC_FREQSEL_170M_140M;
	} else if ((freq <= XLNX_SDHC_140_FREQ) && (freq > XLNX_SDHC_110_FREQ)) {
		selfreq = XLNX_SDHC_FREQSEL_140M_110M;
	} else if ((freq <= XLNX_SDHC_110_FREQ) && (freq > XLNX_SDHC_80_FREQ)) {
		selfreq = XLNX_SDHC_FREQSEL_110M_80M;
	} else {
		selfreq = XLNX_SDHC_FREQSEL_80M_50M;
	}

	reg->phy_ctrl2 |= (selfreq << XLNX_SDHC_PHYREG2_FREQ_SEL_SHIFT);
}

/**
 * @brief
 * Disable and configure dll clock
 */
static void xlnx_sdhc_config_dll_clock(volatile struct reg_base *reg,
		enum sdhc_clock_speed speed)
{
	/*
	 * Configure dll based clk if speed is greater than or equal to 50MHZ else configure
	 * delay chain based clock
	 */
	reg->phy_ctrl2 &= ~XLNX_SDHC_PHYREG2_DLL_EN_MASK;
	if (speed >= SD_CLOCK_50MHZ) {
		reg->phy_ctrl2 &= ~XLNX_SDHC_PHYREG2_FREQ_SEL;
		reg->phy_ctrl2 &= ~XLNX_SDHC_PHYREG2_TRIM_ICP;
		reg->phy_ctrl2 &= ~XLNX_SDHC_PHYREG2_DLYTX_SEL_MASK;
		reg->phy_ctrl2 &= ~XLNX_SDHC_PHYREG2_DLYRX_SEL_MASK;
		reg->phy_ctrl2 |= (XLNX_SDHC_PHYREG2_TRIM_ICP_DEF_VAL <<
				XLNX_SDHC_PHYREG2_TRIM_ICP_SHIFT);
		xlnx_sdhc_select_dll_feq(reg, speed);
	} else {
		reg->phy_ctrl2 |= XLNX_SDHC_PHYREG2_DLYTX_SEL_MASK;
		reg->phy_ctrl2 |= XLNX_SDHC_PHYREG2_DLYRX_SEL_MASK;
	}
}

/**
 * @brief
 * Enable dll clock
 */
static int8_t xlnx_sdhc_enable_dll_clock(volatile struct reg_base *reg)
{
	int8_t ret;

	reg->phy_ctrl2 |= XLNX_SDHC_PHYREG2_DLL_EN_MASK;

	/* Wait max 100ms for dll clock to stable */
	ret = xlnx_sdhc_waitl_events((void *)&reg->phy_ctrl2, 100,
			XLNX_SDHC_PHYREG2_DLL_RDY_MASK, XLNX_SDHC_PHYREG2_DLL_RDY_MASK);
	if (ret != 0) {
		LOG_ERR("Failed to enable dll clock");
	}

	return ret;
}

/**
 * @brief
 * Set clock and wait for clock to be stable
 */
static int xlnx_sdhc_set_clock(const struct device *dev, enum sdhc_clock_speed speed)
{
	const struct xlnx_sdhc_config *config = dev->config;
	volatile struct reg_base *reg = (struct reg_base *)DEVICE_MMIO_GET(dev);
	struct sd_data *dev_data = dev->data;
	int ret;
	uint16_t value;

	/* Disable clock */
	reg->clock_ctrl = 0;
	if (speed == 0U) {
		return 0;
	}

	/* Get input clock rate */
	ret = clock_control_get_rate(config->clock_dev, NULL, &dev_data->maxclock);
	if (ret != 0) {
		LOG_ERR("Failed to get clock\n");
		return ret;
	}

	/* Calculate clock */
	value = xlnx_sdhc_cal_clock(dev_data->maxclock, speed);
	value |= XLNX_SDHC_CC_INT_CLK_EN_MASK;

	/* Configure dll clock */
	if (dev_data->has_phy == true) {
		xlnx_sdhc_config_dll_clock(reg, speed);
	}

	/* Wait max 150ms for internal clock to be stable */
	reg->clock_ctrl = value;
	ret = xlnx_sdhc_waitb_events((void *)&reg->clock_ctrl, 150,
			XLNX_SDHC_CC_INT_CLK_STABLE_MASK, XLNX_SDHC_CC_INT_CLK_STABLE_MASK);
	if (ret != 0) {
		return ret;
	}

	/* Enable div clock */
	reg->clock_ctrl |= XLNX_SDHC_CC_SD_CLK_EN_MASK;

	/* Enable dll clock */
	if ((dev_data->has_phy == true) && (speed >= SD_CLOCK_50MHZ)) {
		ret = xlnx_sdhc_enable_dll_clock(reg);
	}

	return ret;
}

/**
 * @brief
 * Set bus width on the controller
 */
static int8_t xlnx_sdhc_set_buswidth(volatile struct reg_base *reg,
		enum sdhc_bus_width width)
{
	switch (width) {
	case SDHC_BUS_WIDTH1BIT:
		reg->host_ctrl1 &= ~XLNX_SDHC_DAT_WIDTH8_MASK;
		reg->host_ctrl1 &= ~XLNX_SDHC_DAT_WIDTH4_MASK;
		break;

	case SDHC_BUS_WIDTH4BIT:
		reg->host_ctrl1 &= ~XLNX_SDHC_DAT_WIDTH8_MASK;
		reg->host_ctrl1 |= XLNX_SDHC_DAT_WIDTH4_MASK;
		break;

	case SDHC_BUS_WIDTH8BIT:
		reg->host_ctrl1 |= XLNX_SDHC_DAT_WIDTH8_MASK;
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

/**
 * @brief
 * Enable or disable power
 */
static void xlnx_sdhc_set_power(const struct device *dev, enum sdhc_power power)
{
	volatile struct reg_base *reg = (struct reg_base *)DEVICE_MMIO_GET(dev);

	if (XLNX_SDHC_SLOT_TYPE(dev) == XLNX_SDHC_EMMC_SLOT) {
		if (power == SDHC_POWER_ON) {
			reg->power_ctrl &= ~XLNX_SDHC_PC_EMMC_HW_RST_MASK;
			reg->power_ctrl |= XLNX_SDHC_PC_BUS_PWR_MASK;
		} else {
			reg->power_ctrl |= XLNX_SDHC_PC_EMMC_HW_RST_MASK;
			reg->power_ctrl &= ~XLNX_SDHC_PC_BUS_PWR_MASK;
		}
	} else {
		if (power == SDHC_POWER_ON) {
			reg->power_ctrl |= XLNX_SDHC_PC_BUS_PWR_MASK;
		} else {
			reg->power_ctrl &= ~XLNX_SDHC_PC_BUS_PWR_MASK;
		}
	}
}

/**
 * @brief
 * Set voltage level and signalling voltage
 */
static int8_t xlnx_sdhc_set_voltage(volatile struct reg_base *reg, enum sd_voltage voltage)
{
	switch (voltage) {
	case SD_VOL_3_3_V:
		reg->power_ctrl = XLNX_SDHC_PC_BUS_VSEL_3V3;
		reg->host_ctrl2 &= ~XLNX_SDHC_HC2_1V8_EN_MASK;
		break;

	case SD_VOL_3_0_V:
		reg->power_ctrl = XLNX_SDHC_PC_BUS_VSEL_3V0;
		reg->host_ctrl2 &= ~XLNX_SDHC_HC2_1V8_EN_MASK;
		break;

	case SD_VOL_1_8_V:
		reg->host_ctrl2 |= XLNX_SDHC_HC2_1V8_EN_MASK;
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

/**
 * @brief
 * Set otap delay based on selected speed mode for SD 3.0
 */
static void xlnx_sdhc_config_sd_otap_delay(const struct device *dev,
		enum sdhc_timing_mode timing)
{
	volatile struct reg_base *reg = (struct reg_base *)DEVICE_MMIO_GET(dev);
	uint32_t def_degrees[SDHC_TIMING_HS400 + 1] = XLNX_SDHC_SD_OTAP_DEFAULT_PHASES;
	uint32_t degrees = 0, otapdly = 0;
	uint8_t tap_max = 0;

	switch (timing) {
	case SDHC_TIMING_SDR104:
	case SDHC_TIMING_HS200:
		tap_max = XLNX_SDHC_SD_200HZ_MAX_OTAP;
		break;

	case SDHC_TIMING_DDR50:
	case SDHC_TIMING_SDR25:
	case SDHC_TIMING_HS:
		tap_max = XLNX_SDHC_SD_50HZ_MAX_OTAP;
		break;

	case SDHC_TIMING_SDR50:
		tap_max = XLNX_SDHC_SD_100HZ_MAX_OTAP;
		break;

	default:
		return;
	}

	if ((timing == SDHC_TIMING_HS) && (XLNX_SDHC_SLOT_TYPE(dev) == XLNX_SDHC_EMMC_SLOT)) {
		degrees = def_degrees[XLNX_SDHC_TIMING_MMC_HS];
	} else {
		degrees = def_degrees[timing];
	}

	otapdly = (degrees * tap_max) / XLNX_SDHC_MAX_CLK_PHASE;

	/* Set the clock phase */
	reg->otap_dly = otapdly;
}

/**
 * @brief
 * Set itap delay based on selected speed mode for SD 3.0
 */
static void xlnx_sdhc_config_sd_itap_delay(const struct device *dev,
		enum sdhc_timing_mode timing)
{
	volatile struct reg_base *reg = (struct reg_base *)DEVICE_MMIO_GET(dev);
	uint32_t def_degrees[SDHC_TIMING_HS400 + 1] = XLNX_SDHC_SD_ITAP_DEFAULT_PHASES;
	uint32_t degrees = 0, itapdly = 0;
	uint8_t tap_max = 0;

	switch (timing) {
	case SDHC_TIMING_SDR104:
	case SDHC_TIMING_HS200:
		tap_max = XLNX_SDHC_SD_200HZ_MAX_ITAP;
		break;

	case SDHC_TIMING_DDR50:
	case SDHC_TIMING_SDR25:
	case SDHC_TIMING_HS:
		tap_max = XLNX_SDHC_SD_50HZ_MAX_ITAP;
		break;

	case SDHC_TIMING_SDR50:
		tap_max = XLNX_SDHC_SD_100HZ_MAX_ITAP;
		break;

	default:
		return;
	}

	if ((timing == SDHC_TIMING_HS) && (XLNX_SDHC_SLOT_TYPE(dev) == XLNX_SDHC_EMMC_SLOT)) {
		degrees = def_degrees[XLNX_SDHC_TIMING_MMC_HS];
	} else {
		degrees = def_degrees[timing];
	}

	itapdly = (degrees * tap_max) / XLNX_SDHC_MAX_CLK_PHASE;

	/* Set the clock phase */
	if (itapdly != 0U) {
		reg->itap_dly = XLNX_SDHC_ITAPCHGWIN;
		reg->itap_dly |= XLNX_SDHC_ITAPDLYENA;
		reg->itap_dly |= itapdly;
		reg->itap_dly &= ~XLNX_SDHC_ITAPCHGWIN;
	}
}

/**
 * @brief
 * Set otap delay based on selected speed mode for EMMC 5.1
 */
static void xlnx_sdhc_config_emmc_otap_delay(const struct device *dev,
		enum sdhc_timing_mode timing)
{
	volatile struct reg_base *reg = (struct reg_base *)DEVICE_MMIO_GET(dev);
	uint32_t def_degrees[SDHC_TIMING_HS400 + 1] = XLNX_SDHC_EMMC_OTAP_DEFAULT_PHASES;
	uint32_t degrees = 0, otapdly = 0;
	uint8_t tap_max = 0;

	switch (timing) {
	case SDHC_TIMING_HS400:
	case SDHC_TIMING_HS200:
		tap_max = XLNX_SDHC_EMMC_200HZ_MAX_OTAP;
		break;
	case SDHC_TIMING_HS:
		tap_max = XLNX_SDHC_EMMC_50HZ_MAX_OTAP;
		break;
	default:
		return;
	}

	if (timing == SDHC_TIMING_HS) {
		degrees = def_degrees[XLNX_SDHC_TIMING_MMC_HS];
	} else {
		degrees = def_degrees[timing];
	}

	otapdly = (degrees * tap_max) / XLNX_SDHC_MAX_CLK_PHASE;

	/* Set the clock phase */
	if (otapdly != 0U) {
		reg->phy_ctrl1 |= XLNX_SDHC_PHYREG1_OTAP_EN_MASK;
		reg->phy_ctrl1 &= ~XLNX_SDHC_PHYREG1_OTAP_DLY;
		reg->phy_ctrl1 |= otapdly << XLNX_SDHC_PHYREG1_OTAP_DLY_SHIFT;
	}
}

/**
 * @brief
 * Set itap delay based on selected speed mode for EMMC 5.1
 */
static void xlnx_sdhc_config_emmc_itap_delay(const struct device *dev,
		enum sdhc_timing_mode timing)
{
	volatile struct reg_base *reg = (struct reg_base *)DEVICE_MMIO_GET(dev);
	uint32_t def_degrees[SDHC_TIMING_HS400 + 1] = XLNX_SDHC_EMMC_ITAP_DEFAULT_PHASES;
	uint32_t degrees = 0, itapdly = 0;
	uint8_t tap_max = 0;

	/* Select max tap based on speed mode */
	switch (timing) {
	case SDHC_TIMING_HS400:
	case SDHC_TIMING_HS200:
		/* Strobe select tap point for strb90 and strb180 */
		reg->phy_ctrl1 &= ~XLNX_SDHC_PHYREG1_STROBE_SEL;
		if (timing == SDHC_TIMING_HS400) {
			reg->phy_ctrl1 |=
				(XLNX_SDHC_PHY_STRB_SEL_SIG) << XLNX_SDHC_PHYREG1_STROBE_SEL_SHIFT;
		}
		break;
	case SDHC_TIMING_HS:
		tap_max = XLNX_SDHC_EMMC_50HZ_MAX_ITAP;
		break;
	default:
		return;
	}

	/* default clock phase based on speed mode */
	if (timing == SDHC_TIMING_HS) {
		degrees = def_degrees[XLNX_SDHC_TIMING_MMC_HS];
	} else {
		degrees = def_degrees[timing];
	}

	itapdly = (degrees * tap_max) / XLNX_SDHC_MAX_CLK_PHASE;

	/* Set the clock phase */
	if (itapdly != 0U) {
		reg->phy_ctrl1 |= XLNX_SDHC_PHYREG1_ITAP_CHGWIN_MASK;
		reg->phy_ctrl1 |= XLNX_SDHC_PHYREG1_ITAP_EN_MASK;
		reg->phy_ctrl1 &= ~XLNX_SDHC_PHYREG1_ITAP_DLY;
		reg->phy_ctrl1 |= itapdly << XLNX_SDHC_PHYREG1_ITAP_DLY_SHIFT;
		reg->phy_ctrl1 &= ~XLNX_SDHC_PHYREG1_ITAP_CHGWIN_MASK;
	}
}

/**
 * @brief
 * Set speed mode and config tap delay
 */
static int8_t xlnx_sdhc_set_timing(const struct device *dev, enum sdhc_timing_mode timing)
{
	volatile struct reg_base *reg = (struct reg_base *)DEVICE_MMIO_GET(dev);
	const struct sd_data *dev_data = dev->data;
	uint16_t mode = 0;

	switch (timing) {
	case SDHC_TIMING_LEGACY:
		reg->host_ctrl1 &= ~XLNX_SDHC_HS_SPEED_MODE_EN_MASK;
		break;

	case SDHC_TIMING_SDR25:
	case SDHC_TIMING_HS:
		reg->host_ctrl1 |= XLNX_SDHC_HS_SPEED_MODE_EN_MASK;
		break;

	case SDHC_TIMING_SDR12:
		mode = XLNX_SDHC_UHS_SPEED_MODE_SDR12;
		break;

	case SDHC_TIMING_SDR50:
		mode = XLNX_SDHC_UHS_SPEED_MODE_SDR50;
		break;

	case SDHC_TIMING_HS200:
	case SDHC_TIMING_SDR104:
		mode = XLNX_SDHC_UHS_SPEED_MODE_SDR104;
		break;

	case SDHC_TIMING_DDR50:
	case SDHC_TIMING_DDR52:
		mode = XLNX_SDHC_UHS_SPEED_MODE_DDR50;
		break;

	case SDHC_TIMING_HS400:
		mode = XLNX_SDHC_UHS_SPEED_MODE_DDR200;
		break;

	default:
		return -EINVAL;
	}

	/* Select one of UHS mode */
	if (timing > SDHC_TIMING_HS) {
		reg->host_ctrl2 &= ~XLNX_SDHC_HC2_UHS_MODE;
		reg->host_ctrl2 |= mode;
	}

	/* clock phase delays are different for SD 3.0 and EMMC 5.1 */
	if (dev_data->has_phy == true) {
		xlnx_sdhc_config_emmc_otap_delay(dev, timing);
		xlnx_sdhc_config_emmc_itap_delay(dev, timing);
	} else {
		xlnx_sdhc_config_sd_otap_delay(dev, timing);
		xlnx_sdhc_config_sd_itap_delay(dev, timing);
	}

	return 0;
}

/**
 * @brief
 * Set voltage, power, clock, timing, bus width on host controller
 */
static int xlnx_sdhc_set_io(const struct device *dev, struct sdhc_io *ios)
{
	int ret;
	struct sd_data *dev_data = dev->data;
	struct sdhc_io *host_io = (struct sdhc_io *)&dev_data->host_io;
	volatile struct reg_base *reg = (struct reg_base *)DEVICE_MMIO_GET(dev);

	/* Check given clock is valid */
	if ((ios->clock != 0) && ((ios->clock > dev_data->props.f_max) ||
				(ios->clock < dev_data->props.f_min))) {
		LOG_ERR("Invalid clock value");
		return -EINVAL;
	}

	/* Set power on or off */
	if (ios->power_mode != host_io->power_mode) {
		xlnx_sdhc_set_power(dev, ios->power_mode);
		host_io->power_mode = ios->power_mode;
	}

	/* Set voltage level */
	if (ios->signal_voltage != host_io->signal_voltage) {
		ret = xlnx_sdhc_set_voltage(reg, ios->signal_voltage);
		if (ret != 0) {
			LOG_ERR("Failed to set voltage level");
			return ret;
		}
		host_io->signal_voltage = ios->signal_voltage;
	}

	/* Set speed mode */
	if (ios->timing != host_io->timing) {
		ret = xlnx_sdhc_set_timing(dev, ios->timing);
		if (ret != 0) {
			LOG_ERR("Failed to set speed mode");
			return ret;
		}
		host_io->timing = ios->timing;
	}

	/* Set clock */
	if (ios->clock != host_io->clock) {
		ret = xlnx_sdhc_set_clock(dev, ios->clock);
		if (ret != 0) {
			LOG_ERR("Failed to set clock");
			return ret;
		}
		host_io->clock = ios->clock;
	}

	/* Set bus width */
	if (ios->bus_width != host_io->bus_width) {
		ret = xlnx_sdhc_set_buswidth(reg, ios->bus_width);
		if (ret != 0) {
			LOG_ERR("Failed to set bus width");
			return ret;
		}
		host_io->bus_width = ios->bus_width;
	}

	return 0;
}

/**
 * @brief
 * Perform reset and enable status registers
 */
static int xlnx_sdhc_host_reset(const struct device *dev)
{
	const struct xlnx_sdhc_config *config = dev->config;
	volatile struct reg_base *reg = (struct reg_base *)DEVICE_MMIO_GET(dev);
	int ret;

	/* Perform software reset  */
	reg->sw_reset = XLNX_SDHC_SWRST_ALL_MASK;
	/* Wait max 100ms for software reset to complete */
	ret = xlnx_sdhc_waitb_events((void *)&reg->sw_reset, 100,
			XLNX_SDHC_SWRST_ALL_MASK, 0);
	if (ret != 0) {
		LOG_ERR("Device is busy");
		return -EBUSY;
	}

	/* Enable status reg and configure interrupt */
	reg->normal_int_stat_en = XLNX_SDHC_NORM_INTR_ALL;
	reg->err_int_stat_en = XLNX_SDHC_ERROR_INTR_ALL;
	reg->err_int_signal_en = 0;

	if (config->irq_config_func == NULL) {
		reg->normal_int_signal_en = 0;
	} else {
		/*
		 * Enable command complete, transfer complete, read buffer ready and error status
		 * interrupt
		 */
		reg->normal_int_signal_en = XLNX_SDHC_TXFR_INTR_EN_MASK;
	}

	/* Data line time out interval */
	reg->timeout_ctrl = XLNX_SDHC_DAT_LINE_TIMEOUT;

	/* Select ADMA2 */
	reg->host_ctrl1 = XLNX_SDHC_ADMA2_64;

	reg->block_size = XLNX_SDHC_BLK_SIZE_512;

	xlnx_sdhc_clear_intr(reg);

	return ret;
}

/**
 * @brief
 * Check for card busy
 */
static int xlnx_sdhc_card_busy(const struct device *dev)
{
	int8_t ret;
	volatile struct reg_base *reg = (struct reg_base *)DEVICE_MMIO_GET(dev);

	/* Wait max 2ms for card to send next command */
	ret = xlnx_sdhc_waitl_events((void *)&reg->present_state, 2,
			XLNX_SDHC_CARD_BUSY, 0);
	if (ret != 0) {
		return 0;
	}

	return 1;
}

/**
 * @brief
 * Enable tuning clock
 */
static int xlnx_sdhc_card_tuning(const struct device *dev)
{
	struct sd_data *dev_data = dev->data;
	const struct sdhc_io *io = &dev_data->host_io;
	volatile struct reg_base *reg = (struct reg_base *)DEVICE_MMIO_GET(dev);
	struct sdhc_command cmd = {0};
	uint8_t blksize;
	uint8_t count;
	int ret;

	if ((io->timing == SDHC_TIMING_HS200) || (io->timing == SDHC_TIMING_HS400)) {
		cmd.opcode = MMC_SEND_TUNING_BLOCK;
	} else {
		cmd.opcode = SD_SEND_TUNING_BLOCK;
	}

	cmd.response_type = SD_RSP_TYPE_R1;
	cmd.timeout_ms = CONFIG_SD_CMD_TIMEOUT;

	blksize = XLNX_SDHC_TUNING_CMD_BLKSIZE;
	if (io->bus_width == SDHC_BUS_WIDTH8BIT) {
		blksize = XLNX_SDHC_TUNING_CMD_BLKSIZE * 2;
	}

	dev_data->transfermode = XLNX_SDHC_TM_DAT_DIR_SEL_MASK;
	reg->block_size = blksize;
	reg->block_count = XLNX_SDHC_TUNING_CMD_BLKCOUNT;

	/* Execute tuning */
	reg->host_ctrl2 |= XLNX_SDHC_HC2_EXEC_TNG_MASK;

	for (count = 0; count < XLNX_SDHC_MAX_TUNING_COUNT; count++) {
		ret = xlnx_sdhc_cmd(dev, &cmd, true);
		if (ret != 0) {
			return ret;
		}
		if ((reg->host_ctrl2 & XLNX_SDHC_HC2_EXEC_TNG_MASK) == 0U) {
			break;
		}
	}

	/* Check tuning completed successfully */
	if ((reg->host_ctrl2 & XLNX_SDHC_HC2_SAMP_CLK_SEL_MASK) == 0U) {
		return -EINVAL;
	}

	dev_data->transfermode = 0;

	return 0;
}

/**
 * @brief
 * Perform early system init for SDHC
 */
static int xlnx_sdhc_init(const struct device *dev)
{
	const struct xlnx_sdhc_config *config = dev->config;
	struct sd_data *dev_data = dev->data;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	if (device_is_ready(config->clock_dev) == 0) {
		LOG_ERR("Clock control device not ready");
		return -ENODEV;
	}

	if (config->irq_config_func != NULL) {
		k_event_init(&dev_data->irq_event);
		config->irq_config_func(dev);
	}

	return xlnx_sdhc_host_reset(dev);
}

static const struct sdhc_driver_api xlnx_sdhc_api = {
	.reset = xlnx_sdhc_host_reset,
	.request = xlnx_sdhc_request,
	.set_io = xlnx_sdhc_set_io,
	.get_card_present = xlnx_sdhc_card_detect,
	.execute_tuning = xlnx_sdhc_card_tuning,
	.card_busy = xlnx_sdhc_card_busy,
	.get_host_props = xlnx_sdhc_host_props,
};

#define XLNX_SDHC_INTR_CONFIG(n)                                                                  \
	static void xlnx_sdhc_irq_handler##n(const struct device *dev)                            \
	{                                                                                         \
		volatile struct reg_base *reg = (struct reg_base *)DEVICE_MMIO_GET(dev);          \
		struct sd_data *dev_data = dev->data;                                             \
		if ((reg->normal_int_stat & XLNX_SDHC_INTR_CC_MASK) != 0U) {                      \
			reg->normal_int_stat = XLNX_SDHC_INTR_CC_MASK;                            \
			k_event_post(&dev_data->irq_event, XLNX_SDHC_INTR_CC_MASK);               \
		}                                                                                 \
		if ((reg->normal_int_stat & XLNX_SDHC_INTR_BRR_MASK) != 0U) {                     \
			reg->normal_int_stat = XLNX_SDHC_INTR_BRR_MASK;                           \
			k_event_post(&dev_data->irq_event, XLNX_SDHC_INTR_BRR_MASK);              \
		}                                                                                 \
		if ((reg->normal_int_stat & XLNX_SDHC_INTR_TC_MASK) != 0U) {                      \
			reg->normal_int_stat = XLNX_SDHC_INTR_TC_MASK;                            \
			k_event_post(&dev_data->irq_event, XLNX_SDHC_INTR_TC_MASK);               \
		}                                                                                 \
		if ((reg->normal_int_stat & XLNX_SDHC_INTR_ERR_MASK) != 0U) {                     \
			reg->normal_int_stat = XLNX_SDHC_INTR_ERR_MASK;                           \
			reg->err_int_stat = XLNX_SDHC_ERROR_INTR_ALL;                             \
			k_event_post(&dev_data->irq_event, XLNX_SDHC_INTR_ERR_MASK);              \
		}                                                                                 \
	}                                                                                         \
	static void xlnx_sdhc_config_intr##n(const struct device *dev)                            \
	{                                                                                         \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),                            \
				xlnx_sdhc_irq_handler##n, DEVICE_DT_INST_GET(n),                  \
				DT_INST_IRQ(n, flags));                                           \
		irq_enable(DT_INST_IRQN(n));                                                      \
	}
#define XLNX_SDHC_INTR_FUNC_REG(n) .irq_config_func = xlnx_sdhc_config_intr##n,

#define XLNX_SDHC_INTR_CONFIG_NULL
#define XLNX_SDHC_INTR_FUNC_REG_NULL .irq_config_func = NULL,

#define XLNX_SDHC_INTR_CONFIG_API(n) COND_CODE_1(DT_INST_NODE_HAS_PROP(n, interrupts),            \
		(XLNX_SDHC_INTR_CONFIG(n)), (XLNX_SDHC_INTR_CONFIG_NULL))

#define XLNX_SDHC_INTR_FUNC_REG_API(n) COND_CODE_1(DT_INST_NODE_HAS_PROP(n, interrupts),          \
		(XLNX_SDHC_INTR_FUNC_REG(n)), (XLNX_SDHC_INTR_FUNC_REG_NULL))

#define XLNX_SDHC_INIT(n)                                                                         \
	XLNX_SDHC_INTR_CONFIG_API(n)                                                              \
	const static struct xlnx_sdhc_config xlnx_sdhc_inst_##n = {                               \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),                                             \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                               \
		XLNX_SDHC_INTR_FUNC_REG_API(n)                                                    \
		.broken_cd = DT_INST_PROP_OR(n, broken_cd, 0),                                    \
		.powerdelay = DT_INST_PROP_OR(n, power_delay_ms, 0),                              \
		.hs200_mode = DT_INST_PROP_OR(n, mmc_hs200_1_8v, 0),                              \
		.hs400_mode = DT_INST_PROP_OR(n, mmc_hs400_1_8v, 0),                              \
	};                                                                                        \
	static struct sd_data data##n;                                                            \
	                                                                                          \
	DEVICE_DT_INST_DEFINE(n, xlnx_sdhc_init, NULL, &data##n,                                  \
			&xlnx_sdhc_inst_##n, POST_KERNEL,                                         \
			CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &xlnx_sdhc_api);

DT_INST_FOREACH_STATUS_OKAY(XLNX_SDHC_INIT)
