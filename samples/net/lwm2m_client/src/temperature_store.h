#ifndef TEMPERATURE_STORE_H
#define TEMPERATURE_STORE_H

#include <zephyr/net/lwm2m.h>

#define TEMPERATURE_DATA_BUF_SIZE       (50 + 1)

struct temperature_rw_ctx {
	struct lwm2m_time_series_elem data[TEMPERATURE_DATA_BUF_SIZE];
	int read_index;
	int write_index;
	int temp_read_index;
};

extern struct temperature_rw_ctx temperature_rw_ctx;

size_t temperature_size_cb(void *ctx);
bool temperature_write_cb(void *ctx, struct lwm2m_time_series_elem *buf);
void temperature_read_begin_cb(void *ctx);
bool temperature_read_next_cb(void *ctx, struct lwm2m_time_series_elem *buf);
void temperature_read_end_cb(void *ctx, bool success);
void temperature_read_reply_cb(void *ctx, bool timeout, uint8_t reply);

#endif /* TEMPERATURE_STORE_H */
