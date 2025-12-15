/*
 * Copyright (c) 2026 Conclusive Engineering Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _TI_CC35XX_I2S_
#define _TI_CC35XX_I2S_

#include <zephyr/drivers/i2s.h>
#include <zephyr/kernel.h>

#include <i2s.h>

struct ti_cc35xx_i2s_stream;
struct ti_cc35xx_i2s_transaction;

typedef void (*ti_cc35xx_i2s_set_pointer_cb)(const struct device *dev, void *next_pointer);

typedef void (*ti_cc35xx_i2s_set_stamp_trigger_cb)(const struct device *dev, uint32_t value);

typedef void (*ti_cc35xx_i2s_stop_stream_cb)(const struct device *dev,
					     struct ti_cc35xx_i2s_stream *stream);

typedef struct ti_cc35xx_i2s_transfer *(*ti_cc35xx_i2s_peek_next_trasnfer_cb)(
	struct ti_cc35xx_i2s_stream *stream);

typedef struct ti_cc35xx_i2s_transfer *(*ti_cc35xx_i2s_alloc_transfer_cb)(
	struct ti_cc35xx_i2s_stream *stream);

typedef void (*ti_cc35xx_i2s_purge_queue_cb)(struct ti_cc35xx_i2s_stream *stream);

#define TI_CC35XX_I2S_DUAL_PHASE_CHANNEL_MAX   2
#define TI_CC35XX_I2S_SINGLE_PHASE_CHANNEL_MAX 8

enum TI_CC35XX_I2S_CHANNEL_COUNT {
	TI_CC35XX_I2S_CHANNEL_COUNT_1 = 1,
	TI_CC35XX_I2S_CHANNEL_COUNT_2 = 2,
};

enum TI_CC35XX_I2S_WORD_SIZE {
	TI_CC35XX_I2S_WORD_SIZE_8 = 8,
	TI_CC35XX_I2S_WORD_SIZE_16 = 16,
	TI_CC35XX_I2S_WORD_SIZE_24 = 24,
	TI_CC35XX_I2S_WORD_SIZE_32 = 32,
};

enum TI_CC35XX_I2S_ROLE {
	TI_CC35XX_I2S_ROLE_TARGET = 0,
	TI_CC35XX_I2S_ROLE_CONTROLLER = 1
};

enum TI_CC35XX_I2S_SAMPLING_EDGE {
	TI_CC35XX_I2S_SAMPLING_EDGE_FALLING = 0,
	TI_CC35XX_I2S_SAMPLING_EDGE_RISING = 1,
};

enum TI_CC35XX_I2S_PHASE_TYPE {
	TI_CC35XX_I2S_PHASE_TYPE_SINGLE = 0,
	TI_CC35XX_I2S_PHASE_TYPE_DUAL = 1,
};

enum TI_CC35XX_I2S_CHANNEL_BITMASK {
	TI_CC35XX_I2S_CHANNEL_BITMASK_NONE = 0x00,
	TI_CC35XX_I2S_CHANNEL_BITMASK_1 = 0x01,
	TI_CC35XX_I2S_CHANNEL_BITMASK_2 = 0x03,
	TI_CC35XX_I2S_CHANNEL_BITMASK_3 = 0x07,
	TI_CC35XX_I2S_CHANNEL_BITMASK_4 = 0x0F,
	TI_CC35XX_I2S_CHANNEL_BITMASK_5 = 0x1F,
	TI_CC35XX_I2S_CHANNEL_BITMASK_6 = 0x3F,
	TI_CC35XX_I2S_CHANNEL_BITMASK_7 = 0x7F,
	TI_CC35XX_I2S_CHANNEL_BITMASK_8 = 0xFF,
	TI_CC35XX_I2S_CHANNELS_ALL = 0xFF
};

enum TI_CC35XX_I2S_PIN_DIR {
	TI_CC35XX_I2S_PIN_DIR_DISABLED = 0x00,
	TI_CC35XX_I2S_PIN_DIR_IN = 0x01,
	TI_CC35XX_I2S_PIN_DIR_OUT = 0x02,
};

enum TI_CC35XX_I2S_CLK_SRC {
	TI_CC35XX_I2S_CLK_SRC_SOC_CLK = 0x10,
	TI_CC35XX_I2S_CLK_SRC_SOC_PLL_CLK = 0x20,
	TI_CC35XX_I2S_CLK_SRC_HFXT_CLK = 0x30,
};

struct ti_cc35xx_i2s_adfs {
	/* Source clock period in picoseconds. */
	uint32_t tref;
	/*
	 * Error in picoseconds that quantifies how far the achieved period
	 * (using the chosen divider) deviates from the desired period.
	 */
	uint32_t delta;
	/* Represents whether delta is negative (1) or non-negative (0). */
	uint32_t delta_sign;
	/*
	 * Divisor used to approximate the ratio between Fref and Freq.
	 * It is calculated to minimize the error (delta) between the actual
	 * anddesired frequency ratios.
	 * Should be an integer higher or equal than 1.
	 */
	uint32_t div;
};

