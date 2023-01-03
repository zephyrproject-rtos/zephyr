#define LOG_MODULE_NAME net_lwm2m_ram_cache
#define LOG_LEVEL	CONFIG_LWM2M_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <zephyr/sys/ring_buffer.h>

#include "lwm2m_ram_cache.h"

static struct lwm2m_ram_cache_rw_ctx lwm2m_time_series_ram_cache_rw_ctx[CONFIG_LWM2M_MAX_STORED_TIME_SERIES_RESOURCES];

void* lwm2m_time_series_ram_cache_init(int entry_index, struct lwm2m_time_series_elem *cache_buf, size_t cache_len)
{
	size_t cache_entry_size = sizeof(struct lwm2m_time_series_elem);

	LOG_INF("Init ring buffer in ram_cache_rw_ctx[%d]", entry_index);
	ring_buf_init(&lwm2m_time_series_ram_cache_rw_ctx[entry_index].rb, cache_entry_size * cache_len, (uint8_t *)cache_buf);

	return &lwm2m_time_series_ram_cache_rw_ctx[entry_index];
}

bool lwm2m_time_series_ram_cache_write(void *read_write_ctx, struct lwm2m_time_series_elem *buf)
{
#if defined(CONFIG_LWM2M_RESOURCE_TIME_SERIES_RAM_CACHE)
	uint32_t length;
	uint8_t *buf_ptr;
	uint32_t element_size = sizeof(struct lwm2m_time_series_elem);
	struct lwm2m_ram_cache_rw_ctx *ctx = (struct lwm2m_ram_cache_rw_ctx*)read_write_ctx;

	if (ring_buf_space_get(&ctx->rb) < element_size) {
		/* No space  */
		if (IS_ENABLED(CONFIG_LWM2M_RAM_CACHE_DROP_LATEST)) {
			return false;
		}
		/* Free entry */
		length = ring_buf_get_claim(&ctx->rb, &buf_ptr, element_size);
		ring_buf_get_finish(&ctx->rb, length);
	}

	length = ring_buf_put_claim(&ctx->rb, &buf_ptr, element_size);

	if (length != element_size) {
		ring_buf_put_finish(&ctx->rb, 0);
		LOG_ERR("Allocation failed %u", length);
		return false;
	}

	ring_buf_put_finish(&ctx->rb, length);
	/* Store data */
	memcpy(buf_ptr, buf, element_size);
	return true;
#else
	return NULL;
#endif
}

void lwm2m_time_series_ram_cache_read_begin(void *read_write_ctx)
{
#if defined(CONFIG_LWM2M_RESOURCE_TIME_SERIES_RAM_CACHE)
	struct lwm2m_ram_cache_rw_ctx *ctx = (struct lwm2m_ram_cache_rw_ctx*)read_write_ctx;
	ctx->original_get_base = ctx->rb.get_base;
	ctx->original_get_head = ctx->rb.get_head;
	ctx->original_get_tail = ctx->rb.get_tail;
#endif
}

bool lwm2m_time_series_ram_cache_read_next(void *read_write_ctx, struct lwm2m_time_series_elem *buf)
{
#if defined(CONFIG_LWM2M_RESOURCE_TIME_SERIES_RAM_CACHE)
	uint32_t length;
	uint8_t *buf_ptr;
	uint32_t element_size = sizeof(struct lwm2m_time_series_elem);
	struct lwm2m_ram_cache_rw_ctx *ctx = (struct lwm2m_ram_cache_rw_ctx*)read_write_ctx;

	if (ring_buf_is_empty(&ctx->rb)) {
		return false;
	}

	length = ring_buf_get_claim(&ctx->rb, &buf_ptr, element_size);

	if (length != element_size) {
		LOG_ERR("Time series read fail %u", length);
		ring_buf_get_finish(&ctx->rb, 0);
		return false;
	}

	/* Read Data */
	memcpy(buf, buf_ptr, element_size);
	ring_buf_get_finish(&ctx->rb, length);
	return true;

#else
	return NULL;
#endif
}

void lwm2m_time_series_ram_cache_read_end(void *read_write_ctx, bool success)
{
#if defined(CONFIG_LWM2M_RESOURCE_TIME_SERIES_RAM_CACHE)
	struct lwm2m_ram_cache_rw_ctx *ctx = (struct lwm2m_ram_cache_rw_ctx*)read_write_ctx;
	if (!success) {
		/* Reset ring buffer state */
		ctx->rb.get_head = ctx->original_get_head;
		ctx->rb.get_tail = ctx->original_get_tail;
		ctx->rb.get_base = ctx->original_get_base;
	}
#endif
}

size_t lwm2m_time_series_ram_cache_size(void *read_write_ctx)
{
#if defined(CONFIG_LWM2M_RESOURCE_TIME_SERIES_RAM_CACHE)
	struct lwm2m_ram_cache_rw_ctx *ctx = (struct lwm2m_ram_cache_rw_ctx*)read_write_ctx;
	uint32_t bytes_available;

	if (ring_buf_is_empty(&ctx->rb)) {
		return 0;
	}

	bytes_available = ring_buf_size_get(&ctx->rb);

	return (bytes_available / sizeof(struct lwm2m_time_series_elem));
#else
	return 0;
#endif
}
