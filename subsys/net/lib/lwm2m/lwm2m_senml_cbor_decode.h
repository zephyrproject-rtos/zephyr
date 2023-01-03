/*
 * Generated using zcbor version 0.6.0
 * https://github.com/NordicSemiconductor/zcbor
 * Generated with a --default-max-qty of 99
 */

#ifndef LWM2M_SENML_CBOR_DECODE_H__
#define LWM2M_SENML_CBOR_DECODE_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "zcbor_decode.h"
#include "lwm2m_senml_cbor_types.h"

#if DEFAULT_MAX_QTY != 99
#error "The type file was generated with a different default_max_qty than this file"
#endif

int cbor_decode_lwm2m_senml(const uint8_t *payload, size_t payload_len, struct lwm2m_senml *result,
			    size_t *payload_len_out);

#endif /* LWM2M_SENML_CBOR_DECODE_H__ */
