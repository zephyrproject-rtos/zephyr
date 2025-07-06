/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef H_SMP_STUB_
#define H_SMP_STUB_

#include <inttypes.h>
#include <zephyr/net_buf.h>
#include <zephyr/mgmt/mcumgr/grp/img_mgmt/img_mgmt.h>


#ifdef __cplusplus
extern "C" {
#endif

typedef void (*mcmgr_client_data_check_fn)(struct net_buf *nb);

void smp_stub_set_rx_data_verify(mcmgr_client_data_check_fn cb);
void smp_client_send_status_stub(int status);
void smp_client_response_buf_clean(void);
struct net_buf *smp_response_buf_allocation(void);
void stub_smp_client_transport_register(void);

#ifdef __cplusplus
}
#endif

#endif /* H_SMP_STUB_ */
