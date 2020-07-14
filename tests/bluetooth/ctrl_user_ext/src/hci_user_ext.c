/*
 * Copyright (c) 2019 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_BT_CTLR_USER_EXT)

#include <toolchain.h>
#include <zephyr/types.h>
#include <net/buf.h>

#include "util/memq.h"
#include "ll_sw/pdu.h"
#include "ll_sw/lll.h"
#include "hci/hci_user_ext.h"

/* The test is made to show that hci.c and other code can compile when
 * certain Kconfig options are set. This includes functions that are
 * used for proprietary user extension. The function implementations
 * in this file are simple stubs without functional behavior.
 */

int8_t hci_user_ext_get_class(struct node_rx_pdu *node_rx)
{
	return 0;
}

void hci_user_ext_encode_control(struct node_rx_pdu *node_rx, struct pdu_data *pdu_data, struct net_buf *buf)
{
	return;
}

#endif
