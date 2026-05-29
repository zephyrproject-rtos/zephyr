/* SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors */
/*
 * Copyright (c) 2026 AMD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/dma.h>
#ifdef CONFIG_SOC_ACP_7_0
#include <acp70_fw_scratch_mem.h>
#include <acp70_chip_offsets.h>
#include <acp70_chip_reg.h>
#endif
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>

/* used for driver binding */
#define DT_DRV_COMPAT amd_acp_sdw_dma

LOG_MODULE_REGISTER(sw_dma_acp, CONFIG_DMA_LOG_LEVEL);

#ifdef CONFIG_SOC_ACP_7_0
#define DMA_CH_COUNT 12

#define SW0_AUDIO_FIFO_SIZE    128
#define SW0_AUDIO_TX_FIFO_ADDR 0
#define SW0_AUDIO_RX_FIFO_ADDR (SW0_AUDIO_TX_FIFO_ADDR + SW0_AUDIO_FIFO_SIZE)

#define SW0_BT_FIFO_SIZE    128
#define SW0_BT_TX_FIFO_ADDR (SW0_AUDIO_RX_FIFO_ADDR + SW0_AUDIO_FIFO_SIZE)
#define SW0_BT_RX_FIFO_ADDR (SW0_BT_TX_FIFO_ADDR + SW0_BT_FIFO_SIZE)

#define SW0_HS_FIFO_SIZE    128
#define SW0_HS_TX_FIFO_ADDR (SW0_BT_RX_FIFO_ADDR + SW0_BT_FIFO_SIZE)
#define SW0_HS_RX_FIFO_ADDR (SW0_HS_TX_FIFO_ADDR + SW0_HS_FIFO_SIZE)

/* initialization of soundwire-1 fifos (Audio, BT and HS) */
#define SW1_AUDIO_FIFO_SIZE    128
#define SW1_AUDIO_TX_FIFO_ADDR (SW0_HS_RX_FIFO_ADDR + SW0_HS_FIFO_SIZE)
#define SW1_AUDIO_RX_FIFO_ADDR (SW1_AUDIO_TX_FIFO_ADDR + SW1_AUDIO_FIFO_SIZE)

#define SW1_BT_FIFO_SIZE    128
#define SW1_BT_TX_FIFO_ADDR (SW1_AUDIO_RX_FIFO_ADDR + SW1_AUDIO_FIFO_SIZE)
#define SW1_BT_RX_FIFO_ADDR (SW1_BT_TX_FIFO_ADDR + SW1_BT_FIFO_SIZE)

#define SW1_HS_FIFO_SIZE    128
#define SW1_HS_TX_FIFO_ADDR (SW1_BT_RX_FIFO_ADDR + SW1_BT_FIFO_SIZE)
#define SW1_HS_RX_FIFO_ADDR (SW1_HS_TX_FIFO_ADDR + SW1_HS_FIFO_SIZE)

#define ACP_SDW_INSTANCES          2
#define ACP_SDW_INDEX_INSTANCE     1
#define ACP_SDW_INDEX_SUBINDEX     2
#define ACP_SDW_INDEX_LIN_POS_LOW  3
#define ACP_SDW_INDEX_LIN_POS_HIGH 4
#define ACP_SDW_PINS_PER_INSTANCE  6
#define ACP_SDW_INVALID            0xFFFF
#define ACP_ACLK_FREQ_HZ           600000000UL
#define ACP_SDW_PIN_RANGE          64

struct sdw_pin_data *pin_data[ACP_SDW_PINS_PER_INSTANCE] = {0};

struct sdw_dev_register {
	uint32_t acp_sdw_dev_en;
	uint32_t acp_sdw_dev_en_status;
	uint32_t acp_sdw_dev_fifo_addr;
	uint32_t fifo_addr;
	uint32_t acp_sdw_dev_fifo_size;
	uint32_t fifo_size;
	uint32_t acp_sdw_dev_ringbuff_addr;
	uint32_t acp_sdw_dev_ringbuff_size;
	uint32_t acp_sdw_dev_dma_size;
	uint32_t acp_sdw_dev_dma_watermark;
	uint32_t acp_sdw_dev_dma_intr_status;
	uint32_t acp_sdw_dev_dma_intr_cntl;
	uint32_t statusindex;
};

