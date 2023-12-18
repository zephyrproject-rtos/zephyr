/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT intel_emmc_host

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sdhc.h>
#include <zephyr/sd/sd_spec.h>
#include <zephyr/cache.h>
#include "intel_emmc_host.h"
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(pcie)
BUILD_ASSERT(IS_ENABLED(CONFIG_PCIE), "DT need CONFIG_PCIE");
#include <zephyr/drivers/pcie/pcie.h>
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(emmc_hc, CONFIG_SDHC_LOG_LEVEL);

typedef void (*emmc_isr_cb_t)(const struct device *dev);

#ifdef CONFIG_INTEL_EMMC_HOST_ADMA_DESC_SIZE
#define ADMA_DESC_SIZE CONFIG_INTEL_EMMC_HOST_ADMA_DESC_SIZE
#else
#define ADMA_DESC_SIZE 0
#endif

struct emmc_config {
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(pcie)
	struct pcie_dev *pcie;
#else
	DEVICE_MMIO_ROM;
#endif
	emmc_isr_cb_t config_func;
	uint32_t max_bus_freq;
	uint32_t min_bus_freq;
	uint32_t power_delay_ms;
	uint8_t hs200_mode: 1;
	uint8_t hs400_mode: 1;
	uint8_t dw_4bit: 1;
	uint8_t dw_8bit: 1;
};

struct emmc_data {
	DEVICE_MMIO_RAM;
	uint32_t rca;
	struct sdhc_io host_io;
	struct k_sem lock;
	struct k_event irq_event;
	uint64_t desc_table[ADMA_DESC_SIZE];
	struct sdhc_host_props props;
	bool card_present;
};

static void enable_interrupts(const struct device *dev)
{
	volatile struct emmc_reg *regs = (struct emmc_reg *)DEVICE_MMIO_GET(dev);

	regs->normal_int_stat_en = EMMC_HOST_NORMAL_INTR_MASK;
	regs->err_int_stat_en = EMMC_HOST_ERROR_INTR_MASK;
	regs->normal_int_signal_en = EMMC_HOST_NORMAL_INTR_MASK;
	regs->err_int_signal_en = EMMC_HOST_ERROR_INTR_MASK;
	regs->timeout_ctrl = EMMC_HOST_MAX_TIMEOUT;
}

static void disable_interrupts(const struct device *dev)
{
	volatile struct emmc_reg *regs = (struct emmc_reg *)DEVICE_MMIO_GET(dev);

	/* Keep enable interrupt status register to update */
	regs->normal_int_stat_en = EMMC_HOST_NORMAL_INTR_MASK;
	regs->err_int_stat_en = EMMC_HOST_ERROR_INTR_MASK;

	/* Disable only interrupt generation */
	regs->normal_int_signal_en &= 0;
	regs->err_int_signal_en &= 0;
	regs->timeout_ctrl = EMMC_HOST_MAX_TIMEOUT;
}

static void clear_interrupts(const struct device *dev)
{
	volatile struct emmc_reg *regs = (struct emmc_reg *)DEVICE_MMIO_GET(dev);

	regs->normal_int_stat = EMMC_HOST_NORMAL_INTR_MASK_CLR;
	regs->err_int_stat = EMMC_HOST_ERROR_INTR_MASK;
}

static int emmc_set_voltage(const struct device *dev, enum sd_voltage signal_voltage)
{
	volatile struct emmc_reg *regs = (struct emmc_reg *)DEVICE_MMIO_GET(dev);
	bool power_state = regs->power_ctrl & EMMC_HOST_POWER_CTRL_SD_BUS_POWER ? true : false;
	int ret = 0;

	if (power_state) {
		/* Turn OFF Bus Power before config clock */
		regs->power_ctrl &= ~EMMC_HOST_POWER_CTRL_SD_BUS_POWER;
	}

	switch (signal_voltage) {
	case SD_VOL_3_3_V:
		if (regs->capabilities & EMMC_HOST_VOL_3_3_V_SUPPORT) {
			regs->host_ctrl2 &=
				~(EMMC_HOST_CTRL2_1P8V_SIG_EN << EMMC_HOST_CTRL2_1P8V_SIG_LOC);

			/* 3.3v voltage select */
			regs->power_ctrl = EMMC_HOST_VOL_3_3_V_SELECT;
			LOG_DBG("3.3V Selected for MMC Card");
		} else {
			LOG_ERR("3.3V not supported by MMC Host");
			ret = -ENOTSUP;
		}
		break;

	case SD_VOL_3_0_V:
		if (regs->capabilities & EMMC_HOST_VOL_3_0_V_SUPPORT) {
			regs->host_ctrl2 &=
				~(EMMC_HOST_CTRL2_1P8V_SIG_EN << EMMC_HOST_CTRL2_1P8V_SIG_LOC);

			/* 3.0v voltage select */
			regs->power_ctrl = EMMC_HOST_VOL_3_0_V_SELECT;
			LOG_DBG("3.0V Selected for MMC Card");
		} else {
			LOG_ERR("3.0V not supported by MMC Host");
			ret = -ENOTSUP;
		}
		break;

	case SD_VOL_1_8_V:
		if (regs->capabilities & EMMC_HOST_VOL_1_8_V_SUPPORT) {
			regs->host_ctrl2 |= EMMC_HOST_CTRL2_1P8V_SIG_EN
					    << EMMC_HOST_CTRL2_1P8V_SIG_LOC;

			/* 1.8v voltage select */
			regs->power_ctrl = EMMC_HOST_VOL_1_8_V_SELECT;
			LOG_DBG("1.8V Selected for MMC Card");
		} else {
			LOG_ERR("1.8V not supported by MMC Host");
			ret = -ENOTSUP;
		}
		break;

	default:
		ret = -EINVAL;
	}

	if (power_state) {
		/* Turn ON Bus Power */
		regs->power_ctrl |= EMMC_HOST_POWER_CTRL_SD_BUS_POWER;
	}

	return ret;
}

static int emmc_set_power(const struct device *dev, enum sdhc_power state)
{
	volatile struct emmc_reg *regs = (struct emmc_reg *)DEVICE_MMIO_GET(dev);

	if (state == SDHC_POWER_ON) {
		/* Turn ON Bus Power */
		regs->power_ctrl |= EMMC_HOST_POWER_CTRL_SD_BUS_POWER;
	} else {
		/* Turn OFF Bus Power */
		regs->power_ctrl &= ~EMMC_HOST_POWER_CTRL_SD_BUS_POWER;
	}

	k_msleep(10u);

	return 0;
}

