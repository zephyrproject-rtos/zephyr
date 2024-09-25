/*
 * Copyright (c) 2023 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rcar_mmc

#include <zephyr/devicetree.h>
#include <zephyr/drivers/disk.h>
#include <zephyr/drivers/sdhc.h>
#include <zephyr/drivers/clock_control/renesas_cpg_mssr.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/cache.h>
#include <zephyr/drivers/regulator.h>

#include "rcar_mmc_registers.h"

#define PINCTRL_STATE_UHS PINCTRL_STATE_PRIV_START

/**
 * @note we don't need any locks here, because SDHC subsystem cares about it
 */

LOG_MODULE_REGISTER(rcar_mmc, CONFIG_LOG_DEFAULT_LEVEL);

#define MMC_POLL_FLAGS_TIMEOUT_US 100000
#define MMC_POLL_FLAGS_ONE_CYCLE_TIMEOUT_US 1
#define MMC_BUS_CLOCK_FREQ 800000000

#ifdef CONFIG_RCAR_MMC_DMA_SUPPORT
#define ALIGN_BUF_DMA __aligned(CONFIG_SDHC_BUFFER_ALIGNMENT)
#else
#define ALIGN_BUF_DMA
#endif

/**
 * @brief Renesas MMC host controller driver data
 *
 */
struct mmc_rcar_data {
	DEVICE_MMIO_RAM; /* Must be first */
	struct sdhc_io host_io;
	struct sdhc_host_props props;
#ifdef CONFIG_RCAR_MMC_DMA_IRQ_DRIVEN_SUPPORT
	struct k_sem irq_xref_fin;
#endif

	uint8_t ver;
	/* in bytes, possible values are 2, 4 or 8 */
	uint8_t width_access_sd_buf0;
	uint8_t ddr_mode;
	uint8_t restore_cfg_after_reset;
	uint8_t is_last_cmd_app_cmd; /* ACMD55 */

#ifdef CONFIG_RCAR_MMC_SCC_SUPPORT
	uint8_t manual_retuning;
	uint8_t tuning_buf[128] ALIGN_BUF_DMA;
#endif /* CONFIG_RCAR_MMC_SCC_SUPPORT */
	uint8_t can_retune;
};

/**
 * @brief Renesas MMC host controller driver configuration
 */
struct mmc_rcar_cfg {
	DEVICE_MMIO_ROM; /* Must be first */
	struct rcar_cpg_clk cpg_clk;
	struct rcar_cpg_clk bus_clk;
	const struct device *cpg_dev;
	const struct pinctrl_dev_config *pcfg;
	const struct device *regulator_vqmmc;
	const struct device *regulator_vmmc;

	uint32_t max_frequency;

#ifdef CONFIG_RCAR_MMC_DMA_IRQ_DRIVEN_SUPPORT
	void (*irq_config_func)(const struct device *dev);
#endif

	uint8_t non_removable;
	uint8_t uhs_support;
	uint8_t mmc_hs200_1_8v;
	uint8_t mmc_hs400_1_8v;
	uint8_t bus_width;
	uint8_t mmc_sdr104_support;
};

#ifdef CONFIG_RCAR_MMC_SCC_SUPPORT
static int rcar_mmc_execute_tuning(const struct device *dev);
static int rcar_mmc_retune_if_needed(const struct device *dev, bool request_retune);
#endif
static int rcar_mmc_disable_scc(const struct device *dev);

static uint32_t rcar_mmc_read_reg32(const struct device *dev, uint32_t reg)
{
	return sys_read32(DEVICE_MMIO_GET(dev) + reg);
}

static void rcar_mmc_write_reg32(const struct device *dev, uint32_t reg, uint32_t val)
{
	sys_write32(val, DEVICE_MMIO_GET(dev) + reg);
}

/* cleanup SD card interrupt flag register and mask their interrupts */
static inline void rcar_mmc_reset_and_mask_irqs(const struct device *dev)
{
	struct mmc_rcar_data *data = dev->data;

	rcar_mmc_write_reg32(dev, RCAR_MMC_INFO1, 0);
	rcar_mmc_write_reg32(dev, RCAR_MMC_INFO1_MASK, ~0);

	rcar_mmc_write_reg32(dev, RCAR_MMC_INFO2, RCAR_MMC_INFO2_CLEAR);
	rcar_mmc_write_reg32(dev, RCAR_MMC_INFO2_MASK, ~0);

#ifdef CONFIG_RCAR_MMC_DMA_SUPPORT
	/* default value of Seq suspend should be 0 */
	rcar_mmc_write_reg32(dev, RCAR_MMC_DMA_INFO1_MASK, 0xfffffeff);
	rcar_mmc_write_reg32(dev, RCAR_MMC_DMA_INFO1, 0x0);
	rcar_mmc_write_reg32(dev, RCAR_MMC_DMA_INFO2_MASK, 0xffffffff);
	rcar_mmc_write_reg32(dev, RCAR_MMC_DMA_INFO2, 0x0);
#ifdef CONFIG_RCAR_MMC_DMA_IRQ_DRIVEN_SUPPORT
	k_sem_reset(&data->irq_xref_fin);
#endif
#endif /* CONFIG_RCAR_MMC_DMA_SUPPORT */
}

/**
 * @brief check if MMC is busy
 *
 * This check should generally be implemented as checking the controller
 * state. No MMC commands need to be sent.
 *
 * @param dev MMC device
 * @retval 0 card is not busy
 * @retval 1 card is busy
 * @retval -EINVAL: the dev pointer is NULL
 */
static int rcar_mmc_card_busy(const struct device *dev)
{
	uint32_t reg;

	if (!dev) {
		return -EINVAL;
	}

	reg = rcar_mmc_read_reg32(dev, RCAR_MMC_INFO2);
	return (reg & RCAR_MMC_INFO2_DAT0) ? 0 : 1;
}

/**
 * @brief Check error flags inside INFO2 MMC register
 *
 * @note in/out parameters should be checked by a caller function
 *
 * @param dev MMC device
 *
 * @retval 0 INFO2 register hasn't errors
 * @retval -ETIMEDOUT: timed out while tx/rx
 * @retval -EIO: I/O error
 * @retval -EILSEQ: communication out of sync
 */
static int rcar_mmc_check_errors(const struct device *dev)
{
	uint32_t info2 = rcar_mmc_read_reg32(dev, RCAR_MMC_INFO2);

	if (info2 & (RCAR_MMC_INFO2_ERR_TO | RCAR_MMC_INFO2_ERR_RTO)) {
		LOG_DBG("timeout error 0x%08x", info2);
		return -ETIMEDOUT;
	}

	if (info2 & (RCAR_MMC_INFO2_ERR_END | RCAR_MMC_INFO2_ERR_CRC | RCAR_MMC_INFO2_ERR_IDX)) {
		LOG_DBG("communication out of sync 0x%08x", info2);
		return -EILSEQ;
	}

	if (info2 & (RCAR_MMC_INFO2_ERR_ILA | RCAR_MMC_INFO2_ERR_ILR | RCAR_MMC_INFO2_ERR_ILW)) {
		LOG_DBG("illegal access 0x%08x", info2);
		return -EIO;
	}

	return 0;
}

/**
 * @brief Poll flag(s) in MMC register and check errors
 *
 * @note in/out parameters should be checked by a caller function
 *
 * @param dev MMC device
 * @param reg register offset relative to the base address
 * @param flag polling flag(s)
 * @param state state of flag(s) when we should stop polling
 * @param check_errors call @ref rcar_mmc_check_errors function or not
 * @param check_dma_errors check if there are DMA errors inside info2
 * @param timeout_us timeout in microseconds how long we should poll flag(s)
 *
 * @retval 0 poll of flag(s) was successful
 * @retval -ETIMEDOUT: timed out while tx/rx
 * @retval -EIO: I/O error
 * @retval -EILSEQ: communication out of sync
 */
static int rcar_mmc_poll_reg_flags_check_err(const struct device *dev, unsigned int reg,
					     uint32_t flag, uint32_t state, bool check_errors,
					     bool check_dma_errors, int64_t timeout_us)
{
	int ret;

	while ((rcar_mmc_read_reg32(dev, reg) & flag) != state) {
		if (timeout_us < 0) {
			LOG_DBG("timeout error during polling flag(s) 0x%08x in reg 0x%08x", flag,
				reg);
			return -ETIMEDOUT;
		}

		if (check_errors) {
			ret = rcar_mmc_check_errors(dev);
			if (ret) {
				return ret;
			}
		}

		if (check_dma_errors && rcar_mmc_read_reg32(dev, RCAR_MMC_DMA_INFO2)) {
			LOG_DBG("%s: an error occurs on the DMAC channel #%u", dev->name,
				(reg & RCAR_MMC_DMA_INFO2_ERR_RD) ? 1U : 0U);
			return -EIO;
		}

		k_usleep(MMC_POLL_FLAGS_ONE_CYCLE_TIMEOUT_US);
		timeout_us -= MMC_POLL_FLAGS_ONE_CYCLE_TIMEOUT_US;
	}

	return 0;
}

/* reset DMA MMC controller */
static inline void rcar_mmc_reset_dma(const struct device *dev)
{
	uint32_t reg = RCAR_MMC_DMA_RST_DTRAN0 | RCAR_MMC_DMA_RST_DTRAN1;

	rcar_mmc_write_reg32(dev, RCAR_MMC_EXTMODE, 0);
	rcar_mmc_write_reg32(dev, RCAR_MMC_DMA_RST, ~reg);
	rcar_mmc_write_reg32(dev, RCAR_MMC_DMA_RST, ~0);
	rcar_mmc_write_reg32(dev, RCAR_MMC_EXTMODE, 1);
}

/**
 * @brief reset MMC controller state
 *
 * Used when the MMC has encountered an error. Resetting the MMC controller
 * should clear all errors on the MMC, but does not necessarily reset I/O
 * settings to boot (this can be done with @ref sdhc_set_io)
 *
 * @note during reset the clock input is disabled, also this call changes rate
 *
 * @param dev MMC controller device
 * @retval 0 reset succeeded
 * @retval -ETIMEDOUT: controller reset timed out
 * @retval -EINVAL: the dev pointer is NULL
 * @retval -EILSEQ: communication out of sync
 * @retval -ENOTSUP: controller does not support I/O
 *
 * @details List of affected registers and their bits during the soft reset trigger:
 *              * RCAR_MMC_STOP all bits reset to default (0x0);
 *              * RCAR_MMC_INFO1 affected bits:
 *                  * RCAR_MMC_INFO1_CMP default state 0;
 *                  * RCAR_MMC_INFO1_RSP default state 0;
 *                  * HPIRES Response Reception Completion (16), default state 0;
 *              * RCAR_MMC_INFO2 all bits reset 0, except the next:
 *                  * RCAR_MMC_INFO2_DAT0 state unknown after reset;
 *                  * RCAR_MMC_INFO2_SCLKDIVEN default state 1;
 *              * RCAR_MMC_CLKCTL affected bit(s):
 *                  * RCAR_MMC_CLKCTL_SCLKEN default state 0;
 *              * RCAR_MMC_OPTION affected bits:
 *                  * WIDTH (15) and WIDTH8 (13) set to 0, which equal to 4-bits bus;
 *                  * Timeout Mode Select (EXTOP - 9) is set to 0;
 *                  * Timeout Mask (TOUTMASK - 8) is set to 0;
 *                  * Timeout Counter (TOP27-TOP24 bits 7-4) is equal to 0b1110;
 *                  * Card Detect Time Counter (CTOP24-CTOP21 bits 3-0) is equal to 0b1110;
 *              * RCAR_MMC_ERR_STS1 all bits after reset 0, except the next:
 *                  * E13 default state 1 (E12-E14 it is CRC status 0b010);
 *              * RCAR_MMC_ERR_STS2 all bits after reset 0;
 *              * IO_INFO1 all bits after reset 0;
 *              * RCAR_MMC_IF_MODE all bits after reset 0.
 */
