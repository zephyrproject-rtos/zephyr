/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * SWO trace capture: DAP_SWO_* command handlers and the trace buffer
 * between the application's capture backend and the host transport.
 *
 * The capture hardware (typically a UART receiver on the SWO pin) is
 * application property; see struct dap_swo_backend. Captured bytes
 * land in a ring buffer and leave either through DAP_SWO_Data
 * responses (transport 1) or through the DAP backend's dedicated
 * trace endpoint (transport 2, e.g. the USB backend's bulk IN trace
 * endpoint).
 *
 * Concurrency: the ring buffer has one producer (the capture backend,
 * usually ISR context via dap_swo_rx()) and, by design, one consumer
 * at a time -- but the consumer side cannot be left to the transport
 * setting alone. The streaming transport drains from a work item
 * that outlives both a capture stop (the post-stop residue drain is
 * intentional) and a transport switch, and capture teardown is
 * reachable from the command executor (DAP_SWO_Control, DAP_Disconnect)
 * and the DAP backend (transport loss). The context lock serializes
 * all of it: every consumer-side ring access, the ring reset on
 * capture start, and the stop path hold it, so a restart cannot race
 * a late drain and a transport switch cannot leave two live
 * consumers. dap_swo_rx() stays lock-free (ISR): the ring buffer
 * supports one concurrent producer against one concurrent consumer,
 * and the reset in dap_swo_control_cmd() runs with capture stopped,
 * when no producer exists.
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/dap/dap_link.h>
#include <zephyr/dap/dap_swo.h>

#include <cmsis_dap.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(dap, CONFIG_DAP_LOG_LEVEL);

/* Latched error flags in dap_swo_context.err. Split so a lost USB
 * transfer (stream error, bit 6) stays distinguishable from a full
 * ring or receiver overrun (buffer overrun, bit 7), as the protocol
 * intends.
 */
#define SWO_ERR_OVERRUN BIT(0)
#define SWO_ERR_STREAM  BIT(1)

/*
 * Compose the trace status byte and consume the latched error flags,
 * mirroring the reference implementation: error bits are reported
 * once (in a DAP_SWO_Status or DAP_SWO_Data response) and cleared, so
 * a single transient loss does not read as continuous loss for the
 * rest of the capture. The atomic read-and-clear gives the same
 * guarantee the reference's banked flags do -- an error racing the
 * read lands in the next report instead of being lost.
 */
static uint8_t swo_trace_status_take(struct dap_swo_context *const swo)
{
	atomic_val_t err = atomic_clear(&swo->err);
	uint8_t status = 0U;

	if (swo->active) {
		status |= DAP_SWO_STATUS_ACTIVE;
	}

	if (err & SWO_ERR_STREAM) {
		status |= DAP_SWO_STATUS_ERROR;
	}

	if (err & SWO_ERR_OVERRUN) {
		status |= DAP_SWO_STATUS_OVERRUN;
	}

	return status;
}

void dap_swo_init(struct dap_link_context *const ctx)
{
	struct dap_swo_context *const swo = &ctx->swo;

	swo->backend = NULL;
	swo->stream_kick = NULL;
	swo->baudrate = 0U;
	swo->transport = DAP_SWO_TRANSPORT_NONE;
	swo->mode = DAP_SWO_MODE_OFF;
	swo->active = false;
	atomic_clear(&swo->err);
	k_mutex_init(&swo->lock);
	ring_buf_init(&swo->rb, sizeof(swo->buf), swo->buf);
}

int dap_swo_backend_register(struct dap_link_context *const ctx,
			     const struct dap_swo_backend *backend)
{
	struct dap_swo_context *const swo = &ctx->swo;
	int ret = 0;

	if (backend == NULL || backend->configure == NULL ||
	    backend->start == NULL || backend->stop == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&swo->lock, K_FOREVER);

	if (swo->active) {
		ret = -EBUSY;
	} else {
		swo->backend = backend;
		ctx->capabilities |= DAP_SWO_SUPPORTS_UART;
		/* Streaming Trace is advertised from the same fact the
		 * transport-2 admission checks: a bound stream kick. A
		 * compile-time check would advertise it even when the
		 * streaming-capable DAP backend failed to come up, and
		 * the host would get DAP_ERROR from DAP_SWO_Transport(2)
		 * on a capability the probe itself reported.
		 */
		if (swo->stream_kick != NULL) {
			ctx->capabilities |= DAP_SWO_SUPPORTS_STREAM;
		}
	}

	k_mutex_unlock(&swo->lock);

	return ret;
}

void dap_swo_stream_bind(struct dap_link_context *const ctx,
			 void (*kick)(void))
{
	ctx->swo.stream_kick = kick;

	/* Registration may have run first; the capability follows the
	 * bind in either order.
	 */
	if (kick != NULL && ctx->swo.backend != NULL) {
		ctx->capabilities |= DAP_SWO_SUPPORTS_STREAM;
	}
}

