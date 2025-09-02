/**
 * @file
 * @brief Bluetooth BAP Broadcast Sink
 *
 * This file manages the core functionalities of the broadcast sink and handles connection-related logic for the broadcast assistant in the sample
 */

#ifndef BROADCAST_SINK_H
#define BROADCAST_SINK_H


#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/audio/lc3.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/pacs.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/sys_clock.h>
#include <zephyr/toolchain.h>


int init_bap_sink(void);
int bap_sink_reset(void);
int pa_sync_create(void);

extern struct bt_bap_stream *bap_streams_p[CONFIG_BT_BAP_BROADCAST_SNK_STREAM_COUNT];
extern struct bt_bap_broadcast_sink *broadcast_sink;
extern struct bt_conn *broadcast_assistant_conn;

#endif /* BROADCAST_SINK_H  */