static int rcar_mmc_reset(const struct device *dev)
{
	int ret = 0;
	uint32_t reg;
	struct mmc_rcar_data *data;
	uint8_t can_retune;

	if (!dev) {
		return -EINVAL;
	}

	data = dev->data;

	/*
	 * soft reset of the host
	 */
	reg = rcar_mmc_read_reg32(dev, RCAR_MMC_SOFT_RST);
	reg &= ~RCAR_MMC_SOFT_RST_RSTX;
	rcar_mmc_write_reg32(dev, RCAR_MMC_SOFT_RST, reg);
	reg |= RCAR_MMC_SOFT_RST_RSTX;
	rcar_mmc_write_reg32(dev, RCAR_MMC_SOFT_RST, reg);

	rcar_mmc_reset_and_mask_irqs(dev);

	/*
	 * note: DMA reset can be triggered only in case of error in
	 * DMA Info2 otherwise the SDIP will not accurately operate
	 */
#ifdef CONFIG_RCAR_MMC_DMA_SUPPORT
	rcar_mmc_reset_dma(dev);
#endif

	can_retune = data->can_retune;
	if (can_retune) {
		rcar_mmc_disable_scc(dev);
	}

	/* note: be careful soft reset stops SDCLK */
	if (data->restore_cfg_after_reset) {
		struct sdhc_io ios;

		memcpy(&ios, &data->host_io, sizeof(ios));
		memset(&data->host_io, 0, sizeof(ios));

		data->host_io.power_mode = ios.power_mode;

		ret = sdhc_set_io(dev, &ios);

		rcar_mmc_write_reg32(dev, RCAR_MMC_STOP, RCAR_MMC_STOP_SEC);

#ifdef CONFIG_RCAR_MMC_SCC_SUPPORT
		/* tune if this reset isn't invoked during tuning */
		if (can_retune && (ios.timing == SDHC_TIMING_SDR50 ||
				   ios.timing == SDHC_TIMING_SDR104 ||
				   ios.timing == SDHC_TIMING_HS200)) {
			ret = rcar_mmc_execute_tuning(dev);
		}
#endif

		return ret;
	}

	data->ddr_mode = 0;
	data->host_io.bus_width = SDHC_BUS_WIDTH4BIT;
	data->host_io.timing = SDHC_TIMING_LEGACY;
	data->is_last_cmd_app_cmd = 0;

	return 0;
}

/**
 * @brief SD Clock (SD_CLK) Output Control Enable
 *
 * @note in/out parameters should be checked by a caller function.
 *
 * @param dev MMC device
 * @param enable
 *          false: SD_CLK output is disabled. The SD_CLK signal is fixed 0.
 *          true:  SD_CLK output is enabled.
 *
 * @retval 0 I/O was configured correctly
 * @retval -ETIMEDOUT: card busy flag is set during long time
 */
static int rcar_mmc_enable_clock(const struct device *dev, bool enable)
{
	int ret;
	uint32_t mmc_clk_ctl = rcar_mmc_read_reg32(dev, RCAR_MMC_CLKCTL);

	if (enable == true) {
		mmc_clk_ctl &= ~RCAR_MMC_CLKCTL_OFFEN;
		mmc_clk_ctl |= RCAR_MMC_CLKCTL_SCLKEN;
	} else {
		mmc_clk_ctl |= RCAR_MMC_CLKCTL_OFFEN;
		mmc_clk_ctl &= ~RCAR_MMC_CLKCTL_SCLKEN;
	}

	/*
	 * Do not change the values of these bits
	 * when the CBSY bit in SD_INFO2 is 1
	 */
	ret = rcar_mmc_poll_reg_flags_check_err(dev, RCAR_MMC_INFO2, RCAR_MMC_INFO2_CBSY, 0, false,
						false, MMC_POLL_FLAGS_TIMEOUT_US);
	if (ret) {
		return -ETIMEDOUT;
	}
	rcar_mmc_write_reg32(dev, RCAR_MMC_CLKCTL, mmc_clk_ctl);

	/* SD spec recommends at least 1 ms of delay */
	k_msleep(1);

	return 0;
}

/**
 * @brief Convert SDHC response to Renesas MMC response
 *
 * Function performs a conversion from SDHC response to Renesas MMC
 * CMD register response.
 *
 * @note in/out parameters should be checked by a caller function.
 *
 * @param response_type SDHC response type without SPI flags
 *
 * @retval positiv number (partial configuration of CMD register) on
 *         success, negative errno code otherwise
 */
static int32_t rcar_mmc_convert_sd_to_mmc_resp(uint32_t response_type)
{
	uint32_t mmc_resp = 0U;

	switch (response_type) {
	case SD_RSP_TYPE_NONE:
		mmc_resp = RCAR_MMC_CMD_RSP_NONE;
		break;
	case SD_RSP_TYPE_R1:
	case SD_RSP_TYPE_R5:
	case SD_RSP_TYPE_R6:
	case SD_RSP_TYPE_R7:
		mmc_resp = RCAR_MMC_CMD_RSP_R1;
		break;
	case SD_RSP_TYPE_R1b:
	case SD_RSP_TYPE_R5b:
		mmc_resp = RCAR_MMC_CMD_RSP_R1B;
		break;
	case SD_RSP_TYPE_R2:
		mmc_resp = RCAR_MMC_CMD_RSP_R2;
		break;
	case SD_RSP_TYPE_R3:
	case SD_RSP_TYPE_R4:
		mmc_resp = RCAR_MMC_CMD_RSP_R3;
		break;
	default:
		LOG_ERR("unknown response type 0x%08x", response_type);
		return -EINVAL;
	}

	__ASSERT((int32_t)mmc_resp >= 0, "%s: converted response shouldn't be negative", __func__);

	return mmc_resp;
}

/**
 * @brief Convert response from Renesas MMC to SDHC
 *
 * Function writes a response to response array of @ref sdhc_command structure
 *
 * @note in/out parameters should be checked by a caller function.
 *
 * @param dev MMC device
 * @param cmd MMC command
 * @param response_type SDHC response type without SPI flags
 *
 * @retval none
 */
static void rcar_mmc_extract_resp(const struct device *dev, struct sdhc_command *cmd,
				  uint32_t response_type)
{
	if (response_type == SD_RSP_TYPE_R2) {
		uint32_t rsp_127_104 = rcar_mmc_read_reg32(dev, RCAR_MMC_RSP76);
		uint32_t rsp_103_72 = rcar_mmc_read_reg32(dev, RCAR_MMC_RSP54);
		uint32_t rsp_71_40 = rcar_mmc_read_reg32(dev, RCAR_MMC_RSP32);
		uint32_t rsp_39_8 = rcar_mmc_read_reg32(dev, RCAR_MMC_RSP10);

		cmd->response[0] = (rsp_39_8 & 0xffffff) << 8;
		cmd->response[1] =
			((rsp_71_40 & 0x00ffffff) << 8) | ((rsp_39_8 & 0xff000000) >> 24);
		cmd->response[2] =
			((rsp_103_72 & 0x00ffffff) << 8) | ((rsp_71_40 & 0xff000000) >> 24);
		cmd->response[3] =
			((rsp_127_104 & 0x00ffffff) << 8) | ((rsp_103_72 & 0xff000000) >> 24);

		LOG_DBG("Response 2\n\t[0]: 0x%08x\n\t[1]: 0x%08x"
			"\n\t[2]: 0x%08x\n\t[3]: 0x%08x",
			cmd->response[0], cmd->response[1], cmd->response[2], cmd->response[3]);
	} else {
		cmd->response[0] = rcar_mmc_read_reg32(dev, RCAR_MMC_RSP10);
		LOG_DBG("Response %u\n\t[0]: 0x%08x", response_type, cmd->response[0]);
	}
}

/* configure CMD register for tx/rx data */
static uint32_t rcar_mmc_gen_data_cmd(struct sdhc_command *cmd, struct sdhc_data *data)
{
	uint32_t cmd_reg = RCAR_MMC_CMD_DATA;

	switch (cmd->opcode) {
	case MMC_SEND_EXT_CSD:
	case SD_READ_SINGLE_BLOCK:
	case MMC_SEND_TUNING_BLOCK:
	case SD_SEND_TUNING_BLOCK:
	case SD_SWITCH:
	case SD_APP_SEND_NUM_WRITTEN_BLK:
	case SD_APP_SEND_SCR:
		cmd_reg |= RCAR_MMC_CMD_RD;
		break;
	case SD_READ_MULTIPLE_BLOCK:
		cmd_reg |= RCAR_MMC_CMD_RD;
		cmd_reg |= RCAR_MMC_CMD_MULTI;
		break;
	case SD_WRITE_MULTIPLE_BLOCK:
		cmd_reg |= RCAR_MMC_CMD_MULTI;
		break;
	case SD_WRITE_SINGLE_BLOCK:
		/* fall through */
	default:
		break;
	}

	if (data->blocks > 1) {
		cmd_reg |= RCAR_MMC_CMD_MULTI;
	}

	return cmd_reg;
}

/**
 * @brief Transmit/Receive data to/from MMC using DMA
 *
 * Sends/Receives data to/from the MMC controller.
 *
 * @note in/out parameters should be checked by a caller function.
 *
 * @param dev MMC device
 * @param data MMC data buffer for tx/rx
 * @param is_read it is read or write operation
 *
 * @retval 0 tx/rx was successful
 * @retval -ENOTSUP: cache flush/invalidate aren't supported
 * @retval -ETIMEDOUT: timed out while tx/rx
 * @retval -EIO: I/O error
 * @retval -EILSEQ: communication out of sync
 */
