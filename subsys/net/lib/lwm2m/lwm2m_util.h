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

int lwm2m_string_to_path(const char *pathstr, struct lwm2m_obj_path *path, char delim);

bool lwm2m_obj_path_equal(const struct lwm2m_obj_path *a, const struct lwm2m_obj_path *b);

/**
 * @brief Used for debugging to print ip addresses.
 *
 * @param addr sockaddr for socket using ipv4 or ipv6
 * @return ip address in readable form
 */
char *lwm2m_sprint_ip_addr(const struct sockaddr *addr);

/**
 * @brief Converts the token to a printable format.
 *
 * @param[in] token Token to be printed
 * @param[in] tkl Lengths of token
 * @return char buffer with the string representation of the token
 */
char *sprint_token(const uint8_t *token, uint8_t tkl);

#endif /* LWM2M_UTIL_H_ */