static bool emmc_disable_clock(const struct device *dev)
{
	volatile struct emmc_reg *regs = (struct emmc_reg *)DEVICE_MMIO_GET(dev);

	if (regs->present_state & EMMC_HOST_PSTATE_CMD_INHIBIT) {
		LOG_ERR("present_state:%x", regs->present_state);
		return false;
	}
	if (regs->present_state & EMMC_HOST_PSTATE_DAT_INHIBIT) {
		LOG_ERR("present_state:%x", regs->present_state);
		return false;
	}

	regs->clock_ctrl &= ~EMMC_HOST_INTERNAL_CLOCK_EN;
	regs->clock_ctrl &= ~EMMC_HOST_SD_CLOCK_EN;

	while ((regs->clock_ctrl & EMMC_HOST_SD_CLOCK_EN) != 0) {
		;
	}

	return true;
}

static bool emmc_enable_clock(const struct device *dev)
{
	volatile struct emmc_reg *regs = (struct emmc_reg *)DEVICE_MMIO_GET(dev);

	regs->clock_ctrl |= EMMC_HOST_INTERNAL_CLOCK_EN;
	/* Wait for the stable Internal Clock */
	while ((regs->clock_ctrl & EMMC_HOST_INTERNAL_CLOCK_STABLE) == 0) {
		;
	}

	/* Enable SD Clock */
	regs->clock_ctrl |= EMMC_HOST_SD_CLOCK_EN;
	while ((regs->clock_ctrl & EMMC_HOST_SD_CLOCK_EN) == 0) {
		;
	}

	return true;
}

static bool emmc_clock_set(const struct device *dev, enum sdhc_clock_speed speed)
{
	volatile struct emmc_reg *regs = (struct emmc_reg *)DEVICE_MMIO_GET(dev);
	uint8_t base_freq;
	uint32_t clock_divider;
	float freq;
	bool ret;

	switch (speed) {
	case SDMMC_CLOCK_400KHZ:
		freq = EMMC_HOST_CLK_FREQ_400K;
		break;

	case SD_CLOCK_25MHZ:
	case MMC_CLOCK_26MHZ:
		freq = EMMC_HOST_CLK_FREQ_25M;
		break;

	case SD_CLOCK_50MHZ:
	case MMC_CLOCK_52MHZ:
		freq = EMMC_HOST_CLK_FREQ_50M;
		break;

	case SD_CLOCK_100MHZ:
		freq = EMMC_HOST_CLK_FREQ_100M;
		break;

	case MMC_CLOCK_HS200:
		freq = EMMC_HOST_CLK_FREQ_200M;
		break;

	case SD_CLOCK_208MHZ:
	default:
		return false;
	}

	ret = emmc_disable_clock(dev);
	if (!ret) {
		return false;
	}

	base_freq = regs->capabilities >> 8;
	clock_divider = (int)(base_freq / (freq * 2));

	LOG_DBG("Clock divider for MMC Clk: %d Hz is %d", speed, clock_divider);

	SET_BITS(regs->clock_ctrl, EMMC_HOST_CLK_SDCLCK_FREQ_SEL_LOC,
		 EMMC_HOST_CLK_SDCLCK_FREQ_SEL_MASK, clock_divider);
	SET_BITS(regs->clock_ctrl, EMMC_HOST_CLK_SDCLCK_FREQ_SEL_UPPER_LOC,
		 EMMC_HOST_CLK_SDCLCK_FREQ_SEL_UPPER_MASK, clock_divider >> 8);

	emmc_enable_clock(dev);

	return true;
}

static int set_timing(const struct device *dev, enum sdhc_timing_mode timing)
{
	volatile struct emmc_reg *regs = (struct emmc_reg *)DEVICE_MMIO_GET(dev);
	int ret = 0;
	uint8_t mode;

	LOG_DBG("UHS Mode: %d", timing);

	switch (timing) {
	case SDHC_TIMING_LEGACY:
	case SDHC_TIMING_HS:
	case SDHC_TIMING_SDR12:
		mode = EMMC_HOST_UHSMODE_SDR12;
		break;

	case SDHC_TIMING_SDR25:
		mode = EMMC_HOST_UHSMODE_SDR25;
		break;

	case SDHC_TIMING_SDR50:
		mode = EMMC_HOST_UHSMODE_SDR50;
		break;

	case SDHC_TIMING_SDR104:
		mode = EMMC_HOST_UHSMODE_SDR104;
		break;

	case SDHC_TIMING_DDR50:
	case SDHC_TIMING_DDR52:
		mode = EMMC_HOST_UHSMODE_DDR50;
		break;

	case SDHC_TIMING_HS400:
	case SDHC_TIMING_HS200:
		mode = EMMC_HOST_UHSMODE_HS400;
		break;

	default:
		ret = -ENOTSUP;
	}

	if (!ret) {
		if (!emmc_disable_clock(dev)) {
			LOG_ERR("Disable clk failed");
			return -EIO;
		}
		regs->host_ctrl2 |= EMMC_HOST_CTRL2_1P8V_SIG_EN << EMMC_HOST_CTRL2_1P8V_SIG_LOC;
		SET_BITS(regs->host_ctrl2, EMMC_HOST_CTRL2_UHS_MODE_SEL_LOC,
			 EMMC_HOST_CTRL2_UHS_MODE_SEL_MASK, mode);

		emmc_enable_clock(dev);
	}

	return ret;
}

static int wait_for_cmd_complete(struct emmc_data *emmc, uint32_t time_out)
{
	int ret;
	k_timeout_t wait_time;
	uint32_t events;

	if (time_out == SDHC_TIMEOUT_FOREVER) {
		wait_time = K_FOREVER;
	} else {
		wait_time = K_MSEC(time_out);
	}

	events = k_event_wait(&emmc->irq_event,
			      EMMC_HOST_CMD_COMPLETE | ERR_INTR_STATUS_EVENT(EMMC_HOST_ERR_STATUS),
			      false, wait_time);

	if (events & EMMC_HOST_CMD_COMPLETE) {
		ret = 0;
	} else if (events & ERR_INTR_STATUS_EVENT(EMMC_HOST_ERR_STATUS)) {
		LOG_ERR("wait for cmd complete error: %x", events);
		ret = -EIO;
	} else {
		LOG_ERR("wait for cmd complete timeout");
		ret = -EAGAIN;
	}

	return ret;
}

static int poll_cmd_complete(const struct device *dev, uint32_t time_out)
{
	volatile struct emmc_reg *regs = (struct emmc_reg *)DEVICE_MMIO_GET(dev);
	int ret = -EAGAIN;
	int32_t retry = time_out;

	while (retry > 0) {
		if (regs->normal_int_stat & EMMC_HOST_CMD_COMPLETE) {
			regs->normal_int_stat = EMMC_HOST_CMD_COMPLETE;
			ret = 0;
			break;
		}

		k_busy_wait(1000u);
		retry--;
	}

	if (regs->err_int_stat) {
		LOG_ERR("err_int_stat:%x", regs->err_int_stat);
		regs->err_int_stat &= regs->err_int_stat;
		ret = -EIO;
	}

	if (IS_ENABLED(CONFIG_INTEL_EMMC_HOST_ADMA)) {
		if (regs->adma_err_stat) {
			LOG_ERR("adma error: %x", regs->adma_err_stat);
			ret = -EIO;
		}
	}
	return ret;
}

