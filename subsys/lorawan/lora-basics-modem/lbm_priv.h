/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_LORAWAN_LORA_BASICS_MODEM_LBM_PRIV_H_
#define ZEPHYR_SUBSYS_LORAWAN_LORA_BASICS_MODEM_LBM_PRIV_H_

#include <smtc_modem_api.h>

int lbm_rc2errno(smtc_modem_return_code_t rc);
const char *lbm_rc2str(smtc_modem_return_code_t rc);

int lbm_txdone2errno(smtc_modem_event_txdone_status_t status, bool confirmed);
const char *lbm_txdone2str(smtc_modem_event_txdone_status_t status);

#endif /* ZEPHYR_SUBSYS_LORAWAN_LORA_BASICS_MODEM_LBM_PRIV_H_ */
