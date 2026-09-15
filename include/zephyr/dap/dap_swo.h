/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup dap_link_interface
 * @brief SWO trace capture support for the DAP Link API.
 */

#ifndef ZEPHYR_INCLUDE_DAP_DAP_SWO_H
#define ZEPHYR_INCLUDE_DAP_DAP_SWO_H

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/ring_buffer.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup dap_link_interface
 * @{
 */

struct dap_link_context;

/**
 * @brief SWO capture backend.
 *
 * The DAP subsystem is hardware agnostic: capturing the SWO pin
 * (typically with a UART receiver) is the application's job. The
 * application implements this backend, registers it with
 * dap_swo_backend_register(), and pushes captured bytes into the
 * trace buffer with dap_swo_rx() while capture is started.
 */
struct dap_swo_backend {
	/**
	 * Configure the capture rate, in bits per second.
	 *
	 * Called with capture stopped. The backend may adjust the rate
	 * to the nearest one it supports and must write the actual rate
	 * back through @a baudrate; the actual rate is reported to the
	 * host (DAP_SWO_Baudrate response).
	 *
	 * @retval 0 on success, negative errno otherwise.
	 */
	int (*configure)(uint32_t *baudrate);
	/**
	 * Start capturing. While started, the backend pushes received
	 * bytes with dap_swo_rx() (ISR context is fine).
	 *
	 * @retval 0 on success, negative errno otherwise.
	 */
	int (*start)(void);
	/**
	 * Stop capturing. No dap_swo_rx() calls may arrive after this
	 * returns.
	 *
	 * @retval 0 on success, negative errno otherwise.
	 */
	int (*stop)(void);
};

/** @cond INTERNAL_HIDDEN */
struct dap_swo_context {
	const struct dap_swo_backend *backend;
	/* Streaming transport data-available callback (USB backend). */
	void (*stream_kick)(void);
	/* Serializes the consumer side and the capture state machine:
	 * ring reads, the reset on capture start, and the stop paths
	 * (command, disconnect, transport loss) may run on different
	 * threads. The producer (dap_swo_rx, ISR) stays lock-free.
	 */
	struct k_mutex lock;
	struct ring_buf rb;
	uint32_t baudrate;
	/* Latched error flags (trace overrun, stream error), set from ISR
	 * and thread context, reported through the trace status byte and
	 * cleared once reported — see swo_trace_status_take().
	 */
	atomic_t err;
	uint8_t transport;
	uint8_t mode;
	bool active;
	uint8_t buf[CONFIG_DAP_SWO_BUFFER_SIZE];
};
/** @endcond */

/**
 * @brief Register the SWO capture backend.
 *
 * Registering advertises SWO UART support (and, with a streaming
 * capable DAP backend, SWO Streaming Trace) in the DAP capabilities.
 *
 * @param[in] dap_link_ctx DAP Link context.
 * @param[in] backend Capture backend to register.
 *
 * @retval 0 Successfully registered.
 * @retval -EINVAL on a malformed backend (missing callbacks).
 * @retval -EBUSY if trace capture is currently active.
 */
int dap_swo_backend_register(struct dap_link_context *const dap_link_ctx,
			     const struct dap_swo_backend *backend);

/**
 * @brief Whether SWO trace capture is currently started.
 *
 * @param[in] dap_link_ctx DAP Link context.
 *
 * @return true between a successful capture start and the matching
 * stop (from DAP_SWO_Control, DAP_Disconnect, or transport loss).
 */
bool dap_swo_is_active(struct dap_link_context *const dap_link_ctx);

/**
 * @brief Push captured trace bytes into the trace buffer.
 *
 * Called by the capture backend, typically from the receiver ISR.
 * Bytes that do not fit are dropped and the buffer overrun status
 * flag is set until the next capture start.
 *
 * @param[in] dap_link_ctx DAP Link context.
 * @param[in] data Captured bytes.
 * @param[in] len Number of captured bytes.
 *
 * @return Number of bytes stored.
 */
uint32_t dap_swo_rx(struct dap_link_context *const dap_link_ctx,
		    const uint8_t *data, uint32_t len);

/**
 * @brief Report trace data lost in the capture path.
 *
 * Called by the capture backend when its receiver lost trace bytes
 * before they could reach dap_swo_rx() (hardware FIFO overrun, DMA
 * overwrite, ...). Sets the same overrun status flag as an internal
 * trace-buffer overflow. Like the reference implementation, the flag
 * is latched and cleared once reported through a DAP_SWO_Status or
 * DAP_SWO_Data response (and on capture start), so one transient
 * overrun does not read as continuous loss for the rest of the
 * capture. Without this, a receiver-side overrun would read back as
 * a clean capture. Ignored while capture is stopped. ISR context is
 * fine.
 *
 * @param[in] dap_link_ctx DAP Link context.
 */
void dap_swo_overrun(struct dap_link_context *const dap_link_ctx);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ZEPHYR_INCLUDE_DAP_DAP_SWO_H */
