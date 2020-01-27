/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/console/uart_pipe.h>

struct gsm_mux;

u8_t *gsm_mux_recv(struct gsm_mux *mux, u8_t *buf, size_t *off);
int gsm_mux_send(struct gsm_mux *mux, const u8_t *buf, size_t size);
struct gsm_mux *gsm_mux_alloc(struct modem_iface *iface,
			      uart_pipe_recv_cb cb,
			      int (*write)(struct modem_iface *iface,
					   const u8_t *buf,
					   size_t size));
