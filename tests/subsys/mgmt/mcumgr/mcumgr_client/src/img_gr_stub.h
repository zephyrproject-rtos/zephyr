/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef H_IMG_GR_STUB_
#define H_IMG_GR_STUB_

#include <inttypes.h>
#include <zephyr/net/buf.h>


#ifdef __cplusplus
extern "C" {
#endif

#define TEST_IMAGE_NUM 1
#define TEST_IMAGE_SIZE 2048
#define TEST_SLOT_NUMBER 2

void img_upload_stub_init(void);
void img_upload_response(size_t offset, int status);
void img_fail_response(int status);
void img_read_response(int count);
void img_erase_response(int status);
void img_upload_init_verify(struct net_buf *nb);
void img_state_write_verify(struct net_buf *nb);
void img_gr_stub_data_init(uint8_t *hash_ptr);

#ifdef __cplusplus
}
#endif

#endif /* H_IMG_GR_STUB_ */
