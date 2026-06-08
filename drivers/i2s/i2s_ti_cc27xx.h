/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_I2S_I2S_TI_CC27XX_H_
#define ZEPHYR_DRIVERS_I2S_I2S_TI_CC27XX_H_

#include <zephyr/drivers/i2s.h>
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Callback to set DMA pointer register (INPTR/OUTPTR) */
typedef void (*ti_cc27xx_i2s_set_pointer_cb)(const struct device *dev, void *next_pointer);

/* Callback to configure sample stamp trigger (for DMA synchronization) */
typedef void (*ti_cc27xx_i2s_set_stamp_trigger_cb)(const struct device *dev, uint32_t value);

struct ti_cc27xx_i2s_stream;

/* Callback to stop an I2S stream */
typedef void (*ti_cc27xx_i2s_stop_stream_cb)(const struct device *dev,
					     struct ti_cc27xx_i2s_stream *stream);

/* Callback to peek at next transfer without dequeuing */
typedef struct ti_cc27xx_i2s_transfer *(*ti_cc27xx_i2s_peek_next_transfer_cb)(
	struct ti_cc27xx_i2s_stream *stream);

/* Callback to allocate transfer descriptor from mem_slab */
typedef struct ti_cc27xx_i2s_transfer *(*ti_cc27xx_i2s_alloc_transfer_cb)(
	struct ti_cc27xx_i2s_stream *stream);

/* Callback to purge all transfers from stream queue */
typedef void (*ti_cc27xx_i2s_purge_queue_cb)(struct ti_cc27xx_i2s_stream *stream);

/* Hardware limits */
#define TI_CC27XX_I2S_CLOCK_DIVIDER_MAX        1024U
#define TI_CC27XX_I2S_CLOCK_DIVIDER_MIN        2U
#define TI_CC27XX_I2S_NB_CHANNELS_MAX          8U
#define TI_CC27XX_I2S_DUAL_PHASE_CHANNEL_MAX   2
#define TI_CC27XX_I2S_SINGLE_PHASE_CHANNEL_MAX 8

/* Default configuration values */
#define I2S_CC27XX_STARTUP_DELAY 2U
#define I2S_CC27XX_QUEUE_DEPTH   32U

/* Channel count configuration */
enum TI_CC27XX_I2S_CHANNEL_COUNT {
	TI_CC27XX_I2S_CHANNEL_COUNT_1 = 1,
	TI_CC27XX_I2S_CHANNEL_COUNT_2 = 2,
};

/* Audio word size in bits */
enum TI_CC27XX_I2S_WORD_SIZE {
	TI_CC27XX_I2S_WORD_SIZE_16 = 16,
	TI_CC27XX_I2S_WORD_SIZE_32 = 32,
};

/* I2S module role: Controller generates clocks, Target receives clocks */
enum TI_CC27XX_I2S_ROLE {
	TI_CC27XX_I2S_ROLE_TARGET = 0,
	TI_CC27XX_I2S_ROLE_CONTROLLER = 1
};

/* Sampling edge: when data is sampled/changed relative to SCK */
enum TI_CC27XX_I2S_SAMPLING_EDGE {
	TI_CC27XX_I2S_SAMPLING_EDGE_FALLING = 0,
	TI_CC27XX_I2S_SAMPLING_EDGE_RISING = 1,
};

/* Phase type: DUAL for I2S/LJF/RJF, SINGLE for TDM */
enum TI_CC27XX_I2S_PHASE_TYPE {
	TI_CC27XX_I2S_PHASE_TYPE_SINGLE = 0,
	TI_CC27XX_I2S_PHASE_TYPE_DUAL = 1,
};

