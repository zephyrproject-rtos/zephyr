/**
 * Common functions and helpers for BSIM audio tests
 *
 * Copyright (c) 2019 Bose Corporation
 * Copyright (c) 2020-2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TEST_BSIM_BT_AUDIO_TEST_
#define ZEPHYR_TEST_BSIM_BT_AUDIO_TEST_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/audio/csip.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic_types.h>
#include <zephyr/sys/clock.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/sys_clock.h>
#include <zephyr/types.h>

#include "bstests.h"
#include "bs_types.h"
#include "bs_tracing.h"

/* Generate 1 KiB of mock data going 0x00, 0x01, ..., 0xff, 0x00, 0x01, ..., 0xff, etc */
#define ISO_DATA_GEN(_i, _) ((uint8_t)(_i))
static const uint8_t mock_iso_data[] = {
	LISTIFY(1024, ISO_DATA_GEN, (,)),
};

/* The sample SIRK as defined by the CSIS spec Appendix A.1.
 * Sample data is Big Endian, so we reverse it for little-endian
 */
#define TEST_SAMPLE_SIRK                                                                           \
	{REVERSE_ARGS(0x45U, 0x7DU, 0x7DU, 0x09U, 0x21U, 0xA1U, 0xFDU, 0x22U, 0xCEU, 0xCDU, 0x8CU, \
		      0x86U, 0xDDU, 0x72U, 0xCCU, 0xCDU)}

#define MIN_SEND_COUNT 100U
#define WAIT_SECONDS   100U                          /* seconds */
#define WAIT_TIME (WAIT_SECONDS * USEC_PER_SEC) /* microseconds*/

#define WAIT_FOR_COND(cond)                                                                        \
	while (!(cond)) {                                                                          \
		(void)k_sleep(K_MSEC(1U));                                                         \
	}

#define CREATE_FLAG(flag) static atomic_t flag = (atomic_t)false
#define SET_FLAG(flag) (void)atomic_set(&flag, (atomic_t)true)
#define UNSET_FLAG(flag) (void)atomic_clear(&flag)
#define TEST_FLAG(flag) (atomic_get(&flag) == (atomic_t)true)
#define WAIT_FOR_FLAG(flag)                                                                        \
	while (!(bool)atomic_get(&flag)) {                                                         \
		(void)k_sleep(K_MSEC(1U));                                                         \
	}
#define WAIT_FOR_AND_CLEAR_FLAG(flag)                                                              \
	while (!(bool)atomic_clear(&flag)) {                                                       \
		(void)k_sleep(K_MSEC(1U));                                                         \
	}
#define WAIT_FOR_UNSET_FLAG(flag)                                                                  \
	while (atomic_get(&flag) != (atomic_t) false) {                                            \
		(void)k_sleep(K_MSEC(1U));                                                         \
	}

extern enum bst_result_t bst_result;
#define FAIL(...) \
	do { \
		bst_result = Failed; \
		bs_trace_error_time_line(__VA_ARGS__); \
	} while (0)

#define PASS(...) \
	do { \
		bst_result = Passed; \
		bs_trace_info_time(1, "PASSED: " __VA_ARGS__); \
	} while (0)

#define PA_SYNC_INTERVAL_TO_TIMEOUT_RATIO 20U /* Set the timeout relative to interval */
#define PA_SYNC_SKIP                      5U

#define PBP_STREAMS_TO_SEND 2U

#define SINK_CONTEXT                                                                               \
	(BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED | BT_AUDIO_CONTEXT_TYPE_MEDIA |                         \
	 BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL)
#define SOURCE_CONTEXT                                                                             \
	(BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED | BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL |                \
	 BT_AUDIO_CONTEXT_TYPE_NOTIFICATIONS)

#define TMAP_ROLE_SUPPORTED                                                                        \
	((IS_ENABLED(CONFIG_BT_TMAP_CG_SUPPORTED) ? BT_TMAP_ROLE_CG : 0U) |                        \
	 (IS_ENABLED(CONFIG_BT_TMAP_CT_SUPPORTED) ? BT_TMAP_ROLE_CT : 0U) |                        \
	 (IS_ENABLED(CONFIG_BT_TMAP_UMS_SUPPORTED) ? BT_TMAP_ROLE_UMS : 0U) |                      \
	 (IS_ENABLED(CONFIG_BT_TMAP_UMR_SUPPORTED) ? BT_TMAP_ROLE_UMR : 0U) |                      \
	 (IS_ENABLED(CONFIG_BT_TMAP_BMS_SUPPORTED) ? BT_TMAP_ROLE_BMS : 0U) |                      \
	 (IS_ENABLED(CONFIG_BT_TMAP_BMR_SUPPORTED) ? BT_TMAP_ROLE_BMR : 0U))

extern struct bt_le_scan_cb common_scan_cb;
extern struct bt_conn *default_conn;
extern atomic_t flag_connected;
extern atomic_t flag_disconnected;
extern atomic_t flag_conn_updated;
extern atomic_t flag_security_changed;
extern volatile bt_security_t security_level;
extern uint8_t csip_rsi[BT_CSIP_RSI_SIZE];

void disconnected(struct bt_conn *conn, uint8_t reason);
void update_security(struct bt_conn *conn);
void setup_connectable_adv(struct bt_le_ext_adv **ext_adv);
void setup_broadcast_adv(struct bt_le_ext_adv **adv);
void start_broadcast_adv(struct bt_le_ext_adv *adv);
void test_tick(bs_time_t HW_device_time);
void test_init(void);
uint16_t get_dev_cnt(void);
uint16_t interval_to_sync_timeout(uint16_t pa_interval);
void backchannel_sync_send(uint dev);
void backchannel_sync_send_all(void);
void backchannel_sync_wait(uint dev);
void backchannel_sync_wait_all(void);
void backchannel_sync_wait_any(void);
void backchannel_sync_clear(uint dev);
void backchannel_sync_clear_all(void);

struct audio_test_stream {
	struct bt_cap_stream stream;

	uint16_t seq_num;
	size_t tx_cnt;
	uint16_t tx_sdu_size;

	struct bt_iso_recv_info last_info;
	size_t rx_cnt;
	size_t valid_rx_cnt;
	atomic_t flag_audio_received;
	bool last_rx_failed;
};

static inline struct bt_cap_stream *cap_stream_from_bap_stream(struct bt_bap_stream *bap_stream)
{
	return CONTAINER_OF(bap_stream, struct bt_cap_stream, bap_stream);
}

static inline struct bt_bap_stream *bap_stream_from_cap_stream(struct bt_cap_stream *cap_stream)
{
	return &cap_stream->bap_stream;
}

static inline struct audio_test_stream *
audio_test_stream_from_cap_stream(struct bt_cap_stream *cap_stream)
{
	return CONTAINER_OF(cap_stream, struct audio_test_stream, stream);
}

static inline struct audio_test_stream *
audio_test_stream_from_bap_stream(struct bt_bap_stream *bap_stream)
{
	return audio_test_stream_from_cap_stream(cap_stream_from_bap_stream(bap_stream));
}

static inline struct bt_cap_stream *
cap_stream_from_audio_test_stream(struct audio_test_stream *test_stream)
{
	return &test_stream->stream;
}

static inline struct bt_bap_stream *
bap_stream_from_audio_test_stream(struct audio_test_stream *test_stream)
{
	return bap_stream_from_cap_stream(cap_stream_from_audio_test_stream(test_stream));
}

#endif /* ZEPHYR_TEST_BSIM_BT_AUDIO_TEST_ */