static uint64_t prev_tx_pos_sdw, curr_tx_pos_sdw, curr_rx_pos_sdw, prev_rx_pos_sdw;
static uint64_t tx_low_sdw, tx_high_sdw;
static uint64_t rx_low_sdw, rx_high_sdw;
static uint32_t sdw_buff_size_playback;
static uint32_t sdw_buff_size_capture;
static uint32_t sdw_pin_map_7_0[2][6] = {{0, 1, 2, 3, 4, 5}, {0, 1, 2, 3, 4, 5}};
static uint32_t linearposition_low_registers[2][6] = {
	/* Instance 0 */
	{
		ACP_AUDIO_TX_LINEARPOSITIONCNTR_LOW, /* index 0 */
		ACP_BT_TX_LINEARPOSITIONCNTR_LOW,    /* index 1 */
		ACP_HS_TX_LINEARPOSITIONCNTR_LOW,    /* index 2 */
		ACP_AUDIO_RX_LINEARPOSITIONCNTR_LOW, /* index 3 */
		ACP_BT_RX_LINEARPOSITIONCNTR_LOW,    /* index 4 */
		ACP_HS_RX_LINEARPOSITIONCNTR_LOW     /* index 5 */
	},
	/* Instance 1 */
	{
		ACP_P1_AUDIO_TX_LINEARPOSITIONCNTR_LOW, /* index 0 (pin 64) */
		ACP_P1_BT_TX_LINEARPOSITIONCNTR_LOW,    /* index 1 (pin 65) */
		ACP_P1_HS_TX_LINEARPOSITIONCNTR_LOW,    /* index 2 (pin 66) */
		ACP_P1_AUDIO_RX_LINEARPOSITIONCNTR_LOW, /* index 3 (pin 67) */
		ACP_P1_BT_RX_LINEARPOSITIONCNTR_LOW,    /* index 4 (pin 68) */
		ACP_P1_HS_RX_LINEARPOSITIONCNTR_LOW     /* index 5 (pin 69) */
	}};

/* Linear Position Counter HIGH registers array [instance][index] */
static uint32_t linearposition_high_registers[2][6] = {
	/* Instance 0 */
	{
		ACP_AUDIO_TX_LINEARPOSITIONCNTR_HIGH, /* index 0 */
		ACP_BT_TX_LINEARPOSITIONCNTR_HIGH,    /* index 1 */
		ACP_HS_TX_LINEARPOSITIONCNTR_HIGH,    /* index 2 */
		ACP_AUDIO_RX_LINEARPOSITIONCNTR_HIGH, /* index 3 */
		ACP_BT_RX_LINEARPOSITIONCNTR_HIGH,    /* index 4 */
		ACP_HS_RX_LINEARPOSITIONCNTR_HIGH     /* index 5 */
	},
	/* Instance 1 */
	{
		ACP_P1_AUDIO_TX_LINEARPOSITIONCNTR_HIGH, /* index 0 (pin 64) */
		ACP_P1_BT_TX_LINEARPOSITIONCNTR_HIGH,    /* index 1 (pin 65) */
		ACP_P1_HS_TX_LINEARPOSITIONCNTR_HIGH,    /* index 2 (pin 66) */
		ACP_P1_AUDIO_RX_LINEARPOSITIONCNTR_HIGH, /* index 3 (pin 67) */
		ACP_P1_BT_RX_LINEARPOSITIONCNTR_HIGH,    /* index 4 (pin 68) */
		ACP_P1_HS_RX_LINEARPOSITIONCNTR_HIGH     /* index 5 (pin 69) */
	}};

static uint32_t acp_sdw_index_get(uint32_t pin_index, uint32_t type)
{
	uint32_t acp_sdw_instance = 0;

	if (pin_index == ACP_SDW_INVALID) {
		return pin_index;
	}
	/* Corrected condition checks for acp_sdw_instance */
	if (pin_index < ACP_SDW_PIN_RANGE) {
		acp_sdw_instance = 0;
	} else if (pin_index < ACP_SDW_PIN_RANGE * 2) {
		acp_sdw_instance = 1;
	}
	if (type == ACP_SDW_INDEX_INSTANCE) {
		return acp_sdw_instance;
	}
	if (type == ACP_SDW_INDEX_SUBINDEX) {
		return sdw_pin_map_7_0[acp_sdw_instance][pin_index % 64];
	}
	if (type == ACP_SDW_INDEX_LIN_POS_LOW) {
		return linearposition_low_registers[acp_sdw_instance][pin_index % 64];
	}
	if (type == ACP_SDW_INDEX_LIN_POS_HIGH) {
		return linearposition_high_registers[acp_sdw_instance][pin_index % 64];
	}

	return 0;
}