void emmc_host_sw_reset(const struct device *dev, enum emmc_sw_reset reset)
{
	volatile struct emmc_reg *regs = (struct emmc_reg *)DEVICE_MMIO_GET(dev);

	if (reset == EMMC_HOST_SW_RESET_DATA_LINE) {
		regs->sw_reset = EMMC_HOST_SW_RESET_REG_DATA;
	} else if (reset == EMMC_HOST_SW_RESET_CMD_LINE) {
		regs->sw_reset = EMMC_HOST_SW_RESET_REG_CMD;
	} else if (reset == EMMC_HOST_SW_RESET_ALL) {
		regs->sw_reset = EMMC_HOST_SW_RESET_REG_ALL;
	}

	while (regs->sw_reset != 0) {
		;
	}

	k_sleep(K_MSEC(100u));
}

static int emmc_dma_init(const struct device *dev, struct sdhc_data *data, bool read)
{
	struct emmc_data *emmc = dev->data;
	volatile struct emmc_reg *regs = (struct emmc_reg *)DEVICE_MMIO_GET(dev);

	if (IS_ENABLED(CONFIG_DCACHE) && !read) {
		sys_cache_data_flush_range(data->data, (data->blocks * data->block_size));
	}

	if (IS_ENABLED(CONFIG_INTEL_EMMC_HOST_ADMA)) {
		uint8_t *buff = data->data;

		/* Setup DMA trasnfer using ADMA2 */
		memset(emmc->desc_table, 0, sizeof(emmc->desc_table));

#if defined(CONFIG_INTEL_EMMC_HOST_ADMA_DESC_SIZE)
		__ASSERT_NO_MSG(data->blocks < CONFIG_INTEL_EMMC_HOST_ADMA_DESC_SIZE);
#endif
		for (int i = 0; i < data->blocks; i++) {
			emmc->desc_table[i] = ((uint64_t)buff) << EMMC_HOST_ADMA_BUFF_ADD_LOC;
			emmc->desc_table[i] |= data->block_size << EMMC_HOST_ADMA_BUFF_LEN_LOC;

			if (i == (data->blocks - 1u)) {
				emmc->desc_table[i] |= EMMC_HOST_ADMA_BUFF_LINK_LAST;
				emmc->desc_table[i] |= EMMC_HOST_ADMA_INTR_EN;
				emmc->desc_table[i] |= EMMC_HOST_ADMA_BUFF_LAST;
			} else {
				emmc->desc_table[i] |= EMMC_HOST_ADMA_BUFF_LINK_NEXT;
			}
			emmc->desc_table[i] |= EMMC_HOST_ADMA_BUFF_VALID;
			buff += data->block_size;
			LOG_DBG("desc_table:%llx", emmc->desc_table[i]);
		}

		regs->adma_sys_addr1 = (uint32_t)((uintptr_t)emmc->desc_table & ADDRESS_32BIT_MASK);
		regs->adma_sys_addr2 =
			(uint32_t)(((uintptr_t)emmc->desc_table >> 32) & ADDRESS_32BIT_MASK);

		LOG_DBG("adma: %llx %x %p", emmc->desc_table[0], regs->adma_sys_addr1,
			emmc->desc_table);
	} else {
		/* Setup DMA trasnfer using SDMA */
		regs->sdma_sysaddr = (uint32_t)((uintptr_t)data->data);
		LOG_DBG("sdma_sysaddr: %x", regs->sdma_sysaddr);
	}
	return 0;
}

static int emmc_init_xfr(const struct device *dev, struct sdhc_data *data, bool read)
{
	struct emmc_data *emmc = dev->data;
	volatile struct emmc_reg *regs = (struct emmc_reg *)DEVICE_MMIO_GET(dev);
	uint16_t multi_block = 0u;

	if (IS_ENABLED(CONFIG_INTEL_EMMC_HOST_DMA)) {
		emmc_dma_init(dev, data, read);
	}

	if (IS_ENABLED(CONFIG_INTEL_EMMC_HOST_ADMA)) {
		SET_BITS(regs->host_ctrl1, EMMC_HOST_CTRL1_DMA_SEL_LOC,
			 EMMC_HOST_CTRL1_DMA_SEL_MASK, 2u);
	} else {
		SET_BITS(regs->host_ctrl1, EMMC_HOST_CTRL1_DMA_SEL_LOC,
			 EMMC_HOST_CTRL1_DMA_SEL_MASK, 0u);
	}

	/* Set Block Size Register */
	SET_BITS(regs->block_size, EMMC_HOST_DMA_BUF_SIZE_LOC, EMMC_HOST_DMA_BUF_SIZE_MASK,
		 EMMC_HOST_SDMA_BOUNDARY);
	SET_BITS(regs->block_size, EMMC_HOST_BLOCK_SIZE_LOC, EMMC_HOST_BLOCK_SIZE_MASK,
		 data->block_size);
	if (data->blocks > 1) {
		multi_block = 1u;
	}
	if (IS_ENABLED(CONFIG_INTEL_EMMC_HOST_AUTO_STOP)) {
		if (IS_ENABLED(CONFIG_INTEL_EMMC_HOST_ADMA) &&
		    emmc->host_io.timing == SDHC_TIMING_SDR104) {
			/* Auto cmd23 only applicable for ADMA */
			SET_BITS(regs->transfer_mode, EMMC_HOST_XFER_AUTO_CMD_EN_LOC,
				 EMMC_HOST_XFER_AUTO_CMD_EN_MASK, multi_block ? 2 : 0);
		} else {
			SET_BITS(regs->transfer_mode, EMMC_HOST_XFER_AUTO_CMD_EN_LOC,
				 EMMC_HOST_XFER_AUTO_CMD_EN_MASK, multi_block ? 1 : 0);
		}
	} else {
		SET_BITS(regs->transfer_mode, EMMC_HOST_XFER_AUTO_CMD_EN_LOC,
			 EMMC_HOST_XFER_AUTO_CMD_EN_MASK, 0);
	}

	if (!IS_ENABLED(CONFIG_INTEL_EMMC_HOST_AUTO_STOP)) {
		/* Set block count regitser to 0 for infinite transfer mode */
		regs->block_count = 0;
		SET_BITS(regs->transfer_mode, EMMC_HOST_XFER_BLOCK_CNT_EN_LOC,
			 EMMC_HOST_XFER_BLOCK_CNT_EN_MASK, 0);
	} else {
		regs->block_count = (uint16_t)data->blocks;
		/* Enable block count in transfer register */
		SET_BITS(regs->transfer_mode, EMMC_HOST_XFER_BLOCK_CNT_EN_LOC,
			 EMMC_HOST_XFER_BLOCK_CNT_EN_MASK, multi_block ? 1 : 0);
	}

	SET_BITS(regs->transfer_mode, EMMC_HOST_XFER_MULTI_BLOCK_SEL_LOC,
		 EMMC_HOST_XFER_MULTI_BLOCK_SEL_MASK, multi_block);

	/* Set data transfer direction, Read = 1, Write = 0 */
	SET_BITS(regs->transfer_mode, EMMC_HOST_XFER_DATA_DIR_LOC, EMMC_HOST_XFER_DATA_DIR_MASK,
		 read ? 1u : 0u);

	if (IS_ENABLED(CONFIG_INTEL_EMMC_HOST_DMA)) {
		/* Enable DMA or not */
		SET_BITS(regs->transfer_mode, EMMC_HOST_XFER_DMA_EN_LOC, EMMC_HOST_XFER_DMA_EN_MASK,
			 1u);
	} else {
		SET_BITS(regs->transfer_mode, EMMC_HOST_XFER_DMA_EN_LOC, EMMC_HOST_XFER_DMA_EN_MASK,
			 0u);
	}

	if (IS_ENABLED(CONFIG_INTEL_EMMC_HOST_BLOCK_GAP)) {
		/* Set an interrupt at the block gap */
		SET_BITS(regs->block_gap_ctrl, EMMC_HOST_BLOCK_GAP_LOC, EMMC_HOST_BLOCK_GAP_MASK,
			 1u);
	} else {
		SET_BITS(regs->block_gap_ctrl, EMMC_HOST_BLOCK_GAP_LOC, EMMC_HOST_BLOCK_GAP_MASK,
			 0u);
	}

	/* Set data timeout time */
	regs->timeout_ctrl = data->timeout_ms;

	return 0;
}

