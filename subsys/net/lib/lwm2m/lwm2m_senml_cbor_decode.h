/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Generated using zcbor version 0.8.0
 * https://github.com/zephyrproject-rtos/zcbor
 * Generated with a --default-max-qty of 99
 */

#ifndef LWM2M_SENML_CBOR_DECODE_H__
#define LWM2M_SENML_CBOR_DECODE_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "lwm2m_senml_cbor_types.h"

#ifdef __cplusplus
extern "C" {
#endif

int cbor_decode_lwm2m_senml(const uint8_t *payload, size_t payload_len, struct lwm2m_senml *result,
			    size_t *payload_len_out);

#ifdef __cplusplus
}
#endif

#endif /* LWM2M_SENML_CBOR_DECODE_H__ */