static struct sdw_dev_register sdw_dev[ACP_SDW_INSTANCES][ACP_SDW_PINS_PER_INSTANCE] = {
	{{ACP_SW_AUDIO_TX_EN, ACP_SW_AUDIO_TX_EN_STATUS, ACP_AUDIO_TX_FIFOADDR,
	  SW0_AUDIO_TX_FIFO_ADDR, ACP_AUDIO_TX_FIFOSIZE, SW0_AUDIO_FIFO_SIZE,
	  ACP_AUDIO_TX_RINGBUFADDR, ACP_AUDIO_TX_RINGBUFSIZE, ACP_AUDIO_TX_DMA_SIZE,
	  ACP_AUDIO_TX_INTR_WATERMARK_SIZE, ACP_DSP0_INTR_STAT, ACP_DSP0_INTR_CNTL, 5},
	 {ACP_SW_BT_TX_EN, ACP_SW_BT_TX_EN_STATUS, ACP_BT_TX_FIFOADDR, SW0_BT_TX_FIFO_ADDR,
	  ACP_BT_TX_FIFOSIZE, SW0_BT_FIFO_SIZE, ACP_BT_TX_RINGBUFADDR, ACP_BT_TX_RINGBUFSIZE,
	  ACP_BT_TX_DMA_SIZE, ACP_BT_TX_INTR_WATERMARK_SIZE, ACP_DSP0_INTR_STAT, ACP_DSP0_INTR_CNTL,
	  3},
	 {ACP_SW_HS_TX_EN, ACP_SW_HS_TX_EN_STATUS, ACP_HS_TX_FIFOADDR, SW0_HS_TX_FIFO_ADDR,
	  ACP_HS_TX_FIFOSIZE, SW0_HS_FIFO_SIZE, ACP_HS_TX_RINGBUFADDR, ACP_HS_TX_RINGBUFSIZE,
	  ACP_HS_TX_DMA_SIZE, ACP_HS_TX_INTR_WATERMARK_SIZE, ACP_DSP0_INTR_STAT, ACP_DSP0_INTR_CNTL,
	  1},
	 {ACP_SW_AUDIO_RX_EN, ACP_SW_AUDIO_RX_EN_STATUS, ACP_AUDIO_RX_FIFOADDR,
	  SW0_AUDIO_RX_FIFO_ADDR, ACP_AUDIO_RX_FIFOSIZE, SW0_AUDIO_FIFO_SIZE,
	  ACP_AUDIO_RX_RINGBUFADDR, ACP_AUDIO_RX_RINGBUFSIZE, ACP_AUDIO_RX_DMA_SIZE,
	  ACP_AUDIO_RX_INTR_WATERMARK_SIZE, ACP_DSP0_INTR_STAT, ACP_DSP0_INTR_CNTL, 4},
	 {ACP_SW_BT_RX_EN, ACP_SW_BT_RX_EN_STATUS, ACP_BT_RX_FIFOADDR, SW0_BT_RX_FIFO_ADDR,
	  ACP_BT_RX_FIFOSIZE, SW0_BT_FIFO_SIZE, ACP_BT_RX_RINGBUFADDR, ACP_BT_RX_RINGBUFSIZE,
	  ACP_BT_RX_DMA_SIZE, ACP_BT_RX_INTR_WATERMARK_SIZE, ACP_DSP0_INTR_STAT, ACP_DSP0_INTR_CNTL,
	  2},
	 {ACP_SW_HS_RX_EN, ACP_SW_HS_RX_EN_STATUS, ACP_HS_RX_FIFOADDR, SW0_HS_RX_FIFO_ADDR,
	  ACP_HS_RX_FIFOSIZE, SW0_HS_FIFO_SIZE, ACP_HS_RX_RINGBUFADDR, ACP_HS_RX_RINGBUFSIZE,
	  ACP_HS_RX_DMA_SIZE, ACP_HS_RX_INTR_WATERMARK_SIZE, ACP_DSP0_INTR_STAT, ACP_DSP0_INTR_CNTL,
	  0}},
	{{ACP_P1_SW_AUDIO_TX_EN, ACP_P1_SW_AUDIO_TX_EN_STATUS, ACP_P1_AUDIO_TX_FIFOADDR,
	  SW1_AUDIO_TX_FIFO_ADDR, ACP_P1_AUDIO_TX_FIFOSIZE, SW1_AUDIO_FIFO_SIZE,
	  ACP_P1_AUDIO_TX_RINGBUFADDR, ACP_P1_AUDIO_TX_RINGBUFSIZE, ACP_P1_AUDIO_TX_DMA_SIZE,
	  ACP_P1_AUDIO_TX_INTR_WATERMARK_SIZE, ACP_DSP0_INTR_STAT1, ACP_DSP0_INTR_CNTL1, 5},
	 {ACP_P1_SW_BT_TX_EN, ACP_P1_SW_BT_TX_EN_STATUS, ACP_P1_BT_TX_FIFOADDR, SW1_BT_TX_FIFO_ADDR,
	  ACP_P1_BT_TX_FIFOSIZE, SW1_BT_FIFO_SIZE, ACP_P1_BT_TX_RINGBUFADDR,
	  ACP_P1_BT_TX_RINGBUFSIZE, ACP_P1_BT_TX_DMA_SIZE, ACP_P1_BT_TX_INTR_WATERMARK_SIZE,
	  ACP_DSP0_INTR_STAT1, ACP_DSP0_INTR_CNTL1, 3},
	 {ACP_P1_SW_HEADSET_TX_EN, ACP_P1_SW_HEADSET_TX_EN_STATUS, ACP_P1_HS_TX_FIFOADDR,
	  SW1_HS_TX_FIFO_ADDR, ACP_P1_HS_TX_FIFOSIZE, SW1_HS_FIFO_SIZE, ACP_P1_HS_TX_RINGBUFADDR,
	  ACP_P1_HS_TX_RINGBUFSIZE, ACP_P1_HS_TX_DMA_SIZE, ACP_P1_HS_TX_INTR_WATERMARK_SIZE,
	  ACP_DSP0_INTR_STAT1, ACP_DSP0_INTR_CNTL1, 1},
	 {ACP_P1_SW_AUDIO_RX_EN, ACP_P1_SW_AUDIO_RX_EN_STATUS, ACP_P1_AUDIO_RX_FIFOADDR,
	  SW1_AUDIO_RX_FIFO_ADDR, ACP_P1_AUDIO_RX_FIFOSIZE, SW1_AUDIO_FIFO_SIZE,
	  ACP_P1_AUDIO_RX_RINGBUFADDR, ACP_P1_AUDIO_RX_RINGBUFSIZE, ACP_P1_AUDIO_RX_DMA_SIZE,
	  ACP_P1_AUDIO_RX_INTR_WATERMARK_SIZE, ACP_DSP0_INTR_STAT1, ACP_DSP0_INTR_CNTL1, 4},
	 {ACP_P1_SW_BT_RX_EN, ACP_P1_SW_BT_RX_EN_STATUS, ACP_P1_BT_RX_FIFOADDR, SW1_BT_RX_FIFO_ADDR,
	  ACP_P1_BT_RX_FIFOSIZE, SW1_BT_FIFO_SIZE, ACP_P1_BT_RX_RINGBUFADDR,
	  ACP_P1_BT_RX_RINGBUFSIZE, ACP_P1_BT_RX_DMA_SIZE, ACP_P1_BT_RX_INTR_WATERMARK_SIZE,
	  ACP_DSP0_INTR_STAT1, ACP_DSP0_INTR_CNTL1, 2},
	 {ACP_P1_SW_HEADSET_RX_EN, ACP_P1_SW_HEADSET_RX_EN_STATUS, ACP_P1_HS_RX_FIFOADDR,
	  SW1_HS_RX_FIFO_ADDR, ACP_P1_HS_RX_FIFOSIZE, SW1_HS_FIFO_SIZE, ACP_P1_HS_RX_RINGBUFADDR,
	  ACP_P1_HS_RX_RINGBUFSIZE, ACP_P1_HS_RX_DMA_SIZE, ACP_P1_HS_RX_INTR_WATERMARK_SIZE,
	  ACP_DSP0_INTR_STAT1, ACP_DSP0_INTR_CNTL1, 0}}};
