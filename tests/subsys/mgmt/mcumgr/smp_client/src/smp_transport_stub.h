/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef H_OS_GR_STUB_
#define H_OS_GR_STUB_

#include <inttypes.h>
#include <zephyr/net/buf.h>
#include <mgmt/mcumgr/transport/smp_internal.h>

#ifdef __cplusplus
extern "C" {
#endif

void smp_transport_read_hdr(const struct net_buf *nb, struct smp_hdr *dst_hdr);
void stub_smp_client_transport_register(void);

#ifdef __cplusplus
}
#endif

#endif /* H_OS_GR_STUB_ */
