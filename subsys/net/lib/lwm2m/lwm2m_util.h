/*
 * Copyright (c) 2018-2019 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LWM2M_UTIL_H_
#define LWM2M_UTIL_H_

#include <zephyr/net/lwm2m.h>

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

uint16_t lwm2m_atou16(const uint8_t *buf, uint16_t buflen, uint16_t *len);

/* Forms a string from the path given as a structure
 * Level zero seems to mean NONE, not the root path.
 *
 * returns used buffer space with '\0'
 */
int lwm2m_path_to_string(char *buf, size_t buf_size, struct lwm2m_obj_path *input, int level_max);

int lwm2m_string_to_path(const char *pathstr, struct lwm2m_obj_path *path, char delim);

bool lwm2m_obj_path_equal(struct lwm2m_obj_path *a, struct lwm2m_obj_path *b);
#endif /* LWM2M_UTIL_H_ */
