/*
 * Copyright (c) 2021 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SRC_MOCK_BACKEND_H__
#define SRC_MOCK_BACKEND_H__

#include <zephyr/logging/log_backend.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const struct log_backend_api mock_log_backend_api;

struct mock_log_backend_msg {
	log_timestamp_t timestamp;
	uint16_t source_id;
	uint16_t domain_id;
	uint8_t level;
	bool check;
	char str[128];
	uint8_t data[32];
	uint32_t data_len;
};

struct mock_log_backend {
	bool do_check;
	bool panic;
	struct mock_log_backend_msg exp_msgs[64];
	int msg_rec_idx;
	int msg_proc_idx;
	int exp_drop_cnt;
	int drop_cnt;
};

void mock_log_backend_reset(const struct log_backend *backend);
void mock_log_backend_check_enable(const struct log_backend *backend);
void mock_log_backend_check_disable(const struct log_backend *backend);

void mock_log_backend_dummy_record(const struct log_backend *backend, int cnt);

void mock_log_backend_drop_record(const struct log_backend *backend, int cnt);

void mock_log_backend_generic_record(const struct log_backend *backend,
				     uint16_t source_id,
				     uint16_t domain_id,
				     uint8_t level,
				     log_timestamp_t timestamp,
				     const char *str,
				     uint8_t *data,
				     uint32_t data_len);

static inline void mock_log_backend_record(const struct log_backend *backend,
					   uint16_t source_id,
					   uint16_t domain_id,
					   uint8_t level,
					   log_timestamp_t timestamp,
					   const char *str)
{
	mock_log_backend_generic_record(backend, source_id, domain_id, level,
					timestamp, str, NULL, 0);
}

void mock_log_backend_validate(const struct log_backend *backend, bool panic);


#define MOCK_LOG_BACKEND_DEFINE(name, autostart) \
	static struct mock_log_backend name##_mock; \
	LOG_BACKEND_DEFINE(name, mock_log_backend_api, autostart, &name##_mock)

#ifdef __cplusplus
}
#endif

#endif /* SRC_MOCK_BACKEND_H__ */
