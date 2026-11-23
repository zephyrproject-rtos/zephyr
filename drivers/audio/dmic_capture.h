/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief DMIC capture helper functions.
 */

#ifndef ZEPHYR_DRIVERS_AUDIO_DMIC_CAPTURE_H_
#define ZEPHYR_DRIVERS_AUDIO_DMIC_CAPTURE_H_

#include <stddef.h>
#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>

/** Capture parameters; the caller fills these in before dmic_capture_start(). */
struct dmic_capture_cfg {
	uint32_t sample_rate;       /**< Sample rate in Hz. */
	uint8_t channels;           /**< Number of channels, 1 or 2. */
	uint8_t pcm_width;          /**< Sample width in bits: 8, 16, 24 or 32. */
	uint16_t block_duration_ms; /**< Milliseconds of audio per block. */
	uint8_t block_count;        /**< Number of buffers in the pool (>= 2). */
	uint32_t min_pdm_clk_freq;  /**< Minimum PDM clock frequency in Hz. */
	uint32_t max_pdm_clk_freq;  /**< Maximum PDM clock frequency in Hz. */
	uint8_t min_pdm_clk_dc;     /**< Minimum PDM clock duty cycle in percent. */
	uint8_t max_pdm_clk_dc;     /**< Maximum PDM clock duty cycle in percent. */
};

/** Capture state; treat as opaque and pass by pointer to the helpers. */
struct dmic_capture {
	const struct device *dev; /**< Captured-from device. */
	struct k_mem_slab slab;   /**< Slab managing the pool. */
	void *pool;               /**< Heap-allocated buffer pool. */
	uint32_t block_size;      /**< Bytes per captured block, computed from the config. */
};

/**
 * @brief Allocate the pool, configure the DMIC and start it.
 *
 * On success the caller owns the running capture and must call dmic_capture_stop(); on failure
 * everything is unwound here.
 *
 * @param cap Capture state to initialise.
 * @param dev DMIC device.
 * @param cfg Capture parameters.
 *
 * @return 0 On success or a negative errno from underlying driver operations.
 * @retval -EINVAL @p cfg is out of range.
 * @retval -ENOMEM The pool does not fit in the heap.
 */
int dmic_capture_start(struct dmic_capture *cap, const struct device *dev,
		       const struct dmic_capture_cfg *cfg);

/**
 * @brief Read one captured block.
 *
 * @param cap        Running capture.
 * @param[out] buf   Receives a pointer to the captured block.
 * @param[out] size  Receives the block size in bytes.
 * @param timeout_ms Read timeout in milliseconds.
 *
 * @return 0 on success or a negative errno.
 */
int dmic_capture_read(struct dmic_capture *cap, void **buf, size_t *size, int32_t timeout_ms);

/**
 * @brief Return a block obtained from dmic_capture_read() to the pool.
 *
 * @param cap Running capture.
 * @param buf Block to release.
 */
void dmic_capture_free_block(struct dmic_capture *cap, void *buf);

/**
 * @brief Stop a running capture and release its heap pool.
 *
 * @param cap Capture to stop.
 */
void dmic_capture_stop(struct dmic_capture *cap);

#endif /* ZEPHYR_DRIVERS_AUDIO_DMIC_CAPTURE_H_ */
