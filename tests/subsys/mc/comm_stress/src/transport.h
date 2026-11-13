/*
 * Copyright (c) 2026 Mahdi Sahebi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TESTS_SUBSYS_MC_COMM_STRESS_TRANSPORT_H_
#define TESTS_SUBSYS_MC_COMM_STRESS_TRANSPORT_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/mc/comm.h>

int stress_transport_init(void);
void stress_transport_reset(uint8_t loss_rate, bool duplicate, bool corrupt,
			    bool shorten, bool disconnect);
void *stress_transport_context(bool sender);
void stress_transport_set_peer(bool sender, struct mc_comm *peer);
int stress_transport_receive(void *context, uint8_t *data, size_t capacity);
int stress_transport_transmit(void *context, const uint8_t *data, size_t length);

#endif
