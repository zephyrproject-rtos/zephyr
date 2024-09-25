/*
 * Copyright (c) 2022-2023 Nordic Semiconductor ASA
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_BT_CAP_INITIATOR)

#include <zephyr/types.h>
#include <stddef.h>
#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap_lc3_preset.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/audio/bap.h>

static struct k_work_delayable audio_send_work;

static struct bt_cap_stream unicast_streams[CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT +
					    CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT];
static struct bt_bap_ep *unicast_sink_eps[CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT];
static struct bt_bap_ep *unicast_source_eps[CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT];

NET_BUF_POOL_FIXED_DEFINE(tx_pool, CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT,
			  CONFIG_BT_ISO_TX_MTU + BT_ISO_CHAN_SEND_RESERVE, 8, NULL);

static K_SEM_DEFINE(sem_cas_discovery, 0, 1);
static K_SEM_DEFINE(sem_discover_sink, 0, 1);
static K_SEM_DEFINE(sem_discover_source, 0, 1);
static K_SEM_DEFINE(sem_audio_start, 0, 1);

static void unicast_stream_configured(struct bt_bap_stream *stream,
				      const struct bt_audio_codec_qos_pref *pref)
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

	k_sem_give(&sem_audio_start);
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
	printk("Stopped stream %p with reason 0x%02X\n", stream, reason);

	/* Stop send timer */
	k_work_cancel_delayable(&audio_send_work);
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

static struct bt_bap_lc3_preset unicast_preset_48_2_1 =
	BT_BAP_LC3_UNICAST_PRESET_48_2_1(BT_AUDIO_LOCATION_FRONT_LEFT,
					 BT_AUDIO_CONTEXT_TYPE_MEDIA);

static void cap_discovery_complete_cb(struct bt_conn *conn, int err,
				      const struct bt_csip_set_coordinator_set_member *member,
				      const struct bt_csip_set_coordinator_csis_inst *csis_inst)
{
	if (err != 0) {
		printk("Failed to discover CAS: %d", err);
		return;
	}

	if (IS_ENABLED(CONFIG_BT_CAP_ACCEPTOR_SET_MEMBER)) {
		if (csis_inst == NULL) {
			printk("Failed to discover CAS CSIS");
			return;
		}

		printk("Found CAS with CSIS %p\n", csis_inst);
	} else {
		printk("Found CAS\n");
	}

	k_sem_give(&sem_cas_discovery);
}

static void unicast_start_complete_cb(int err, struct bt_conn *conn)
{
	if (err != 0) {
		printk("Failed to start (failing conn %p): %d", conn, err);
		return;
	}

	k_sem_give(&sem_audio_start);
}

static void unicast_update_complete_cb(int err, struct bt_conn *conn)
{
	if (err != 0) {
		printk("Failed to update (failing conn %p): %d", conn, err);
		return;
	}
}

static void unicast_stop_complete_cb(int err, struct bt_conn *conn)
{
	if (err != 0) {
		printk("Failed to stop (failing conn %p): %d", conn, err);
		return;
	}
}

static struct bt_cap_initiator_cb cap_cb = {
	.unicast_discovery_complete = cap_discovery_complete_cb,
	.unicast_start_complete = unicast_start_complete_cb,
	.unicast_update_complete = unicast_update_complete_cb,
	.unicast_stop_complete = unicast_stop_complete_cb,
};

static int discover_cas(struct bt_conn *conn)
{
	int err;

	err = bt_cap_initiator_unicast_discover(conn);
	if (err != 0) {
		printk("Failed to discover CAS: %d\n", err);
		return err;
	}

	err = k_sem_take(&sem_cas_discovery, K_FOREVER);
	if (err != 0) {
		printk("failed to take sem_cas_discovery (err %d)\n", err);
		return err;
	}

	return err;
}

static void print_hex(const uint8_t *ptr, size_t len)
{
	while (len-- != 0) {
		printk("%02x", *ptr++);
	}
}

static bool print_cb(struct bt_data *data, void *user_data)
{
	const char *str = (const char *)user_data;

	printk("%s: type 0x%02x value_len %u\n", str, data->type, data->data_len);
	print_hex(data->data, data->data_len);
	printk("\n");

	return true;
}

