/*
 * Copyright (c) 2025 Ambiq Micro Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ambiq_i2s

#include <soc.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/cache.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/pm/device.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ambiq_i2s, CONFIG_I2S_LOG_LEVEL);

struct i2s_ambiq_data {
	int inst_idx;
	void *i2s_handler;
	am_hal_i2s_config_t hal_cfg;
	am_hal_i2s_data_format_t i2s_data_format;
	am_hal_i2s_io_signal_t i2s_iocfg;
	am_hal_i2s_transfer_t i2s_transfer;
	struct i2s_config tx_cfg;
	struct i2s_config rx_cfg;
	struct k_msgq tx_dma_queue;
	struct k_msgq rx_dma_queue;
	void *tx_tip_buffer;
	void *rx_tip_buffer;
	bool rx_dma_stop;
	bool tx_dma_stop;
	bool tx_dma_drain;
	enum i2s_dir configured_dir;
	enum i2s_state i2s_state;
	bool i2s_power_on;
};

typedef struct {
	void *dma_buf;
	size_t size;
} dma_msg;

struct i2s_ambiq_cfg {
	void (*irq_config_func)(void);
	const struct pinctrl_dev_config *pcfg;
};

static inline bool i2s_ambiq_dir_includes_rx(enum i2s_dir dir)
{
	return dir == I2S_DIR_RX || dir == I2S_DIR_BOTH;
}

static inline bool i2s_ambiq_dir_includes_tx(enum i2s_dir dir)
{
	return dir == I2S_DIR_TX || dir == I2S_DIR_BOTH;
}

static void i2s_ambiq_dma_stop(const struct device *dev)
{
	struct i2s_ambiq_data *data = dev->data;

	am_hal_i2s_dma_transfer_complete(data->i2s_handler);
	am_hal_i2s_disable(data->i2s_handler);
}

static void i2s_ambiq_dma_reload(const struct device *dev, dma_msg *msg, enum i2s_dir dir)
{
	struct i2s_ambiq_data *data = dev->data;
	am_hal_i2s_transfer_t dma_transfer;
	am_hal_i2s_config_t i2s_hal_cfg;

	if (dir == I2S_DIR_TX) {
		dma_transfer.ui32TxTargetAddr = (uint32_t)msg->dma_buf;
		dma_transfer.ui32TxTotalCount = data->tx_cfg.block_size / 4U;
		dma_transfer.ui32TxTargetAddrReverse = 0xFFFFFFFF;
		data->tx_tip_buffer = msg->dma_buf;
		i2s_hal_cfg.eXfer = AM_HAL_I2S_XFER_TX;
	} else if (dir == I2S_DIR_RX) {
		dma_transfer.ui32RxTargetAddr = (uint32_t)msg->dma_buf;
		dma_transfer.ui32RxTotalCount = data->rx_cfg.block_size / 4U;
		dma_transfer.ui32RxTargetAddrReverse = 0xFFFFFFFF;
		data->rx_tip_buffer = msg->dma_buf;
		i2s_hal_cfg.eXfer = AM_HAL_I2S_XFER_RX;
	}

	am_hal_i2s_dma_transfer_continue(data->i2s_handler, &i2s_hal_cfg, &dma_transfer);
}

static void i2s_ambiq_tx_dmacpl_handler(const struct device *dev)
{
	int ret;
	dma_msg item;
	struct i2s_ambiq_data *data = dev->data;

	if (data->tx_tip_buffer == NULL) {
		goto tx_error_exit;
	}

	k_mem_slab_free(data->tx_cfg.mem_slab, data->tx_tip_buffer);
	data->tx_tip_buffer = NULL;

	ret = k_msgq_get(&data->tx_dma_queue, &item, K_NO_WAIT);
	if (ret < 0) {
		am_hal_i2s_interrupt_enable(data->i2s_handler, AM_HAL_I2S_INT_TXFIFO_EMPTY);
		return;
	}

	i2s_ambiq_dma_reload(dev, &item, I2S_DIR_TX);
	return;

tx_error_exit:
	data->i2s_state = I2S_STATE_ERROR;
	i2s_ambiq_dma_stop(dev);
}

static void i2s_ambiq_tx_fifo_empty_handler(const struct device *dev)
{
	int ret;
	struct i2s_ambiq_data *data = dev->data;

	if (data->i2s_state != I2S_STATE_STOPPING && data->i2s_state != I2S_STATE_RUNNING) {
		return;
	}

	am_hal_delay_us(100);
	am_hal_i2s_interrupt_disable(data->i2s_handler, AM_HAL_I2S_INT_TXFIFO_EMPTY);

#if defined(CONFIG_I2S_TEST_USE_I2S_DIR_BOTH)
	if (data->configured_dir == I2S_DIR_BOTH) {
		return;
	}
#endif

	i2s_ambiq_dma_stop(dev);

	if (data->i2s_state == I2S_STATE_STOPPING) {
#if CONFIG_PM_DEVICE
		if (data->i2s_power_on == true) {
			ret = am_hal_i2s_power_control(data->i2s_handler, AM_HAL_I2S_POWER_OFF,
						       true);
			if (ret != AM_HAL_STATUS_SUCCESS) {
				LOG_ERR("i2s_configure: failed to power off I2S");
				return;
			}
			data->i2s_power_on = false;
		}
#endif
		data->i2s_state = I2S_STATE_READY;
	} else {
		data->i2s_state = I2S_STATE_ERROR;
	}
}

static void i2s_ambiq_rx_dmacpl_handler(const struct device *dev)
{
	int ret;
	dma_msg item;
	struct i2s_ambiq_data *data = dev->data;
#if defined(CONFIG_I2S_TEST_USE_I2S_DIR_BOTH)
	uint32_t ui32Module = data->inst_idx;
#endif

	if (data->rx_tip_buffer == NULL) {
		goto rx_error_exit;
	}

	item.dma_buf = data->rx_tip_buffer;
	item.size = data->rx_cfg.block_size;
	ret = k_msgq_put(&data->rx_dma_queue, &item, K_NO_WAIT);
	if (ret < 0) {
		k_mem_slab_free(data->rx_cfg.mem_slab, data->rx_tip_buffer);

		goto rx_error_exit;
	}

	data->rx_tip_buffer = NULL;

#if defined(CONFIG_I2S_TEST_USE_I2S_DIR_BOTH)
	if ((data->i2s_state != I2S_STATE_STOPPING) || I2Sn(ui32Module)->TXDMASTAT_b.TXDMATIP) {
#else
	if (data->i2s_state != I2S_STATE_STOPPING) {
#endif
		ret = k_mem_slab_alloc(data->rx_cfg.mem_slab, &item.dma_buf, K_NO_WAIT);
		if (ret < 0) {
			goto rx_error_exit;
		}

		i2s_ambiq_dma_reload(dev, &item, I2S_DIR_RX);
	} else {
		i2s_ambiq_dma_stop(dev);
#if CONFIG_PM_DEVICE
		if (data->i2s_power_on == true) {
			ret = am_hal_i2s_power_control(data->i2s_handler, AM_HAL_I2S_POWER_OFF,
						       true);
			if (ret != AM_HAL_STATUS_SUCCESS) {
				LOG_ERR("i2s_configure: failed to power off I2S");
				return;
			}
			data->i2s_power_on = false;
		}
#endif
		data->i2s_state = I2S_STATE_READY;
		return;
	}
	return;

rx_error_exit:
	data->i2s_state = I2S_STATE_ERROR;
	i2s_ambiq_dma_stop(dev);
}

static void i2s_ambiq_isr(const struct device *dev)
{
	uint32_t ui32Status;
	struct i2s_ambiq_data *data = dev->data;

	am_hal_i2s_interrupt_status_get(data->i2s_handler, &ui32Status, true);
	am_hal_i2s_interrupt_clear(data->i2s_handler, ui32Status);

	if (ui32Status & AM_HAL_I2S_INT_TXDMACPL) {
		i2s_ambiq_tx_dmacpl_handler(dev);
	}

	if (ui32Status & AM_HAL_I2S_INT_RXDMACPL) {
		i2s_ambiq_rx_dmacpl_handler(dev);
	}

	if (ui32Status & AM_HAL_I2S_INT_IPB) {
		if (I2Sn(data->inst_idx)->IPBIRPT & AM_HAL_I2S_INT_IPBIRPT_TXE) {
			i2s_ambiq_tx_fifo_empty_handler(dev);
		}
	}
}

static int i2s_ambiq_init(const struct device *dev)
{
	struct i2s_ambiq_data *data = dev->data;
	const struct i2s_ambiq_cfg *config = dev->config;

	int ret = 0;

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Fail to config I2S pins\n");
	}

	data->hal_cfg.eData = &data->i2s_data_format;
	data->hal_cfg.eIO = &data->i2s_iocfg;
	data->hal_cfg.eTransfer = &data->i2s_transfer;

	ret = am_hal_i2s_initialize(data->inst_idx, &data->i2s_handler);
	if (ret != AM_HAL_STATUS_SUCCESS) {
		LOG_ERR("Fail to initialize I2S: %d", ret);
		return -EIO;
	}

#if CONFIG_PM_DEVICE
	MCUCTRL->APBDMACTRL_b.HYSTERESIS = 0x0;

	/* Configure XTAL for deepsleep */
	am_hal_pwrctrl_control(AM_HAL_PWRCTRL_CONTROL_XTAL_PWDN_DEEPSLEEP, 0);
	MCUCTRL->XTALCTRL = 0;
	am_hal_rtc_osc_disable();

	VCOMP->PWDKEY = VCOMP_PWDKEY_PWDKEY_Key;

	am_hal_pwrctrl_sram_memcfg_t SRAMMemCfg = {.eSRAMCfg = AM_HAL_PWRCTRL_SRAM_1M,
						   .eActiveWithMCU = AM_HAL_PWRCTRL_SRAM_NONE,
						   .eActiveWithGFX = AM_HAL_PWRCTRL_SRAM_NONE,
						   .eActiveWithDISP = AM_HAL_PWRCTRL_SRAM_NONE,
						   .eSRAMRetain = AM_HAL_PWRCTRL_SRAM_1M};

	am_hal_pwrctrl_mcu_memory_config_t McuMemCfg = {
		.eROMMode = AM_HAL_PWRCTRL_ROM_AUTO,
		.eDTCMCfg = AM_HAL_PWRCTRL_ITCM32K_DTCM128K,
		.eRetainDTCM = AM_HAL_PWRCTRL_MEMRETCFG_TCMPWDSLP_RETAIN,
		.eNVMCfg = AM_HAL_PWRCTRL_NVM0_ONLY,
		.bKeepNVMOnInDeepSleep = false};

	am_hal_pwrctrl_mcu_memory_config(&McuMemCfg);

	/* Disable SRAM */
	am_hal_pwrctrl_sram_config(&SRAMMemCfg);
