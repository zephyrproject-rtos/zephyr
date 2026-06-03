/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_PULSE_IO_H_
#define ZEPHYR_INCLUDE_DRIVERS_PULSE_IO_H_

/**
 * @file
 * @ingroup pulse_io_interface
 * @brief Main header file for the Pulse IO subsystem API.
 */

#include <errno.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Pulse IO Interface
 * @defgroup pulse_io_interface Pulse IO
 * @since 4.4
 * @version 0.1.0
 * @ingroup io_interfaces
 * @{
 *
 * The pulse_io subsystem abstracts hardware peripherals that generate and
 * capture timed digital edges on a single GPIO line. It is intended as a
 * common API for the dedicated edge-timing peripherals that several MCU
 * families provide under different names.
 *
 * The API supports two submission modes, advertised through
 * @ref pulse_io_caps "capabilities":
 *
 *  - @ref PULSE_IO_MODE_SYMBOL - arbitrary {level, duration} pairs.
 *  - @ref PULSE_IO_MODE_CELL   - fixed-period cells with varying duty
 *                                or level (suits PWM-style hardware).
 *
 * Clients query @ref pulse_io_get_capabilities at probe time to decide
 * which mode and feature set to use.
 */

/** @brief Pulse stream submission modes. */
enum pulse_io_mode {
	/** Arbitrary {level, duration} symbol stream. */
	PULSE_IO_MODE_SYMBOL = BIT(0),
	/** Fixed-period cells with per-cell duty or level. */
	PULSE_IO_MODE_CELL = BIT(1),
};

/** @brief Direction of a configured channel. */
enum pulse_io_dir {
	/** Transmit (generate edges). */
	PULSE_IO_DIR_TX,
	/** Receive (capture edges). */
	PULSE_IO_DIR_RX,
};

/**
 * @brief One symbol of a pulse stream.
 *
 * @c duration is expressed in ticks of the configured @c resolution_hz.
 *
 * @note The bitfield layout is the in-RAM API contract, not a wire or DMA
 * format. Backends that feed hardware by DMA must convert to their own
 * packed representation rather than transferring this struct directly.
 */
struct pulse_symbol {
	/** Duration in ticks of the configured resolution. */
	uint32_t duration: 31;
	/** Line level for the duration (0 or 1). */
	uint32_t level: 1;
};

/**
 * @brief One cell of a fixed-period pulse stream.
 *
 * The cell period is set globally in @ref pulse_io_config.
 * Backends with @c cell_duty_bits > 0 honour @c duty; backends with
 * @c cell_duty_bits == 0 honour @c level only.
 *
 * @note As with @ref pulse_symbol, the bitfield layout is the in-RAM API
 * contract, not a DMA format; backends convert as needed.
 */
struct pulse_cell {
	/** High-time within the cell period, in ticks. */
	uint16_t duty;
	/** Line level for the cell (level-only backends). */
	uint8_t level: 1;
	/** Reserved; must be zero. */
	uint8_t rsvd: 7;
};

/**
 * @brief Per-instance capability descriptor.
 *
 * Returned by @ref pulse_io_get_capabilities. All fields describe the
 * hardware instance, not a specific channel; per-channel restrictions
 * (if any) are documented by the backend.
 */
struct pulse_io_caps {
	/* Fields are ordered widest-first to pack without padding holes. */

	/** Minimum chunk size for streaming (hardware FIFO depth). */
	size_t tx_min_chunk_symbols;
	/** Maximum size of a single streaming transmit; SIZE_MAX if unbounded. */
	size_t tx_max_streaming;

	/** Bitmask of supported @ref pulse_io_mode. */
	uint32_t modes;
	/** Finest tick the hardware can produce, in nanoseconds. */
	uint32_t min_tick_ns;
	/** Coarsest tick with maximum prescaler, in nanoseconds. */
	uint32_t max_tick_ns;
	/** Maximum @c duration per symbol in ticks. */
	uint32_t max_duration_ticks;
	/**
	 * Maximum loop count for TX.
	 *   0           - no loop support
	 *   1           - infinite loop only
	 *   N (>1)      - finite loops up to N, plus infinite
	 */
	uint32_t tx_loop_max;
	/** Maximum end-of-frame idle threshold, in ticks. */
	uint32_t rx_idle_threshold_max;
	/** Maximum glitch-filter width, in ticks; 0 if no filter. */
	uint32_t rx_filter_max_ticks;

	/** Width of the duty field in CELL mode; 0 for level-only cells. */
	uint8_t cell_duty_bits;
	/** Total number of independent channels on this instance. */
	uint8_t num_channels;

	/** Transmit is supported. */
	bool supports_tx;
	/** Receive is supported. */
	bool supports_rx;
	/** Hardware carrier modulation on TX. */
	bool tx_carrier;
	/** Hardware stops automatically at end of loop. */
	bool tx_loop_auto_stop;
	/** Multiple channels can start on the same clock edge. */
	bool tx_sync_multi_channel;
	/** Hardware carrier demodulation on RX. */
	bool rx_carrier_demod;
	/** RX uses a DMA-backed ring buffer (vs. interrupt-drained). */
	bool rx_dma_buffered;
	/** RX supports ping-pong streaming beyond one buffer. */
	bool rx_streaming;
	/** Period can change between submissions in CELL mode. */
	bool cell_period_per_chunk;
};

/**
 * @brief Channel configuration.
 *
 * Passed to @ref pulse_io_channel_configure. Fields not relevant to the
 * selected @c mode or @c dir are ignored.
 */
struct pulse_io_config {
	/** Submission mode for the channel. */
	enum pulse_io_mode mode;
	/** Channel direction. */
	enum pulse_io_dir dir;

	/** Tick rate in Hz. The backend selects the nearest achievable. */
	uint32_t resolution_hz;

	/** CELL mode only: cell period in ticks. */
	uint32_t cell_period_ticks;

	/** Line state when idle. */
	bool idle_high;

	/** Enable TX carrier modulation. */
	bool carrier_en;
	/** TX carrier frequency in Hz. */
	uint32_t carrier_hz;
	/** TX carrier duty cycle in percent. */
	uint8_t carrier_duty_pct;

	/** RX end-of-frame idle threshold in ticks; 0 to disable. */
	uint32_t rx_idle_threshold_ticks;
	/** RX glitch filter width in ticks; 0 to disable. */
	uint32_t rx_filter_ticks;
	/** Enable HW carrier demodulation on RX. */
	bool rx_carrier_demod;
};

/**
 * @brief Opaque per-channel handle.
 *
 * Obtained from @ref pulse_io_channel_get and passed to subsequent calls.
 * The syscall layer does not validate the handle; backends must treat it
 * as untrusted from user mode and range-check or look it up internally.
 */
struct pulse_io_channel;

/* Forward declarations for the RTIO codec vtables (defined below). */
struct pulse_io_encoder_api;
struct pulse_io_decoder_api;

/**
 * @brief TX submission descriptor.
 *
 * Exactly one of @c symbols / @c cells is consulted, selected by the
 * channel's configured @ref pulse_io_mode.
 */
struct pulse_io_tx_req {
	/** Source buffer, interpreted per the configured mode. */
	union {
		/** SYMBOL mode source. */
		const struct pulse_symbol *symbols;
		/** CELL mode source. */
		const struct pulse_cell *cells;
	};
	/** Number of entries in the source buffer. */
	size_t count;
	/** 0 = single shot; 1..tx_loop_max = finite; UINT32_MAX = infinite. */
	uint32_t loop_count;
};

/**
 * @brief RX submission descriptor.
 */
struct pulse_io_rx_req {
	/** Destination buffer, interpreted per the configured mode. */
	union {
		/** SYMBOL mode destination. */
		struct pulse_symbol *symbols;
		/** CELL mode destination. */
		struct pulse_cell *cells;
	};
	/** Capacity of the destination buffer in entries. */
	size_t capacity;
};

/**
 * @brief Driver vtable.
 *
 * Implemented by each backend; not intended for direct application use.
 * The core ops are mandatory and must not be NULL; the RTIO ops are
 * optional and the wrappers return -ENOSYS when they are NULL.
 */
__subsystem struct pulse_io_driver_api {
	/** @driver_ops_optional NULL returns -ENOSYS from the wrapper. */
	int (*get_capabilities)(const struct device *dev, struct pulse_io_caps *caps);

	/** @driver_ops_mandatory */
	int (*channel_get)(const struct device *dev, uint8_t channel_idx,
			   struct pulse_io_channel **chan);

	/** @driver_ops_mandatory */
	int (*channel_release)(const struct device *dev, struct pulse_io_channel *chan);

	/** @driver_ops_mandatory */
	int (*channel_configure)(const struct device *dev, struct pulse_io_channel *chan,
				 const struct pulse_io_config *cfg);

	/** @driver_ops_mandatory */
	int (*transmit_sync)(const struct device *dev, struct pulse_io_channel *chan,
			     const struct pulse_io_tx_req *req, k_timeout_t timeout);

	/** @driver_ops_mandatory */
	int (*receive_sync)(const struct device *dev, struct pulse_io_channel *chan,
			    const struct pulse_io_rx_req *req, size_t *count, k_timeout_t timeout);

	/** @driver_ops_mandatory */
	int (*stop)(const struct device *dev, struct pulse_io_channel *chan);

	/** @driver_ops_optional RTIO submit; NULL if the backend has no RTIO support. */
	void (*submit)(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe);
	/** @driver_ops_optional */
	int (*get_encoder)(const struct device *dev, const struct pulse_io_encoder_api **api);
	/** @driver_ops_optional */
	int (*get_decoder)(const struct device *dev, const struct pulse_io_decoder_api **api);
};

/**
 * @brief Query backend capabilities.
 *
 * @retval 0 on success.
 * @retval -ENOSYS if the backend does not implement this.
 */
__syscall int pulse_io_get_capabilities(const struct device *dev, struct pulse_io_caps *caps);

static inline int z_impl_pulse_io_get_capabilities(const struct device *dev,
						   struct pulse_io_caps *caps)
{
	const struct pulse_io_driver_api *api = DEVICE_API_GET(pulse_io, dev);

	if (api->get_capabilities == NULL) {
		return -ENOSYS;
	}
	return api->get_capabilities(dev, caps);
}

/**
 * @brief Reserve a channel for exclusive use.
 *
 * @param dev          Pulse IO device.
 * @param channel_idx  Hardware channel index, 0..caps.num_channels - 1.
 * @param chan         Output: handle for subsequent calls.
 *
 * @retval 0 on success.
 * @retval -EBUSY if the channel is already reserved.
 * @retval -ENODEV if @c channel_idx is out of range.
 */
__syscall int pulse_io_channel_get(const struct device *dev, uint8_t channel_idx,
				   struct pulse_io_channel **chan);

static inline int z_impl_pulse_io_channel_get(const struct device *dev, uint8_t channel_idx,
					      struct pulse_io_channel **chan)
{
	return DEVICE_API_GET(pulse_io, dev)->channel_get(dev, channel_idx, chan);
}

/**
 * @brief Release a channel previously obtained via @ref pulse_io_channel_get.
 */
__syscall int pulse_io_channel_release(const struct device *dev, struct pulse_io_channel *chan);

static inline int z_impl_pulse_io_channel_release(const struct device *dev,
						  struct pulse_io_channel *chan)
{
	return DEVICE_API_GET(pulse_io, dev)->channel_release(dev, chan);
}

/**
 * @brief Configure a reserved channel.
 *
 * Must be called before any transmit / receive on the channel. May be
 * called again to reconfigure between operations.
 *
 * @retval 0 on success.
 * @retval -ENOTSUP if @c cfg requests an unsupported mode or feature.
 * @retval -EINVAL if @c cfg has out-of-range values.
 */
__syscall int pulse_io_channel_configure(const struct device *dev, struct pulse_io_channel *chan,
					 const struct pulse_io_config *cfg);

static inline int z_impl_pulse_io_channel_configure(const struct device *dev,
						    struct pulse_io_channel *chan,
						    const struct pulse_io_config *cfg)
{
	return DEVICE_API_GET(pulse_io, dev)->channel_configure(dev, chan, cfg);
}

/**
 * @brief Blocking transmit.
 *
 * Returns once the entire @c req has been transmitted or the timeout
 * elapses. Infinite loops (@c loop_count == UINT32_MAX) cannot be used
 * with this call; use the RTIO path instead.
 */
__syscall int pulse_io_transmit_sync(const struct device *dev, struct pulse_io_channel *chan,
				     const struct pulse_io_tx_req *req, k_timeout_t timeout);

static inline int z_impl_pulse_io_transmit_sync(const struct device *dev,
						struct pulse_io_channel *chan,
						const struct pulse_io_tx_req *req,
						k_timeout_t timeout)
{
	return DEVICE_API_GET(pulse_io, dev)->transmit_sync(dev, chan, req, timeout);
}

/**
 * @brief Blocking receive.
 *
 * One-shot capture terminated by idle threshold or buffer full.
 * @c count is set to the number of valid entries on success.
 */
__syscall int pulse_io_receive_sync(const struct device *dev, struct pulse_io_channel *chan,
				    const struct pulse_io_rx_req *req, size_t *count,
				    k_timeout_t timeout);

static inline int z_impl_pulse_io_receive_sync(const struct device *dev,
					       struct pulse_io_channel *chan,
					       const struct pulse_io_rx_req *req, size_t *count,
					       k_timeout_t timeout)
{
	return DEVICE_API_GET(pulse_io, dev)->receive_sync(dev, chan, req, count, timeout);
}

/**
 * @brief Stop any in-flight transmit or receive on the channel.
 */
__syscall int pulse_io_stop(const struct device *dev, struct pulse_io_channel *chan);

static inline int z_impl_pulse_io_stop(const struct device *dev, struct pulse_io_channel *chan)
{
	return DEVICE_API_GET(pulse_io, dev)->stop(dev, chan);
}

/**
 * @name Encoder helpers (pre-expansion)
 *
 * v1 ships pre-expansion helpers only: each call expands a protocol
 * payload into a fully-resolved @ref pulse_symbol array which the caller
 * then submits via @ref pulse_io_transmit_sync or the RTIO path.
 *
 * A future revision may add a streaming variant that consumes payload
 * incrementally from the TX ISR; that path will be additive and will
 * not change the entry points defined here.
 * @{
 */

/**
 * @brief Per-bit template used by @ref pulse_io_encode_bytes.
 *
 * Each input bit expands to a pair of @ref pulse_symbol entries.
 */
struct pulse_io_bit_template {
	/** Symbol pair emitted for a zero bit. */
	struct pulse_symbol zero[2];
	/** Symbol pair emitted for a one bit. */
	struct pulse_symbol one[2];
	/** true: MSB first; false: LSB first. */
	bool msb_first;
};

/**
 * @brief Expand a byte stream into pulse symbols using a bit template.
 *
 * @param tmpl     Bit template.
 * @param bytes    Input byte stream.
 * @param nbytes   Length of @c bytes.
 * @param out      Output buffer.
 * @param out_cap  Capacity of @c out in symbols.
 * @param produced Output: number of symbols written.
 *
 * @retval 0 on success.
 * @retval -ENOMEM if @c out_cap is insufficient.
 */
int pulse_io_encode_bytes(const struct pulse_io_bit_template *tmpl, const uint8_t *bytes,
			  size_t nbytes, struct pulse_symbol *out, size_t out_cap,
			  size_t *produced);

/** @} */

/**
 * @name RTIO integration
 *
 * A backend may expose an RTIO submit path plus protocol encoder and
 * decoder vtables, mirroring the sensor subsystem. With these the
 * userspace and asynchronous I/O machinery is provided by RTIO: the
 * client encodes a payload into a symbol buffer, submits it through an
 * RTIO queue, and decodes any captured reply.
 * @{
 */

/**
 * @brief Encoder vtable: protocol bytes to pulse symbols.
 */
struct pulse_io_encoder_api {
	/** Expand a byte stream into pulse symbols. */
	int (*encode)(const struct pulse_io_bit_template *tmpl, const uint8_t *bytes, size_t nbytes,
		      struct pulse_symbol *out, size_t out_cap, size_t *produced);
};

/**
 * @brief Decoder vtable: pulse symbols to protocol bytes.
 *
 * @c tolerance_ticks gives the per-symbol slack used when matching a
 * captured duration against the bit template, to absorb timing drift on
 * received streams.
 */
struct pulse_io_decoder_api {
	/** Rebuild a byte stream from captured pulse symbols. */
	int (*decode)(const struct pulse_io_bit_template *tmpl, uint32_t tolerance_ticks,
		      const struct pulse_symbol *in, size_t nsyms, uint8_t *out, size_t out_cap,
		      size_t *produced);
};

/**
 * @brief Fetch the backend encoder vtable.
 *
 * @retval 0 on success.
 * @retval -ENOSYS if the backend exposes no RTIO codec.
 */
__syscall int pulse_io_get_encoder(const struct device *dev,
				   const struct pulse_io_encoder_api **api);

static inline int z_impl_pulse_io_get_encoder(const struct device *dev,
					      const struct pulse_io_encoder_api **api)
{
	const struct pulse_io_driver_api *drv = DEVICE_API_GET(pulse_io, dev);

	if (drv->get_encoder == NULL) {
		return -ENOSYS;
	}
	return drv->get_encoder(dev, api);
}

/**
 * @brief Fetch the backend decoder vtable.
 *
 * @retval 0 on success.
 * @retval -ENOSYS if the backend exposes no RTIO codec.
 */
__syscall int pulse_io_get_decoder(const struct device *dev,
				   const struct pulse_io_decoder_api **api);

static inline int z_impl_pulse_io_get_decoder(const struct device *dev,
					      const struct pulse_io_decoder_api **api)
{
	const struct pulse_io_driver_api *drv = DEVICE_API_GET(pulse_io, dev);

	if (drv->get_decoder == NULL) {
		return -ENOSYS;
	}
	return drv->get_decoder(dev, api);
}

#ifdef CONFIG_RTIO
/**
 * @brief Submit an RTIO request to a pulse_io device.
 *
 * Routes an RTIO submission queue entry to the backend. Intended to be
 * called from a backend iodev submit handler, not directly by clients.
 */
static inline void pulse_io_iodev_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct pulse_io_driver_api *drv = DEVICE_API_GET(pulse_io, dev);

	if (drv->submit == NULL) {
		rtio_iodev_sqe_err(iodev_sqe, -ENOSYS);
		return;
	}
	drv->submit(dev, iodev_sqe);
}
#endif /* CONFIG_RTIO */

/**
 * @brief Default symbol-stream decoder.
 *
 * Inverse of @ref pulse_io_encode_bytes(): matches consecutive symbol
 * pairs against @c tmpl within @c tolerance_ticks and rebuilds the byte
 * stream. Backends may point their decoder vtable at this.
 */
int pulse_io_decode_bytes(const struct pulse_io_bit_template *tmpl, uint32_t tolerance_ticks,
			  const struct pulse_symbol *in, size_t nsyms, uint8_t *out, size_t out_cap,
			  size_t *produced);

/** @} */

/** @} */

#ifdef __cplusplus
}
#endif

#include <zephyr/syscalls/pulse_io.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_PULSE_IO_H_ */