static void print_remote_codec(const struct bt_audio_codec_cap *codec_cap, enum bt_audio_dir dir)
{
	printk("codec id 0x%02x cid 0x%04x vid 0x%04x count %u\n", codec_cap->id, codec_cap->cid,
	       codec_cap->vid, codec_cap->data_len);

	if (codec_cap->id == BT_HCI_CODING_FORMAT_LC3) {
		bt_audio_data_parse(codec_cap->data, codec_cap->data_len, print_cb, "data");
	} else { /* If not LC3, we cannot assume it's LTV */
		printk("data: ");
		print_hex(codec_cap->data, codec_cap->data_len);
		printk("\n");
	}

	bt_audio_data_parse(codec_cap->meta, codec_cap->meta_len, print_cb, "meta");
}

static void add_remote_sink(struct bt_bap_ep *ep)
{
	for (size_t i = 0U; i < ARRAY_SIZE(unicast_sink_eps); i++) {
		if (unicast_sink_eps[i] == NULL) {
			printk("Sink #%zu: ep %p\n", i, ep);
			unicast_sink_eps[i] = ep;
			return;
		}
	}
}

static void add_remote_source(struct bt_bap_ep *ep)
{
	for (size_t i = 0U; i < ARRAY_SIZE(unicast_source_eps); i++) {
		if (unicast_source_eps[i] == NULL) {
			printk("Source #%zu: ep %p\n", i, ep);
			unicast_source_eps[i] = ep;
			return;
		}
	}
}

static void discover_cb(struct bt_conn *conn, int err, enum bt_audio_dir dir)
{
	if (err != 0) {
		printk("Discovery failed: %d\n", err);
		return;
	}

	if (dir == BT_AUDIO_DIR_SINK) {
		printk("Sink discover complete\n");
		k_sem_give(&sem_discover_sink);
	} else if (dir == BT_AUDIO_DIR_SOURCE) {
		printk("Discover sources complete: err %d\n", err);
		k_sem_give(&sem_discover_source);
	}
}

static void pac_record_cb(struct bt_conn *conn, enum bt_audio_dir dir,
			  const struct bt_audio_codec_cap *codec_cap)
{
	print_remote_codec(codec_cap, dir);
}

static void endpoint_cb(struct bt_conn *conn, enum bt_audio_dir dir, struct bt_bap_ep *ep)
{
	if (dir == BT_AUDIO_DIR_SOURCE) {
		add_remote_source(ep);
	} else if (dir == BT_AUDIO_DIR_SINK) {
		add_remote_sink(ep);
	}
}

static int discover_sinks(struct bt_conn *conn)
{
	int err;

	err = bt_bap_unicast_client_discover(conn, BT_AUDIO_DIR_SINK);
	if (err != 0) {
		printk("Failed to discover sink: %d\n", err);
		return err;
	}

	err = k_sem_take(&sem_discover_sink, K_FOREVER);
	if (err != 0) {
		printk("failed to take sem_discover_sink (err %d)\n", err);
		return err;
	}

	return err;
}

static int discover_sources(struct bt_conn *conn)
{
	int err;

	err = bt_bap_unicast_client_discover(conn, BT_AUDIO_DIR_SOURCE);
	if (err != 0) {
		printk("Failed to discover sources: %d\n", err);
		return err;
	}

	err = k_sem_take(&sem_discover_source, K_FOREVER);
	if (err != 0) {
		printk("failed to take sem_discover_source (err %d)\n", err);
		return err;
	}

	return 0;
}

static struct bt_bap_unicast_client_cb unicast_client_cbs = {
	.discover = discover_cb,
	.pac_record = pac_record_cb,
	.endpoint = endpoint_cb,
};

static int unicast_group_create(struct bt_bap_unicast_group **out_unicast_group)
{
	int err = 0;
	struct bt_bap_unicast_group_stream_param group_stream_params;
	struct bt_bap_unicast_group_stream_pair_param pair_params;
	struct bt_bap_unicast_group_param group_param;

	group_stream_params.qos = &unicast_preset_48_2_1.qos;
	group_stream_params.stream = &unicast_streams[0].bap_stream;
	pair_params.tx_param = &group_stream_params;
	pair_params.rx_param = NULL;

	group_param.packing = BT_ISO_PACKING_SEQUENTIAL;
	group_param.params_count = 1;
	group_param.params = &pair_params;

	err = bt_bap_unicast_group_create(&group_param, out_unicast_group);
	if (err != 0) {
		printk("Failed to create group: %d\n", err);
		return err;
	}
	printk("Created group\n");

	return err;
}

