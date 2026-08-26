/*
 * Copyright (c) 2026 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef GATT_TPUT_COMMON_H
#define GATT_TPUT_COMMON_H

#include <stdint.h>
#include <stdbool.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/bluetooth/conn.h>

/*
 * Custom GATT contract. The 16-byte UUID arrays are little-endian, which is
 * exactly what BT_UUID_INIT_128() expects, and are shared with the Python
 * client (see tput_client/) so it can drive this firmware unchanged.
 */

/* Throughput Measurement service + Notify (TX) and WriteMe (RX) characteristics */
#define TPUT_MEAS_SVC_BYTES                                                                        \
	0xCC, 0x7B, 0xCB, 0x32, 0x07, 0x08, 0x17, 0xAF, 0xD3, 0x43, 0x1E, 0x5D, 0x20, 0x0D, 0xEC,  \
		0x1A
#define TPUT_NOTIFY_CHR_BYTES                                                                      \
	0x1E, 0x25, 0x21, 0x59, 0x67, 0x84, 0x78, 0x9E, 0x30, 0x4D, 0xE9, 0x91, 0x81, 0x13, 0xB0,  \
		0xF7
#define TPUT_WRITEME_CHR_BYTES                                                                     \
	0xC7, 0x58, 0xCF, 0x70, 0xB3, 0xAF, 0xE4, 0xAD, 0x65, 0x44, 0xA3, 0x85, 0x26, 0x7B, 0x70,  \
		0xD4

/* Diagnostics service + Throttle characteristic (1-2 byte LE target in kbps) */
#define TPUT_DIAG_SVC_BYTES                                                                        \
	0xBD, 0x90, 0x93, 0xF5, 0x7A, 0xBC, 0x39, 0x85, 0xF6, 0x4B, 0xA9, 0x1C, 0xFF, 0x1C, 0x4E,  \
		0x6E
#define TPUT_THROTTLE_CHR_BYTES                                                                    \
	0x0C, 0xF3, 0xC4, 0x97, 0x70, 0x00, 0x7B, 0x97, 0x98, 0x4E, 0xB7, 0x77, 0x66, 0xFD, 0x40,  \
		0x19

/* Largest application payload the buffers can hold. With DLE=251 a 244-byte
 * value fits one LL PDU; 495 spans two (used when the ATT MTU is large enough).
 */
#define APP_MAX_PAYLOAD 495

/* ATT and L2CAP header overhead, and the two payload tiers. */
#define APP_ATT_HEADER_SIZE    3U
#define APP_L2CAP_HEADER_SIZE  4U
#define APP_DATA_PACKET_SIZE_1 244U /* one LL PDU  (251 - 4 - 3) */
#define APP_DATA_PACKET_SIZE_2 495U /* two LL PDUs (244 + 251)   */

/* In-flight ATT operation budget; keep <= CONFIG_BT_L2CAP_TX_BUF_COUNT. */
#define APP_TX_CREDITS 8

/* Throughput counters, updated by the active role and printed by the reporter. */
extern atomic_t app_tx_bytes;
extern atomic_t app_rx_bytes;

/* Reset cumulative totals (call on each new connection). */
void app_reset_totals(void);

/* Clamp an application payload to what the negotiated ATT MTU allows. */
uint16_t app_payload_len(struct bt_conn *conn);

/* Request 2M PHY and maximum data length on a fresh connection. */
void app_link_optimize(struct bt_conn *conn);

/* Role entry points (only the selected one is compiled in). */
void app_peripheral_start(void);
void app_central_start(void);

/* Central control API, invoked from the shell (central role only). */
void app_central_set_notify(bool on);
void app_central_set_write(bool on);
void app_central_set_throttle(uint16_t kbps);
bool app_central_ready(void);

#endif /* GATT_TPUT_COMMON_H */
