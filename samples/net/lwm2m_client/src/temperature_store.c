#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(app_temp);

#include "temperature_store.h"

/* Simple ring buffer implementation to show application storage for
 * LwM2M time series.
 */

struct temperature_rw_ctx temperature_rw_ctx;

size_t temperature_size_cb(void *ctx)
{
	struct temperature_rw_ctx *rw_ctx = (struct temperature_rw_ctx*)ctx;
	return (TEMPERATURE_DATA_BUF_SIZE + rw_ctx->write_index - rw_ctx->read_index) % TEMPERATURE_DATA_BUF_SIZE;
}

bool temperature_write_cb(void *ctx, struct lwm2m_time_series_elem *buf)
{
	struct temperature_rw_ctx *rw_ctx = (struct temperature_rw_ctx*)ctx;

	LOG_INF("Write temperature data (t=%lld, v=%f) at index %d", buf->t, buf->f, rw_ctx->write_index);
	memcpy(&rw_ctx->data[rw_ctx->write_index], buf, sizeof(struct lwm2m_time_series_elem));

	if (((rw_ctx->write_index + 1) % TEMPERATURE_DATA_BUF_SIZE) == rw_ctx->read_index) {
		/* Buffer is full, drop oldest */
		rw_ctx->read_index = (rw_ctx->read_index + 1) % TEMPERATURE_DATA_BUF_SIZE;
	}
	rw_ctx->write_index = (rw_ctx->write_index + 1) % TEMPERATURE_DATA_BUF_SIZE;

	return true;
}

void temperature_read_begin_cb(void *ctx)
{
	struct temperature_rw_ctx *read_ctx = (struct temperature_rw_ctx*)ctx;
	read_ctx->temp_read_index = read_ctx->read_index;
}

bool temperature_read_next_cb(void *ctx, struct lwm2m_time_series_elem *buf)
{
	struct temperature_rw_ctx *rw_ctx = (struct temperature_rw_ctx*)ctx;

	buf->t = rw_ctx->data[rw_ctx->read_index].t;
	buf->f = rw_ctx->data[rw_ctx->read_index].f;
	rw_ctx->read_index = (rw_ctx->read_index + 1) % TEMPERATURE_DATA_BUF_SIZE;

	return true;
}

void temperature_read_end_cb(void *ctx, bool success)
{
	struct temperature_rw_ctx *read_ctx = (struct temperature_rw_ctx*)ctx;
	if (!success) {
		read_ctx->read_index = read_ctx->temp_read_index;
	}

	LOG_INF("Read end, up to index %d returned %d",
	        (TEMPERATURE_DATA_BUF_SIZE + read_ctx->read_index - 1) % TEMPERATURE_DATA_BUF_SIZE,
	        success);
}

void temperature_read_reply_cb(void *ctx, bool timeout, uint8_t reply)
{
	struct temperature_rw_ctx *read_ctx = (struct temperature_rw_ctx*)ctx;
	LOG_INF("Read acked, up to index %d, got reply %d (timeout=%d)",
	        (TEMPERATURE_DATA_BUF_SIZE + read_ctx->read_index - 1) % TEMPERATURE_DATA_BUF_SIZE,
	        reply, timeout);
}