static int unicast_audio_start(struct bt_conn *conn)
{
	int err = 0;
	struct bt_cap_unicast_audio_start_stream_param stream_param;
	struct bt_cap_unicast_audio_start_param param;

	param.type = BT_CAP_SET_TYPE_AD_HOC;
	param.count = 1u;
	param.stream_params = &stream_param;

	stream_param.member.member = conn;
	stream_param.stream = &unicast_streams[0];
	stream_param.ep = unicast_sink_eps[0];
	stream_param.codec_cfg = &unicast_preset_48_2_1.codec_cfg;

	err = bt_cap_initiator_unicast_audio_start(&param);
	if (err != 0) {
		printk("Failed to start unicast audio: %d\n", err);
		return err;
	}

	return err;
}

/**
 * @brief Send audio data on timeout
 *
 * This will send an amount of data equal to the configured QoS SDU.
 * The data is just mock data, and does not actually represent any audio.

 *
 * @param work Pointer to the work structure
 */
static void audio_timer_timeout(struct k_work *work)
{
	static uint8_t buf_data[CONFIG_BT_ISO_TX_MTU];
	static bool data_initialized;
	struct net_buf *buf;
	struct net_buf *buf_to_send;
	int ret;
	static size_t len_to_send;
	struct bt_bap_stream *stream = &unicast_streams[0].bap_stream;

	len_to_send = unicast_preset_48_2_1.qos.sdu;

	if (!data_initialized) {
		/* TODO: Actually encode some audio data */
		for (size_t i = 0; i < ARRAY_SIZE(buf_data); i++) {
			buf_data[i] = (uint8_t)i;
		}

		data_initialized = true;
	}

	buf = net_buf_alloc(&tx_pool, K_FOREVER);
	net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);
	net_buf_add_mem(buf, buf_data, len_to_send);
	buf_to_send = buf;

	ret = bt_bap_stream_send(stream, buf_to_send, 0);
	if (ret < 0) {
		printk("Failed to send audio data on streams: (%d)\n", ret);
		net_buf_unref(buf_to_send);
	} else {
		printk("Sending mock data with len %zu\n", len_to_send);
	}

	k_work_schedule(&audio_send_work, K_MSEC(1000));
}

int cap_initiator_init(void)
{
	int err = 0;

	if (IS_ENABLED(CONFIG_BT_BAP_UNICAST_CLIENT)) {
		err = bt_cap_initiator_register_cb(&cap_cb);
		if (err != 0) {
			printk("Failed to register CAP callbacks (err %d)\n", err);
			return err;
		}

		err = bt_bap_unicast_client_register_cb(&unicast_client_cbs);
		if (err != 0) {
			printk("Failed to register BAP unicast client callbacks (err %d)\n", err);
			return err;
		}

		for (size_t i = 0U; i < ARRAY_SIZE(unicast_streams); i++) {
			bt_cap_stream_ops_register(&unicast_streams[i],
						   &unicast_stream_ops);
		}
		k_work_init_delayable(&audio_send_work, audio_timer_timeout);
	}

	return 0;
}

int cap_initiator_setup(struct bt_conn *conn)
{
	int err = 0;
	struct bt_bap_unicast_group *unicast_group;

	k_sem_reset(&sem_cas_discovery);
	k_sem_reset(&sem_discover_sink);
	k_sem_reset(&sem_discover_source);
	k_sem_reset(&sem_audio_start);

	err = discover_cas(conn);
	if (err != 0) {
		return err;
	}

	err = discover_sinks(conn);
	if (err != 0) {
		return err;
	}

	err = discover_sources(conn);
	if (err != 0) {
		return err;
	}

	err = unicast_group_create(&unicast_group);
	if (err != 0) {
		return err;
	}

	err = unicast_audio_start(conn);
	if (err != 0) {
		return err;
	}

	k_sem_take(&sem_audio_start, K_FOREVER);

	k_work_schedule(&audio_send_work, K_MSEC(0));

	return err;
}

#endif /* CONFIG_BT_CAP_INITIATOR */