static int wait_xfr_intr_complete(const struct device *dev, uint32_t time_out)
{
	struct emmc_data *emmc = dev->data;
	uint32_t events;
	int ret;
	k_timeout_t wait_time;

	LOG_DBG("");

	if (time_out == SDHC_TIMEOUT_FOREVER) {
		wait_time = K_FOREVER;
	} else {
		wait_time = K_MSEC(time_out);
	}

	events = k_event_wait(&emmc->irq_event,
			      EMMC_HOST_XFER_COMPLETE |
				      ERR_INTR_STATUS_EVENT(EMMC_HOST_DMA_TXFR_ERR),
			      false, wait_time);

	if (events & EMMC_HOST_XFER_COMPLETE) {
		ret = 0;
	} else if (events & ERR_INTR_STATUS_EVENT(0xFFFF)) {
		LOG_ERR("wait for xfer complete error: %x", events);
		ret = -EIO;
	} else {
		LOG_ERR("wait for xfer complete timeout");
		ret = -EAGAIN;
	}

	return ret;
}

static int wait_xfr_poll_complete(const struct device *dev, uint32_t time_out)
{
	volatile struct emmc_reg *regs = (struct emmc_reg *)DEVICE_MMIO_GET(dev);
	int ret = -EAGAIN;
	int32_t retry = time_out;

	LOG_DBG("");

	while (retry > 0) {
		if (regs->normal_int_stat & EMMC_HOST_XFER_COMPLETE) {
			regs->normal_int_stat |= EMMC_HOST_XFER_COMPLETE;
			ret = 0;
			break;
		}

		k_busy_wait(EMMC_HOST_MSEC_DELAY);
		retry--;
	}

	return ret;
}

static int wait_xfr_complete(const struct device *dev, uint32_t time_out)
{
	int ret;

	if (IS_ENABLED(CONFIG_INTEL_EMMC_HOST_INTR)) {
		ret = wait_xfr_intr_complete(dev, time_out);
	} else {
		ret = wait_xfr_poll_complete(dev, time_out);
	}
	return ret;
}

static enum emmc_response_type emmc_decode_resp_type(enum sd_rsp_type type)
{
	enum emmc_response_type resp_type;

	switch (type & 0xF) {
	case SD_RSP_TYPE_NONE:
		resp_type = EMMC_HOST_RESP_NONE;
		break;
	case SD_RSP_TYPE_R1:
	case SD_RSP_TYPE_R3:
	case SD_RSP_TYPE_R4:
	case SD_RSP_TYPE_R5:
		resp_type = EMMC_HOST_RESP_LEN_48;
		break;
	case SD_RSP_TYPE_R1b:
		resp_type = EMMC_HOST_RESP_LEN_48B;
		break;
	case SD_RSP_TYPE_R2:
		resp_type = EMMC_HOST_RESP_LEN_136;
		break;

	case SD_RSP_TYPE_R5b:
	case SD_RSP_TYPE_R6:
	case SD_RSP_TYPE_R7:
	default:
		resp_type = EMMC_HOST_INVAL_HOST_RESP_LEN;
	}

	return resp_type;
}

static void update_cmd_response(const struct device *dev, struct sdhc_command *sdhc_cmd)
{
	volatile struct emmc_reg *regs = (struct emmc_reg *)DEVICE_MMIO_GET(dev);
	uint32_t resp0, resp1, resp2, resp3;

	if (sdhc_cmd->response_type == SD_RSP_TYPE_NONE) {
		return;
	}

	resp0 = regs->resp_01;

	if (sdhc_cmd->response_type == SD_RSP_TYPE_R2) {
		resp1 = regs->resp_2 | (regs->resp_3 << 16u);
		resp2 = regs->resp_4 | (regs->resp_5 << 16u);
		resp3 = regs->resp_6 | (regs->resp_7 << 16u);

		LOG_DBG("cmd resp: %x %x %x %x", resp0, resp1, resp2, resp3);

		sdhc_cmd->response[0u] = resp3;
		sdhc_cmd->response[1U] = resp2;
		sdhc_cmd->response[2U] = resp1;
		sdhc_cmd->response[3U] = resp0;
	} else {
		LOG_DBG("cmd resp: %x", resp0);
		sdhc_cmd->response[0u] = resp0;
	}
}