#endif

	return ret;
}

#if defined(CONFIG_SOC_APOLLO510)
static int i2s_ambiq_clock_settings_derive(uint32_t i2s_bclk_freq, am_hal_i2s_config_t *hal_cfg)
{
	int ret;
	bool valid_settings_found = false;

	uint32_t clock_divider_pairs[][2] = {{6, 1}, {8, 1}, {6, 3}, {8, 3}};

	ARRAY_FOR_EACH(clock_divider_pairs, i) {
		uint32_t pll_freq =
			clock_divider_pairs[i][0] * clock_divider_pairs[i][1] * i2s_bclk_freq;
		ret = am_hal_clkmgr_clock_config(AM_HAL_CLKMGR_CLK_ID_SYSPLL, pll_freq, NULL);
		if (ret == AM_HAL_STATUS_SUCCESS) {
			hal_cfg->eClock = (clock_divider_pairs[i][0] == 6)
						  ? eAM_HAL_I2S_CLKSEL_PLL_FOUT3
						  : eAM_HAL_I2S_CLKSEL_PLL_FOUT4;
			hal_cfg->eDiv3 = (clock_divider_pairs[i][1] == 3) ? 1 : 0;
			valid_settings_found = true;
			break;
		}
	}

	return (valid_settings_found == true ? 0 : -EINVAL);
}
#else
#error "Unsupported device."
#endif

