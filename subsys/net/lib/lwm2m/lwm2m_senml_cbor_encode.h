/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/*
 * Generated using zcbor version 0.3.99
 * https://github.com/zephyrproject-rtos/zcbor
 * Generated with a --default-max-qty of 99
 */

#ifndef LWM2M_SENML_CBOR_ENCODE_H__
#define LWM2M_SENML_CBOR_ENCODE_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "zcbor_encode.h"
#include "lwm2m_senml_cbor_types.h"

uint_fast8_t cbor_encode_lwm2m_senml(uint8_t *payload, size_t payload_len,
				     const struct lwm2m_senml *input, size_t *payload_len_out);

#endif /* LWM2M_SENML_CBOR_ENCODE_H__ */