#endif

static int acp_sdw_dma_config(const struct device *dev, uint32_t channel, struct dma_config *cfg)
{
	uint32_t sdw_ringbuff_addr;
	uint32_t sdw_fifo_addr;
	struct acp_dma_dev_data *dev_data = dev->data;

	dev_data->chan_data[channel].direction = cfg->channel_direction;
	struct sdw_pin_data *pin_data_temp;

	pin_data_temp = (struct sdw_pin_data *)dev_data->dai_index_ptr;
#ifdef CONFIG_SOC_ACP_7_0
	uint32_t pin_number = pin_data_temp->pin_num;

	for (int i = 0; i < ACP_SDW_PINS_PER_INSTANCE; i++) {
		if (pin_data[i]->dma_channel == ACP_SDW_INVALID &&
		    (i == pin_data_temp->pin_num % 64)) {
			pin_data[i]->dma_channel = channel;
			pin_data[i]->pin_dir = cfg->channel_direction;
			pin_data[i]->pin_num = pin_number;
			pin_data[i]->index = acp_sdw_index_get(pin_number, ACP_SDW_INDEX_SUBINDEX);
			pin_data[i]->instance =
				acp_sdw_index_get(pin_number, ACP_SDW_INDEX_INSTANCE);
			pin_data[i]->acp_sdw_index =
				acp_sdw_index_get(pin_number, ACP_SDW_INDEX_SUBINDEX);
		}
	}

	uint32_t acp_sdw_index = ACP_SDW_INVALID, instance = ACP_SDW_INVALID;

	for (int i = 0; i < ACP_SDW_PINS_PER_INSTANCE; i++) {
		if (pin_data[i]->dma_channel == channel &&
		    pin_data[i]->pin_dir == cfg->channel_direction) {
			acp_sdw_index = pin_data[i]->acp_sdw_index;
			instance = pin_data[i]->instance;
			break;
		}
	}

	if (acp_sdw_index == ACP_SDW_INVALID) {
		return -EINVAL;
	}

	switch (cfg->channel_direction) {
	case MEMORY_TO_PERIPHERAL:
		sdw_buff_size_playback = cfg->head_block[0].block_size * cfg->block_count;
		/* SDW Transmit FIFO Address and FIFO Size */
		sdw_fifo_addr = sdw_dev[instance][acp_sdw_index].fifo_addr;
		io_reg_write(PU_REGISTER_BASE +
				     sdw_dev[instance][acp_sdw_index].acp_sdw_dev_fifo_addr,
			     sdw_fifo_addr);
		io_reg_write(PU_REGISTER_BASE +
				     sdw_dev[instance][acp_sdw_index].acp_sdw_dev_fifo_size,
			     (uint32_t)(sdw_dev[instance][acp_sdw_index].fifo_size));
		/* Transmit RINGBUFFER Address and size */
		cfg->head_block->source_address =
			(cfg->head_block->source_address & ACP_DRAM_ADDRESS_MASK);
		sdw_ringbuff_addr = (cfg->head_block->source_address | ACP_SRAM);
		io_reg_write(PU_REGISTER_BASE +
				     sdw_dev[instance][acp_sdw_index].acp_sdw_dev_ringbuff_addr,
			     sdw_ringbuff_addr);
		io_reg_write(PU_REGISTER_BASE +
				     sdw_dev[instance][acp_sdw_index].acp_sdw_dev_ringbuff_size,
			     sdw_buff_size_playback);
		/* Transmit DMA transfer size in bytes */
		io_reg_write(PU_REGISTER_BASE +
				     sdw_dev[instance][acp_sdw_index].acp_sdw_dev_dma_size,
			     (uint32_t)(ACP_DMA_TRANS_SIZE));
		/* Watermark size for SW transmit FIFO */
		io_reg_write(PU_REGISTER_BASE +
				     sdw_dev[instance][acp_sdw_index].acp_sdw_dev_dma_watermark,
			     (sdw_buff_size_playback >> 1));
		break;
	case PERIPHERAL_TO_MEMORY:
		sdw_buff_size_capture = cfg->head_block[0].block_size * cfg->block_count;
		/* SDW Receive FIFO Address and FIFO Size*/
		sdw_fifo_addr = sdw_dev[instance][acp_sdw_index].fifo_addr;
		io_reg_write(PU_REGISTER_BASE +
				     sdw_dev[instance][acp_sdw_index].acp_sdw_dev_fifo_addr,
			     sdw_fifo_addr);
		io_reg_write(PU_REGISTER_BASE +
				     sdw_dev[instance][acp_sdw_index].acp_sdw_dev_fifo_size,
			     (uint32_t)(sdw_dev[instance][acp_sdw_index].fifo_size));
		/* Receive RINGBUFFER Address and size */
		cfg->head_block->dest_address &= ACP_DRAM_ADDRESS_MASK;
		sdw_ringbuff_addr = (cfg->head_block->dest_address | ACP_SRAM);
		io_reg_write(PU_REGISTER_BASE +
				     sdw_dev[instance][acp_sdw_index].acp_sdw_dev_ringbuff_addr,
			     sdw_ringbuff_addr);
		io_reg_write(PU_REGISTER_BASE +
				     sdw_dev[instance][acp_sdw_index].acp_sdw_dev_ringbuff_size,
			     sdw_buff_size_capture);
		/* Receive DMA transfer size in bytes */
		io_reg_write(PU_REGISTER_BASE +
				     sdw_dev[instance][acp_sdw_index].acp_sdw_dev_dma_size,
			     (uint32_t)(ACP_DMA_TRANS_SIZE));
		/* Watermark size for SDW receive fifo */
		io_reg_write(PU_REGISTER_BASE +
				     sdw_dev[instance][acp_sdw_index].acp_sdw_dev_dma_watermark,
			     (sdw_buff_size_capture >> 1));
		break;
	default:
		return -EINVAL;
	}
#endif
	return 0;
}

