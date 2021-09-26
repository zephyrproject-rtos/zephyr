/*
 * Copyright (c) 2018-2019 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LWM2M_UTIL_H_
#define LWM2M_UTIL_H_

#include <net/lwm2m.h>

/* convert float struct to binary format */
int lwm2m_f32_to_b32(float32_value_t *f32, uint8_t *b32, size_t len);
int lwm2m_f32_to_b64(float32_value_t *f32, uint8_t *b64, size_t len);

/* convert binary format to float struct */
int lwm2m_b32_to_f32(uint8_t *b32, size_t len, float32_value_t *f32);
int lwm2m_b64_to_f32(uint8_t *b64, size_t len, float32_value_t *f32);

/* convert string to float struct */
int lwm2m_atof32(const char *input, float32_value_t *out);

#endif /* LWM2M_UTIL_H_ */
