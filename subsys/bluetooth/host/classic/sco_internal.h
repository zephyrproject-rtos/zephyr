/** @file
 *  @brief Internal APIs for Bluetooth SCO handling.
 */
/*
 * Copyright 2024-2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @brief sco channel connected.
 *
 *  sco channel connected
 *
 *  @param sco SCO connection object.
 */
void bt_sco_connected(struct bt_conn *sco);

/** @brief sco channel disconnected.
 *
 *  sco channel disconnected
 *
 *  @param sco SCO connection object.
 */
void bt_sco_disconnected(struct bt_conn *sco);

uint8_t bt_esco_conn_req(struct bt_hci_evt_conn_request *evt);

#if defined(CONFIG_BT_CONN_LOG_LEVEL_DBG)
void bt_sco_chan_set_state_debug(struct bt_sco_chan *chan,
				 enum bt_sco_state state,
				 const char *func, int line);
#define bt_sco_chan_set_state(_chan, _state) \
	bt_sco_chan_set_state_debug(_chan, _state, __func__, __LINE__)
#else
void bt_sco_chan_set_state(struct bt_sco_chan *chan, enum bt_sco_state state);
#endif /* CONFIG_BT_CONN_LOG_LEVEL_DBG */