static void drop_dma_queue(const struct device *dev)
{
	dma_msg item;
	struct i2s_ambiq_data *data = dev->data;

	if (data->tx_tip_buffer != NULL) {
		k_mem_slab_free(data->tx_cfg.mem_slab, data->tx_tip_buffer);
		data->tx_tip_buffer = NULL;
	}

	if (data->rx_tip_buffer != NULL) {
		k_mem_slab_free(data->rx_cfg.mem_slab, data->rx_tip_buffer);
		data->rx_tip_buffer = NULL;
	}

	while (k_msgq_get(&data->tx_dma_queue, &item, K_NO_WAIT) == 0) {
		k_mem_slab_free(data->tx_cfg.mem_slab, item.dma_buf);
	}

	while (k_msgq_get(&data->rx_dma_queue, &item, K_NO_WAIT) == 0) {
		k_mem_slab_free(data->rx_cfg.mem_slab, item.dma_buf);
	}
}

static int i2s_ambiq_dma_start(const struct device *dev, enum i2s_dir dir)
{
	int ret;
	void *buf;
	dma_msg item;
	struct i2s_ambiq_data *data = dev->data;

	if (i2s_ambiq_dir_includes_rx(dir)) {
		ret = k_mem_slab_alloc(data->rx_cfg.mem_slab, &buf, K_NO_WAIT);
		if (ret < 0) {
			return -ENOMEM;
		}
		data->i2s_transfer.ui32RxTargetAddr = (uint32_t)buf;
		data->i2s_transfer.ui32RxTotalCount = data->rx_cfg.block_size / 4U;
		data->i2s_transfer.ui32RxTargetAddrReverse = 0xFFFFFFFF;
		data->rx_tip_buffer = buf;
	}

	if (i2s_ambiq_dir_includes_tx(dir)) {
		ret = k_msgq_get(&data->tx_dma_queue, &item, K_NO_WAIT);
		if (ret < 0) {
			if (i2s_ambiq_dir_includes_rx(dir) && data->rx_tip_buffer != NULL) {
				k_mem_slab_free(data->rx_cfg.mem_slab, data->rx_tip_buffer);
				data->rx_tip_buffer = NULL;
			}
			return -ENOMSG;
		}
		data->i2s_transfer.ui32TxTargetAddr = (uint32_t)item.dma_buf;
		data->i2s_transfer.ui32TxTotalCount = data->tx_cfg.block_size / 4U;
		data->i2s_transfer.ui32TxTargetAddrReverse = 0xFFFFFFFF;
		data->tx_tip_buffer = item.dma_buf;
	}

	if (AM_HAL_STATUS_SUCCESS != am_hal_i2s_enable(data->i2s_handler)) {
		LOG_ERR("i2s_trigger: HAL failed to enable i2s");
		return -EIO;
	}
	am_hal_i2s_dma_configure(data->i2s_handler, &data->hal_cfg, &data->i2s_transfer);
	am_hal_i2s_dma_transfer_start(data->i2s_handler, &data->hal_cfg);

	return 0;
}