/* Channel bitmask for enabling specific channels */
enum TI_CC27XX_I2S_CHANNEL_BITMASK {
	TI_CC27XX_I2S_CHANNEL_BITMASK_NONE = 0x00,
	TI_CC27XX_I2S_CHANNEL_BITMASK_MONO = 0x01,
	TI_CC27XX_I2S_CHANNEL_BITMASK_MONO_INV = 0x02,
	TI_CC27XX_I2S_CHANNEL_BITMASK_STEREO = 0x03,
	TI_CC27XX_I2S_CHANNEL_BITMASK_1 = 0x01,
	TI_CC27XX_I2S_CHANNEL_BITMASK_2 = 0x03,
	TI_CC27XX_I2S_CHANNEL_BITMASK_3 = 0x07,
	TI_CC27XX_I2S_CHANNEL_BITMASK_4 = 0x0F,
	TI_CC27XX_I2S_CHANNEL_BITMASK_5 = 0x1F,
	TI_CC27XX_I2S_CHANNEL_BITMASK_6 = 0x3F,
	TI_CC27XX_I2S_CHANNEL_BITMASK_7 = 0x7F,
	TI_CC27XX_I2S_CHANNEL_BITMASK_8 = 0xFF,
	TI_CC27XX_I2S_CHANNELS_ALL = 0xFF
};

/* Pin direction: IN (receive), OUT (transmit), DISABLED (not used) */
enum TI_CC27XX_I2S_PIN_DIR {
	TI_CC27XX_I2S_PIN_DIR_DISABLED = 0x00,
	TI_CC27XX_I2S_PIN_DIR_IN = 0x01,
	TI_CC27XX_I2S_PIN_DIR_OUT = 0x02,
};

/* Audio frequency clock source */
enum TI_CC27XX_I2S_CLK_SRC {
	TI_CC27XX_I2S_CLK_SRC_CLKREF = 0x00, /* 48MHz reference clock */
	TI_CC27XX_I2S_CLK_SRC_CLKHF = 0x01,  /* 96MHz high-frequency clock */
	TI_CC27XX_I2S_CLK_SRC_CLKAF = 0x02,  /* AFOSC (80/90.3168/98.304 MHz) */
};

/* AFOSC frequency options (exact sample rate generation) */
enum TI_CC27XX_I2S_AFOSC_FREQ {
	TI_CC27XX_AFOSC_FREQ_80MHZ = 80000000U,
	TI_CC27XX_AFOSC_FREQ_90P3168MHZ = 90316800U, /* Optimized for 44.1 kHz */
	TI_CC27XX_AFOSC_FREQ_98P304MHZ = 98304000U,  /* Optimized for 48 kHz */
};

/* Memory length: 16-bit or 32-bit DMA word size */
enum TI_CC27XX_I2S_MEMORY_LENGTH {
	TI_CC27XX_I2S_MEMORY_LENGTH_16BITS = 16U,
	TI_CC27XX_I2S_MEMORY_LENGTH_32BITS = 32U,
};

/**
 * @brief DMA transfer descriptor
 *
 * Represents a single DMA transfer with audio data.
 * Used in double-buffering scheme (active_transfer + next_transfer).
 */
struct ti_cc27xx_i2s_transfer {
	void *fifo_reserved;               /* k_fifo linked-list management */
	struct k_mem_slab *own_mem_slab;   /* Descriptor mem_slab */
	struct k_mem_slab *audio_mem_slab; /* Audio buffer mem_slab */
	void *mem_block;                   /* Audio data buffer pointer */
	size_t size;                       /* Buffer size in bytes */
	uint32_t frames_total;             /* Total frames in buffer */
	uint32_t frames_transferred;       /* Frames transferred by DMA */
	uint32_t frames_scheduled;         /* Frames scheduled to hardware */
};

/**
 * @brief I2S stream configuration (derived from i2s_config)
 *
 * Contains computed parameters specific to one stream (TX or RX).
 * Cached to avoid recalculation during runtime.
 */
struct ti_cc27xx_i2s_stream_cfg {
	uint32_t frame_clk_freq;                            /* Sample rate (Hz) */
	uint32_t bits_per_memory_word;                      /* 16 or 32 bits */
	uint32_t bytes_per_frame;                           /* channels × bytes_per_sample */
	size_t bytes_per_audio_block;                       /* Block size in bytes */
	size_t frames_per_audio_block;                      /* Frames per DMA interrupt */
	uint8_t data_shift;                                 /* I2S format timing delay */
	uint8_t bits_per_audio_word;                        /* Actual bits per sample (8-24) */
	uint8_t before_word_padding;                        /* SCK periods before MSB */
	uint8_t after_word_padding;                         /* SCK periods after LSB */
	enum TI_CC27XX_I2S_CHANNEL_BITMASK channel_bitmask; /* Active channels */
	uint8_t channel_count;                              /* Number of active channels */
	k_timeout_t timeout;                                /* Buffer operation timeout */
};

