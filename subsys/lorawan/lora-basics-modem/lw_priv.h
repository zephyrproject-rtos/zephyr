/*
 * Copyright (c) 2025 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_LORAWAN_LBM_LW_PRIV_H_
#define ZEPHYR_SUBSYS_LORAWAN_LBM_LW_PRIV_H_

#include <smtc_modem_api.h>

int lorawan_txstatus2errno(smtc_modem_event_txdone_status_t status, bool confirmed);

#endif /* ZEPHYR_SUBSYS_LORAWAN_LBM_LW_PRIV_H_ */