static int rcar_mmc_dma_rx_tx_data(const struct device *dev, struct sdhc_data *data, bool is_read)
{
	uintptr_t dma_addr;
	uint32_t reg;
	int ret = 0;
	uint32_t dma_info1_poll_flag;
#ifdef CONFIG_RCAR_MMC_DMA_IRQ_DRIVEN_SUPPORT
	struct mmc_rcar_data *dev_data = dev->data;
#endif

	ret = sys_cache_data_flush_range(data->data, data->blocks * data->block_size);
	if (ret < 0) {
		LOG_ERR("%s: can't invalidate data cache before write", dev->name);
		return ret;
	}

	reg = rcar_mmc_read_reg32(dev, RCAR_MMC_DMA_MODE);
	if (is_read) {
		dma_info1_poll_flag = RCAR_MMC_DMA_INFO1_END_RD2;
		reg |= RCAR_MMC_DMA_MODE_DIR_RD;
	} else {
		dma_info1_poll_flag = RCAR_MMC_DMA_INFO1_END_WR;
		reg &= ~RCAR_MMC_DMA_MODE_DIR_RD;
	}
	rcar_mmc_write_reg32(dev, RCAR_MMC_DMA_MODE, reg);

	reg = rcar_mmc_read_reg32(dev, RCAR_MMC_EXTMODE);
	reg |= RCAR_MMC_EXTMODE_DMA_EN;
	rcar_mmc_write_reg32(dev, RCAR_MMC_EXTMODE, reg);

	dma_addr = k_mem_phys_addr(data->data);

	rcar_mmc_write_reg32(dev, RCAR_MMC_DMA_ADDR_L, dma_addr);
	rcar_mmc_write_reg32(dev, RCAR_MMC_DMA_ADDR_H, 0);

#ifdef CONFIG_RCAR_MMC_DMA_IRQ_DRIVEN_SUPPORT
	rcar_mmc_write_reg32(
		dev, RCAR_MMC_DMA_INFO2_MASK,
		(uint32_t)(is_read ? (~RCAR_MMC_DMA_INFO2_ERR_RD) : (~RCAR_MMC_DMA_INFO2_ERR_WR)));

	reg = rcar_mmc_read_reg32(dev, RCAR_MMC_DMA_INFO1_MASK);
	reg &= ~dma_info1_poll_flag;
	rcar_mmc_write_reg32(dev, RCAR_MMC_DMA_INFO1_MASK, reg);
	rcar_mmc_write_reg32(dev, RCAR_MMC_DMA_CTL, RCAR_MMC_DMA_CTL_START);

	ret = k_sem_take(&dev_data->irq_xref_fin, K_MSEC(data->timeout_ms));
	if (ret < 0) {
		LOG_ERR("%s: interrupt signal timeout error %d", dev->name, ret);
	}

	reg = rcar_mmc_read_reg32(dev, RCAR_MMC_DMA_INFO2);
	if (reg) {
		LOG_ERR("%s: an error occurs on the DMAC channel #%u", dev->name,
			(reg & RCAR_MMC_DMA_INFO2_ERR_RD) ? 1U : 0U);
		ret = -EIO;
	}
#else
	rcar_mmc_write_reg32(dev, RCAR_MMC_DMA_CTL, RCAR_MMC_DMA_CTL_START);
	ret = rcar_mmc_poll_reg_flags_check_err(dev, RCAR_MMC_DMA_INFO1, dma_info1_poll_flag,
						dma_info1_poll_flag, false, true,
						data->timeout_ms * 1000LL);
#endif

	if (is_read) {
		if (sys_cache_data_invd_range(data->data, data->blocks * data->block_size) < 0) {
			LOG_ERR("%s: can't invalidate data cache after read", dev->name);
		}
	}

	/* in case when we get to here and there wasn't IRQ trigger */
	rcar_mmc_write_reg32(dev, RCAR_MMC_DMA_INFO1_MASK, 0xfffffeff);
	rcar_mmc_write_reg32(dev, RCAR_MMC_DMA_INFO2_MASK, ~0);

	if (ret == -EIO) {
		rcar_mmc_reset_dma(dev);
	}

	reg = rcar_mmc_read_reg32(dev, RCAR_MMC_EXTMODE);
	reg &= ~RCAR_MMC_EXTMODE_DMA_EN;
	rcar_mmc_write_reg32(dev, RCAR_MMC_EXTMODE, reg);

	return ret;
}

/* read from SD/MMC controller buf0 register */
static inline uint64_t rcar_mmc_read_buf0(const struct device *dev)
{
	uint64_t buf0 = 0ULL;
	struct mmc_rcar_data *dev_data = dev->data;
	uint8_t sd_buf0_size = dev_data->width_access_sd_buf0;
	mm_reg_t buf0_addr = DEVICE_MMIO_GET(dev) + RCAR_MMC_BUF0;

	switch (sd_buf0_size) {
	case 8:
		buf0 = sys_read64(buf0_addr);
		break;
	case 4:
		buf0 = sys_read32(buf0_addr);
		break;
	case 2:
		buf0 = sys_read16(buf0_addr);
		break;
	default:
		k_panic();
		break;
	}

	return buf0;
}

/* write to SD/MMC controller buf0 register */
static inline void rcar_mmc_write_buf0(const struct device *dev, uint64_t val)
{
	struct mmc_rcar_data *dev_data = dev->data;
	uint8_t sd_buf0_size = dev_data->width_access_sd_buf0;
	mm_reg_t buf0_addr = DEVICE_MMIO_GET(dev) + RCAR_MMC_BUF0;

	switch (sd_buf0_size) {
	case 8:
		sys_write64(val, buf0_addr);
		break;
	case 4:
		sys_write32(val, buf0_addr);
		break;
	case 2:
		sys_write16(val, buf0_addr);
		break;
	default:
		k_panic();
		break;
	}
}

/**
 * @brief Transmit/Receive data to/from MMC without DMA
 *
 * Sends/Receives data to/from the MMC controller.
 *
 * @note in/out parameters should be checked by a caller function.
 *
 * @param dev MMC device
 * @param data MMC data buffer for tx/rx
 * @param is_read it is read or write operation
 *
 * @retval 0 tx/rx was successful
 * @retval -EINVAL: invalid block size
 * @retval -ETIMEDOUT: timed out while tx/rx
 * @retval -EIO: I/O error
 * @retval -EILSEQ: communication out of sync
 */
static int rcar_mmc_sd_buf_rx_tx_data(const struct device *dev, struct sdhc_data *data,
				      bool is_read)
{
	struct mmc_rcar_data *dev_data = dev->data;
	uint32_t block;
	int ret = 0;
	uint32_t info2_poll_flag = is_read ? RCAR_MMC_INFO2_BRE : RCAR_MMC_INFO2_BWE;
	uint8_t sd_buf0_size = dev_data->width_access_sd_buf0;
	uint16_t aligned_block_size = ROUND_UP(data->block_size, sd_buf0_size);
	uint32_t cmd_reg = 0;
	int64_t remaining_timeout_us = data->timeout_ms * 1000LL;

	/*
	 * note: below code should work for all possible block sizes, but
	 *       we need below check, because code isn't tested with smaller
	 *       block sizes.
	 */
	if ((data->block_size % dev_data->width_access_sd_buf0) ||
	    (data->block_size < dev_data->width_access_sd_buf0)) {
		LOG_ERR("%s: block size (%u) less or not align on SD BUF0 access width (%hhu)",
			dev->name, data->block_size, dev_data->width_access_sd_buf0);
		return -EINVAL;
	}

	/*
	 * JEDEC Standard No. 84-B51
	 * 6.6.24 Dual Data Rate mode operation:
	 * Therefore, all single or multiple block data transfer read or write will operate on
	 * a fixed block size of 512 bytes while the Device remains in dual data rate.
	 *
	 * Physical Layer Specification Version 3.01
	 * 4.12.6 Timing Changes in DDR50 Mode
	 * 4.12.6.2 Protocol Principles
	 * * Read and Write data block length size is always 512 bytes (same as SDHC).
	 */
	if (dev_data->ddr_mode && data->block_size != 512) {
		LOG_ERR("%s: block size (%u) isn't equal to 512 in DDR mode", dev->name,
			data->block_size);
		return -EINVAL;
	}

	/*
	 * note: the next restrictions we have according to description of
	 *       transfer data length register from R-Car S4 series User's Manual
	 */
	if (data->block_size > 512 || data->block_size == 0) {
		LOG_ERR("%s: block size (%u) must not be bigger than 512 bytes and equal to zero",
			dev->name, data->block_size);
		return -EINVAL;
	}

	cmd_reg = rcar_mmc_read_reg32(dev, RCAR_MMC_CMD);
	if (cmd_reg & RCAR_MMC_CMD_MULTI) {
		/* CMD12 is automatically issued at multiple block transfer */
		if (!(cmd_reg & RCAR_MMC_CMD_NOSTOP) && data->block_size != 512) {
			LOG_ERR("%s: illegal block size (%u) for multi-block xref with CMD12",
				dev->name, data->block_size);
			return -EINVAL;
		}

		switch (data->block_size) {
		case 32:
		case 64:
		case 128:
		case 256:
		case 512:
			break;
		default:
			LOG_ERR("%s: illegal block size (%u) for multi-block xref without CMD12",
				dev->name, data->block_size);
			return -EINVAL;
		}
	}

	if (data->block_size == 1 && dev_data->host_io.bus_width == SDHC_BUS_WIDTH8BIT) {
		LOG_ERR("%s: block size can't be equal to 1 with 8-bits bus width", dev->name);
		return -EINVAL;
	}

	for (block = 0; block < data->blocks; block++) {
		uint8_t *buf = (uint8_t *)data->data + (block * data->block_size);
		uint32_t info2_reg;
		uint16_t w_off; /* word offset in a block */
		uint64_t start_block_xref_us = k_ticks_to_us_ceil64(k_uptime_ticks());

		/* wait until the buffer is filled with data */
		ret = rcar_mmc_poll_reg_flags_check_err(dev, RCAR_MMC_INFO2, info2_poll_flag,
							info2_poll_flag, true, false,
							remaining_timeout_us);
		if (ret) {
			return ret;
		}

		/* clear write/read buffer ready flag */
		info2_reg = rcar_mmc_read_reg32(dev, RCAR_MMC_INFO2);
		info2_reg &= ~info2_poll_flag;
		rcar_mmc_write_reg32(dev, RCAR_MMC_INFO2, info2_reg);

		for (w_off = 0; w_off < aligned_block_size; w_off += sd_buf0_size) {
			uint64_t buf0 = 0ULL;
			uint8_t copy_size = MIN(sd_buf0_size, data->block_size - w_off);

			if (is_read) {
				buf0 = rcar_mmc_read_buf0(dev);
				memcpy(buf + w_off, &buf0, copy_size);
			} else {
				memcpy(&buf0, buf + w_off, copy_size);
				rcar_mmc_write_buf0(dev, buf0);
			}
		}

		remaining_timeout_us -=
			k_ticks_to_us_ceil64(k_uptime_ticks()) - start_block_xref_us;
		if (remaining_timeout_us < 0) {
			return -ETIMEDOUT;
		}
	}

	return ret;
}