static int i2s_ambiq_configure(const struct device *dev, enum i2s_dir dir,
			       const struct i2s_config *i2s_config_in)
{
	int ret;
	uint32_t i2s_bclk_freq;
	uint32_t num_of_channels;
	struct i2s_ambiq_data *data = dev->data;
	const struct i2s_ambiq_cfg *config = dev->config;

	if (data->i2s_state != I2S_STATE_NOT_READY && data->i2s_state != I2S_STATE_READY) {
		LOG_ERR("i2s_configure: invalid state %d", data->i2s_state);
		return -EINVAL;
	}

	if (i2s_config_in->frame_clk_freq == 0U) {
		LOG_ERR("i2s_configure: invalid frame_clk_freq %u", i2s_config_in->frame_clk_freq);
		data->i2s_state = I2S_STATE_NOT_READY;
		return 0;
	}

	ret = am_hal_i2s_power_control(data->i2s_handler, AM_HAL_I2S_POWER_ON, false);
	if (ret != AM_HAL_STATUS_SUCCESS) {
		LOG_ERR("i2s_configure: failed to power on I2S (status %d)", ret);
		return -EIO;
	}

	am_hal_i2s_config_t *hal_cfg = &data->hal_cfg;

	hal_cfg->eData->ePhase = AM_HAL_I2S_DATA_PHASE_SINGLE;
	hal_cfg->eData->ui32ChannelNumbersPhase2 = 0;
	hal_cfg->eData->eChannelLenPhase2 = AM_HAL_I2S_FRAME_WDLEN_8BITS;
	hal_cfg->eData->eSampleLenPhase2 = AM_HAL_I2S_SAMPLE_LENGTH_8BITS;
	hal_cfg->eASRC = 0;

	switch (i2s_config_in->word_size) {
	case 8:
		hal_cfg->eData->eChannelLenPhase1 = AM_HAL_I2S_FRAME_WDLEN_8BITS;
		hal_cfg->eData->eSampleLenPhase1 = AM_HAL_I2S_SAMPLE_LENGTH_8BITS;
		break;
	case 16:
		hal_cfg->eData->eChannelLenPhase1 = AM_HAL_I2S_FRAME_WDLEN_16BITS;
		hal_cfg->eData->eSampleLenPhase1 = AM_HAL_I2S_SAMPLE_LENGTH_16BITS;
		break;
	case 24:
		hal_cfg->eData->eChannelLenPhase1 = AM_HAL_I2S_FRAME_WDLEN_32BITS;
		hal_cfg->eData->eSampleLenPhase1 = AM_HAL_I2S_SAMPLE_LENGTH_24BITS;
		break;
	case 32:
		hal_cfg->eData->eChannelLenPhase1 = AM_HAL_I2S_FRAME_WDLEN_32BITS;
		hal_cfg->eData->eSampleLenPhase1 = AM_HAL_I2S_SAMPLE_LENGTH_32BITS;
		break;
	default:
		LOG_ERR("Invalid word size: %u", i2s_config_in->word_size);
		return -EINVAL;
	}

	switch (i2s_config_in->format) {
	case I2S_FMT_DATA_FORMAT_I2S:
		num_of_channels = 2;
		hal_cfg->eData->eDataJust = AM_HAL_I2S_DATA_JUSTIFIED_LEFT;
		hal_cfg->eData->eDataDelay = 1;
		hal_cfg->eIO->sFsyncPulseCfg.eFsyncPulseType = AM_HAL_I2S_FSYNC_PULSE_ONE_SUBFRAME;
		hal_cfg->eIO->eFyncCpol = AM_HAL_I2S_IO_FSYNC_CPOL_LOW;
		hal_cfg->eIO->eTxCpol = AM_HAL_I2S_IO_TX_CPOL_FALLING;
		hal_cfg->eIO->eRxCpol = AM_HAL_I2S_IO_RX_CPOL_RISING;
		break;
	case I2S_FMT_DATA_FORMAT_PCM_SHORT:
		num_of_channels = i2s_config_in->channels;
		hal_cfg->eData->eDataJust = AM_HAL_I2S_DATA_JUSTIFIED_LEFT;
		hal_cfg->eData->eDataDelay = 1;
		hal_cfg->eIO->sFsyncPulseCfg.eFsyncPulseType = AM_HAL_I2S_FSYNC_PULSE_ONE_BIT_CLOCK;
		hal_cfg->eIO->eFyncCpol = AM_HAL_I2S_IO_FSYNC_CPOL_HIGH;
		hal_cfg->eIO->eTxCpol = AM_HAL_I2S_IO_TX_CPOL_RISING;
		hal_cfg->eIO->eRxCpol = AM_HAL_I2S_IO_RX_CPOL_FALLING;
		break;
	case I2S_FMT_DATA_FORMAT_PCM_LONG:
		num_of_channels = i2s_config_in->channels;
		hal_cfg->eData->eDataJust = AM_HAL_I2S_DATA_JUSTIFIED_LEFT;
		hal_cfg->eData->eDataDelay = 0;
		hal_cfg->eIO->sFsyncPulseCfg.eFsyncPulseType = AM_HAL_I2S_FSYNC_PULSE_ONE_BIT_CLOCK;
		hal_cfg->eIO->eFyncCpol = AM_HAL_I2S_IO_FSYNC_CPOL_HIGH;
		hal_cfg->eIO->eTxCpol = AM_HAL_I2S_IO_TX_CPOL_RISING;
		hal_cfg->eIO->eRxCpol = AM_HAL_I2S_IO_RX_CPOL_FALLING;
		break;
	case I2S_FMT_DATA_FORMAT_LEFT_JUSTIFIED:
		num_of_channels = 2;
		hal_cfg->eData->eDataJust = AM_HAL_I2S_DATA_JUSTIFIED_LEFT;
		hal_cfg->eData->eDataDelay = 0;
		hal_cfg->eIO->sFsyncPulseCfg.eFsyncPulseType = AM_HAL_I2S_FSYNC_PULSE_ONE_SUBFRAME;
		hal_cfg->eIO->eFyncCpol = AM_HAL_I2S_IO_FSYNC_CPOL_HIGH;
		hal_cfg->eIO->eTxCpol = AM_HAL_I2S_IO_TX_CPOL_FALLING;
		hal_cfg->eIO->eRxCpol = AM_HAL_I2S_IO_RX_CPOL_RISING;
		break;
	case I2S_FMT_DATA_FORMAT_RIGHT_JUSTIFIED:
		num_of_channels = 2;
		hal_cfg->eData->eDataJust = AM_HAL_I2S_DATA_JUSTIFIED_RIGHT;
		hal_cfg->eData->eDataDelay = 0;
		hal_cfg->eIO->sFsyncPulseCfg.eFsyncPulseType = AM_HAL_I2S_FSYNC_PULSE_ONE_SUBFRAME;
		hal_cfg->eIO->eFyncCpol = AM_HAL_I2S_IO_FSYNC_CPOL_HIGH;
		hal_cfg->eIO->eTxCpol = AM_HAL_I2S_IO_TX_CPOL_FALLING;
		hal_cfg->eIO->eRxCpol = AM_HAL_I2S_IO_RX_CPOL_RISING;
		break;
	default:
		LOG_ERR("i2s_configure: invalid data format %d", i2s_config_in->format);
		return -EINVAL;
	}

	if (num_of_channels != i2s_config_in->channels) {
		LOG_ERR("i2s_configure: wrong channel number %d with data format %d",
			i2s_config_in->channels, i2s_config_in->format);
		return -EINVAL;
	}

	hal_cfg->eData->ui32ChannelNumbersPhase1 = num_of_channels;
	i2s_bclk_freq = i2s_config_in->frame_clk_freq * num_of_channels *
			(i2s_config_in->word_size > 16U ? 32U : 16U);

	switch (dir) {
	case I2S_DIR_RX:
		if ((data->configured_dir == I2S_DIR_TX) && (data->i2s_state == I2S_STATE_READY) &&
		    IS_ENABLED(CONFIG_I2S_TEST_USE_I2S_DIR_BOTH)) {
			hal_cfg->eXfer = AM_HAL_I2S_XFER_RXTX;
			data->configured_dir = I2S_DIR_BOTH;
		} else {
			hal_cfg->eXfer = AM_HAL_I2S_XFER_RX;
			data->configured_dir = dir;
		}
		break;
	case I2S_DIR_TX:
		if ((data->configured_dir == I2S_DIR_RX) && (data->i2s_state == I2S_STATE_READY) &&
		    IS_ENABLED(CONFIG_I2S_TEST_USE_I2S_DIR_BOTH)) {
			hal_cfg->eXfer = AM_HAL_I2S_XFER_RXTX;
			data->configured_dir = I2S_DIR_BOTH;
		} else {
			hal_cfg->eXfer = AM_HAL_I2S_XFER_TX;
			data->configured_dir = dir;
		}
		break;
	case I2S_DIR_BOTH:
		hal_cfg->eXfer = AM_HAL_I2S_XFER_RXTX;
		data->configured_dir = dir;
		break;
	default:
		LOG_ERR("i2s_configure: invalid transfer direction %d", dir);
		return -EINVAL;
	}

	switch (i2s_config_in->options & (BIT(1) | BIT(2))) {
	case I2S_OPT_BIT_CLK_CONTROLLER | I2S_OPT_FRAME_CLK_CONTROLLER:
		hal_cfg->eMode = AM_HAL_I2S_IO_MODE_MASTER;
		ret = i2s_ambiq_clock_settings_derive(i2s_bclk_freq, hal_cfg);
		if (ret != 0) {
			LOG_ERR("i2s_configure: unsupported bit clock %d Hz", i2s_bclk_freq);
			return -EINVAL;
		}
		break;
	case I2S_OPT_BIT_CLK_TARGET | I2S_OPT_FRAME_CLK_TARGET:
		hal_cfg->eMode = AM_HAL_I2S_IO_MODE_SLAVE;
		break;
	default:
		LOG_ERR("i2s_configure: unsupported option in bits 1-2.");
		return -EINVAL;
	}

	if (AM_HAL_STATUS_SUCCESS != am_hal_i2s_configure(data->i2s_handler, &data->hal_cfg)) {
		LOG_ERR("i2s_confugre: HAL failed to configure i2s");
		return -EINVAL;
	}

	uint32_t txfifo_limit = 16;
	uint32_t rxfifo_limit = 8;

	am_hal_i2s_control(data->i2s_handler, AM_HAL_I2S_REQ_WRITE_TXLOWERLIMIT, &txfifo_limit);
	am_hal_i2s_control(data->i2s_handler, AM_HAL_I2S_REQ_WRITE_RXUPPERLIMIT, &rxfifo_limit);
	config->irq_config_func();

	if (dir == I2S_DIR_RX || dir == I2S_DIR_BOTH) {
		memcpy(&(data->rx_cfg), i2s_config_in, sizeof(struct i2s_config));
	}

	if (dir == I2S_DIR_TX || dir == I2S_DIR_BOTH) {
		memcpy(&(data->tx_cfg), i2s_config_in, sizeof(struct i2s_config));
	}

#if CONFIG_PM_DEVICE
	ret = am_hal_i2s_power_control(data->i2s_handler, AM_HAL_I2S_POWER_OFF, true);
	if (ret != AM_HAL_STATUS_SUCCESS) {
		LOG_ERR("i2s_configure: failed to power off I2S");
		return -EIO;
	}
	data->i2s_power_on = false;
#else
	data->i2s_power_on = true;
#endif

	data->i2s_state = I2S_STATE_READY;
	return 0;
}

