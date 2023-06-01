/*
 * Copyright (c) 2022-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_BT_CAP_INITIATOR) && defined(CONFIG_BT_BAP_UNICAST_CLIENT)

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/byteorder.h>
#include <zephyr/bluetooth/audio/bap_lc3_preset.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/audio/bap.h>
#include "common.h"
#include "bap_unicast_common.h"

extern enum bst_result_t bst_result;

static struct bt_bap_lc3_preset unicast_preset_16_2_1 = BT_BAP_LC3_UNICAST_PRESET_16_2_1(
	BT_AUDIO_LOCATION_FRONT_LEFT, BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);

static struct bt_cap_stream unicast_client_streams[CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT];
static struct bt_bap_ep *unicast_sink_eps[CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT];

CREATE_FLAG(flag_discovered);
CREATE_FLAG(flag_codec_found);
CREATE_FLAG(flag_endpoint_found);
CREATE_FLAG(flag_started);
CREATE_FLAG(flag_start_timeout);
CREATE_FLAG(flag_updated);
CREATE_FLAG(flag_stopped);
CREATE_FLAG(flag_mtu_exchanged);
CREATE_FLAG(flag_sink_discovered);

static void unicast_stream_configured(struct bt_bap_stream *stream,
				      const struct bt_codec_qos_pref *pref)
{
	printk("Configured stream %p\n", stream);

	/* TODO: The preference should be used/taken into account when
	 * setting the QoS
	 */
}

static void unicast_stream_qos_set(struct bt_bap_stream *stream)
{
	printk("QoS set stream %p\n", stream);
}

static void unicast_stream_enabled(struct bt_bap_stream *stream)
{
	printk("Enabled stream %p\n", stream);
}

static void unicast_stream_started(struct bt_bap_stream *stream)
{
	printk("Started stream %p\n", stream);
}

static void unicast_stream_metadata_updated(struct bt_bap_stream *stream)
{
	printk("Metadata updated stream %p\n", stream);
}

static void unicast_stream_disabled(struct bt_bap_stream *stream)
{
	printk("Disabled stream %p\n", stream);
}

static void unicast_stream_stopped(struct bt_bap_stream *stream, uint8_t reason)
{
	printk("Stopped stream with reason 0x%02X%p\n", stream, reason);
}

static void unicast_stream_released(struct bt_bap_stream *stream)
{
	printk("Released stream %p\n", stream);
}

static struct bt_bap_stream_ops unicast_stream_ops = {
	.configured = unicast_stream_configured,
	.qos_set = unicast_stream_qos_set,
	.enabled = unicast_stream_enabled,
	.started = unicast_stream_started,
	.metadata_updated = unicast_stream_metadata_updated,
	.disabled = unicast_stream_disabled,
	.stopped = unicast_stream_stopped,
	.released = unicast_stream_released,
};

static void cap_discovery_complete_cb(struct bt_conn *conn, int err,
				      const struct bt_csip_set_coordinator_csis_inst *csis_inst)
{
	if (err != 0) {
		FAIL("Failed to discover CAS: %d", err);

		return;
	}

	if (IS_ENABLED(CONFIG_BT_CAP_ACCEPTOR_SET_MEMBER)) {
		if (csis_inst == NULL) {
			FAIL("Failed to discover CAS CSIS");

			return;
		}

		printk("Found CAS with CSIS %p\n", csis_inst);
	} else {
		printk("Found CAS\n");
	}

	SET_FLAG(flag_discovered);
}

static void unicast_start_complete_cb(struct bt_bap_unicast_group *unicast_group, int err,
				      struct bt_conn *conn)
{
	if (err == -ECANCELED) {
		SET_FLAG(flag_start_timeout);
	} else if (err != 0) {
		FAIL("Failed to start (failing conn %p): %d", conn, err);
	} else {
		SET_FLAG(flag_started);
	}
}

static void unicast_update_complete_cb(int err, struct bt_conn *conn)
{
	if (err != 0) {
		FAIL("Failed to update (failing conn %p): %d", conn, err);

		return;
	}

	SET_FLAG(flag_updated);
}

static void unicast_stop_complete_cb(struct bt_bap_unicast_group *unicast_group, int err,
				     struct bt_conn *conn)
{
	if (err != 0) {
		FAIL("Failed to stop (failing conn %p): %d", conn, err);

		return;
	}

	SET_FLAG(flag_stopped);
}

static struct bt_cap_initiator_cb cap_cb = {
	.unicast_discovery_complete = cap_discovery_complete_cb,
	.unicast_start_complete = unicast_start_complete_cb,
	.unicast_update_complete = unicast_update_complete_cb,
	.unicast_stop_complete = unicast_stop_complete_cb,
};

