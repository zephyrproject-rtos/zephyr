/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ADSP_TYPES_H_
#define _ADSP_TYPES_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct byte_array {
	uint32_t *data;
	uint32_t size;
};

#ifdef __cplusplus
}
#endif

#endif /* _ADSP_TYPES_H_ */