/**
 * @brief Transmit/Receive data to/from MMC
 *
 * Sends/Receives data to/from the MMC controller.
 *
 * @note in/out parameters should be checked by a caller function.
 *
 * @param dev MMC device
 * @param data MMC data buffer for tx/rx
 * @param is_read it is read or write operation
 *
 * @retval 0 tx/rx was successful
 * @retval -EINVAL: invalid block size
 * @retval -ETIMEDOUT: timed out while tx/rx
 * @retval -EIO: I/O error
 * @retval -EILSEQ: communication out of sync
 */
static int rcar_mmc_rx_tx_data(const struct device *dev, struct sdhc_data *data, bool is_read)
{
	uint32_t info1_reg;
	int ret = 0;

#ifdef CONFIG_RCAR_MMC_DMA_SUPPORT
	if (!(k_mem_phys_addr(data->data) >> 32)) {
		ret = rcar_mmc_dma_rx_tx_data(dev, data, is_read);
	} else
#endif
	{
		ret = rcar_mmc_sd_buf_rx_tx_data(dev, data, is_read);
	}

	if (ret < 0) {
		return ret;
	}

	ret = rcar_mmc_poll_reg_flags_check_err(dev, RCAR_MMC_INFO1, RCAR_MMC_INFO1_CMP,
						RCAR_MMC_INFO1_CMP, true, false,
						MMC_POLL_FLAGS_TIMEOUT_US);
	if (ret) {
		return ret;
	}

	/* clear access end flag  */
	info1_reg = rcar_mmc_read_reg32(dev, RCAR_MMC_INFO1);
	info1_reg &= ~RCAR_MMC_INFO1_CMP;
	rcar_mmc_write_reg32(dev, RCAR_MMC_INFO1, info1_reg);

	return ret;
}

/**
 * @brief Send command to MMC
 *
 * Sends a command to the MMC controller.
 *
 * @param dev MMC device
 * @param cmd MMC command
 * @param data MMC data. Leave NULL to send SD command without data.
 *
 * @retval 0 command was sent successfully
 * @retval -ETIMEDOUT: command timed out while sending
 * @retval -ENOTSUP: host controller does not support command
 * @retval -EIO: I/O error
 * @retval -EILSEQ: communication out of sync
 */
static int rcar_mmc_request(const struct device *dev, struct sdhc_command *cmd,
			    struct sdhc_data *data)
{
	int ret = -ENOTSUP;
	uint32_t reg;
	uint32_t response_type;
	bool is_read = true;
	int attempts;
	struct mmc_rcar_data *dev_data;

	if (!dev || !cmd) {
		return -EINVAL;
	}

	dev_data = dev->data;
	response_type = cmd->response_type & SDHC_NATIVE_RESPONSE_MASK;
	attempts = cmd->retries + 1;

	while (ret && attempts-- > 0) {
		if (ret != -ENOTSUP) {
			rcar_mmc_reset(dev);
#ifdef CONFIG_RCAR_MMC_SCC_SUPPORT
			rcar_mmc_retune_if_needed(dev, true);
#endif
		}

		ret = rcar_mmc_poll_reg_flags_check_err(dev, RCAR_MMC_INFO2, RCAR_MMC_INFO2_CBSY, 0,
							false, false, MMC_POLL_FLAGS_TIMEOUT_US);
		if (ret) {
			ret = -EBUSY;
			continue;
		}

		rcar_mmc_reset_and_mask_irqs(dev);

		rcar_mmc_write_reg32(dev, RCAR_MMC_ARG, cmd->arg);

		reg = cmd->opcode;

		if (data) {
			rcar_mmc_write_reg32(dev, RCAR_MMC_SIZE, data->block_size);
			rcar_mmc_write_reg32(dev, RCAR_MMC_SECCNT, data->blocks);
			reg |= rcar_mmc_gen_data_cmd(cmd, data);
			is_read = (reg & RCAR_MMC_CMD_RD) ? true : false;
		}

		/* CMD55 is always sended before ACMD */
		if (dev_data->is_last_cmd_app_cmd) {
			reg |= RCAR_MMC_CMD_APP;
		}

		ret = rcar_mmc_convert_sd_to_mmc_resp(response_type);
		if (ret < 0) {
			/* don't need to retry we will always have the same result */
			return -EINVAL;
		}

		reg |= ret;

		LOG_DBG("(SD_CMD=%08x, SD_ARG=%08x)", cmd->opcode, cmd->arg);
		rcar_mmc_write_reg32(dev, RCAR_MMC_CMD, reg);

		/* wait until response end flag is set or errors occur */
		ret = rcar_mmc_poll_reg_flags_check_err(dev, RCAR_MMC_INFO1, RCAR_MMC_INFO1_RSP,
							RCAR_MMC_INFO1_RSP, true, false,
							cmd->timeout_ms * 1000LL);
		if (ret) {
			continue;
		}

		/* clear response end flag */
		reg = rcar_mmc_read_reg32(dev, RCAR_MMC_INFO1);
		reg &= ~RCAR_MMC_INFO1_RSP;
		rcar_mmc_write_reg32(dev, RCAR_MMC_INFO1, reg);

		rcar_mmc_extract_resp(dev, cmd, response_type);

		if (data) {
			ret = rcar_mmc_rx_tx_data(dev, data, is_read);
			if (ret) {
				continue;
			}
		}

		/* wait until the SD bus (CMD, DAT) is free or errors occur */
		ret = rcar_mmc_poll_reg_flags_check_err(
			dev, RCAR_MMC_INFO2, RCAR_MMC_INFO2_SCLKDIVEN, RCAR_MMC_INFO2_SCLKDIVEN,
			true, false, MMC_POLL_FLAGS_TIMEOUT_US);
	}

	if (ret) {
		rcar_mmc_reset(dev);
#ifdef CONFIG_RCAR_MMC_SCC_SUPPORT
		rcar_mmc_retune_if_needed(dev, true);
#endif
	}

	dev_data->is_last_cmd_app_cmd = (cmd->opcode == SD_APP_CMD);

	return ret;
}

/* convert sd_voltage to string */
static inline const char *const rcar_mmc_get_signal_voltage_str(enum sd_voltage voltage)
{
	static const char *const sig_vol_str[] = {
		[0] = "Unset",		 [SD_VOL_3_3_V] = "3.3V", [SD_VOL_3_0_V] = "3.0V",
		[SD_VOL_1_8_V] = "1.8V", [SD_VOL_1_2_V] = "1.2V",
	};

	if (voltage >= 0 && voltage < ARRAY_SIZE(sig_vol_str)) {
		return sig_vol_str[voltage];
	} else {
		return "Unknown";
	}
}

/* convert sdhc_timing_mode to string */
static inline const char *const rcar_mmc_get_timing_str(enum sdhc_timing_mode timing)
{
	static const char *const timing_str[] = {
		[0] = "Unset",
		[SDHC_TIMING_LEGACY] = "LEGACY",
		[SDHC_TIMING_HS] = "HS",
		[SDHC_TIMING_SDR12] = "SDR12",
		[SDHC_TIMING_SDR25] = "SDR25",
		[SDHC_TIMING_SDR50] = "SDR50",
		[SDHC_TIMING_SDR104] = "SDR104",
		[SDHC_TIMING_DDR50] = "DDR50",
		[SDHC_TIMING_DDR52] = "DDR52",
		[SDHC_TIMING_HS200] = "HS200",
		[SDHC_TIMING_HS400] = "HS400",
	};

	if (timing >= 0 && timing < ARRAY_SIZE(timing_str)) {
		return timing_str[timing];
	} else {
		return "Unknown";
	}
}

/* change voltage of MMC */
static int rcar_mmc_change_voltage(const struct mmc_rcar_cfg *cfg, struct sdhc_io *host_io,
				   struct sdhc_io *ios)
{
	int ret = 0;

	/* Set host signal voltage */
	if (!ios->signal_voltage || ios->signal_voltage == host_io->signal_voltage) {
		return 0;
	}

	switch (ios->signal_voltage) {
	case SD_VOL_3_3_V:
		ret = regulator_set_voltage(cfg->regulator_vqmmc, 3300000, 3300000);
		if (ret && ret != -ENOSYS) {
			break;
		}

		ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
		break;
	case SD_VOL_1_8_V:
		ret = regulator_set_voltage(cfg->regulator_vqmmc, 1800000, 1800000);
		if (ret && ret != -ENOSYS) {
			break;
		}

		ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_UHS);
		break;
	case SD_VOL_3_0_V:
	case SD_VOL_1_2_V:
		/* fall through */
	default:
		ret = -ENOTSUP;
		return ret;
	}

	if (!ret) {
		host_io->signal_voltage = ios->signal_voltage;
	}

	return ret;
}

/* note: for zero val function returns zero */
static inline uint32_t round_up_next_pwr_of_2(uint32_t val)
{
	__ASSERT(val, "Zero val passed to %s", __func__);

	val--;
	val |= val >> 1;
	val |= val >> 2;
	val |= val >> 4;
	val |= val >> 8;
	val |= val >> 16;
	return ++val;
}

/**
 * @brief configure clock divider on MMC controller
 *
 * @note In/out parameters should be checked by a caller function.
 * @note In the case of data transfer in HS400 mode (HS400 bit in
 *       SDIF_MODE = 1), do not set this width equal to 1.
 * @note In the case of writing of one-byte block, 8-bit width cannot
 *       be specified for the bus width. Change the bus width to 4 bits
 *       or 1 bit before writing one-byte block.
 *
 * @param dev MMC device
 * @param io I/O properties
 *
 * @retval 0 I/O was configured correctly
 * @retval -ENOTSUP: controller does not support these I/O settings
 * @retval -ETIMEDOUT: card busy flag is set during long time
 */