bool dap_swo_is_active(struct dap_link_context *const ctx)
{
	return ctx->swo.active;
}

uint32_t dap_swo_rx(struct dap_link_context *const ctx,
		    const uint8_t *data, uint32_t len)
{
	struct dap_swo_context *const swo = &ctx->swo;
	uint32_t stored;

	if (!swo->active) {
		return 0U;
	}

	stored = ring_buf_put(&swo->rb, data, len);
	if (stored < len) {
		atomic_or(&swo->err, SWO_ERR_OVERRUN);
	}

	if (swo->transport == DAP_SWO_TRANSPORT_EP &&
	    swo->stream_kick != NULL && stored > 0U) {
		swo->stream_kick();
	}

	return stored;
}

void dap_swo_overrun(struct dap_link_context *const ctx)
{
	struct dap_swo_context *const swo = &ctx->swo;

	if (!swo->active) {
		return;
	}

	atomic_or(&swo->err, SWO_ERR_OVERRUN);
}

uint32_t dap_swo_read(struct dap_link_context *const ctx,
		      uint8_t *dst, uint32_t max_len)
{
	struct dap_swo_context *const swo = &ctx->swo;
	uint32_t len;

	k_mutex_lock(&swo->lock, K_FOREVER);

	/*
	 * The streaming drain may still be scheduled after the host
	 * switched transports (legal while capture is inactive): a
	 * DAP_SWO_Data poll on the new transport would then contend
	 * for the same ring. Deciding transport and consuming under
	 * the lock keeps "one consumer" true across the switch --
	 * a stale drain reads zero instead of stealing bytes.
	 */
	if (swo->transport != DAP_SWO_TRANSPORT_EP) {
		len = 0U;
	} else {
		len = ring_buf_get(&swo->rb, dst, max_len);
	}

	k_mutex_unlock(&swo->lock);

	return len;
}

void dap_swo_stream_error(struct dap_link_context *const ctx)
{
	struct dap_swo_context *const swo = &ctx->swo;

	/*
	 * Trace bytes already consumed out of the ring were lost in
	 * the transport (e.g. a cancelled USB transfer). This is the
	 * stream-error status bit, distinct from a buffer overrun,
	 * and unlike dap_swo_overrun() it must report even after a
	 * capture stop: the post-stop residue drain can lose bytes
	 * the same way.
	 */
	atomic_or(&swo->err, SWO_ERR_STREAM);
}

void dap_swo_capture_stop(struct dap_link_context *const ctx)
{
	struct dap_swo_context *const swo = &ctx->swo;

	/*
	 * Reachable from DAP_SWO_Control(0), from DAP_Disconnect, and
	 * from the DAP backend when its transport goes away (e.g. USB
	 * configuration lost). The last two exist because capture
	 * state must not outlive the host that created it: a debugger
	 * that crashed or was unplugged mid-capture never sends the
	 * stop command, and the leaked capture would hold its receiver
	 * (and, in the application, whatever the backend claimed)
	 * until reboot. Serialized by the lock so concurrent stop
	 * paths cannot double-invoke the backend.
	 */
	k_mutex_lock(&swo->lock, K_FOREVER);

	if (swo->active) {
		swo->active = false;
		(void)swo->backend->stop();
		LOG_DBG("SWO capture stopped");
	}

	k_mutex_unlock(&swo->lock);
}

uint16_t dap_swo_transport_cmd(struct dap_link_context *const ctx,
			       const uint8_t *const request,
			       uint8_t *const response)
{
	struct dap_swo_context *const swo = &ctx->swo;
	uint8_t transport = request[0];
	bool supported;

	switch (transport) {
	case DAP_SWO_TRANSPORT_NONE:
	case DAP_SWO_TRANSPORT_CMD:
		supported = true;
		break;
	case DAP_SWO_TRANSPORT_EP:
		/* Only with a streaming capable DAP backend bound. */
		supported = swo->stream_kick != NULL;
		break;
	default:
		supported = false;
		break;
	}

	k_mutex_lock(&swo->lock, K_FOREVER);

	if (!supported || swo->active) {
		k_mutex_unlock(&swo->lock);
		LOG_ERR("SWO transport %u rejected (%s)", transport,
			swo->active ? "capture active" : "unsupported");
		response[0] = DAP_ERROR;
		return 1U;
	}

	swo->transport = transport;
	k_mutex_unlock(&swo->lock);

	LOG_DBG("SWO transport %u", transport);
	response[0] = DAP_OK;

	return 1U;
}

uint16_t dap_swo_mode_cmd(struct dap_link_context *const ctx,
			  const uint8_t *const request,
			  uint8_t *const response)
{
	struct dap_swo_context *const swo = &ctx->swo;
	uint8_t mode = request[0];

	k_mutex_lock(&swo->lock, K_FOREVER);

	if ((mode != DAP_SWO_MODE_OFF && mode != DAP_SWO_MODE_UART) ||
	    swo->active) {
		k_mutex_unlock(&swo->lock);
		LOG_ERR("SWO mode %u rejected (%s)", mode,
			swo->active ? "capture active" : "unsupported");
		response[0] = DAP_ERROR;
		return 1U;
	}

	swo->mode = mode;
	k_mutex_unlock(&swo->lock);

	LOG_DBG("SWO mode %u", mode);
	response[0] = DAP_OK;

	return 1U;
}

