/*
 * Copyright (c) 2022 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SRC_MOCK_FRONTEND_H__
#define SRC_MOCK_FRONTEND_H__

#ifdef __cplusplus
extern "C" {
#endif

void mock_log_frontend_reset(void);
void mock_log_frontend_dummy_record(int cnt);

void mock_log_frontend_generic_record(uint16_t source_id,
				      uint16_t domain_id,
				      uint8_t level,
				      const char *str,
				      uint8_t *data,
				      uint32_t data_len);

static inline void mock_log_frontend_record(uint16_t source_id, uint8_t level, const char *str)
{
	mock_log_frontend_generic_record(source_id, 0, level, str, NULL, 0);
}

void mock_log_frontend_validate(bool panic);
void mock_log_frontend_check_enable(void);
void mock_log_frontend_check_disable(void);
#ifdef __cplusplus
}
#endif

#endif /* SRC_MOCK_FRONTEND_H__ */