/**
 * @brief I2S stream (TX or RX direction)
 *
 * Manages one direction of I2S transfers with queue, DMA, and state tracking.
 * Each device has two streams: stream_tx (playback), stream_rx (recording).
 */
struct ti_cc27xx_i2s_stream {
	struct ti_cc27xx_i2s_transfer *active_transfer;         /* Currently in DMA */
	struct ti_cc27xx_i2s_transfer *next_transfer;           /* Pre-loaded to hardware */
	struct ti_cc27xx_i2s_stream_cfg cfg;                    /* Stream configuration */
	struct k_fifo queue;                                    /* Pending transfers */
	struct k_mem_slab *audio_mem_slab;                      /* Audio buffer pool */
	struct k_mem_slab *transfer_mem_slab;                   /* Descriptor pool */
	enum i2s_state state;                                   /* Stream state machine */
	bool configured;                                        /* Configuration valid */
	bool drain;                                             /* STOP vs DROP mode */
	bool enabled;                                           /* DMA active */
	ti_cc27xx_i2s_set_pointer_cb set_dma_pointer;           /* Set INPTR/OUTPTR */
	ti_cc27xx_i2s_set_stamp_trigger_cb set_stamp_trigger;   /* Sync trigger */
	ti_cc27xx_i2s_peek_next_transfer_cb peek_next_transfer; /* Peek queue */
	ti_cc27xx_i2s_purge_queue_cb purge_queue;               /* Clear queue */
	struct i2s_config i2s_config_copy;                      /* Power event restore */
	uint32_t irq_flag;                                      /* I2S_INT_DMA_IN/OUT */
};

/**
 * @brief I2S runtime data (RAM)
 *
 * Per-instance runtime state combining both streams and shared hardware state.
 */
struct ti_cc27xx_i2s_data {
	enum i2s_dir dir;                      /* TX, RX, or BOTH */
	bool invert_ws;                        /* WS signal inversion */
	struct ti_cc27xx_i2s_stream stream_tx; /* Transmit stream */
	struct ti_cc27xx_i2s_stream stream_rx; /* Receive stream */
	uint32_t dma_frame_count;              /* Frames per DMA transfer */
	bool is_dma_frame_count_fixed;         /* Fixed vs variable buffer size */
	uint32_t ws_divider;                   /* Word select divider */
	uint32_t sck_divider;                  /* Serial clock divider */
	bool clocks_active;                    /* PM constraints held by start_clocks */
};

/**
 * @brief I2S device configuration (from device tree, ROM/flash)
 *
 * Compile-time configuration from DT bindings. Const, stored in ROM.
 */
struct ti_cc27xx_i2s_cfg {
	const struct pinctrl_dev_config *pin_cfg;       /* Pin muxing */
	uint32_t reg_base;                              /* Peripheral base address */
	uint32_t startup_delay;                         /* WS cycles before first DMA */
	uint32_t cclk_divider;                          /* Controller clock divider */
	enum TI_CC27XX_I2S_ROLE module_role;            /* Controller or Target */
	enum TI_CC27XX_I2S_PHASE_TYPE phase_type;       /* Dual or Single phase */
	enum TI_CC27XX_I2S_SAMPLING_EDGE sampling_edge; /* Rising or Falling */
	enum TI_CC27XX_I2S_CLK_SRC afclk_src;           /* Audio clock source */
	uint32_t afosc_freq;                            /* AFOSC frequency (if used) */
	enum TI_CC27XX_I2S_PIN_DIR pin_dir_sd0;         /* SD0 pin direction */
	enum TI_CC27XX_I2S_PIN_DIR pin_dir_sd1;         /* SD1 pin direction */
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_I2S_I2S_TI_CC27XX_H_ */
