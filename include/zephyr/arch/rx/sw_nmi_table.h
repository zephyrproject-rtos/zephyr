/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_RX_SW_NMI_TABLE_H
#define ZEPHYR_INCLUDE_ARCH_RX_SW_NMI_TABLE_H

#include <stdint.h>
#include <soc.h>

#define NMI_TABLE_SIZE (5)

typedef void (*nmi_callback_t)(void *arg);

struct nmi_vector_entry {
	nmi_callback_t callback;
	void *arg;
};

extern struct nmi_vector_entry _nmi_vector_table[NMI_TABLE_SIZE];

void nmi_enable(uint8_t nmi_vector, nmi_callback_t callback, void *arg);
int get_nmi_request(void);
void handle_nmi(uint8_t nmi_vector);

#endif /* ZEPHYR_INCLUDE_ARCH_RX_SW_NMI_TABLE_H */