static int emmc_host_send_cmd(const struct device *dev, const struct emmc_cmd_config *config)
{
	volatile struct emmc_reg *regs = (struct emmc_reg *)DEVICE_MMIO_GET(dev);
	struct emmc_data *emmc = dev->data;
	struct sdhc_command *sdhc_cmd = config->sdhc_cmd;
	enum emmc_response_type resp_type = emmc_decode_resp_type(sdhc_cmd->response_type);
	uint16_t cmd_reg;
	int ret;

	LOG_DBG("");

	/* Check if CMD line is available */
	if (regs->present_state & EMMC_HOST_PSTATE_CMD_INHIBIT) {
		LOG_ERR("CMD line is not available");
		return -EBUSY;
	}

	if (config->data_present && (regs->present_state & EMMC_HOST_PSTATE_DAT_INHIBIT)) {
		LOG_ERR("Data line is not available");
		return -EBUSY;
	}

	if (resp_type == EMMC_HOST_INVAL_HOST_RESP_LEN) {
		LOG_ERR("Invalid eMMC resp type:%d", resp_type);
		return -EINVAL;
	}

	k_event_clear(&emmc->irq_event, EMMC_HOST_CMD_COMPLETE);

	regs->argument = sdhc_cmd->arg;

	cmd_reg = config->cmd_idx << EMMC_HOST_CMD_INDEX_LOC |
		  config->cmd_type << EMMC_HOST_CMD_TYPE_LOC |
		  config->data_present << EMMC_HOST_CMD_DATA_PRESENT_LOC |
		  config->idx_check_en << EMMC_HOST_CMD_IDX_CHECK_EN_LOC |
		  config->crc_check_en << EMMC_HOST_CMD_CRC_CHECK_EN_LOC |
		  resp_type << EMMC_HOST_CMD_RESP_TYPE_LOC;
	regs->cmd = cmd_reg;

	LOG_DBG("CMD REG:%x %x", cmd_reg, regs->cmd);
	if (IS_ENABLED(CONFIG_INTEL_EMMC_HOST_INTR)) {
		ret = wait_for_cmd_complete(emmc, sdhc_cmd->timeout_ms);
	} else {
		ret = poll_cmd_complete(dev, sdhc_cmd->timeout_ms);
	}
	if (ret) {
		LOG_ERR("Error on send cmd: %d, status:%d", config->cmd_idx, ret);
		return ret;
	}

	update_cmd_response(dev, sdhc_cmd);

	return 0;
}

static int emmc_stop_transfer(const struct device *dev)
{
	struct emmc_data *emmc = dev->data;
	struct sdhc_command hdc_cmd = {0};
	struct emmc_cmd_config cmd;

	hdc_cmd.arg = emmc->rca << EMMC_HOST_RCA_SHIFT;
	hdc_cmd.response_type = SD_RSP_TYPE_R1;
	hdc_cmd.timeout_ms = 1000;

	cmd.sdhc_cmd = &hdc_cmd;
	cmd.cmd_idx = SD_STOP_TRANSMISSION;
	cmd.cmd_type = EMMC_HOST_CMD_NORMAL;
	cmd.data_present = false;
	cmd.idx_check_en = false;
	cmd.crc_check_en = false;

	return emmc_host_send_cmd(dev, &cmd);
}

static int emmc_reset(const struct device *dev)
{
	volatile struct emmc_reg *regs = (struct emmc_reg *)DEVICE_MMIO_GET(dev);

	LOG_DBG("");

	if (!(regs->present_state & EMMC_HOST_PSTATE_CARD_INSERTED)) {
		LOG_ERR("No EMMC card found");
		return -ENODEV;
	}

	/* Reset device to idle state */
	emmc_host_sw_reset(dev, EMMC_HOST_SW_RESET_ALL);

	clear_interrupts(dev);

	if (IS_ENABLED(CONFIG_INTEL_EMMC_HOST_INTR)) {
		enable_interrupts(dev);
	} else {
		disable_interrupts(dev);
	}

	return 0;
}

static int read_data_port(const struct device *dev, struct sdhc_data *sdhc)
{
	struct emmc_data *emmc = dev->data;
	volatile struct emmc_reg *regs = (struct emmc_reg *)DEVICE_MMIO_GET(dev);
	uint32_t block_size = sdhc->block_size;
	uint32_t i, block_cnt = sdhc->blocks;
	uint32_t *data = (uint32_t *)sdhc->data;
	k_timeout_t wait_time;

	if (sdhc->timeout_ms == SDHC_TIMEOUT_FOREVER) {
		wait_time = K_FOREVER;
	} else {
		wait_time = K_MSEC(sdhc->timeout_ms);
	}

	LOG_DBG("");

	while (block_cnt--) {
		if (IS_ENABLED(CONFIG_INTEL_EMMC_HOST_INTR)) {
			uint32_t events;

			events = k_event_wait(&emmc->irq_event, EMMC_HOST_BUF_RD_READY, false,
					      wait_time);
			k_event_clear(&emmc->irq_event, EMMC_HOST_BUF_RD_READY);
			if (!(events & EMMC_HOST_BUF_RD_READY)) {
				LOG_ERR("time out on EMMC_HOST_BUF_RD_READY:%d",
					(sdhc->blocks - block_cnt));
				return -EIO;
			}
		} else {
			while ((regs->present_state & EMMC_HOST_PSTATE_BUF_READ_EN) == 0) {
				;
			}
		}

		if (regs->present_state & EMMC_HOST_PSTATE_DAT_INHIBIT) {
			for (i = block_size >> 2u; i != 0u; i--) {
				*data = regs->data_port;
				data++;
			}
		}
	}

	return wait_xfr_complete(dev, sdhc->timeout_ms);
}

static int write_data_port(const struct device *dev, struct sdhc_data *sdhc)
{
	struct emmc_data *emmc = dev->data;
	volatile struct emmc_reg *regs = (struct emmc_reg *)DEVICE_MMIO_GET(dev);
	uint32_t block_size = sdhc->block_size;
	uint32_t i, block_cnt = sdhc->blocks;
	uint32_t *data = (uint32_t *)sdhc->data;
	k_timeout_t wait_time;

	if (sdhc->timeout_ms == SDHC_TIMEOUT_FOREVER) {
		wait_time = K_FOREVER;
	} else {
		wait_time = K_MSEC(sdhc->timeout_ms);
	}

	LOG_DBG("");

	while ((regs->present_state & EMMC_HOST_PSTATE_BUF_WRITE_EN) == 0) {
		;
	}

	while (1) {
		uint32_t events;

		if (IS_ENABLED(CONFIG_INTEL_EMMC_HOST_INTR)) {
			k_event_clear(&emmc->irq_event, EMMC_HOST_BUF_WR_READY);
		}

		if (regs->present_state & EMMC_HOST_PSTATE_DAT_INHIBIT) {
			for (i = block_size >> 2u; i != 0u; i--) {
				regs->data_port = *data;
				data++;
			}
		}

		LOG_DBG("EMMC_HOST_BUF_WR_READY");

		if (!(--block_cnt)) {
			break;
		}
		if (IS_ENABLED(CONFIG_INTEL_EMMC_HOST_INTR)) {
			events = k_event_wait(&emmc->irq_event, EMMC_HOST_BUF_WR_READY, false,
					      wait_time);
			k_event_clear(&emmc->irq_event, EMMC_HOST_BUF_WR_READY);

			if (!(events & EMMC_HOST_BUF_WR_READY)) {
				LOG_ERR("time out on EMMC_HOST_BUF_WR_READY");
				return -EIO;
			}
		} else {
			while ((regs->present_state & EMMC_HOST_PSTATE_BUF_WRITE_EN) == 0) {
				;
			}
		}
	}

	return wait_xfr_complete(dev, sdhc->timeout_ms);
}

