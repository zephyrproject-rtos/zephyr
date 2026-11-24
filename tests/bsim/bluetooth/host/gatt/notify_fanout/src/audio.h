/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NOTIFY_FANOUT_AUDIO_H_
#define NOTIFY_FANOUT_AUDIO_H_

#include <zephyr/bluetooth/conn.h>

#define AUDIO_PHASE_COUNT 4
#define AUDIO_MARKER      0xfe

#if defined(CONFIG_BT_AICS)
void audio_server_init(void);
void audio_client_subscribe(struct bt_conn *conn, bool large);
void audio_server_run(struct bt_conn *conns[2], void (*checkpoint)(int phase));
void audio_client_check(int phase);
#else
static inline void audio_server_init(void)
{
}
static inline void audio_client_subscribe(struct bt_conn *conn, bool large)
{
}
static inline void audio_server_run(struct bt_conn *conns[2], void (*checkpoint)(int phase))
{
}
static inline void audio_client_check(int phase)
{
}
#endif

#endif /* NOTIFY_FANOUT_AUDIO_H_ */