static int rcar_mmc_set_clk_rate(const struct device *dev, struct sdhc_io *ios)
{
	int ret = 0;
	uint32_t divisor;
	uint32_t mmc_clk_ctl;
	struct mmc_rcar_data *data = dev->data;
	const struct mmc_rcar_cfg *cfg = dev->config;
	struct sdhc_io *host_io = &data->host_io;

	if (host_io->clock == ios->clock) {
		return 0;
	}

	if (ios->clock == 0) {
		host_io->clock = 0;
		return rcar_mmc_enable_clock(dev, false);
	}

	if (ios->clock > data->props.f_max || ios->clock < data->props.f_min) {
		LOG_ERR("SDHC I/O: clock (%d) isn't in range %d - %d Hz", ios->clock,
			data->props.f_min, data->props.f_max);
		return -EINVAL;
	}

	divisor = DIV_ROUND_UP(cfg->max_frequency, ios->clock);

	/* Do not set divider to 0xff in DDR mode */
	if (data->ddr_mode && (divisor == 1)) {
		divisor = 2;
	}

	divisor = round_up_next_pwr_of_2(divisor);
	if (divisor == 1) {
		divisor = RCAR_MMC_CLKCTL_RCAR_DIV1;
	} else {
		divisor >>= 2;
	}

	/*
	 * Stop the clock before changing its rate
	 * to avoid a glitch signal
	 */
	ret = rcar_mmc_enable_clock(dev, false);
	if (ret) {
		return ret;
	}

	mmc_clk_ctl = rcar_mmc_read_reg32(dev, RCAR_MMC_CLKCTL);
	if ((mmc_clk_ctl & RCAR_MMC_CLKCTL_SCLKEN) &&
	    (mmc_clk_ctl & RCAR_MMC_CLKCTL_DIV_MASK) == divisor) {
		host_io->clock = ios->clock;
		return rcar_mmc_enable_clock(dev, false);
	}

	/*
	 * Do not change the values of these bits
	 * when the CBSY bit in SD_INFO2 is 1
	 */
	ret = rcar_mmc_poll_reg_flags_check_err(dev, RCAR_MMC_INFO2, RCAR_MMC_INFO2_CBSY, 0, false,
						false, MMC_POLL_FLAGS_TIMEOUT_US);
	if (ret) {
		return -ETIMEDOUT;
	}

	mmc_clk_ctl &= ~RCAR_MMC_CLKCTL_DIV_MASK;
	mmc_clk_ctl |= divisor;

	rcar_mmc_write_reg32(dev, RCAR_MMC_CLKCTL, mmc_clk_ctl);
	ret = rcar_mmc_enable_clock(dev, true);
	if (ret) {
		return ret;
	}

	host_io->clock = ios->clock;

	LOG_DBG("%s: set clock rate to %d", dev->name, ios->clock);

	return 0;
}

/**
 * @brief set bus width of MMC
 *
 * @note In/out parameters should be checked by a caller function.
 * @note In the case of data transfer in HS400 mode (HS400 bit in
 *       SDIF_MODE = 1), do not set this width equal to 1.
 * @note In the case of writing of one-byte block, 8-bit width cannot
 *       be specified for the bus width. Change the bus width to 4 bits
 *       or 1 bit before writing one-byte block.
 *
 * @param dev MMC device
 * @param io I/O properties
 *
 * @retval 0 I/O was configured correctly
 * @retval -ENOTSUP: controller does not support these I/O settings
 * @retval -ETIMEDOUT: card busy flag is set during long time
 */
static int rcar_mmc_set_bus_width(const struct device *dev, struct sdhc_io *ios)
{
	int ret = 0;
	uint32_t mmc_option_reg;
	uint32_t reg_width;
	struct mmc_rcar_data *data = dev->data;
	struct sdhc_io *host_io = &data->host_io;

	/* Set bus width */
	if (host_io->bus_width == ios->bus_width) {
		return 0;
	}

	if (!ios->bus_width) {
		return 0;
	}

	switch (ios->bus_width) {
	case SDHC_BUS_WIDTH1BIT:
		reg_width = RCAR_MMC_OPTION_WIDTH_1;
		break;
	case SDHC_BUS_WIDTH4BIT:
		if (data->props.host_caps.bus_4_bit_support) {
			reg_width = RCAR_MMC_OPTION_WIDTH_4;
		} else {
			LOG_ERR("SDHC I/O: 4-bits bus width isn't supported");
			return -ENOTSUP;
		}
		break;
	case SDHC_BUS_WIDTH8BIT:
		if (data->props.host_caps.bus_8_bit_support) {
			reg_width = RCAR_MMC_OPTION_WIDTH_8;
		} else {
			LOG_ERR("SDHC I/O: 8-bits bus width isn't supported");
			return -ENOTSUP;
		}
		break;
	default:
		return -ENOTSUP;
	}

	/*
	 * Do not change the values of these bits
	 * when the CBSY bit in SD_INFO2 is 1
	 */
	ret = rcar_mmc_poll_reg_flags_check_err(dev, RCAR_MMC_INFO2, RCAR_MMC_INFO2_CBSY, 0, false,
						false, MMC_POLL_FLAGS_TIMEOUT_US);
	if (ret) {
		return -ETIMEDOUT;
	}

	mmc_option_reg = rcar_mmc_read_reg32(dev, RCAR_MMC_OPTION);
	mmc_option_reg &= ~RCAR_MMC_OPTION_WIDTH_MASK;
	mmc_option_reg |= reg_width;
	rcar_mmc_write_reg32(dev, RCAR_MMC_OPTION, mmc_option_reg);

	host_io->bus_width = ios->bus_width;

	LOG_DBG("%s: set bus-width to %d", dev->name, host_io->bus_width);
	return 0;
}

/**
 * set DDR mode on MMC controller according to value inside
 * ddr_mode field from @ref mmc_rcar_data structure.
 */
static int rcar_mmc_set_ddr_mode(const struct device *dev)
{
	int ret = 0;
	uint32_t if_mode_reg;
	struct mmc_rcar_data *data = dev->data;

	/*
	 * Do not change the values of these bits
	 * when the CBSY bit in SD_INFO2 is 1
	 */
	ret = rcar_mmc_poll_reg_flags_check_err(dev, RCAR_MMC_INFO2, RCAR_MMC_INFO2_CBSY, 0, false,
						false, MMC_POLL_FLAGS_TIMEOUT_US);
	if (ret) {
		return -ETIMEDOUT;
	}

	if_mode_reg = rcar_mmc_read_reg32(dev, RCAR_MMC_IF_MODE);
	if (data->ddr_mode) {
		/* HS400 mode (DDR mode) */
		if_mode_reg |= RCAR_MMC_IF_MODE_DDR;
	} else {
		/* Normal mode (default, high speed, or SDR) */
		if_mode_reg &= ~RCAR_MMC_IF_MODE_DDR;
	}
	rcar_mmc_write_reg32(dev, RCAR_MMC_IF_MODE, if_mode_reg);

	return 0;
}

/**
 * @brief set timing property of MMC
 *
 * For now function only can enable DDR mode and call the function for
 * changing voltage. It is expectable that we change clock using another
 * I/O option.
 * @note In/out parameters should be checked by a caller function.
 *
 * @param dev MMC device
 * @param io I/O properties
 *
 * @retval 0 I/O was configured correctly
 * @retval -ENOTSUP: controller does not support these I/O settings
 * @retval -ETIMEDOUT: card busy flag is set during long time
 */
static int rcar_mmc_set_timings(const struct device *dev, struct sdhc_io *ios)
{
	int ret;
	struct mmc_rcar_data *data = dev->data;
	struct sdhc_io *host_io = &data->host_io;
	enum sd_voltage new_voltage = host_io->signal_voltage;

	if (host_io->timing == ios->timing) {
		return 0;
	}

	if (!host_io->timing) {
		return 0;
	}

	data->ddr_mode = 0;

	switch (ios->timing) {
	case SDHC_TIMING_LEGACY:
		break;
	case SDHC_TIMING_HS:
		if (!data->props.host_caps.high_spd_support) {
			LOG_ERR("SDHC I/O: HS timing isn't supported");
			return -ENOTSUP;
		}
		break;
	case SDHC_TIMING_SDR12:
	case SDHC_TIMING_SDR25:
	case SDHC_TIMING_SDR50:
		break;
	case SDHC_TIMING_SDR104:
		if (!data->props.host_caps.sdr104_support) {
			LOG_ERR("SDHC I/O: SDR104 timing isn't supported");
			return -ENOTSUP;
		}
		break;
	case SDHC_TIMING_HS400:
		if (!data->props.host_caps.hs400_support) {
			LOG_ERR("SDHC I/O: HS400 timing isn't supported");
			return -ENOTSUP;
		}
		new_voltage = SD_VOL_1_8_V;
		data->ddr_mode = 1;
		break;
	case SDHC_TIMING_DDR50:
	case SDHC_TIMING_DDR52:
		if (!data->props.host_caps.ddr50_support) {
			LOG_ERR("SDHC I/O: DDR50/DDR52 timing isn't supported");
			return -ENOTSUP;
		}
		data->ddr_mode = 1;
		break;
	case SDHC_TIMING_HS200:
		if (!data->props.host_caps.hs200_support) {
			LOG_ERR("SDHC I/O: HS200 timing isn't supported");
			return -ENOTSUP;
		}
		new_voltage = SD_VOL_1_8_V;
		break;
	default:
		return -ENOTSUP;
	}

	ios->signal_voltage = new_voltage;
	if (rcar_mmc_change_voltage(dev->config, host_io, ios)) {
		return -ENOTSUP;
	}

	ret = rcar_mmc_set_ddr_mode(dev);
	if (ret) {
		return ret;
	}

	host_io->timing = ios->timing;
	return 0;
}

/**
 * @brief set I/O properties of MMC
 *
 * I/O properties should be reconfigured when the card has been sent a command
 * to change its own MMC settings. This function can also be used to toggle
 * power to the SD card.
 *
 * @param dev MMC device
 * @param io I/O properties
 *
 * @retval 0 I/O was configured correctly
 * @retval -ENOTSUP: controller does not support these I/O settings
 * @retval -EINVAL: some of pointers provided to the function are NULL
 * @retval -ETIMEDOUT: card busy flag is set during long time
 */
