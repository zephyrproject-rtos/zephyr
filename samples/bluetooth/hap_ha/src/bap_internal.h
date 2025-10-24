/*
 * Copyright (c) 2025 Demant A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BAP_INTERNAL_H
#define BAP_INTERNAL_H

#include <stdbool.h>

/**
 * @brief Check if a default unicast connection exists
 *
 * @return true if default_conn is not NULL, false otherwise.
 */
bool bap_unicast_sr_has_connection(void);

/**
 * @brief Give the semaphore sem_conencted
 *
 */
void bap_broadcast_snk_signal_connected(void);

#endif /* BAP_INTERNAL_H */