static int poll_for_register_delay(uint32_t reg, uint32_t mask, uint32_t val, uint64_t us)
{
	if (!WAIT_FOR((sys_read32(reg) & mask) == val, us, k_busy_wait(1))) {
		LOG_ERR("poll timeout reg %u mask %u val %u us %u", reg, mask, val, (uint32_t)us);
		return -EIO;
	}

	return 0;
}

static int acp_sdw_dma_start(const struct device *dev, uint32_t channel)
{
	struct acp_dma_dev_data *const dev_data = dev->data;
	struct acp_dma_chan_data *chan = &dev_data->chan_data[channel];
	uint32_t acp_sdw0_audio_tx_en = 0;
	uint32_t acp_sdw0_audio_rx_en = 0;
	uint32_t acp_pdm_en;

#ifdef CONFIG_SOC_ACP_7_0
	uint32_t acp_sdw_index = ACP_SDW_INVALID, instance = ACP_SDW_INVALID;

	for (int i = 0; i < ACP_SDW_PINS_PER_INSTANCE; i++) {
		if (pin_data[i]->dma_channel == channel &&
		    pin_data[i]->pin_dir == chan->direction) {
			acp_sdw_index = pin_data[i]->acp_sdw_index;
			instance = pin_data[i]->instance;
			break;
		}
	}

	if (acp_sdw_index == ACP_SDW_INVALID) {
		return -EINVAL;
	}

	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < 3; j++) {
			acp_sdw0_audio_tx_en |=
				io_reg_read(PU_REGISTER_BASE + sdw_dev[i][j].acp_sdw_dev_en);
			acp_sdw0_audio_rx_en |=
				io_reg_read(PU_REGISTER_BASE + sdw_dev[i][j + 3].acp_sdw_dev_en);
		}
	}

	acp_pdm_en = (uint32_t)io_reg_read(PU_REGISTER_BASE + ACP_WOV_PDM_ENABLE);

	if (!acp_sdw0_audio_tx_en && !acp_sdw0_audio_rx_en && !acp_pdm_en) {
		/* Request SMU to set aclk to 600 Mhz */
		acp_change_clock_notify(ACP_ACLK_FREQ_HZ);
		io_reg_write(PU_REGISTER_BASE + ACP_CLKMUX_SEL, ACP_ACLK_CLK_SEL);
	}

	switch (chan->direction) {
	case MEMORY_TO_PERIPHERAL:
		prev_tx_pos_sdw = 0;
		chan->state = ACP_DMA_ACTIVE;
		io_reg_write(PU_REGISTER_BASE + sdw_dev[instance][acp_sdw_index].acp_sdw_dev_en, 1);
		poll_for_register_delay(
			PU_REGISTER_BASE + sdw_dev[instance][acp_sdw_index].acp_sdw_dev_en_status,
			0x1, 0x1, 15);
		break;
	case PERIPHERAL_TO_MEMORY:
		prev_rx_pos_sdw = 0;
		chan->state = ACP_DMA_ACTIVE;
		io_reg_write(PU_REGISTER_BASE + sdw_dev[instance][acp_sdw_index].acp_sdw_dev_en, 1);
		poll_for_register_delay(
			PU_REGISTER_BASE + sdw_dev[instance][acp_sdw_index].acp_sdw_dev_en_status,
			0x1, 0x1, 15);
		break;
	default:
		return -EINVAL;
	}