static int rcar_mmc_set_io(const struct device *dev, struct sdhc_io *ios)
{
	int ret = 0;
	struct mmc_rcar_data *data;
	struct sdhc_io *host_io;

	if (!dev || !ios || !dev->data || !dev->config) {
		return -EINVAL;
	}

	data = dev->data;
	host_io = &data->host_io;

	LOG_DBG("SDHC I/O: bus width %d, clock %dHz, card power %s, "
		"timing %s, voltage %s",
		ios->bus_width, ios->clock, ios->power_mode == SDHC_POWER_ON ? "ON" : "OFF",
		rcar_mmc_get_timing_str(ios->timing),
		rcar_mmc_get_signal_voltage_str(ios->signal_voltage));

	/* Set host clock */
	ret = rcar_mmc_set_clk_rate(dev, ios);
	if (ret) {
		LOG_ERR("SDHC I/O: can't change clock rate error %d old %d new %d", ret,
			host_io->clock, ios->clock);
		return ret;
	}

	/*
	 * Set card bus mode
	 *
	 * SD Specifications Part 1 Physical Layer Simplified Specification Version 9.00
	 * 4.7.1 Command Types: "... there is no Open Drain mode in SD Memory Card"
	 *
	 * The use of open-drain mode is not possible in SD memory cards because the SD bus uses
	 * push-pull signaling, where both the host and the card can actively drive the data lines
	 * high or low.
	 * In an SD card, the command and response signaling needs to be bidirectional, and each
	 * signal line needs to be actively driven high or low. The use of open-drain mode in this
	 * scenario would not allow for the necessary bidirectional signaling and could result in
	 * communication errors.
	 *
	 * JEDEC Standard No. 84-B51, 10 The eMMC bus:
	 * "The e•MMC bus has eleven communication lines:
	 *  - CMD: Command is a bidirectional signal. The host and Device drivers are operating in
	 *    two modes, open drain and push/pull.
	 *  - DAT0-7: Data lines are bidirectional signals. Host and Device drivers are operating
	 *    in push-pull mode.
	 *  - CLK: Clock is a host to Device signal. CLK operates in push-pull mode.
	 *  - Data Strobe: Data Strobe is a Device to host signal. Data Strobe operates in
	 *    push-pull mode."
	 *
	 * So, open-drain mode signaling is supported in eMMC as one of the signaling modes for
	 * the CMD line. But Gen3 and Gen4 boards has MMC/SD controller which is a specialized
	 * component designed specifically for managing communication with MMC/SD devices. It
	 * handles low-level operations such as protocol handling, data transfer, and error
	 * checking and should take care of the low-level details of communicating with the
	 * MMC/SD card, including setting the bus mode. Moreover, we can use only MMIO mode, the
	 * processor communicates with the MMC/SD controller through memory read and write
	 * operations, rather than through dedicated I/O instructions or specialized data transfer
	 * protocols like SPI or SDIO. Finally, R-Car Gen3 and Gen4 "User’s manuals: Hardware"
	 * don't have direct configurations for open-drain mode for both PFC and GPIO and Zephyr
	 * SDHC subsystem doesn't support any bus mode except push-pull.
	 */
	if (ios->bus_mode != SDHC_BUSMODE_PUSHPULL) {
		LOG_ERR("SDHC I/O: not supported bus mode %d", ios->bus_mode);
		return -ENOTSUP;
	}
	host_io->bus_mode = ios->bus_mode;

	/* Set card power */
	if (ios->power_mode && host_io->power_mode != ios->power_mode) {
		const struct mmc_rcar_cfg *cfg = dev->config;

		switch (ios->power_mode) {
		case SDHC_POWER_ON:
			ret = regulator_enable(cfg->regulator_vmmc);
			if (ret) {
				break;
			}

			k_msleep(data->props.power_delay);

			ret = regulator_enable(cfg->regulator_vqmmc);
			if (ret) {
				break;
			}

			k_msleep(data->props.power_delay);
			ret = rcar_mmc_enable_clock(dev, true);
			break;
		case SDHC_POWER_OFF:
			if (regulator_is_enabled(cfg->regulator_vqmmc)) {
				ret = regulator_disable(cfg->regulator_vqmmc);
				if (ret) {
					break;
				}
			}

			if (regulator_is_enabled(cfg->regulator_vmmc)) {
				ret = regulator_disable(cfg->regulator_vmmc);
				if (ret) {
					break;
				}
			}

			ret = rcar_mmc_enable_clock(dev, false);
			break;
		default:
			LOG_ERR("SDHC I/O: not supported power mode %d", ios->power_mode);
			return -ENOTSUP;
		}

		if (ret) {
			return ret;
		}
		host_io->power_mode = ios->power_mode;
	}

	ret = rcar_mmc_set_bus_width(dev, ios);
	if (ret) {
		LOG_ERR("SDHC I/O: can't change bus width error %d old %d new %d", ret,
			host_io->bus_width, ios->bus_width);
		return ret;
	}

	ret = rcar_mmc_set_timings(dev, ios);
	if (ret) {
		LOG_ERR("SDHC I/O: can't change timing error %d old %d new %d", ret,
			host_io->timing, ios->timing);
		return ret;
	}

	ret = rcar_mmc_change_voltage(dev->config, host_io, ios);
	if (ret) {
		LOG_ERR("SDHC I/O: can't change voltage! error %d old %d new %d", ret,
			host_io->signal_voltage, ios->signal_voltage);
		return ret;
	}

	return 0;
}

/**
 * @brief check for MMC card presence
 *
 * Checks if card is present on the bus.
 *
 * @param dev MMC device
 *
 * @retval 1 card is present
 * @retval 0 card is not present
 * @retval -EINVAL: some of pointers provided to the function are NULL
 */
static int rcar_mmc_get_card_present(const struct device *dev)
{
	const struct mmc_rcar_cfg *cfg;

	if (!dev || !dev->config) {
		return -EINVAL;
	}

	cfg = dev->config;
	if (cfg->non_removable) {
		return 1;
	}

	return !!(rcar_mmc_read_reg32(dev, RCAR_MMC_INFO1) & RCAR_MMC_INFO1_CD);
}

#ifdef CONFIG_RCAR_MMC_SCC_SUPPORT

/* JESD84-B51, 6.6.5.1 Sampling Tuning Sequence for HS200 */
static const uint8_t tun_block_8_bits_bus[] = {
	0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00,
	0xff, 0xff, 0xcc, 0xcc, 0xcc, 0x33, 0xcc, 0xcc,
	0xcc, 0x33, 0x33, 0xcc, 0xcc, 0xcc, 0xff, 0xff,
	0xff, 0xee, 0xff, 0xff, 0xff, 0xee, 0xee, 0xff,
	0xff, 0xff, 0xdd, 0xff, 0xff, 0xff, 0xdd, 0xdd,
	0xff, 0xff, 0xff, 0xbb, 0xff, 0xff, 0xff, 0xbb,
	0xbb, 0xff, 0xff, 0xff, 0x77, 0xff, 0xff, 0xff,
	0x77, 0x77, 0xff, 0x77, 0xbb, 0xdd, 0xee, 0xff,
	0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00,
	0x00, 0xff, 0xff, 0xcc, 0xcc, 0xcc, 0x33, 0xcc,
	0xcc, 0xcc, 0x33, 0x33, 0xcc, 0xcc, 0xcc, 0xff,
	0xff, 0xff, 0xee, 0xff, 0xff, 0xff, 0xee, 0xee,
	0xff, 0xff, 0xff, 0xdd, 0xff, 0xff, 0xff, 0xdd,
	0xdd, 0xff, 0xff, 0xff, 0xbb, 0xff, 0xff, 0xff,
	0xbb, 0xbb, 0xff, 0xff, 0xff, 0x77, 0xff, 0xff,
	0xff, 0x77, 0x77, 0xff, 0x77, 0xbb, 0xdd, 0xee,
};

/*
 * In 4 bit mode the same pattern is used as shown above,
 * but only first 4 bits least significant from every byte is used, examle:
 *    8-bits pattern: 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00 ...
 *                       f     f     0     f     f     f     0     0 ...
 *    4-bits pattern:      0xff        0x0f        0xff        0x00  ...
 */
static const uint8_t tun_block_4_bits_bus[] = {
	0xff, 0x0f, 0xff, 0x00, 0xff, 0xcc, 0xc3, 0xcc,
	0xc3, 0x3c, 0xcc, 0xff, 0xfe, 0xff, 0xfe, 0xef,
	0xff, 0xdf, 0xff, 0xdd, 0xff, 0xfb, 0xff, 0xfb,
	0xbf, 0xff, 0x7f, 0xff, 0x77, 0xf7, 0xbd, 0xef,
	0xff, 0xf0, 0xff, 0xf0, 0x0f, 0xfc, 0xcc, 0x3c,
	0xcc, 0x33, 0xcc, 0xcf, 0xff, 0xef, 0xff, 0xee,
	0xff, 0xfd, 0xff, 0xfd, 0xdf, 0xff, 0xbf, 0xff,
	0xbb, 0xff, 0xf7, 0xff, 0xf7, 0x7f, 0x7b, 0xde,
};

#define RENESAS_TAPNUM 8

/**
 * @brief run MMC tuning
 *
 * MMC cards require signal tuning for UHS modes SDR104, HS200 or HS400.
 * This function allows an application to request the SD host controller
 * to tune the card.
 *
 * @param dev MMC device
 *
 * @retval 0 tuning succeeded (card is ready for commands), otherwise negative number is returned
 */