static const struct i2s_config *i2s_ambiq_config_get(const struct device *dev, enum i2s_dir dir)
{
	struct i2s_ambiq_data *data = dev->data;

	if (data->i2s_state == I2S_STATE_NOT_READY) {
		return NULL;
	}

	if (dir == I2S_DIR_RX) {
		return &(data->rx_cfg);
	}

	if (dir == I2S_DIR_TX) {
		return &(data->tx_cfg);
	}

	if (dir == I2S_DIR_BOTH && data->configured_dir == I2S_DIR_BOTH) {
		return &(data->tx_cfg);
	}

	return NULL;
}

static bool i2s_ambiq_trigger_dir_valid(struct i2s_ambiq_data *data, enum i2s_dir dir)
{
	if (data->configured_dir == I2S_DIR_BOTH && IS_ENABLED(CONFIG_I2S_TEST_USE_I2S_DIR_BOTH)) {
		return true;
	}

	return dir == data->configured_dir;
}

static int i2s_ambiq_trigger(const struct device *dev, enum i2s_dir dir, enum i2s_trigger_cmd cmd)
{
	uint32_t txfifocnt;
	struct i2s_ambiq_data *data = dev->data;
	int ret = 0;

	if (!i2s_ambiq_trigger_dir_valid(data, dir)) {
		LOG_ERR("trigger dir %d does not match configured dir %d", dir,
			data->configured_dir);
		return -EINVAL;
	}

	switch (cmd) {
	case I2S_TRIGGER_START:
		if (data->i2s_state != I2S_STATE_READY) {
			LOG_ERR("START trigger: invalid state %d", data->i2s_state);
			ret = -EIO;
			break;
		}
		if (data->i2s_power_on == false) {
			ret = am_hal_i2s_power_control(data->i2s_handler, AM_HAL_I2S_POWER_ON,
						       true);
			if (ret != AM_HAL_STATUS_SUCCESS) {
				LOG_ERR("i2s trigger: failed to power on I2S (status %d)", ret);
				return -EIO;
			}
			data->i2s_power_on = true;
		}
		ret = i2s_ambiq_dma_start(dev, data->configured_dir);
		if (ret < 0) {
			LOG_ERR("START trigger failed %d", ret);
			return ret;
		}
#if defined(CONFIG_PM_DEVICE) && defined(CONFIG_SOC_APOLLO510)
		MCUCTRL->XTALHSTRIMS_b.XTALHSIBIASTRIM = 0x18;
		MCUCTRL->XTALHSTRIMS_b.XTALHSIBIASCOMPTRIM = 0x8;
		MCUCTRL->XTALHSTRIMS_b.XTALHSDRIVETRIM = 0x0;

		MCUCTRL->XTALHSCTRL_b.XTALHSIBSTENABLE = 0x0;
#endif

		data->rx_dma_stop = false;
		data->tx_dma_stop = false;
		data->tx_dma_drain = false;
		data->i2s_state = I2S_STATE_RUNNING;
		break;

	case I2S_TRIGGER_STOP:
		if (data->i2s_state != I2S_STATE_RUNNING) {
			LOG_ERR("STOP trigger: invalid state %d", data->i2s_state);
			ret = -EIO;
			break;
		}
		if (i2s_ambiq_dir_includes_rx(dir)) {
			data->rx_dma_stop = true;
		}
		if (i2s_ambiq_dir_includes_tx(dir)) {
			txfifocnt = I2Sn(data->inst_idx)->TXFIFOSTATUS_b.TXFIFOCNT;
			if ((data->tx_tip_buffer == NULL) && (txfifocnt != 0)) {
				am_hal_i2s_interrupt_enable(data->i2s_handler,
							    AM_HAL_I2S_INT_TXFIFO_EMPTY);
			} else {
				data->tx_dma_stop = true;
				data->tx_dma_drain = false;
			}
		}
		data->i2s_state = I2S_STATE_STOPPING;
		ret = 0;
		break;

	case I2S_TRIGGER_DRAIN:
		if (data->i2s_state != I2S_STATE_RUNNING) {
			LOG_ERR("DRAIN trigger: invalid state %d", data->i2s_state);
			ret = -EIO;
			break;
		}
		if (i2s_ambiq_dir_includes_rx(dir)) {
			data->rx_dma_stop = true;
		}
		if (i2s_ambiq_dir_includes_tx(dir)) {
			txfifocnt = I2Sn(data->inst_idx)->TXFIFOSTATUS_b.TXFIFOCNT;
			if ((data->tx_tip_buffer == NULL) && (txfifocnt != 0)) {
				am_hal_i2s_interrupt_enable(data->i2s_handler,
							    AM_HAL_I2S_INT_TXFIFO_EMPTY);
			} else {
				data->tx_dma_stop = false;
				data->tx_dma_drain = true;
			}
		}
		data->i2s_state = I2S_STATE_STOPPING;
		ret = 0;
		break;

	case I2S_TRIGGER_DROP:
		if (data->i2s_state == I2S_STATE_NOT_READY) {
			LOG_ERR("DROP trigger: invalid state %d", data->i2s_state);
			ret = -EIO;
			break;
		}
		drop_dma_queue(dev);
		if (data->i2s_state == I2S_STATE_RUNNING) {
			i2s_ambiq_dma_stop(dev);
		} else {
			am_hal_i2s_disable(data->i2s_handler);
		}
		data->rx_dma_stop = false;
		data->tx_dma_stop = false;
		data->tx_dma_drain = false;

		if (data->i2s_power_on == true) {
			ret = am_hal_i2s_power_control(data->i2s_handler, AM_HAL_I2S_POWER_OFF,
						       true);
			if (ret != AM_HAL_STATUS_SUCCESS) {
				LOG_ERR("i2s_configure: failed to power off I2S");
				return -EIO;
			}
			data->i2s_power_on = false;
		}

		data->i2s_state = I2S_STATE_READY;
		ret = 0;
		break;

	case I2S_TRIGGER_PREPARE:
		if (data->i2s_state != I2S_STATE_ERROR) {
			LOG_ERR("PREPARE trigger: invalid state %d", data->i2s_state);
			ret = -EIO;
			break;
		}

		drop_dma_queue(dev);
		if (data->i2s_power_on == true) {
			ret = am_hal_i2s_power_control(data->i2s_handler, AM_HAL_I2S_POWER_OFF,
						       true);
			if (ret != AM_HAL_STATUS_SUCCESS) {
				LOG_ERR("i2s_configure: failed to power off I2S");
				return -EIO;
			}
			data->i2s_power_on = false;
		}

		data->i2s_state = I2S_STATE_READY;
		ret = 0;
		break;

	default:
		LOG_ERR("i2s_trigger: invalid command: %d", cmd);
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int i2s_ambiq_write(const struct device *dev, void *buffer, size_t size)
{
	int ret;
	struct i2s_ambiq_data *data = dev->data;
	dma_msg tx_dma_msg = {.dma_buf = buffer, .size = size};

	if ((data->i2s_state != I2S_STATE_RUNNING) && (data->i2s_state != I2S_STATE_READY)) {
		LOG_ERR("i2s_write: invalid state %d", data->i2s_state);
		return -EIO;
	}

	if (size > data->tx_cfg.block_size) {
		LOG_ERR("i2s_write: size %d exceeds the max block size %d", size,
			data->tx_cfg.block_size);
		return -EIO;
	}

	if ((size % 4) != 0) {
		LOG_ERR("i2s_write: size %d is not the multiple of 4", size);
		return -EIO;
	}

	ret = k_msgq_put(&data->tx_dma_queue, &tx_dma_msg, SYS_TIMEOUT_MS(data->tx_cfg.timeout));
	if (ret < 0) {
		LOG_ERR("i2s_write: k_msgq_put failed with code %d", ret);
		return ret;
	}

#if CONFIG_I2S_AMBIQ_HANDLE_CACHE
	if (!buf_in_nocache((uintptr_t)buffer, size)) {
		sys_cache_data_flush_range((uint32_t *)buffer, size);
	}
#endif /* CONFIG_I2S_AMBIQ_HANDLE_CACHE */

	if (data->i2s_state == I2S_STATE_RUNNING && data->tx_tip_buffer == NULL) {
		dma_msg item;

		ret = k_msgq_get(&data->tx_dma_queue, &item, K_NO_WAIT);
		if (ret < 0) {
			/* No more TX job in queue, this shouldn't happen here. */
			return 0;
		}
		i2s_ambiq_dma_reload(dev, &item, I2S_DIR_TX);
	}

	return ret;
}

static int i2s_ambiq_read(const struct device *dev, void **buffer, size_t *size)
{
	int ret;
	struct i2s_ambiq_data *data = dev->data;
	dma_msg rx_dma_msg;

	if (data->i2s_state == I2S_STATE_NOT_READY) {
		LOG_ERR("i2s_read: invalid state %d", data->i2s_state);
		return -EIO;
	}

	ret = k_msgq_get(&data->rx_dma_queue, &rx_dma_msg, SYS_TIMEOUT_MS(data->rx_cfg.timeout));
	if (ret < 0) {
		if (data->i2s_state == I2S_STATE_ERROR) {
			LOG_ERR("i2s_read: k_msgq_get with invalid state %d", data->i2s_state);
			return -EIO;
		}
		LOG_ERR("i2s_read: k_msgq_get failed with code %d", ret);
		return ret;
	}

#if CONFIG_I2S_AMBIQ_HANDLE_CACHE
	if (!buf_in_nocache((uintptr_t)rx_dma_msg.dma_buf, rx_dma_msg.size)) {
		sys_cache_data_invd_range(rx_dma_msg.dma_buf, rx_dma_msg.size);
	}
#endif /* CONFIG_I2S_AMBIQ_HANDLE_CACHE */

	*buffer = rx_dma_msg.dma_buf;
	*size = rx_dma_msg.size;

	return ret;
}

static DEVICE_API(i2s, i2s_ambiq_driver_api) = {
	.configure = i2s_ambiq_configure,
	.read = i2s_ambiq_read,
	.write = i2s_ambiq_write,
	.config_get = i2s_ambiq_config_get,
	.trigger = i2s_ambiq_trigger,
};

/**
 * @brief I2S Power Management support
 *
 * Power Management: PM Only (2) - This driver implements PM_DEVICE callbacks for
 * system suspend/resume but does not use runtime PM (no pm_device_runtime_get/put).
 *
 * The I2S peripheral must remain powered during active audio streaming. When the
 * system suspends, the HAL's power control function is used to power down the I2S
 * hardware to save power during deep sleep. Configuration is retained and restored
 * on resume.
 *
 * This driver does not benefit from runtime PM because I2S audio streaming is
 * continuous when active, and applications control start/stop at the I2S API level.
 */
#ifdef CONFIG_PM_DEVICE
static int i2s_ambiq_pm_action(const struct device *dev, enum pm_device_action action)
{
	struct i2s_ambiq_data *data = dev->data;
	uint32_t ret;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		ret = am_hal_i2s_power_control(data->i2s_handler, AM_HAL_I2S_POWER_ON, false);
		if (ret != AM_HAL_STATUS_SUCCESS) {
			LOG_ERR("I2S power on failed: %d", ret);
			return -EIO;
		}
		break;

	case PM_DEVICE_ACTION_SUSPEND:
		/* Reject suspend if actively streaming or stopping to prevent DMA corruption */
		if (data->i2s_state == I2S_STATE_RUNNING || data->i2s_state == I2S_STATE_STOPPING) {
			return -EBUSY;
		}
		ret = am_hal_i2s_power_control(data->i2s_handler, AM_HAL_I2S_POWER_OFF, false);
		if (ret != AM_HAL_STATUS_SUCCESS) {
			LOG_ERR("I2S power off failed: %d", ret);
			return -EIO;
		}
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif /* CONFIG_PM_DEVICE */

#define AMBIQ_I2S_DEFINE(n)                                                                        \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static void i2s_irq_config_func_##n(void)                                                  \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), i2s_ambiq_isr,              \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}                                                                                          \
	static dma_msg tx_dma_msgs_##n[CONFIG_I2S_AMBIQ_TX_BLOCK_COUNT];                           \
	static dma_msg rx_dma_msgs_##n[CONFIG_I2S_AMBIQ_RX_BLOCK_COUNT];                           \
	static struct i2s_ambiq_data i2s_ambiq_data##n = {                                         \
		.inst_idx = n,                                                                     \
		.configured_dir = I2S_DIR_RX,                                                      \
		.i2s_state = I2S_STATE_NOT_READY,                                                  \
		.tx_dma_queue = Z_MSGQ_INITIALIZER(i2s_ambiq_data##n.tx_dma_queue,                 \
						   (char *)tx_dma_msgs_##n, sizeof(dma_msg),       \
						   CONFIG_I2S_AMBIQ_TX_BLOCK_COUNT),               \
		.rx_dma_queue = Z_MSGQ_INITIALIZER(i2s_ambiq_data##n.rx_dma_queue,                 \
						   (char *)rx_dma_msgs_##n, sizeof(dma_msg),       \
						   CONFIG_I2S_AMBIQ_RX_BLOCK_COUNT),               \
		.i2s_power_on = false,                                                             \
	};                                                                                         \
	static const struct i2s_ambiq_cfg i2s_ambiq_cfg##n = {                                     \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.irq_config_func = i2s_irq_config_func_##n,                                        \
	};                                                                                         \
	PM_DEVICE_DT_INST_DEFINE(n, i2s_ambiq_pm_action);                                          \
	DEVICE_DT_INST_DEFINE(n, i2s_ambiq_init, PM_DEVICE_DT_INST_GET(n), &i2s_ambiq_data##n,     \
			      &i2s_ambiq_cfg##n, POST_KERNEL, CONFIG_I2S_INIT_PRIORITY,            \
			      &i2s_ambiq_driver_api);

DT_INST_FOREACH_STATUS_OKAY(AMBIQ_I2S_DEFINE)
