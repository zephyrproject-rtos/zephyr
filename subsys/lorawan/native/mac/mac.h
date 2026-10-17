/*
 * Copyright (c) 2026 Carlo Caione <ccaione@baylibre.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SUBSYS_LORAWAN_NATIVE_MAC_MAC_H_
#define SUBSYS_LORAWAN_NATIVE_MAC_MAC_H_

#include <engine.h>
#include <lorawan.h>

#ifdef __cplusplus
extern "C" {
#endif

void mac_process_req(struct lwan_ctx *ctx, const struct lwan_req *req);

#if defined(CONFIG_ZTEST)
void mac_flush_work(void);
#endif /* CONFIG_ZTEST */

#ifdef __cplusplus
}
#endif

#endif /* SUBSYS_LORAWAN_NATIVE_MAC_MAC_H_ */