static int rcar_mmc_execute_tuning(const struct device *dev)
{
	int ret = -ENOTSUP;
	const uint8_t *tun_block_ptr;
	uint8_t tap_idx;
	uint8_t is_mmc_cmd = false;
	struct sdhc_command cmd = {0};
	struct sdhc_data data = {0};
	struct mmc_rcar_data *dev_data;
	uint16_t valid_taps = 0;
	uint16_t smpcmp_bitmask = 0;

	BUILD_ASSERT(sizeof(valid_taps) * 8 >= 2 * RENESAS_TAPNUM);
	BUILD_ASSERT(sizeof(smpcmp_bitmask) * 8 >= 2 * RENESAS_TAPNUM);

	if (!dev) {
		return -EINVAL;
	}

	dev_data = dev->data;
	dev_data->can_retune = 0;

	if (dev_data->host_io.timing == SDHC_TIMING_HS200) {
		cmd.opcode = MMC_SEND_TUNING_BLOCK;
		is_mmc_cmd = true;
	} else if (dev_data->host_io.timing != SDHC_TIMING_HS400) {
		cmd.opcode = SD_SEND_TUNING_BLOCK;
	} else {
		LOG_ERR("%s: tuning isn't possible in HS400 mode, it should be done in HS200",
			dev->name);
		return -EINVAL;
	}

	cmd.response_type = SD_RSP_TYPE_R1;
	cmd.timeout_ms = CONFIG_SD_CMD_TIMEOUT;

	data.blocks = 1;
	data.data = dev_data->tuning_buf;
	data.timeout_ms = CONFIG_SD_DATA_TIMEOUT;
	if (dev_data->host_io.bus_width == SDHC_BUS_WIDTH4BIT) {
		data.block_size = sizeof(tun_block_4_bits_bus);
		tun_block_ptr = tun_block_4_bits_bus;
	} else if (dev_data->host_io.bus_width == SDHC_BUS_WIDTH8BIT) {
		data.block_size = sizeof(tun_block_8_bits_bus);
		tun_block_ptr = tun_block_8_bits_bus;
	} else {
		LOG_ERR("%s: don't support tuning for 1-bit bus width", dev->name);
		return -EINVAL;
	}

	ret = rcar_mmc_enable_clock(dev, false);
	if (ret) {
		return ret;
	}

	/* enable modes SDR104/HS200/HS400 */
	rcar_mmc_write_reg32(dev, RENESAS_SDHI_SCC_DT2FF, 0x300);
	/* SCC sampling clock operation is enabled */
	rcar_mmc_write_reg32(dev, RENESAS_SDHI_SCC_DTCNTL,
			     RENESAS_SDHI_SCC_DTCNTL_TAPEN | RENESAS_TAPNUM << 16);
	/* SCC sampling clock is used */
	rcar_mmc_write_reg32(dev, RENESAS_SDHI_SCC_CKSEL, RENESAS_SDHI_SCC_CKSEL_DTSEL);
	/* SCC sampling clock position correction is disabled */
	rcar_mmc_write_reg32(dev, RENESAS_SDHI_SCC_RVSCNTL, 0);
	/* cleanup errors */
	rcar_mmc_write_reg32(dev, RENESAS_SDHI_SCC_RVSREQ, 0);

	ret = rcar_mmc_enable_clock(dev, true);
	if (ret) {
		return ret;
	}

	/*
	 * two runs is better for detecting TAP ok cases like next:
	 *   - one burn: 0b10000011
	 *   - two burns: 0b1000001110000011
	 * it is more easly to detect 3 OK taps in a row
	 */
	for (tap_idx = 0; tap_idx < 2 * RENESAS_TAPNUM; tap_idx++) {
		/* clear flags */
		rcar_mmc_reset_and_mask_irqs(dev);
		rcar_mmc_write_reg32(dev, RENESAS_SDHI_SCC_TAPSET, tap_idx % RENESAS_TAPNUM);
		memset(dev_data->tuning_buf, 0, data.block_size);
		ret = rcar_mmc_request(dev, &cmd, &data);
		if (ret) {
			LOG_DBG("%s: received an error (%d) during tuning request", dev->name, ret);

			if (is_mmc_cmd) {
				struct sdhc_command stop_cmd = {
					.opcode = SD_STOP_TRANSMISSION,
					.response_type = SD_RSP_TYPE_R1b,
					.timeout_ms = CONFIG_SD_CMD_TIMEOUT,
				};

				rcar_mmc_request(dev, &stop_cmd, NULL);
			}
			continue;
		}

		smpcmp_bitmask |= !rcar_mmc_read_reg32(dev, RENESAS_SDHI_SCC_SMPCMP) << tap_idx;

		if (memcmp(tun_block_ptr, dev_data->tuning_buf, data.block_size)) {
			LOG_DBG("%s: received tuning block doesn't equal to pattert TAP index %u",
				dev->name, tap_idx);
			continue;
		}

		valid_taps |= BIT(tap_idx);

		LOG_DBG("%s: smpcmp_bitmask[%u] 0x%08x", dev->name, tap_idx, smpcmp_bitmask);
	}

	/* both parts of bitmasks have to be the same */
	valid_taps &= (valid_taps >> RENESAS_TAPNUM);
	valid_taps |= (valid_taps << RENESAS_TAPNUM);

	smpcmp_bitmask &= (smpcmp_bitmask >> RENESAS_TAPNUM);
	smpcmp_bitmask |= (smpcmp_bitmask << RENESAS_TAPNUM);

	rcar_mmc_write_reg32(dev, RENESAS_SDHI_SCC_RVSREQ, 0);

	if (!valid_taps) {
		LOG_ERR("%s: there isn't any valid tap during tuning", dev->name);
		goto reset_scc;
	}

	/*
	 * If all of the taps[i] is OK, the sampling clock position is selected by identifying
	 * the change point of data. Change point of the data can be found in the value of
	 * SCC_SMPCMP register
	 */
	if ((valid_taps >> RENESAS_TAPNUM) == (1 << RENESAS_TAPNUM) - 1) {
		valid_taps = smpcmp_bitmask;
	}

	/* do we have 3 set bits in a row at least */
	if (valid_taps & (valid_taps >> 1) & (valid_taps >> 2)) {
		uint32_t max_len_range_pos = 0;
		uint32_t max_bits_in_range = 0;
		uint32_t pos_of_lsb_set = 0;

		/* all bits are set */
		if ((valid_taps >> RENESAS_TAPNUM) == (1 << RENESAS_TAPNUM) - 1) {
			rcar_mmc_write_reg32(dev, RENESAS_SDHI_SCC_TAPSET, 0);

			if (!dev_data->manual_retuning) {
				rcar_mmc_write_reg32(dev, RENESAS_SDHI_SCC_RVSCNTL, 1);
			}
			dev_data->can_retune = 1;
			return 0;
		}

		/* searching the longest range of set bits */
		while (valid_taps) {
			uint32_t num_bits_in_range;
			uint32_t rsh = 0;

			rsh = find_lsb_set(valid_taps) - 1;
			pos_of_lsb_set += rsh;

			/* shift all leading zeros */
			valid_taps >>= rsh;

			num_bits_in_range = find_lsb_set(~valid_taps) - 1;

			/* shift all leading ones */
			valid_taps >>= num_bits_in_range;

			if (max_bits_in_range < num_bits_in_range) {
				max_bits_in_range = num_bits_in_range;
				max_len_range_pos = pos_of_lsb_set;
			}
			pos_of_lsb_set += num_bits_in_range;
		}

		tap_idx = (max_len_range_pos + max_bits_in_range / 2) % RENESAS_TAPNUM;
		rcar_mmc_write_reg32(dev, RENESAS_SDHI_SCC_TAPSET, tap_idx);

		LOG_DBG("%s: valid_taps %08x smpcmp_bitmask %08x tap_idx %u", dev->name, valid_taps,
			smpcmp_bitmask, tap_idx);

		if (!dev_data->manual_retuning) {
			rcar_mmc_write_reg32(dev, RENESAS_SDHI_SCC_RVSCNTL, 1);
		}
		dev_data->can_retune = 1;
		return 0;
	}

reset_scc:
	rcar_mmc_disable_scc(dev);
	return ret;
}

/* retune SCC in case of error during xref */
static int rcar_mmc_retune_if_needed(const struct device *dev, bool request_retune)
{
	struct mmc_rcar_data *dev_data = dev->data;
	int ret = 0;
	uint32_t reg;
	bool scc_pos_err = false;
	uint8_t scc_tapset;

	if (!dev_data->can_retune) {
		return 0;
	}

	reg = rcar_mmc_read_reg32(dev, RENESAS_SDHI_SCC_RVSREQ);
	if (reg & RENESAS_SDHI_SCC_RVSREQ_ERR) {
		scc_pos_err = true;
	}

	scc_tapset = rcar_mmc_read_reg32(dev, RENESAS_SDHI_SCC_TAPSET);

	LOG_DBG("%s: scc_tapset %08x scc_rvsreq %08x request %d is manual tuning %d", dev->name,
		scc_tapset, reg, request_retune, dev_data->manual_retuning);

	if (request_retune || (scc_pos_err && !dev_data->manual_retuning)) {
		return rcar_mmc_execute_tuning(dev);
	}

	rcar_mmc_write_reg32(dev, RENESAS_SDHI_SCC_RVSREQ, 0);

	switch (reg & RENESAS_SDHI_SCC_RVSREQ_REQTAP_MASK) {
	case RENESAS_SDHI_SCC_RVSREQ_REQTAPDOWN:
		scc_tapset = (scc_tapset - 1) % RENESAS_TAPNUM;
		break;
	case RENESAS_SDHI_SCC_RVSREQ_REQTAPUP:
		scc_tapset = (scc_tapset + 1) % RENESAS_TAPNUM;
		break;
	default:
		ret = -EINVAL;
		LOG_ERR("%s: can't perform manual tuning SCC_RVSREQ %08x", dev->name, reg);
		break;
	}

	if (!ret) {
		rcar_mmc_write_reg32(dev, RENESAS_SDHI_SCC_TAPSET, scc_tapset);
	}

	return ret;
}

#endif /* CONFIG_RCAR_MMC_SCC_SUPPORT */

/**
 * @brief Get MMC controller properties
 *
 * Gets host properties from the host controller. Host controller should
 * initialize all values in the @ref sdhc_host_props structure provided.
 *
 * @param dev Renesas MMC device
 * @param props property structure to be filled by MMC driver
 *
 * @retval 0 function succeeded.
 * @retval -EINVAL: some of pointers provided to the function are NULL
 */
static int rcar_mmc_get_host_props(const struct device *dev, struct sdhc_host_props *props)
{
	struct mmc_rcar_data *data;

	if (!props || !dev || !dev->data) {
		return -EINVAL;
	}

	data = dev->data;
	memcpy(props, &data->props, sizeof(*props));
	return 0;
}

static const struct sdhc_driver_api rcar_sdhc_api = {
	.card_busy = rcar_mmc_card_busy,
#ifdef CONFIG_RCAR_MMC_SCC_SUPPORT
	.execute_tuning = rcar_mmc_execute_tuning,
#endif
	.get_card_present = rcar_mmc_get_card_present,
	.get_host_props = rcar_mmc_get_host_props,
	.request = rcar_mmc_request,
	.reset = rcar_mmc_reset,
	.set_io = rcar_mmc_set_io,
};

/* start SD-IF clock at max frequency configured in dts */
static int rcar_mmc_init_start_clk(const struct mmc_rcar_cfg *cfg)
{
	int ret = 0;
	const struct device *cpg_dev = cfg->cpg_dev;
	uintptr_t rate = cfg->max_frequency;

	ret = clock_control_on(cpg_dev, (clock_control_subsys_t *)&cfg->bus_clk);
	if (ret < 0) {
		return ret;
	}

	ret = clock_control_on(cpg_dev, (clock_control_subsys_t *)&cfg->cpg_clk);
	if (ret < 0) {
		return ret;
	}

	ret = clock_control_set_rate(cpg_dev, (clock_control_subsys_t *)&cfg->cpg_clk,
				     (clock_control_subsys_rate_t)rate);
	if (ret < 0) {
		clock_control_off(cpg_dev, (clock_control_subsys_t *)&cfg->cpg_clk);
	}

	rate = MMC_BUS_CLOCK_FREQ;
	ret = clock_control_set_rate(cpg_dev, (clock_control_subsys_t *)&cfg->bus_clk,
				     (clock_control_subsys_rate_t)rate);
	/* SD spec recommends at least 1 ms of delay after start of clock */
	k_msleep(1);

	return ret;
}

static void rcar_mmc_init_host_props(const struct device *dev)
{
	struct mmc_rcar_data *data = dev->data;
	const struct mmc_rcar_cfg *cfg = dev->config;
	struct sdhc_host_props *props = &data->props;
	struct sdhc_host_caps *host_caps = &props->host_caps;

	memset(props, 0, sizeof(*props));

	/* Note: init only properties that are used for mmc/sdhc */

	props->f_max = cfg->max_frequency;
	/*
	 * note: actually, it's possible to get lower frequency
	 *       if we use divider from cpg too
	 */
	props->f_min = (cfg->max_frequency >> 9);

	props->power_delay = 100; /* ms */

	props->is_spi = 0;

	switch (cfg->bus_width) {
	case SDHC_BUS_WIDTH8BIT:
		host_caps->bus_8_bit_support = 1;
	case SDHC_BUS_WIDTH4BIT:
		host_caps->bus_4_bit_support = 1;
	default:
		break;
	}

	host_caps->high_spd_support = 1;
#ifdef CONFIG_RCAR_MMC_SCC_SUPPORT
	host_caps->sdr104_support = cfg->mmc_sdr104_support;
	host_caps->sdr50_support = cfg->uhs_support;
	/* neither Linux nor U-boot support DDR50 mode, that's why we don't support it too */
	host_caps->ddr50_support = 0;
	host_caps->hs200_support = cfg->mmc_hs200_1_8v;
	/* TODO: add support */
	host_caps->hs400_support = 0;
#endif

	host_caps->vol_330_support =
		regulator_is_supported_voltage(cfg->regulator_vqmmc, 3300000, 3300000);
	host_caps->vol_300_support =
		regulator_is_supported_voltage(cfg->regulator_vqmmc, 3000000, 3000000);
	host_caps->vol_180_support =
		regulator_is_supported_voltage(cfg->regulator_vqmmc, 1800000, 1800000);
}

