#ifndef LWM2M_RAM_CACHE_H
#define LWM2M_RAM_CACHE_H

#include <zephyr/sys/ring_buffer.h>
#include <zephyr/net/lwm2m.h>

struct lwm2m_ram_cache_rw_ctx {
	struct ring_buf rb;

	int32_t original_get_head;
	int32_t original_get_tail;
	int32_t original_get_base;
};

void* lwm2m_time_series_ram_cache_init(int entry_index, struct lwm2m_time_series_elem *cache_buf, size_t cache_len);
bool lwm2m_time_series_ram_cache_write(void *read_write_ctx, struct lwm2m_time_series_elem *buf);
void lwm2m_time_series_ram_cache_read_begin(void *read_write_ctx);
bool lwm2m_time_series_ram_cache_read_next(void *read_write_ctx, struct lwm2m_time_series_elem *buf);
void lwm2m_time_series_ram_cache_read_end(void *read_write_ctx, bool success);
size_t lwm2m_time_series_ram_cache_size(void *read_write_ctx);

#endif /* LWM2M_RAM_CACHE_H */