static int emmc_send_cmd_no_data(const struct device *dev, uint32_t cmd_idx,
				     struct sdhc_command *cmd)
{
	struct emmc_cmd_config emmc_cmd;

	emmc_cmd.sdhc_cmd = cmd;
	emmc_cmd.cmd_idx = cmd_idx;
	emmc_cmd.cmd_type = EMMC_HOST_CMD_NORMAL;
	emmc_cmd.data_present = false;
	emmc_cmd.idx_check_en = false;
	emmc_cmd.crc_check_en = false;

	return emmc_host_send_cmd(dev, &emmc_cmd);
}

static int emmc_send_cmd_data(const struct device *dev, uint32_t cmd_idx,
				  struct sdhc_command *cmd, struct sdhc_data *data, bool read)
{
	struct emmc_cmd_config emmc_cmd;
	int ret;

	emmc_cmd.sdhc_cmd = cmd;
	emmc_cmd.cmd_idx = cmd_idx;
	emmc_cmd.cmd_type = EMMC_HOST_CMD_NORMAL;
	emmc_cmd.data_present = true;
	emmc_cmd.idx_check_en = true;
	emmc_cmd.crc_check_en = true;

	ret = emmc_init_xfr(dev, data, read);
	if (ret) {
		LOG_ERR("Error on init xfr");
		return ret;
	}

	ret = emmc_host_send_cmd(dev, &emmc_cmd);
	if (ret) {
		return ret;
	}

	if (IS_ENABLED(CONFIG_INTEL_EMMC_HOST_DMA)) {
		ret = wait_xfr_complete(dev, data->timeout_ms);
	} else {
		if (read) {
			ret = read_data_port(dev, data);
		} else {
			ret = write_data_port(dev, data);
		}
	}

	return ret;
}

static int emmc_xfr(const struct device *dev, struct sdhc_command *cmd, struct sdhc_data *data,
		    bool read)
{
	struct emmc_data *emmc = dev->data;
	int ret;
	struct emmc_cmd_config emmc_cmd;

	ret = emmc_init_xfr(dev, data, read);
	if (ret) {
		LOG_ERR("error emmc init xfr");
		return ret;
	}
	emmc_cmd.sdhc_cmd = cmd;
	emmc_cmd.cmd_type = EMMC_HOST_CMD_NORMAL;
	emmc_cmd.data_present = true;
	emmc_cmd.idx_check_en = true;
	emmc_cmd.crc_check_en = true;

	k_event_clear(&emmc->irq_event, EMMC_HOST_XFER_COMPLETE);
	k_event_clear(&emmc->irq_event, read ? EMMC_HOST_BUF_RD_READY : EMMC_HOST_BUF_WR_READY);

	if (data->blocks > 1) {
		emmc_cmd.cmd_idx = read ? SD_READ_MULTIPLE_BLOCK : SD_WRITE_MULTIPLE_BLOCK;
		ret = emmc_host_send_cmd(dev, &emmc_cmd);
	} else {
		emmc_cmd.cmd_idx = read ? SD_READ_SINGLE_BLOCK : SD_WRITE_SINGLE_BLOCK;
		ret = emmc_host_send_cmd(dev, &emmc_cmd);
	}

	if (ret) {
		return ret;
	}

	if (IS_ENABLED(CONFIG_INTEL_EMMC_HOST_DMA)) {
		ret = wait_xfr_complete(dev, data->timeout_ms);
	} else {
		if (read) {
			ret = read_data_port(dev, data);
		} else {
			ret = write_data_port(dev, data);
		}
	}

	if (!IS_ENABLED(CONFIG_INTEL_EMMC_HOST_AUTO_STOP)) {
		emmc_stop_transfer(dev);
	}
	return ret;
}

static int emmc_request(const struct device *dev, struct sdhc_command *cmd, struct sdhc_data *data)
{
	int ret;

	LOG_DBG("");

	if (data) {
		switch (cmd->opcode) {
		case SD_WRITE_SINGLE_BLOCK:
		case SD_WRITE_MULTIPLE_BLOCK:
			LOG_DBG("SD_WRITE_SINGLE_BLOCK");
			ret = emmc_xfr(dev, cmd, data, false);
			break;

		case SD_READ_SINGLE_BLOCK:
		case SD_READ_MULTIPLE_BLOCK:
			LOG_DBG("SD_READ_SINGLE_BLOCK");
			ret = emmc_xfr(dev, cmd, data, true);
			break;

		case MMC_SEND_EXT_CSD:
			LOG_DBG("EMMC_HOST_SEND_EXT_CSD");
			ret = emmc_send_cmd_data(dev, MMC_SEND_EXT_CSD, cmd, data, true);
			break;

		default:
			ret = emmc_send_cmd_data(dev, cmd->opcode, cmd, data, true);
		}
	} else {
		ret = emmc_send_cmd_no_data(dev, cmd->opcode, cmd);
	}

	return ret;
}