struct ti_cc35xx_i2s_transfer {
	void *fifo_reserved;
	struct k_mem_slab *own_mem_slab;
	struct k_mem_slab *audio_mem_slab;
	void *mem_block;
	size_t size;
	uint32_t frames_total;
	uint32_t frames_transferred;
	uint32_t frames_scheduled;
};

struct ti_cc35xx_i2s_stream_cfg {
	uint32_t frame_clk_freq;
	uint32_t bits_per_memory_word;
	uint32_t bytes_per_frame;
	size_t bytes_per_audio_block;
	size_t frames_per_audio_block;
	uint8_t data_shift;
	uint8_t bits_per_audio_word;
	uint8_t before_word_padding;
	uint8_t after_word_padding;
	enum TI_CC35XX_I2S_CHANNEL_BITMASK channel_bitmask;
	uint8_t channel_count;
	k_timeout_t timeout;
};

struct ti_cc35xx_i2s_stream {
	struct ti_cc35xx_i2s_transfer *active_transfer;
	struct ti_cc35xx_i2s_transfer *next_transfer;
	struct ti_cc35xx_i2s_stream_cfg cfg;
	struct k_fifo queue;
	struct k_mem_slab *audio_mem_slab;
	struct k_mem_slab *transfer_mem_slab;
	enum i2s_state state;
	bool configured;
	bool drain;
	bool enabled;
	ti_cc35xx_i2s_set_pointer_cb set_dma_pointer;
	ti_cc35xx_i2s_set_stamp_trigger_cb set_stamp_trigger;
	ti_cc35xx_i2s_peek_next_trasnfer_cb peek_next_transfer;
	ti_cc35xx_i2s_purge_queue_cb purge_queue;
	struct i2s_config i2s_config_copy;
	uint32_t irq_flag;
};

struct ti_cc35xx_i2s_data {
	enum i2s_dir dir;
	bool invert_ws;
	struct ti_cc35xx_i2s_stream stream_tx;
	struct ti_cc35xx_i2s_stream stream_rx;
	uint32_t dma_frame_count;
	bool is_dma_frame_count_fixed;
	uint32_t ws_divider;
	uint32_t sck_divider;
};

struct ti_cc35xx_i2s_cfg {
	const struct pinctrl_dev_config *pin_cfg;
	uint32_t reg_base;
	uint32_t startup_delay;
	uint32_t cclk_divider;
	enum TI_CC35XX_I2S_ROLE module_role;
	enum TI_CC35XX_I2S_PHASE_TYPE phase_type;
	enum TI_CC35XX_I2S_SAMPLING_EDGE sampling_edge;
	enum TI_CC35XX_I2S_CLK_SRC core_clk_src;
	struct ti_cc35xx_i2s_adfs adfs;
	uint32_t adfs_audio_clock_freq;
	enum TI_CC35XX_I2S_PIN_DIR pin_dir_sd0;
	enum TI_CC35XX_I2S_PIN_DIR pin_dir_sd1;
};

#endif /* _TI_CC35XX_I2S_ */