#endif
	return 0;
}

static bool acp_sdw_dma_chan_filter(const struct device *dev, int channel, void *filter_param)
{
	uint32_t req_chan;

	if (!filter_param) {
		return true;
	}

	req_chan = *(uint32_t *)filter_param;
	if (channel == req_chan) {
		return true;
	}
	return false;
}

static void acp_sdw_dma_chan_release(const struct device *dev, uint32_t channel)
{
	struct acp_dma_dev_data *const dev_data = dev->data;
	struct acp_dma_chan_data *chan = &dev_data->chan_data[channel];

	chan->state = ACP_DMA_READY;
}

/* Stop the requested channel */
static int acp_sdw_dma_stop(const struct device *dev, uint32_t channel)
{
	struct acp_dma_dev_data *const dev_data = dev->data;
	struct acp_dma_chan_data *chan = &dev_data->chan_data[channel];
	uint32_t acp_sdw0_audio_tx_en = 0;
	uint32_t acp_sdw0_audio_rx_en = 0;
	uint32_t acp_pdm_en;

	switch (chan->state) {
	case ACP_DMA_READY:
		/* fallthrough */
	case ACP_DMA_PREPARED:
		return 0;
	case ACP_DMA_SUSPENDED:
		/* fallthrough */
	case ACP_DMA_ACTIVE: /*breaks and continues the stop operation*/
		break;
	default:
		return -EINVAL;
	}

#ifdef CONFIG_SOC_ACP_7_0
	uint32_t acp_sdw_index = ACP_SDW_INVALID, instance = ACP_SDW_INVALID;

	chan->state = ACP_DMA_READY;

	for (int j = 0; j < ACP_SDW_PINS_PER_INSTANCE; j++) {
		if (pin_data[j]->dma_channel == channel &&
		    pin_data[j]->pin_dir == chan->direction) {
			acp_sdw_index = pin_data[j]->acp_sdw_index;
			instance = pin_data[j]->instance;
			break;
		}
	}

	if (acp_sdw_index == ACP_SDW_INVALID) {
		return -EINVAL;
	}

	switch (chan->direction) {
	case MEMORY_TO_PERIPHERAL:
		chan->state = ACP_DMA_ACTIVE;
		io_reg_write(PU_REGISTER_BASE + sdw_dev[instance][acp_sdw_index].acp_sdw_dev_en, 0);
		poll_for_register_delay(
			PU_REGISTER_BASE + sdw_dev[instance][acp_sdw_index].acp_sdw_dev_en_status,
			0x1, 0x0, 15);
		break;
	case PERIPHERAL_TO_MEMORY:
		chan->state = ACP_DMA_ACTIVE;
		io_reg_write(PU_REGISTER_BASE + sdw_dev[instance][acp_sdw_index].acp_sdw_dev_en, 0);
		poll_for_register_delay(
			PU_REGISTER_BASE + sdw_dev[instance][acp_sdw_index].acp_sdw_dev_en_status,
			0x1, 0x0, 15);
		break;
	default:
		return -EINVAL;
	}

	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < 3; j++) {
			acp_sdw0_audio_tx_en |=
				io_reg_read(PU_REGISTER_BASE + sdw_dev[i][j].acp_sdw_dev_en);
			acp_sdw0_audio_rx_en |=
				io_reg_read(PU_REGISTER_BASE + sdw_dev[i][j + 3].acp_sdw_dev_en);
		}
	}

	acp_pdm_en = io_reg_read(PU_REGISTER_BASE + ACP_WOV_PDM_ENABLE);

	if (!acp_sdw0_audio_tx_en && !acp_sdw0_audio_rx_en) {
		/* Request SMU to scale down aclk to minimum clk */
		if (!acp_pdm_en) {
			io_reg_write(PU_REGISTER_BASE + ACP_CLKMUX_SEL, ACP_INTERNAL_CLK_SEL);
			acp_change_clock_notify(0);
		}
	}

	for (int i = 0; i < ACP_SDW_PINS_PER_INSTANCE; i++) {
		if (pin_data[i]->acp_sdw_index == acp_sdw_index) {
			pin_data[i]->dma_channel = ACP_SDW_INVALID;
			pin_data[i]->pin_dir = ACP_SDW_INVALID;
			pin_data[i]->pin_num = ACP_SDW_INVALID;
			pin_data[i]->index = ACP_SDW_INVALID;
			pin_data[i]->instance = ACP_SDW_INVALID;
			pin_data[i]->acp_sdw_index = ACP_SDW_INVALID;
			break;
		}
	}
