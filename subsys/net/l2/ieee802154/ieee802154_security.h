/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_NET_L2_IEEE802154_SECURITY

#include <net/ieee802154.h>

int ieee802154_security_setup_session(struct ieee802154_security_ctx *sec_ctx,
				      u8_t level, u8_t key_mode,
				      u8_t *key, u8_t key_len);

bool ieee802154_decrypt_auth(struct ieee802154_security_ctx *sec_ctx,
			     u8_t *frame,
			     u8_t auth_payload_len,
			     u8_t decrypt_payload_len,
			     u8_t *src_ext_addr,
			     u32_t frame_counter);

bool ieee802154_encrypt_auth(struct ieee802154_security_ctx *sec_ctx,
			     u8_t *frame,
			     u8_t auth_payload_len,
			     u8_t encrypt_payload_len,
			     u8_t *src_ext_addr);

int ieee802154_security_init(struct ieee802154_security_ctx *sec_ctx);

#else

#define ieee802154_decrypt_auth(...) true
#define ieee802154_encrypt_auth(...) true
#define ieee802154_security_init(...) 0

#endif /* CONFIG_NET_L2_IEEE802154_SECURITY */