static void add_remote_sink(struct bt_bap_ep *ep)
{
	for (size_t i = 0U; i < ARRAY_SIZE(unicast_sink_eps); i++) {
		if (unicast_sink_eps[i] == NULL) {
			printk("Sink #%zu: ep %p\n", i, ep);
			unicast_sink_eps[i] = ep;
			return;
		}
	}

	FAIL("Could not add source ep\n");
}

static void print_remote_codec(const struct bt_codec *codec, enum bt_audio_dir dir)
{
	printk("codec %p dir 0x%02x\n", codec, dir);

	print_codec(codec);
}

static void pac_record_cb(struct bt_conn *conn, enum bt_audio_dir dir, const struct bt_codec *codec)
{
	print_remote_codec(codec, dir);
	SET_FLAG(flag_codec_found);
}

static void discover_sink_cb(struct bt_conn *conn, int err, enum bt_audio_dir dir)
{
	if (err != 0) {
		FAIL("Discovery failed: %d\n", err);
		return;
	}

	printk("Sink discover complete\n");

	SET_FLAG(flag_sink_discovered);
}

static void endpoint_cb(struct bt_conn *conn, enum bt_audio_dir dir, struct bt_bap_ep *ep)
{
	if (dir == BT_AUDIO_DIR_SINK) {
		add_remote_sink(ep);
		SET_FLAG(flag_endpoint_found);
	} else {
		FAIL("Invalid param dir: %u\n", dir);
	}
}

static const struct bt_bap_unicast_client_cb unicast_client_cbs = {
	.discover = discover_sink_cb,
	.pac_record = pac_record_cb,
	.endpoint = endpoint_cb,
};

static void att_mtu_updated(struct bt_conn *conn, uint16_t tx, uint16_t rx)
{
	printk("MTU exchanged\n");
	SET_FLAG(flag_mtu_exchanged);
}

static struct bt_gatt_cb gatt_callbacks = {
	.att_mtu_updated = att_mtu_updated,
};

static void init(void)
{
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		FAIL("Bluetooth enable failed (err %d)\n", err);
		return;
	}

	bt_gatt_cb_register(&gatt_callbacks);

	err = bt_bap_unicast_client_register_cb(&unicast_client_cbs);
	if (err != 0) {
		FAIL("Failed to register BAP unicast client callbacks (err %d)\n", err);
		return;
	}

	err = bt_cap_initiator_register_cb(&cap_cb);
	if (err != 0) {
		FAIL("Failed to register CAP callbacks (err %d)\n", err);
		return;
	}

	for (size_t i = 0; i < ARRAY_SIZE(unicast_client_streams); i++) {
		bt_cap_stream_ops_register(&unicast_client_streams[i], &unicast_stream_ops);
	}
}

static void scan_and_connect(void)
{
	int err;

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
	if (err != 0) {
		FAIL("Scanning failed to start (err %d)\n", err);
		return;
	}

	printk("Scanning successfully started\n");
	WAIT_FOR_FLAG(flag_connected);
}

static void discover_sink(void)
{
	int err;

	UNSET_FLAG(flag_sink_discovered);
	UNSET_FLAG(flag_codec_found);
	UNSET_FLAG(flag_endpoint_found);

	err = bt_bap_unicast_client_discover(default_conn, BT_AUDIO_DIR_SINK);
	if (err != 0) {
		printk("Failed to discover sink: %d\n", err);
		return;
	}

	memset(unicast_sink_eps, 0, sizeof(unicast_sink_eps));

	WAIT_FOR_FLAG(flag_sink_discovered);
	WAIT_FOR_FLAG(flag_endpoint_found);
	WAIT_FOR_FLAG(flag_codec_found);
}

static void discover_cas_inval(void)
{
	int err;

	err = bt_cap_initiator_unicast_discover(NULL);
	if (err == 0) {
		FAIL("bt_cap_initiator_unicast_discover with NULL conn did not fail\n");
		return;
	}

	/* Test if it handles concurrent request for same connection */
	UNSET_FLAG(flag_discovered);

	err = bt_cap_initiator_unicast_discover(default_conn);
	if (err != 0) {
		printk("Failed to discover CAS: %d\n", err);
		return;
	}

	err = bt_cap_initiator_unicast_discover(default_conn);
	if (err == 0) {
		FAIL("bt_cap_initiator_unicast_discover while previous discovery has not completed "
		     "did not fail\n");
		return;
	}

	WAIT_FOR_FLAG(flag_discovered);
}