#endif
	return 0;
}

static int acp_sdw_dma_resume(const struct device *dev, uint32_t channel)
{
	struct acp_dma_dev_data *const dev_data = dev->data;
	struct acp_dma_chan_data chan_data = dev_data->chan_data[channel];
	int ret = 0;

	/* Validate channel index */
	if (channel >= dev_data->dma_ctx.dma_channels) {
		ret = -EINVAL;
		goto out;
	}
	/* Validate channel state */
	if (chan_data.state != ACP_DMA_SUSPENDED) {
		ret = -EINVAL;
		goto out;
	}
	/* Channel is now active */
	chan_data.state = ACP_DMA_ACTIVE;
out:
	return ret;
}

static int acp_sdw_dma_suspend(const struct device *dev, uint32_t channel)
{
	struct acp_dma_dev_data *const dev_data = dev->data;
	struct acp_dma_chan_data chan_data = dev_data->chan_data[channel];
	int ret = 0;

	/* Validate channel index */
	if (channel >= dev_data->dma_ctx.dma_channels) {
		ret = -EINVAL;
		goto out;
	}
	/* Validate channel state */
	if (chan_data.state != ACP_DMA_ACTIVE) {
		ret = -EINVAL;
		goto out;
	}
	/* Channel is now active */
	chan_data.state = ACP_DMA_SUSPENDED;
out:
	return ret;
}

static int acp_sdw_dma_reload(const struct device *dev, uint32_t channel, uint32_t src,
			      uint32_t dst, size_t bytes)
{
	struct acp_dma_dev_data *const dev_data = dev->data;
	int ret = 0;

	/* Validate channel index */
	if (channel >= dev_data->dma_ctx.dma_channels) {
		ret = -EINVAL;
	}
	return ret;
}

static int acp_sdw_dma_get_status(const struct device *dev, uint32_t channel,
				  struct dma_status *stat)
{
	struct acp_dma_dev_data *const dev_data = dev->data;
	struct acp_dma_chan_data *chan = &dev_data->chan_data[channel];
	uint32_t reg_addr_low, reg_addr_high;

	if (channel >= dev_data->dma_ctx.dma_channels) {
		return -EINVAL;
	}

#ifdef CONFIG_SOC_ACP_7_0
	uint32_t pin_number;
	uint32_t acp_sdw_index = ACP_SDW_INVALID;
	uint32_t instance = ACP_SDW_INVALID;

	for (int i = 0; i < ACP_SDW_PINS_PER_INSTANCE; i++) {
		if (pin_data[i]->dma_channel == channel &&
		    pin_data[i]->pin_dir == chan->direction) {
			acp_sdw_index = pin_data[i]->index;
			instance = pin_data[i]->instance;
			pin_number = pin_data[i]->pin_num;
			break;
		}
	}

	if (acp_sdw_index == ACP_SDW_INVALID) {
		return -EINVAL;
	}

	if (instance == ACP_SDW_INVALID) {
		return -EINVAL;
	}