static int emmc_set_io(const struct device *dev, struct sdhc_io *ios)
{
	struct emmc_data *emmc = dev->data;
	volatile struct emmc_reg *regs = (struct emmc_reg *)DEVICE_MMIO_GET(dev);
	struct sdhc_io *host_io = &emmc->host_io;
	int ret;

	LOG_DBG("emmc I/O: DW %d, Clk %d Hz, card power state %s, voltage %s", ios->bus_width,
		ios->clock, ios->power_mode == SDHC_POWER_ON ? "ON" : "OFF",
		ios->signal_voltage == SD_VOL_1_8_V ? "1.8V" : "3.3V");

	if (ios->clock && (ios->clock > emmc->props.f_max || ios->clock < emmc->props.f_min)) {
		LOG_ERR("Invalid argument for clock freq: %d Support max:%d and Min:%d", ios->clock,
			emmc->props.f_max, emmc->props.f_min);
		return -EINVAL;
	}

	/* Set HC clock */
	if (host_io->clock != ios->clock) {
		LOG_DBG("Clock: %d", host_io->clock);
		if (ios->clock != 0) {
			/* Enable clock */
			LOG_DBG("CLOCK: %d", ios->clock);
			if (!emmc_clock_set(dev, ios->clock)) {
				return -ENOTSUP;
			}
		} else {
			emmc_disable_clock(dev);
		}
		host_io->clock = ios->clock;
	}

	/* Set data width */
	if (host_io->bus_width != ios->bus_width) {
		LOG_DBG("bus_width: %d", host_io->bus_width);

		if (ios->bus_width == SDHC_BUS_WIDTH4BIT) {
			SET_BITS(regs->host_ctrl1, EMMC_HOST_CTRL1_EXT_DAT_WIDTH_LOC,
				 EMMC_HOST_CTRL1_EXT_DAT_WIDTH_MASK,
				 ios->bus_width == SDHC_BUS_WIDTH8BIT ? 1 : 0);
		} else {
			SET_BITS(regs->host_ctrl1, EMMC_HOST_CTRL1_DAT_WIDTH_LOC,
				 EMMC_HOST_CTRL1_DAT_WIDTH_MASK,
				 ios->bus_width == SDHC_BUS_WIDTH4BIT ? 1 : 0);
		}
		host_io->bus_width = ios->bus_width;
	}

	/* Set HC signal voltage */
	if (ios->signal_voltage != host_io->signal_voltage) {
		LOG_DBG("signal_voltage: %d", ios->signal_voltage);
		ret = emmc_set_voltage(dev, ios->signal_voltage);
		if (ret) {
			LOG_ERR("Set signal volatge failed:%d", ret);
			return ret;
		}
		host_io->signal_voltage = ios->signal_voltage;
	}

	/* Set card power */
	if (host_io->power_mode != ios->power_mode) {
		LOG_DBG("power_mode: %d", ios->power_mode);

		ret = emmc_set_power(dev, ios->power_mode);
		if (ret) {
			LOG_ERR("Set Bus power failed:%d", ret);
			return ret;
		}
		host_io->power_mode = ios->power_mode;
	}

	/* Set I/O timing */
	if (host_io->timing != ios->timing) {
		LOG_DBG("timing: %d", ios->timing);

		ret = set_timing(dev, ios->timing);
		if (ret) {
			LOG_ERR("Set timing failed:%d", ret);
			return ret;
		}
		host_io->timing = ios->timing;
	}

	return 0;
}

static int emmc_get_card_present(const struct device *dev)
{
	struct emmc_data *emmc = dev->data;
	volatile struct emmc_reg *regs = (struct emmc_reg *)DEVICE_MMIO_GET(dev);

	LOG_DBG("");

	emmc->card_present = (bool)((regs->present_state >> 16u) & 1u);

	if (!emmc->card_present) {
		LOG_ERR("No MMC device detected");
	}

	return ((int)emmc->card_present);
}

static int emmc_execute_tuning(const struct device *dev)
{
	if (IS_ENABLED(CONFIG_INTEL_EMMC_HOST_TUNING)) {
		volatile struct emmc_reg *regs = (struct emmc_reg *)DEVICE_MMIO_GET(dev);

		LOG_DBG("Tuning starting...");

		regs->host_ctrl2 |= EMMC_HOST_START_TUNING;
		while (!(regs->host_ctrl2 & EMMC_HOST_START_TUNING)) {
			;
		}

		if (regs->host_ctrl2 & EMMC_HOST_TUNING_SUCCESS) {
			LOG_DBG("Tuning Completed success");
		} else {
			LOG_ERR("Tuning failed");
			return -EIO;
		}
	}
	return 0;
}

static int emmc_card_busy(const struct device *dev)
{
	volatile struct emmc_reg *regs = (struct emmc_reg *)DEVICE_MMIO_GET(dev);

	LOG_DBG("");

	if (regs->present_state & 7u) {
		return 1;
	}

	return 0;
}

static int emmc_get_host_props(const struct device *dev, struct sdhc_host_props *props)
{
	struct emmc_data *emmc = dev->data;
	const struct emmc_config *config = dev->config;
	volatile struct emmc_reg *regs = (struct emmc_reg *)DEVICE_MMIO_GET(dev);
	uint64_t cap = regs->capabilities;

	LOG_DBG("");

	memset(props, 0, sizeof(struct sdhc_host_props));
	props->f_max = config->max_bus_freq;
	props->f_min = config->min_bus_freq;
	props->power_delay = config->power_delay_ms;

	props->host_caps.vol_180_support = (bool)(cap & BIT(26u));
	props->host_caps.vol_300_support = (bool)(cap & BIT(25u));
	props->host_caps.vol_330_support = (bool)(bool)(cap & BIT(24u));
	props->host_caps.suspend_res_support = false;
	props->host_caps.sdma_support = (bool)(cap & BIT(22u));
	props->host_caps.high_spd_support = (bool)(cap & BIT(21u));
	props->host_caps.adma_2_support = (bool)(cap & BIT(19u));

	props->host_caps.max_blk_len = (cap >> 16u) & 0x3u;
	props->host_caps.ddr50_support = (bool)(cap & BIT(34u));
	props->host_caps.sdr104_support = (bool)(cap & BIT(33u));
	props->host_caps.sdr50_support = (bool)(cap & BIT(32u));
	props->host_caps.bus_8_bit_support = true;
	props->host_caps.bus_4_bit_support = true;
	props->host_caps.hs200_support = (bool)config->hs200_mode;
	props->host_caps.hs400_support = (bool)config->hs400_mode;

	emmc->props = *props;

	return 0;
}

static void emmc_isr(const struct device *dev)
{
	struct emmc_data *emmc = dev->data;
	volatile struct emmc_reg *regs = (struct emmc_reg *)DEVICE_MMIO_GET(dev);

	if (regs->normal_int_stat & EMMC_HOST_CMD_COMPLETE) {
		regs->normal_int_stat |= EMMC_HOST_CMD_COMPLETE;
		k_event_post(&emmc->irq_event, EMMC_HOST_CMD_COMPLETE);
	}

	if (regs->normal_int_stat & EMMC_HOST_XFER_COMPLETE) {
		regs->normal_int_stat |= EMMC_HOST_XFER_COMPLETE;
		k_event_post(&emmc->irq_event, EMMC_HOST_XFER_COMPLETE);
	}

	if (regs->normal_int_stat & EMMC_HOST_DMA_INTR) {
		regs->normal_int_stat |= EMMC_HOST_DMA_INTR;
		k_event_post(&emmc->irq_event, EMMC_HOST_DMA_INTR);
	}

	if (regs->normal_int_stat & EMMC_HOST_BUF_WR_READY) {
		regs->normal_int_stat |= EMMC_HOST_BUF_WR_READY;
		k_event_post(&emmc->irq_event, EMMC_HOST_BUF_WR_READY);
	}

	if (regs->normal_int_stat & EMMC_HOST_BUF_RD_READY) {
		regs->normal_int_stat |= EMMC_HOST_BUF_RD_READY;
		k_event_post(&emmc->irq_event, EMMC_HOST_BUF_RD_READY);
	}

	if (regs->err_int_stat) {
		LOG_ERR("err int:%x", regs->err_int_stat);
		k_event_post(&emmc->irq_event, ERR_INTR_STATUS_EVENT(regs->err_int_stat));
		if (regs->err_int_stat & EMMC_HOST_DMA_TXFR_ERR) {
			regs->err_int_stat |= EMMC_HOST_DMA_TXFR_ERR;
		} else {
			regs->err_int_stat |= regs->err_int_stat;
		}
	}

	if (regs->normal_int_stat) {
		k_event_post(&emmc->irq_event, regs->normal_int_stat);
		regs->normal_int_stat |= regs->normal_int_stat;
	}

	if (regs->adma_err_stat) {
		LOG_ERR("adma err:%x", regs->adma_err_stat);
	}
}

