/*
 * Copyright (c) 2016, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the Intel Corporation nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL CORPORATION OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __QM_I2S_H__
#define __QM_I2S_H__

#include "qm_common.h"
#include "qm_soc_regs.h"
#include "qm_dma.h"

/**
 * I2S.
 *
 * @defgroup groupI2S I2S
 * @{
 */

/**
 * I2S master/slave configuration.
 */
typedef enum {
	QM_I2S_MASTER = 0, /**< Master configuration selection */
	QM_I2S_SLAVE       /**< Slave configuration selection */
} qm_i2s_master_slave_t;

/**
 * I2S Audio stream identifiers.
 */
typedef enum {
	QM_I2S_AUDIO_STREAM_TRANSMIT = 0, /**< Playback audio stream */
	QM_I2S_AUDIO_STREAM_RECEIVE       /**< Capture audio stream */
} qm_i2s_audio_stream_t;

/**
 * I2S audio formats.
 */
typedef enum {
	QM_I2S_AUDIO_FORMAT_I2S_MODE = 0, /**< I2S Mode audio format */
	QM_I2S_AUDIO_FORMAT_RIGHT_J,      /**< Right justified audio format */
	QM_I2S_AUDIO_FORMAT_LEFT_J,       /**< Left justified audio format */
	QM_I2S_AUDIO_FORMAT_DSP_MODE      /**< DSP mode audio format */
} qm_i2s_audio_format_t;

/**
 * I2S Audio sample resolution. Resolution ranges between 12 bits and 32 bits.
 */
typedef enum {
	/** 12 bits audio sample resolution */
	QM_I2S_12_BIT_SAMPLE_RESOLUTION = 12,
	/** 13 bits audio sample resolution */
	QM_I2S_13_BIT_SAMPLE_RESOLUTION = 13,
	/** 14 bits audio sample resolution */
	QM_I2S_14_BIT_SAMPLE_RESOLUTION = 14,
	/** 15 bits audio sample resolution */
	QM_I2S_15_BIT_SAMPLE_RESOLUTION = 15,
	/** 16 bits audio sample resolution */
	QM_I2S_16_BIT_SAMPLE_RESOLUTION = 16,
	/** 17 bits audio sample resolution */
	QM_I2S_17_BIT_SAMPLE_RESOLUTION = 17,
	/** 18 bits audio sample resolution */
	QM_I2S_18_BIT_SAMPLE_RESOLUTION = 18,
	/** 19 bits audio sample resolution */
	QM_I2S_19_BIT_SAMPLE_RESOLUTION = 19,
	/** 20 bits audio sample resolution */
	QM_I2S_20_BIT_SAMPLE_RESOLUTION = 20,
	/** 21 bits audio sample resolution */
	QM_I2S_21_BIT_SAMPLE_RESOLUTION = 21,
	/** 22 bits audio sample resolution */
	QM_I2S_22_BIT_SAMPLE_RESOLUTION = 22,
	/** 23 bits audio sample resolution */
	QM_I2S_23_BIT_SAMPLE_RESOLUTION = 23,
	/** 24 bits audio sample resolution */
	QM_I2S_24_BIT_SAMPLE_RESOLUTION = 24,
	/** 25 bits audio sample resolution */
	QM_I2S_25_BIT_SAMPLE_RESOLUTION = 25,
	/** 26 bits audio sample resolution */
	QM_I2S_26_BIT_SAMPLE_RESOLUTION = 26,
	/** 27 bits audio sample resolution */
	QM_I2S_27_BIT_SAMPLE_RESOLUTION = 27,
	/** 28 bits audio sample resolution */
	QM_I2S_28_BIT_SAMPLE_RESOLUTION = 28,
	/** 29 bits audio sample resolution */
	QM_I2S_29_BIT_SAMPLE_RESOLUTION = 29,
	/** 30 bits audio sample resolution */
	QM_I2S_30_BIT_SAMPLE_RESOLUTION = 30,
	/** 31 bits audio sample resolution */
	QM_I2S_31_BIT_SAMPLE_RESOLUTION = 31,
	/** 32 bits audio sample resolution */
	QM_I2S_32_BIT_SAMPLE_RESOLUTION = 32
} qm_i2s_sample_resolution_t;

/**
 * I2S Audio rates available.
 */