	if (chan->direction == MEMORY_TO_PERIPHERAL) {
		reg_addr_low = acp_sdw_index_get(pin_number, ACP_SDW_INDEX_LIN_POS_LOW);
		reg_addr_high = acp_sdw_index_get(pin_number, ACP_SDW_INDEX_LIN_POS_HIGH);
		tx_low_sdw = (uint32_t)io_reg_read(PU_REGISTER_BASE + reg_addr_low);
		tx_high_sdw = (uint32_t)io_reg_read(PU_REGISTER_BASE + reg_addr_high);
		curr_tx_pos_sdw = (uint64_t)(((uint64_t)tx_high_sdw << 32) | tx_low_sdw);
		stat->free = (int32_t)(curr_tx_pos_sdw - prev_tx_pos_sdw);
		stat->pending_length = sdw_buff_size_playback - stat->free;

		if (stat->free >= (sdw_buff_size_playback >> 1)) {
			prev_tx_pos_sdw += sdw_buff_size_playback >> 1;
		} else {
			stat->free = 0;
		}
	} else if (chan->direction == PERIPHERAL_TO_MEMORY) {
		reg_addr_low = acp_sdw_index_get(pin_number, ACP_SDW_INDEX_LIN_POS_LOW);
		reg_addr_high = acp_sdw_index_get(pin_number, ACP_SDW_INDEX_LIN_POS_HIGH);
		rx_low_sdw = (uint32_t)io_reg_read(PU_REGISTER_BASE + reg_addr_low);
		rx_high_sdw = (uint32_t)io_reg_read(PU_REGISTER_BASE + reg_addr_high);
		curr_rx_pos_sdw = (uint64_t)(((uint64_t)rx_high_sdw << 32) | rx_low_sdw);
		stat->pending_length = (int32_t)(curr_rx_pos_sdw - prev_rx_pos_sdw);
		if (stat->pending_length >= (sdw_buff_size_capture >> 1)) {
			prev_rx_pos_sdw += (sdw_buff_size_capture >> 1);
		} else {
			stat->pending_length = 0;
		}
		stat->free = sdw_buff_size_capture - stat->pending_length;
	} else {
		return -EINVAL;
	}
#endif
	return 0;
}

static int acp_sdw_dma_get_attribute(const struct device *dev, uint32_t type, uint32_t *value)
{
	switch (type) {
	case DMA_ATTR_COPY_ALIGNMENT:
		/* fallthrough */
	case DMA_ATTR_BUFFER_SIZE_ALIGNMENT:
		*value = CONFIG_DMA_AMD_ACP_BUFFER_ALIGN;
		break;
	case DMA_ATTR_BUFFER_ADDRESS_ALIGNMENT:
		*value = CONFIG_DMA_AMD_ACP_BUFFER_ADDRESS_ALIGN;
		break;
	case DMA_ATTR_MAX_BLOCK_COUNT:
		*value = 2;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int acp_sdw_dma_init(const struct device *dev)
{
	struct acp_dma_dev_data *const dev_data = dev->data;

	/* Setup context and atomics for channels */
	dev_data->dma_ctx.magic = DMA_MAGIC;
	dev_data->dma_ctx.dma_channels = CONFIG_DMA_AMD_ACP_SDW_CHANNEL_COUNT;
	dev_data->dma_ctx.atomic = dev_data->atomic;

#ifdef CONFIG_SOC_ACP_7_0
	for (int i = 0; i < ACP_SDW_PINS_PER_INSTANCE; i++) {
		if (!pin_data[i]) {
			pin_data[i] = k_malloc(sizeof(struct sdw_pin_data));
		}

		if (pin_data[i]) {
			pin_data[i]->pin_num = ACP_SDW_INVALID;
			pin_data[i]->pin_dir = ACP_SDW_INVALID;
			pin_data[i]->dma_channel = ACP_SDW_INVALID;
			pin_data[i]->index = ACP_SDW_INVALID;
			pin_data[i]->instance = ACP_SDW_INVALID;
			pin_data[i]->acp_sdw_index = ACP_SDW_INVALID;
		}
	}
#endif

	return 0;
}

static DEVICE_API(dma, acp_sdw_dma_driver_api) = {
	.config = acp_sdw_dma_config,
	.start = acp_sdw_dma_start,
	.stop = acp_sdw_dma_stop,
	.chan_filter = acp_sdw_dma_chan_filter,
	.chan_release = acp_sdw_dma_chan_release,
	.reload = acp_sdw_dma_reload,
	.suspend = acp_sdw_dma_suspend,
	.resume = acp_sdw_dma_resume,
	.get_status = acp_sdw_dma_get_status,
	.get_attribute = acp_sdw_dma_get_attribute,
};

static struct acp_dma_dev_data acp_sdw_dma_dev_data;
DEVICE_DT_DEFINE(DT_NODELABEL(acp_sdw_dma), &acp_sdw_dma_init, NULL, &acp_sdw_dma_dev_data,
		 NULL /*&acp_dma_sdw_dev_cfg*/, POST_KERNEL, CONFIG_DMA_INIT_PRIORITY,
		 &acp_sdw_dma_driver_api);