static void discover_cas(void)
{
	int err;

	UNSET_FLAG(flag_discovered);

	err = bt_cap_initiator_unicast_discover(default_conn);
	if (err != 0) {
		printk("Failed to discover CAS: %d\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_discovered);
}

static void unicast_group_create(struct bt_bap_unicast_group **out_unicast_group)
{
	struct bt_bap_unicast_group_stream_param group_stream_params;
	struct bt_bap_unicast_group_stream_pair_param pair_params;
	struct bt_bap_unicast_group_param group_param;
	int err;

	group_stream_params.qos = &unicast_preset_16_2_1.qos;
	group_stream_params.stream = &unicast_client_streams[0].bap_stream;
	pair_params.tx_param = &group_stream_params;
	pair_params.rx_param = NULL;

	group_param.packing = BT_ISO_PACKING_SEQUENTIAL;
	group_param.params_count = 1;
	group_param.params = &pair_params;

	err = bt_bap_unicast_group_create(&group_param, out_unicast_group);
	if (err != 0) {
		FAIL("Failed to create group: %d\n", err);
		return;
	}
}

static void unicast_audio_start_inval(struct bt_bap_unicast_group *unicast_group)
{
	struct bt_codec invalid_codec =
		BT_CODEC_LC3_CONFIG_16_2(BT_AUDIO_LOCATION_FRONT_LEFT, BT_AUDIO_CONTEXT_TYPE_MEDIA);
	struct bt_cap_unicast_audio_start_stream_param invalid_stream_param;
	struct bt_cap_unicast_audio_start_stream_param valid_stream_param;
	struct bt_cap_unicast_audio_start_param invalid_start_param;
	struct bt_cap_unicast_audio_start_param valid_start_param;
	int err;

	valid_start_param.type = BT_CAP_SET_TYPE_AD_HOC;
	valid_start_param.count = 1u;
	valid_start_param.stream_params = &valid_stream_param;

	valid_stream_param.member.member = default_conn;
	valid_stream_param.stream = &unicast_client_streams[0];
	valid_stream_param.ep = unicast_sink_eps[0];
	valid_stream_param.codec = &unicast_preset_16_2_1.codec;
	valid_stream_param.qos = &unicast_preset_16_2_1.qos;

	/* Test NULL parameters */
	err = bt_cap_initiator_unicast_audio_start(NULL, unicast_group);
	if (err == 0) {
		FAIL("bt_cap_initiator_unicast_audio_start with NULL param did not fail\n");
		return;
	}

	err = bt_cap_initiator_unicast_audio_start(&valid_start_param, NULL);
	if (err == 0) {
		FAIL("bt_cap_initiator_unicast_audio_start with NULL group did not fail\n");
		return;
	}

	/* Test invalid parameters */
	memcpy(&invalid_stream_param, &valid_stream_param, sizeof(valid_stream_param));
	memcpy(&invalid_start_param, &valid_start_param, sizeof(valid_start_param));
	invalid_start_param.stream_params = &invalid_stream_param;

	/* Test invalid stream_start parameters */
	invalid_start_param.count = 0U;
	err = bt_cap_initiator_unicast_audio_start(&invalid_start_param, unicast_group);
	if (err == 0) {
		FAIL("bt_cap_initiator_unicast_audio_start with 0 count did not fail\n");
		return;
	}

	memcpy(&invalid_start_param, &valid_start_param, sizeof(valid_start_param));
	invalid_start_param.stream_params = &invalid_stream_param;

	invalid_start_param.stream_params = NULL;
	err = bt_cap_initiator_unicast_audio_start(&invalid_start_param, unicast_group);
	if (err == 0) {
		FAIL("bt_cap_initiator_unicast_audio_start with NULL stream params did not fail\n");
		return;
	}

	memcpy(&invalid_start_param, &valid_start_param, sizeof(valid_start_param));
	invalid_start_param.stream_params = &invalid_stream_param;

	/* Test invalid stream_param parameters */
	invalid_stream_param.member.member = NULL;
	err = bt_cap_initiator_unicast_audio_start(&invalid_start_param, unicast_group);
	if (err == 0) {
		FAIL("bt_cap_initiator_unicast_audio_start with NULL stream params member did not "
		     "fail\n");
		return;
	}

	memcpy(&invalid_stream_param, &valid_stream_param, sizeof(valid_stream_param));

	invalid_stream_param.stream = NULL;
	err = bt_cap_initiator_unicast_audio_start(&invalid_start_param, unicast_group);
	if (err == 0) {
		FAIL("bt_cap_initiator_unicast_audio_start with NULL stream params stream did not "
		     "fail\n");
		return;
	}

	memcpy(&invalid_stream_param, &valid_stream_param, sizeof(valid_stream_param));

	invalid_stream_param.ep = NULL;
	err = bt_cap_initiator_unicast_audio_start(&invalid_start_param, unicast_group);
	if (err == 0) {
		FAIL("bt_cap_initiator_unicast_audio_start with NULL stream params ep did not "
		     "fail\n");
		return;
	}

	memcpy(&invalid_stream_param, &valid_stream_param, sizeof(valid_stream_param));

	invalid_stream_param.codec = NULL;
	err = bt_cap_initiator_unicast_audio_start(&invalid_start_param, unicast_group);
	if (err == 0) {
		FAIL("bt_cap_initiator_unicast_audio_start with NULL stream params codec did not "
		     "fail\n");
		return;
	}

	memcpy(&invalid_stream_param, &valid_stream_param, sizeof(valid_stream_param));

	invalid_stream_param.qos = NULL;
	err = bt_cap_initiator_unicast_audio_start(&invalid_start_param, unicast_group);
	if (err == 0) {
		FAIL("bt_cap_initiator_unicast_audio_start with NULL stream params qos did not "
		     "fail\n");
		return;
	}

	/* Clear metadata so that it does not contain the mandatory stream context */
	memcpy(&invalid_stream_param, &valid_stream_param, sizeof(valid_stream_param));
	memset(&invalid_codec.meta, 0, sizeof(invalid_codec.meta));

	invalid_stream_param.codec = &invalid_codec;
	err = bt_cap_initiator_unicast_audio_start(&invalid_start_param, unicast_group);
	if (err == 0) {
		FAIL("bt_cap_initiator_unicast_audio_start with invalid Codec metadata did not "
		     "fail\n");
		return;
	}
}

static void unicast_audio_start(struct bt_bap_unicast_group *unicast_group, bool wait)
{
	struct bt_cap_unicast_audio_start_stream_param stream_param[1];
	struct bt_cap_unicast_audio_start_param param;
	int err;

	param.type = BT_CAP_SET_TYPE_AD_HOC;
	param.count = 1u;
	param.stream_params = stream_param;
	stream_param[0].member.member = default_conn;
	stream_param[0].stream = &unicast_client_streams[0];
	stream_param[0].ep = unicast_sink_eps[0];
	stream_param[0].codec = &unicast_preset_16_2_1.codec;
	stream_param[0].qos = &unicast_preset_16_2_1.qos;

	UNSET_FLAG(flag_started);

	err = bt_cap_initiator_unicast_audio_start(&param, unicast_group);
	if (err != 0) {
		FAIL("Failed to start unicast audio: %d\n", err);
		return;
	}

	if (wait) {
		WAIT_FOR_FLAG(flag_started);
	}
}

static void unicast_audio_update_inval(void)
{
	struct bt_codec invalid_codec =
		BT_CODEC_LC3_CONFIG_16_2(BT_AUDIO_LOCATION_FRONT_LEFT, BT_AUDIO_CONTEXT_TYPE_MEDIA);
	struct bt_cap_unicast_audio_update_param param;
	int err;

	param.stream = &unicast_client_streams[0];
	param.meta = unicast_preset_16_2_1.codec.meta;
	param.meta_count = unicast_preset_16_2_1.codec.meta_count;

	err = bt_cap_initiator_unicast_audio_update(NULL, 1);
	if (err == 0) {
		FAIL("bt_cap_initiator_unicast_audio_update with NULL params did not fail\n");
		return;
	}

	err = bt_cap_initiator_unicast_audio_update(&param, 0);
	if (err == 0) {
		FAIL("bt_cap_initiator_unicast_audio_update with 0 param count did not fail\n");
		return;
	}

	/* Clear metadata so that it does not contain the mandatory stream context */
	memset(&invalid_codec.meta, 0, sizeof(invalid_codec.meta));
	param.meta = invalid_codec.meta;

	err = bt_cap_initiator_unicast_audio_update(&param, 1);
	if (err == 0) {
		FAIL("bt_cap_initiator_unicast_audio_update with invalid Codec metadata did not "
		     "fail\n");
		return;
	}
}

static void unicast_audio_update(void)
{
	struct bt_cap_unicast_audio_update_param param;
	int err;

	param.stream = &unicast_client_streams[0];
	param.meta = unicast_preset_16_2_1.codec.meta;
	param.meta_count = unicast_preset_16_2_1.codec.meta_count;

	UNSET_FLAG(flag_updated);

	err = bt_cap_initiator_unicast_audio_update(&param, 1);
	if (err != 0) {
		FAIL("Failed to update unicast audio: %d\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_updated);
}

static void unicast_audio_stop_inval(void)
{
	int err;

	err = bt_cap_initiator_unicast_audio_stop(NULL);
	if (err == 0) {
		FAIL("bt_cap_initiator_unicast_audio_stop with NULL group did not fail\n");
		return;
	}
}

static void unicast_audio_stop(struct bt_bap_unicast_group *unicast_group)
{
	int err;

	UNSET_FLAG(flag_stopped);

	err = bt_cap_initiator_unicast_audio_stop(unicast_group);
	if (err != 0) {
		FAIL("Failed to start unicast audio: %d\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_stopped);

	/* Verify that it cannot be stopped twice */
	err = bt_cap_initiator_unicast_audio_stop(unicast_group);
	if (err == 0) {
		FAIL("bt_cap_initiator_unicast_audio_stop with already-stopped unicast group did "
		     "not fail\n");
		return;
	}
}

static void unicast_audio_cancel(void)
{
	int err;

	err = bt_cap_initiator_unicast_audio_cancel();
	if (err != 0) {
		FAIL("Failed to cancel unicast audio: %d\n", err);
		return;
	}
}

static void unicast_group_delete_inval(void)
{
	int err;

	err = bt_bap_unicast_group_delete(NULL);
	if (err == 0) {
		FAIL("bt_bap_unicast_group_delete with NULL group did not fail\n");
		return;
	}
}

static void unicast_group_delete(struct bt_bap_unicast_group *unicast_group)
{
	int err;

	err = bt_bap_unicast_group_delete(unicast_group);
	if (err != 0) {
		FAIL("Failed to create group: %d\n", err);
		return;
	}

	/* Verify that it cannot be deleted twice */
	err = bt_bap_unicast_group_delete(unicast_group);
	if (err == 0) {
		FAIL("bt_bap_unicast_group_delete with already-deleted unicast group did not "
		     "fail\n");
		return;
	}
}

static void test_main_cap_initiator_unicast(void)
{
	struct bt_bap_unicast_group *unicast_group;
	const size_t iterations = 2;

	init();

	scan_and_connect();

	WAIT_FOR_FLAG(flag_mtu_exchanged);

	discover_cas_inval();
	discover_cas();

	discover_sink();

	for (size_t i = 0U; i < iterations; i++) {
		unicast_group_create(&unicast_group);

		for (size_t j = 0U; j < iterations; j++) {
			unicast_audio_start_inval(unicast_group);
			unicast_audio_start(unicast_group, true);

			unicast_audio_update_inval();
			unicast_audio_update();

			unicast_audio_stop_inval();
			unicast_audio_stop(unicast_group);
		}

		unicast_group_delete_inval();
		unicast_group_delete(unicast_group);
		unicast_group = NULL;
	}

	PASS("CAP initiator unicast passed\n");
}

static void test_cap_initiator_unicast_timeout(void)
{
	struct bt_bap_unicast_group *unicast_group;
	const k_timeout_t timeout = K_SECONDS(1);
	const size_t iterations = 2;

	init();

	scan_and_connect();

	WAIT_FOR_FLAG(flag_mtu_exchanged);

	discover_cas();

	discover_sink();

	unicast_group_create(&unicast_group);

	for (size_t j = 0U; j < iterations; j++) {
		unicast_audio_start(unicast_group, false);

		k_sleep(timeout);
		if ((bool)atomic_get(&flag_started)) {
			FAIL("Unexpected start complete\n");
		} else {
			unicast_audio_cancel();
		}

		WAIT_FOR_FLAG(flag_start_timeout);

		unicast_audio_stop(unicast_group);
	}

	unicast_group_delete(unicast_group);
	unicast_group = NULL;

	PASS("CAP initiator unicast passed\n");
}

static const struct bst_test_instance test_cap_initiator_unicast[] = {
	{
		.test_id = "cap_initiator_unicast",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main_cap_initiator_unicast,
	},
	{
		.test_id = "cap_initiator_unicast_timeout",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_cap_initiator_unicast_timeout,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_cap_initiator_unicast_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_cap_initiator_unicast);
}

#else /* !(defined(CONFIG_BT_CAP_INITIATOR) && defined(CONFIG_BT_BAP_UNICAST_CLIENT)) */

struct bst_test_list *test_cap_initiator_unicast_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* defined(CONFIG_BT_CAP_INITIATOR) && defined(CONFIG_BT_BAP_UNICAST_CLIENT) */