typedef enum {
	QM_I2S_RATE_4000KHZ = 0, /**< 4kHz audio rate */
	QM_I2S_RATE_8000KHZ,     /**< 8kHz audio rate */
	QM_I2S_RATE_11025KHZ,    /**< 11.025kHz audio rate */
	QM_I2S_RATE_16000KHZ,    /**< 16kHz audio rate */
	QM_I2S_RATE_22050KHZ,    /**< 22.05kHz audio rate */
	QM_I2S_RATE_32000KHZ,    /**< 32kHz audio rate */
	QM_I2S_RATE_44100KHZ,    /**< 44.1kHz audio rate */
	QM_I2S_RATE_48000KHZ,    /**< 48kHz audio rate */
} qm_i2s_audio_rate_t;

/**
 * I2S Audio buffer termination.
 */
typedef enum {
	/** Used for single audio playback/recording. */
	QM_I2S_TERMINATED_BUFFER = 0,
	QM_I2S_RING_BUFFER /**< Used for continuous audio playback/recording. */
} qm_i2s_buffer_mode_t;

/**
 * I2S Reference Clock Source Select
 */
typedef enum {
	QM_I2S_INT_CLK = 0, /**< Use internal clock. */
	QM_I2S_MCLK,	/**< Take in clock from external source. */
} qm_i2s_clk_src_t;

/**
 * I2S status type.
 */
typedef enum {
	/** Indicates that a FIFO overflow error was detected. */
	QM_I2S_FIFO_OVERFLOW = BIT(0),
	/** Indicates that a FIFO underrun error was detected. */
	QM_I2S_FIFO_UNDERRUN = BIT(1),
	/** Indicates TX FIFO full interrupt occurred */
	QM_I2S_TX_FIFO_FULL = BIT(2),
	/** Indicates TX FIFO almost full interrupt occurred */
	QM_I2S_TX_FIFO_ALMOST_FULL = BIT(3),
	/** Indicates TX FIFO almost empty interrupt occurred */
	QM_I2S_TX_FIFO_ALMOST_EMPTY = BIT(4),
	/** Indicates TX FIFO empty interrupt occurred */
	QM_I2S_TX_FIFO_EMPTY = BIT(5),
	/** Indicates RX FIFO full interrupt occurred */
	QM_I2S_RX_FIFO_FULL = BIT(6),
	/** Indicates RX FIFO almost full interrupt occurred */
	QM_I2S_RX_FIFO_ALMOST_FULL = BIT(7),
	/** Indicates RX FIFO almost empty interrupt occurred */
	QM_I2S_RX_FIFO_ALMOST_EMPTY = BIT(8),
	/** Indicates RX FIFO empty interrupt occurred */
	QM_I2S_RX_FIFO_EMPTY = BIT(9),
	/** Indicates that a DMA error was detected */
	QM_I2S_DMA_ERROR = BIT(10),
	/** Indicates that DMA operation was completed */
	QM_I2S_DMA_COMPLETE = BIT(11),
} qm_i2s_status_t;

/**
 * Ring buffer link definition.
 */
typedef struct qm_i2s_buffer_link {
	uint32_t *buff; /**< Pointer to the buffer base address. */
	/** Pointer to the head of the DMA link list for the buffer link. */
	qm_dma_lli_item_t *dma_link_head;
	/** Link list item's pointer to the next link. */
	struct qm_i2s_buffer_link *next;
} qm_i2s_buffer_link_t;

/**
 * I2S controller configuration.
 * Driver instantiates one of these with given parameters for each I2S
 * channel configured using the "qm_i2s_set_channel_config" function.
 */
