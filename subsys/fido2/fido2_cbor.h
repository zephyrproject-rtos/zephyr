/*
 * Copyright (c) 2026 Siratul Islam <email@sirat.me>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/fido2/fido2_types.h>

/**
 * Encode authenticatorGetInfo response.
 *
 * @param info         Device info to encode
 * @param cbor_out     Output buffer
 * @param cbor_out_cap Capacity of @p cbor_out in bytes
 * @param cbor_out_len Number of bytes written to @p cbor_out
 * @return 0 on success, negative errno on failure
 */
int fido2_cbor_encode_get_info(const struct fido2_device_info *info, uint8_t *cbor_out,
			       size_t cbor_out_cap, size_t *cbor_out_len);
