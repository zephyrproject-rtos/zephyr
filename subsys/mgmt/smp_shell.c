/*
 * Copyright Runtime.io 2018. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Shell transport for the mcumgr SMP protocol.
 */

#include <string.h>
#include <zephyr.h>
#include <init.h>
#include "net/buf.h"
#include "shell/shell.h"
#include "mgmt/mgmt.h"
#include "mgmt/serial.h"
#include "mgmt/buf.h"
#include "mgmt/smp.h"

struct device;

static struct zephyr_smp_transport smp_shell_transport;

static struct mcumgr_serial_rx_ctxt smp_shell_rx_ctxt;

/**
 * Processes a single line (i.e., a single SMP frame)
 */
static int smp_shell_rx_line(const char *line, void *arg)
{
	struct net_buf *nb;
	int line_len;

	/* Strip the trailing newline. */
	line_len = strlen(line) - 1;

	nb = mcumgr_serial_process_frag(&smp_shell_rx_ctxt, line, line_len);
	if (nb != NULL) {
		zephyr_smp_rx_req(&smp_shell_transport, nb);
	}

	return 0;
}

static u16_t smp_shell_get_mtu(const struct net_buf *nb)
{
	return CONFIG_MCUMGR_SMP_SHELL_MTU;
}

static int smp_shell_tx_raw(const void *data, int len, void *arg)
{
	/* Cast away const. */
	k_str_out((void *)data, len);
	return 0;
}

static int smp_shell_tx_pkt(struct zephyr_smp_transport *zst,
			    struct net_buf *nb)
{
	int rc;

	rc = mcumgr_serial_tx_pkt(nb->data, nb->len, smp_shell_tx_raw, NULL);
	mcumgr_buf_free(nb);

	return rc;
}

static int smp_shell_init(struct device *dev)
{
	ARG_UNUSED(dev);

	zephyr_smp_transport_init(&smp_shell_transport, smp_shell_tx_pkt,
				  smp_shell_get_mtu, NULL, NULL);
	shell_register_mcumgr_handler(smp_shell_rx_line, NULL);

	return 0;
}

SYS_INIT(smp_shell_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