typedef struct {
	qm_dma_channel_id_t dma_channel;    /**< DMA controller Channel ID */
	qm_i2s_audio_stream_t audio_stream; /**< I2S audio stream identifier */
	/** I2S audio format configuration */
	qm_i2s_audio_format_t audio_format;
	/** I2S Master or Slave configuration */
	qm_i2s_master_slave_t master_slave;
	/** Sampling resolution */
	qm_i2s_sample_resolution_t sample_resolution;
	qm_i2s_audio_rate_t audio_rate; /**< Audio rate */
	/** Terminated link list or ring buffer configuration */
	qm_i2s_buffer_mode_t audio_buff_cfg;
	/**
	 * DMA block size configuration:
	 * The source and destination transfer width is 32-bits for I2S.
	 * Min = 1, Max = see DMA configuration
	 */
	uint32_t block_size;
	uint32_t num_dma_links;      /**< Total number of DMA links. */
	qm_dma_lli_item_t *dma_link; /**< Pointer to the DMA link list */
	/** Number of buffer links in the ring buffer. */
	uint32_t num_buffer_links;
	qm_i2s_buffer_link_t *buffer_link; /**< Ring buffer head pointer. */
	/** Number of DMA links per audio buffer. */
	uint32_t num_dma_link_per_buffer;
	uint32_t buffer_len;    /**< Length of a buffer in the ring buffer. */
	uint32_t afull_thresh;  /**< TX/RX almost full threshold. */
	uint32_t aempty_thresh; /**< TX/RX almost empty threshold. */

	/**
	 * Transfer callback.
	 *
	 * @param[in] data Callback user data
	 * @param[in] i2s_status I2S status
	 */
	void (*callback)(void *data, qm_i2s_status_t i2s_status);
	void *callback_data; /**< Callback user data. */
} qm_i2s_channel_cfg_data_t;

/**
 * I2S Clock Configuration
 */
typedef struct {
	/** Select Internal NCO or external reference clock for I2S block */
	qm_i2s_clk_src_t i2s_clock_sel;
	/**
	 * Used by the driver when configuring I2S reference clock from
	 * the NCO.
	 */
	uint32_t i2s_clk_freq;
	/** Enable MCLK output. */
	bool i2s_mclk_output_en;
	/**
	 * MCLK output is generated from the Internal
	 * NCO output, normally \@24.576Mhz, using a divisor.
	 * Valid divisor range is 0 to 4095.
	 * E.g. with a 48kHz data rate, and NCO =
	 * 24.576Mhz, then required MCLK = 256 * data rate = 12.288MHz
	 * MCLK divisor = 2 (24.576Mhz/2 = 12.288MHz)
	 */
	uint32_t i2s_mclk_divisor;
	/**
	 * Informs the driver of frequency of external clock
	 * when it is enabled.
	 */
	uint32_t i2s_ext_clk_freq;
} qm_i2s_clock_cfg_data_t;

/**
 * Function to configure specified I2S controller stream
 * Configuration parameters must be valid or an error is returned - see return
 * values below.
 *
 * @param [in] i2s I2S controller identifier
 * @param [in] audio_stream I2S audio_stream identifier
 * @param [in] config Pointer to configuration structure
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_i2s_dma_channel_config(const qm_i2s_t i2s,
			      const qm_i2s_audio_stream_t audio_stream,
			      const qm_i2s_channel_cfg_data_t *const config);

/**
 * Function to place I2S controller into a disabled state.
 * This function assumes that there is no pending transaction on the I2S
 * interface in question.
 * It is the responsibility of the calling application to do so.
 *
 * @param [in] i2s I2S controller identifier
 * @param [in] audio_stream I2S audio_stream identifier
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_i2s_channel_disable(const qm_i2s_t i2s,
			   const qm_i2s_audio_stream_t audio_stream);

/**
 * Function to configure and enable I2S clocks
 * Configuration parameters must be valid or an error is returned - see return
 * values below.
 *
 * @param [in] i2s I2S controller identifier
 * @param [in] i2s_clk_cfg I2S Clock configuration
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_i2s_set_clock_config(const qm_i2s_t i2s,
			    const qm_i2s_clock_cfg_data_t *const i2s_clk_cfg);

/**
 * Function to transmit a block of data to the specified I2S channel.
 *
 * @param [in] i2s I2S controller identifier
 * @param [in] audio_stream I2S audio_stream identifier
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_i2s_dma_transfer(const qm_i2s_t i2s,
			const qm_i2s_audio_stream_t audio_stream);

/**
 * Terminate the current DMA transfer on the given I2S peripheral channel.
 *
 * Terminate the current DMA transfer on the I2S channel.
 * This will cause the relevant callbacks to be invoked.
 *
 * In the case the user is doing a continuous transfer using a circular linked
 * list, it might be useful to fine control at which point in the linked list
 * that we stop the transfer. To do this, set the relevant linked list item's
 * "next" item to NULL.
 *
 * @param [in] i2s I2S controller identifier
 * @param [in] config Pointer to configuration structure
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_i2s_dma_transfer_terminate(const qm_i2s_t i2s,
				  qm_i2s_channel_cfg_data_t *const config);

/**
 * @}
 */

#endif /* __QM_I2S_H__ */
