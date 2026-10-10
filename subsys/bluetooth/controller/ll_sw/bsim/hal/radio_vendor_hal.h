/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* The BabbleSim LLL uses the packet level radio model of the board through
 * lll/lll_radio.h. Common code only needs the default transmit power.
 */
#define RADIO_TXP_DEFAULT CONFIG_BT_CTLR_TX_PWR_DBM
