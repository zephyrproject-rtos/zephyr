/*
 * Copyright (c) 2026 Analog Devices, Inc.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Public interface for the Analog Devices AD463x precision SAR ADC driver.
 *
 * This is a specialized AXI offload driver. The data path is:
 *
 *   PWMGEN (CNV trigger) → SPI Engine (offload program) → DMAC → DDR
 *
 * All sample acquisition happens in hardware with no per-sample CPU cost.
 * The primary API is ad463x_read_buffer(): arm one DMA transfer, block
 * until N samples arrive, return the raw binary buffer.  Call it
 * back-to-back for continuous acquisition.
 *
 * The driver also registers a Zephyr ADC API shim (adc_read / adc_channel_setup)
 * for compatibility with generic Zephyr code.  The shim runs the DMAC in
 * cyclic mode and snapshots the latest ring slot on each adc_read() call.
 * It is suitable for slow polling (DC levels, occasional reads) but discards
 * the vast majority of samples at high throughput — use ad463x_read_buffer()
 * for any application where sample integrity matters.
 *
 * Based on the no-OS reference driver and the Linux IIO driver ad4630.c.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_ADC_AD463X_H_
#define ZEPHYR_INCLUDE_DRIVERS_ADC_AD463X_H_

#include <zephyr/device.h>
#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>

/**
 * AD463x part variants supported by this driver. Maps to the adi,part
 * string in the binding.
 */
enum ad463x_id {
	AD463X_ID_AD4630_24 = 0, /**< AD4630-24, 2 channels, 24-bit. */
	AD463X_ID_AD4630_20,     /**< AD4630-20, 2 channels, 20-bit. */
	AD463X_ID_AD4630_16,     /**< AD4630-16, 2 channels, 16-bit. */
	AD463X_ID_AD4631_24,     /**< AD4631-24, 4 channels, 24-bit. */
	AD463X_ID_AD4631_20,     /**< AD4631-20, 4 channels, 20-bit. */
	AD463X_ID_AD4631_16,     /**< AD4631-16, 4 channels, 16-bit. */
	AD463X_ID_AD4632_24,     /**< AD4632-24, 1 channel, 24-bit. */
	AD463X_ID_AD4632_20,     /**< AD4632-20, 1 channel, 20-bit. */
	AD463X_ID_AD4632_16,     /**< AD4632-16, 1 channel, 16-bit. */
	AD463X_ID_COUNT,         /**< Number of supported part variants. */
};

/**
 * Primary acquisition entry point.
 *
 * Arms a single AXI DMAC transfer and blocks until @p len bytes of raw
 * conversion data have been written to @p buf by the hardware offload path.
 * No sample is touched by the CPU during capture. Call back-to-back for
 * continuous acquisition.
 *
 * Each frame in the buffer is @ref ad463x_get_frame_size() bytes and contains
 * both channels interleaved: [CH0 uint32][CH1 uint32]. The HDL left-aligns
 * the real bits in each uint32; sign-extend with (int32_t)word >> (32 - real_bits).
 *
 * Cannot be called while the Zephyr ADC API shim has armed the cyclic ring
 * (i.e. after the first adc_read() call). The two paths are mutually exclusive.
 *
 * @param dev  AD463x device.
 * @param buf  Destination buffer. Must be 32-byte aligned and __nocache if
 *             the platform has an L2 cache that the DMAC bypasses.
 * @param len  Capacity of @p buf in bytes.
 *
 * @return Number of bytes written on success, or a negative errno.
 */
ssize_t ad463x_read_buffer(const struct device *dev, void *buf, size_t len);

/**
 * Returns the raw bytes the HDL produces per CNV pulse (both channels combined).
 * Use this to compute sample counts from buffer sizes rather than hardcoding
 * the frame width, which varies with output mode and lane configuration.
 */
size_t ad463x_get_frame_size(const struct device *dev);

/**
 * Returns the real-bit precision of a decoded code (see the sign-extend
 * formula on ad463x_read_buffer()). Depends on the configured output mode;
 * use this instead of hardcoding it, since it varies per adi,output-mode.
 */
uint8_t ad463x_get_real_bits(const struct device *dev);

/**
 * Bring the chip out of reset, program the AXI CLKGEN, verify the chip is
 * alive via a scratchpad round-trip, and apply the capture configuration from
 * devicetree. Called automatically on the first capture; exposed here for
 * applications that need explicit control over reset and CNV timing.
 */
int ad463x_init_chip(const struct device *dev);

#endif /* ZEPHYR_INCLUDE_DRIVERS_ADC_AD463X_H_ */
