/*
 * Copyright (c) 2018-2019 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LWM2M_UTIL_H_
#define LWM2M_UTIL_H_

#include <net/lwm2m.h>

/* convert float to binary format */
int lwm2m_float_to_b32(double *in, uint8_t *b32, size_t len);
int lwm2m_float_to_b64(double *in, uint8_t *b64, size_t len);

/* convert binary format to float */
int lwm2m_b32_to_float(uint8_t *b32, size_t len, double *out);
int lwm2m_b64_to_float(uint8_t *b64, size_t len, double *out);

/* convert string to float */
int lwm2m_atof(const char *input, double *out);
/* convert float to string */
int lwm2m_ftoa(double *input, char *out, size_t outlen, int8_t dec_limit);

#endif /* LWM2M_UTIL_H_ */