uint16_t dap_swo_baudrate_cmd(struct dap_link_context *const ctx,
			      const uint8_t *const request,
			      uint8_t *const response)
{
	struct dap_swo_context *const swo = &ctx->swo;
	uint32_t baudrate = sys_get_le32(request);

	k_mutex_lock(&swo->lock, K_FOREVER);

	if (swo->active) {
		/* The rate cannot change under a running capture; refuse
		 * without touching the rate that capture is using.
		 */
		baudrate = 0U;
	} else if (swo->backend == NULL || baudrate == 0U ||
		   swo->backend->configure(&baudrate) != 0) {
		/* A refused configuration leaves NO configured rate: zero
		 * was reported, so a later DAP_SWO_Control(1) must not
		 * silently capture at a stale rate the host no longer
		 * believes in.
		 */
		swo->baudrate = 0U;
		baudrate = 0U;
	} else {
		swo->baudrate = baudrate;
	}

	k_mutex_unlock(&swo->lock);

	LOG_DBG("SWO baudrate %u", baudrate);
	sys_put_le32(baudrate, &response[0]);

	return 4U;
}

uint16_t dap_swo_control_cmd(struct dap_link_context *const ctx,
			     const uint8_t *const request,
			     uint8_t *const response)
{
	struct dap_swo_context *const swo = &ctx->swo;
	uint8_t control = request[0];

	if (control == 0U) {
		dap_swo_capture_stop(ctx);
		response[0] = DAP_OK;
		return 1U;
	}

	k_mutex_lock(&swo->lock, K_FOREVER);

	if (control != 1U || swo->active || swo->backend == NULL ||
	    swo->mode != DAP_SWO_MODE_UART ||
	    swo->transport == DAP_SWO_TRANSPORT_NONE ||
	    swo->baudrate == 0U) {
		k_mutex_unlock(&swo->lock);
		LOG_ERR("SWO control %u rejected "
			"(active %u mode %u transport %u baudrate %u)",
			control, swo->active, swo->mode, swo->transport,
			swo->baudrate);
		response[0] = DAP_ERROR;
		return 1U;
	}

	/*
	 * Under the lock: the streaming transport's work item may
	 * still be draining the previous session's residue, and the
	 * ring buffer tolerates exactly one concurrent consumer --
	 * a reset against a running ring_buf_get() corrupts the
	 * indices with no error. Holding the lock across the reset
	 * (and the get, in dap_swo_read()) makes the restart wait
	 * for a drain in flight instead of racing it.
	 */
	ring_buf_reset(&swo->rb);
	atomic_clear(&swo->err);

	/* active goes true BEFORE the backend enables its receiver: the
	 * capture ISR can fire in the gap, and dap_swo_rx() drops bytes on
	 * !active without raising SWO_ERR_OVERRUN — silent capture-path
	 * loss, the class the error bits exist to report. Setting it early
	 * is safe: the ring was just reset and no producer exists until
	 * start() returns.
	 */
	swo->active = true;

	if (swo->backend->start() != 0) {
		swo->active = false;
		k_mutex_unlock(&swo->lock);
		response[0] = DAP_ERROR;
		return 1U;
	}

	k_mutex_unlock(&swo->lock);

	LOG_DBG("SWO capture started");
	response[0] = DAP_OK;

	return 1U;
}

uint16_t dap_swo_status_cmd(struct dap_link_context *const ctx,
			    uint8_t *const response)
{
	struct dap_swo_context *const swo = &ctx->swo;

	k_mutex_lock(&swo->lock, K_FOREVER);
	response[0] = swo_trace_status_take(swo);
	sys_put_le32(ring_buf_size_get(&swo->rb), &response[1]);
	k_mutex_unlock(&swo->lock);

	return 5U;
}

uint16_t dap_swo_data_cmd(struct dap_link_context *const ctx,
			  const uint8_t *const request,
			  uint8_t *const response)
{
	struct dap_swo_context *const swo = &ctx->swo;
	uint16_t max_count = sys_get_le16(request);
	uint32_t count = 0U;

	k_mutex_lock(&swo->lock, K_FOREVER);

	response[0] = swo_trace_status_take(swo);

	if (swo->transport == DAP_SWO_TRANSPORT_CMD) {
		/*
		 * Response layout: command byte (accounted by the
		 * caller), trace status, two count bytes, data.
		 */
		max_count = MIN(max_count, ctx->pkt_size - 4U);
		count = ring_buf_get(&swo->rb, &response[3], max_count);
	}

	k_mutex_unlock(&swo->lock);

	sys_put_le16(count, &response[1]);

	return 3U + count;
}
