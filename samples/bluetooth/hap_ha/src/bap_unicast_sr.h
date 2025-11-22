/*
 * Copyright (c) 2025 Demant A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BAP_UNICAST_SR_H
#define BAP_UNICAST_SR_H

#include <stdbool.h>

/**
 * @brief Check if a default unicast connection exists
 *
 * @return true if default_conn is not NULL, false otherwise.
 */
bool bap_unicast_sr_has_connection(void);

#endif /* BAP_UNICAST_SR_H */