/* reset sampling clock controller registers */
static int rcar_mmc_disable_scc(const struct device *dev)
{
	int ret;
	uint32_t reg;
	struct mmc_rcar_data *data = dev->data;
	uint32_t mmc_clk_ctl = rcar_mmc_read_reg32(dev, RCAR_MMC_CLKCTL);

	/* just to be to be sure that the SD clock is disabled */
	ret = rcar_mmc_enable_clock(dev, false);
	if (ret) {
		return ret;
	}

	/*
	 * Reset SCC registers, need to disable and enable clock
	 * before and after reset
	 */

	/* Disable SCC sampling clock */
	reg = rcar_mmc_read_reg32(dev, RENESAS_SDHI_SCC_CKSEL);
	reg &= ~RENESAS_SDHI_SCC_CKSEL_DTSEL;
	rcar_mmc_write_reg32(dev, RENESAS_SDHI_SCC_CKSEL, reg);

	/* disable hs400 mode & data output timing */
	reg = rcar_mmc_read_reg32(dev, RENESAS_SDHI_SCC_TMPPORT2);
	reg &= ~(RENESAS_SDHI_SCC_TMPPORT2_HS400EN | RENESAS_SDHI_SCC_TMPPORT2_HS400OSEL);
	rcar_mmc_write_reg32(dev, RENESAS_SDHI_SCC_TMPPORT2, reg);

	ret = rcar_mmc_enable_clock(dev, (mmc_clk_ctl & RCAR_MMC_CLKCTL_OFFEN) ? false : true);
	if (ret) {
		return ret;
	}

	/* disable SCC sampling clock position correction */
	reg = rcar_mmc_read_reg32(dev, RENESAS_SDHI_SCC_RVSCNTL);
	reg &= ~RENESAS_SDHI_SCC_RVSCNTL_RVSEN;
	rcar_mmc_write_reg32(dev, RENESAS_SDHI_SCC_RVSCNTL, reg);

	data->can_retune = 0;

	return 0;
}

/* initialize and configure the Renesas MMC controller registers */
static int rcar_mmc_init_controller_regs(const struct device *dev)
{
	int ret = 0;
	uint32_t reg;
	struct mmc_rcar_data *data = dev->data;
	struct sdhc_io ios = {0};

	rcar_mmc_reset(dev);

	/*  Disable SD clock (SD_CLK) output */
	ret = rcar_mmc_enable_clock(dev, false);
	if (ret) {
		return ret;
	}

	/* set transfer data length to 0 */
	rcar_mmc_write_reg32(dev, RCAR_MMC_SIZE, 0);

	/* disable the SD_BUF read/write DMA transfer */
	reg = rcar_mmc_read_reg32(dev, RCAR_MMC_EXTMODE);
	reg &= ~RCAR_MMC_EXTMODE_DMA_EN;
	rcar_mmc_write_reg32(dev, RCAR_MMC_EXTMODE, reg);
	/* mask DMA irqs and clear dma irq flags */
	rcar_mmc_reset_and_mask_irqs(dev);
	/* set system address increment mode selector & 64-bit bus width */
	reg = rcar_mmc_read_reg32(dev, RCAR_MMC_DMA_MODE);
	reg |= RCAR_MMC_DMA_MODE_ADDR_INC | RCAR_MMC_DMA_MODE_WIDTH;
	rcar_mmc_write_reg32(dev, RCAR_MMC_DMA_MODE, reg);

	/* store version of introductory IP */
	data->ver = rcar_mmc_read_reg32(dev, RCAR_MMC_VERSION);
	data->ver &= RCAR_MMC_VERSION_IP;

	/*
	 * set bus width to 1
	 * timeout counter: SDCLK * 2^27
	 * card detect time counter: SDϕ * 2^24
	 */
	reg = rcar_mmc_read_reg32(dev, RCAR_MMC_OPTION);
	reg |= RCAR_MMC_OPTION_WIDTH_MASK | 0xEE;
	rcar_mmc_write_reg32(dev, RCAR_MMC_OPTION, reg);

	/* block count enable */
	rcar_mmc_write_reg32(dev, RCAR_MMC_STOP, RCAR_MMC_STOP_SEC);
	/* number of transfer blocks */
	rcar_mmc_write_reg32(dev, RCAR_MMC_SECCNT, 0);

	/*
	 * SD_BUF0 data swap disabled.
	 * Read/write access to SD_BUF0 can be performed with the 64-bit access.
	 *
	 * Note: when using the DMA, the bus width should be fixed at 64 bits.
	 */
	rcar_mmc_write_reg32(dev, RCAR_MMC_HOST_MODE, 0);
	data->width_access_sd_buf0 = 8;

	/* disable sampling clock controller, it is used for uhs/sdr104, hs200 and hs400 */
	ret = rcar_mmc_disable_scc(dev);
	if (ret) {
		return ret;
	}

	/*
	 * configure divider inside MMC controller
	 * set maximum possible divider
	 */
	ios.clock = data->props.f_min;
	rcar_mmc_set_clk_rate(dev, &ios);

	data->restore_cfg_after_reset = 1;

	return 0;
}

#ifdef CONFIG_RCAR_MMC_DMA_IRQ_DRIVEN_SUPPORT
static void rcar_mmc_irq_handler(const void *arg)
{
	const struct device *dev = arg;

	uint32_t dma_info1 = rcar_mmc_read_reg32(dev, RCAR_MMC_DMA_INFO1);
	uint32_t dma_info2 = rcar_mmc_read_reg32(dev, RCAR_MMC_DMA_INFO2);

	if (dma_info1 || dma_info2) {
		struct mmc_rcar_data *data = dev->data;

		rcar_mmc_write_reg32(dev, RCAR_MMC_DMA_INFO1_MASK, 0xfffffeff);
		rcar_mmc_write_reg32(dev, RCAR_MMC_DMA_INFO2_MASK, ~0);
		k_sem_give(&data->irq_xref_fin);
	} else {
		LOG_WRN("%s: warning: non-dma event triggers irq", dev->name);
	}
}
#endif /* CONFIG_RCAR_MMC_DMA_IRQ_DRIVEN_SUPPORT */

/* initialize and configure the Renesas MMC driver */
static int rcar_mmc_init(const struct device *dev)
{
	int ret = 0;
	struct mmc_rcar_data *data = dev->data;
	const struct mmc_rcar_cfg *cfg = dev->config;

#ifdef CONFIG_RCAR_MMC_DMA_IRQ_DRIVEN_SUPPORT
	ret = k_sem_init(&data->irq_xref_fin, 0, 1);
	if (ret) {
		LOG_ERR("%s: can't init semaphore", dev->name);
		return ret;
	}
#endif

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	/* Configure dt provided device signals when available */
	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("%s: error can't apply pinctrl state", dev->name);
		goto exit_unmap;
	}

	if (!device_is_ready(cfg->cpg_dev)) {
		LOG_ERR("%s: error cpg_dev isn't ready", dev->name);
		ret = -ENODEV;
		goto exit_unmap;
	}

	ret = rcar_mmc_init_start_clk(cfg);
	if (ret < 0) {
		LOG_ERR("%s: error can't turn on the cpg", dev->name);
		goto exit_unmap;
	}

	/* it's needed for SDHC */
	rcar_mmc_init_host_props(dev);

	ret = rcar_mmc_init_controller_regs(dev);
	if (ret) {
		goto exit_disable_clk;
	}

#ifdef CONFIG_RCAR_MMC_DMA_IRQ_DRIVEN_SUPPORT
	cfg->irq_config_func(dev);
#endif /* CONFIG_RCAR_MMC_DMA_IRQ_DRIVEN_SUPPORT */

	LOG_INF("%s: initialize driver, MMC version 0x%hhx", dev->name, data->ver);

	return 0;

exit_disable_clk:
	clock_control_off(cfg->cpg_dev, (clock_control_subsys_t *)&cfg->cpg_clk);

exit_unmap:
#if defined(DEVICE_MMIO_IS_IN_RAM) && defined(CONFIG_MMU)
	k_mem_unmap_phys_bare((uint8_t *)DEVICE_MMIO_GET(dev), DEVICE_MMIO_ROM_PTR(dev)->size);
#endif
	return ret;
}

#ifdef CONFIG_RCAR_MMC_DMA_IRQ_DRIVEN_SUPPORT
#define RCAR_MMC_CONFIG_FUNC(n)                                                                    \
	static void irq_config_func_##n(const struct device *dev)                                  \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), rcar_mmc_irq_handler,       \
			    DEVICE_DT_INST_GET(n), DT_INST_IRQ(n, flags));                         \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}
#define RCAR_MMC_IRQ_CFG_FUNC_INIT(n) .irq_config_func = irq_config_func_##n,
#else
#define RCAR_MMC_IRQ_CFG_FUNC_INIT(n)
#define RCAR_MMC_CONFIG_FUNC(n)
#endif

#define RCAR_MMC_INIT(n)                                                                           \
	static struct mmc_rcar_data mmc_rcar_data_##n;                                             \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	RCAR_MMC_CONFIG_FUNC(n);                                                                   \
	static const struct mmc_rcar_cfg mmc_rcar_cfg_##n = {                                      \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),                                              \
		.cpg_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                  \
		.cpg_clk.module = DT_INST_CLOCKS_CELL_BY_IDX(n, 0, module),                        \
		.cpg_clk.domain = DT_INST_CLOCKS_CELL_BY_IDX(n, 0, domain),                        \
		.bus_clk.module = DT_INST_CLOCKS_CELL_BY_IDX(n, 1, module),                        \
		.bus_clk.domain = DT_INST_CLOCKS_CELL_BY_IDX(n, 1, domain),                        \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.regulator_vqmmc = DEVICE_DT_GET(DT_PHANDLE(DT_DRV_INST(n), vqmmc_supply)),        \
		.regulator_vmmc = DEVICE_DT_GET(DT_PHANDLE(DT_DRV_INST(n), vmmc_supply)),          \
		.max_frequency = DT_INST_PROP(n, max_bus_freq),                                    \
		.non_removable = DT_INST_PROP(n, non_removable),                                   \
		.mmc_hs200_1_8v = DT_INST_PROP(n, mmc_hs200_1_8v),                                 \
		.mmc_hs400_1_8v = DT_INST_PROP(n, mmc_hs400_1_8v),                                 \
		.mmc_sdr104_support = DT_INST_PROP(n, mmc_sdr104_support),                         \
		.uhs_support = 1,                                                                  \
		.bus_width = DT_INST_PROP(n, bus_width),                                           \
		RCAR_MMC_IRQ_CFG_FUNC_INIT(n)};                                                    \
	DEVICE_DT_INST_DEFINE(n, rcar_mmc_init, NULL, &mmc_rcar_data_##n, &mmc_rcar_cfg_##n,       \
			      POST_KERNEL, CONFIG_SDHC_INIT_PRIORITY, &rcar_sdhc_api);

DT_INST_FOREACH_STATUS_OKAY(RCAR_MMC_INIT)
