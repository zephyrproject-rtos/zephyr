/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Generated using zcbor version 0.8.1
 * https://github.com/zephyrproject-rtos/zcbor
 * Generated with a --default-max-qty of 99
 */

#ifndef LWM2M_SENML_CBOR_ENCODE_H__
#define LWM2M_SENML_CBOR_ENCODE_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "lwm2m_senml_cbor_types.h"

#ifdef __cplusplus
extern "C" {
#endif

int cbor_encode_lwm2m_senml(uint8_t *payload, size_t payload_len, const struct lwm2m_senml *input,
			    size_t *payload_len_out);

#ifdef __cplusplus
}
#endif

#endif /* LWM2M_SENML_CBOR_ENCODE_H__ */