static int emmc_init(const struct device *dev)
{
	struct emmc_data *emmc = dev->data;
	const struct emmc_config *config = dev->config;

	k_sem_init(&emmc->lock, 1, 1);
	k_event_init(&emmc->irq_event);

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(pcie)
	if (config->pcie) {
		struct pcie_bar mbar;

		if (config->pcie->bdf == PCIE_BDF_NONE) {
			LOG_ERR("Cannot probe eMMC PCI device: %x", config->pcie->id);
			return -ENODEV;
		}

		if (!pcie_probe_mbar(config->pcie->bdf, 0, &mbar)) {
			LOG_ERR("eMMC MBAR not found");
			return -EINVAL;
		}

		pcie_get_mbar(config->pcie->bdf, 0, &mbar);
		pcie_set_cmd(config->pcie->bdf, PCIE_CONF_CMDSTAT_MEM, true);
		device_map(DEVICE_MMIO_RAM_PTR(dev), mbar.phys_addr, mbar.size, K_MEM_CACHE_NONE);
		pcie_set_cmd(config->pcie->bdf, PCIE_CONF_CMDSTAT_MASTER, true);
	} else
#endif
	{
		DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);
	}

	LOG_DBG("MMC Device MMIO: %p", (void *)(struct emmc_reg *)DEVICE_MMIO_GET(dev));

	if (IS_ENABLED(CONFIG_INTEL_EMMC_HOST_INTR)) {
		config->config_func(dev);
	}
	return emmc_reset(dev);
}

static const struct sdhc_driver_api emmc_api = {
	.reset = emmc_reset,
	.request = emmc_request,
	.set_io = emmc_set_io,
	.get_card_present = emmc_get_card_present,
	.execute_tuning = emmc_execute_tuning,
	.card_busy = emmc_card_busy,
	.get_host_props = emmc_get_host_props,
};

#define EMMC_HOST_IRQ_FLAGS_SENSE0(n) 0
#define EMMC_HOST_IRQ_FLAGS_SENSE1(n) DT_INST_IRQ(n, sense)
#define EMMC_HOST_IRQ_FLAGS(n)\
	_CONCAT(EMMC_HOST_IRQ_FLAGS_SENSE, DT_INST_IRQ_HAS_CELL(n, sense))(n)

/* Not PCI(e) */
#define EMMC_HOST_IRQ_CONFIG_PCIE0(n)                                                              \
	static void emmc_config_##n(const struct device *port)                                     \
	{                                                                                          \
		ARG_UNUSED(port);                                                                  \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), emmc_isr,                   \
			    DEVICE_DT_INST_GET(n), EMMC_HOST_IRQ_FLAGS(n));                        \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}

/* PCI(e) with auto IRQ detection */
#define EMMC_HOST_IRQ_CONFIG_PCIE1(n)                                                              \
	static void emmc_config_##n(const struct device *port)                                     \
	{                                                                                          \
		BUILD_ASSERT(DT_INST_IRQN(n) == PCIE_IRQ_DETECT,                                   \
			     "Only runtime IRQ configuration is supported");                       \
		BUILD_ASSERT(IS_ENABLED(CONFIG_DYNAMIC_INTERRUPTS),                                \
			     "eMMC PCI device needs CONFIG_DYNAMIC_INTERRUPTS");                   \
		const struct emmc_config *const dev_cfg = port->config;                            \
		unsigned int irq = pcie_alloc_irq(dev_cfg->pcie->bdf);                             \
                                                                                                   \
		if (irq == PCIE_CONF_INTR_IRQ_NONE) {                                              \
			return;                                                                    \
		}                                                                                  \
		pcie_connect_dynamic_irq(dev_cfg->pcie->bdf, irq, DT_INST_IRQ(n, priority),        \
					 (void (*)(const void *))emmc_isr, DEVICE_DT_INST_GET(n),  \
					 EMMC_HOST_IRQ_FLAGS(n));                                  \
		pcie_irq_enable(dev_cfg->pcie->bdf, irq);                                          \
	}

#define EMMC_HOST_IRQ_CONFIG(n) _CONCAT(EMMC_HOST_IRQ_CONFIG_PCIE, DT_INST_ON_BUS(n, pcie))(n)

#define INIT_PCIE0(n)
#define INIT_PCIE1(n) DEVICE_PCIE_INST_INIT(n, pcie),
#define INIT_PCIE(n)  _CONCAT(INIT_PCIE, DT_INST_ON_BUS(n, pcie))(n)

#define REG_INIT_PCIE0(n) DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),
#define REG_INIT_PCIE1(n)
#define REG_INIT(n) _CONCAT(REG_INIT_PCIE, DT_INST_ON_BUS(n, pcie))(n)

#define DEFINE_PCIE0(n)
#define DEFINE_PCIE1(n)          DEVICE_PCIE_INST_DECLARE(n)
#define EMMC_HOST_PCIE_DEFINE(n) _CONCAT(DEFINE_PCIE, DT_INST_ON_BUS(n, pcie))(n)

#define EMMC_HOST_DEV_CFG(n)                                                                       \
	EMMC_HOST_PCIE_DEFINE(n);                                                                  \
	EMMC_HOST_IRQ_CONFIG(n);                                                                   \
	static const struct emmc_config emmc_config_data_##n = {                                   \
		REG_INIT(n) INIT_PCIE(n).config_func = emmc_config_##n,                            \
		.hs200_mode = DT_INST_PROP_OR(n, mmc_hs200_1_8v, 0),                               \
		.hs400_mode = DT_INST_PROP_OR(n, mmc_hs400_1_8v, 0),                               \
		.dw_4bit = DT_INST_ENUM_HAS_VALUE(n, bus_width, 4),                                \
		.dw_8bit = DT_INST_ENUM_HAS_VALUE(n, bus_width, 8),                                \
		.max_bus_freq = DT_INST_PROP_OR(n, max_bus_freq, 40000),                           \
		.min_bus_freq = DT_INST_PROP_OR(n, min_bus_freq, 40000),                           \
		.power_delay_ms = DT_INST_PROP_OR(n, power_delay_ms, 500),                         \
	};                                                                                         \
                                                                                                   \
	static struct emmc_data emmc_priv_data_##n;                                                \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, emmc_init, NULL, &emmc_priv_data_##n, &emmc_config_data_##n,      \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &emmc_api);

DT_INST_FOREACH_STATUS_OKAY(EMMC_HOST_DEV_CFG)
