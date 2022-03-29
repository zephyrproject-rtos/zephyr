/*
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <bluetooth/audio/audio.h>

#define AVAILABLE_SINK_CONTEXT  (BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | \
				 BT_AUDIO_CONTEXT_TYPE_MEDIA)

#define AVAILABLE_SOURCE_CONTEXT BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL

/* HAP_d1.0r00; 3.7 BAP Unicast Server role requirements
 * The HA shall support a Presentation Delay range in the Codec Configured state that includes
 * the value of 20ms, in addition to the requirement of Table 5.2 of [3].
 */
#define PD_MIN_USEC 20000

/* BAP_v1.0; Table 5.2: QoS configuration support setting requirements for the Unicast Client and
 * Unicast Server
 */
#define PD_MAX_USEC 40000

#define MAX_UNICAST_SINK_STREAMS 2
#define MAX_UNICAST_SOURCE_STREAMS 1
#define MAX_BROADCAST_SINK_STREAMS BROADCAST_SNK_STREAM_CNT

int hearing_aid_sink_init(void);
int hearing_aid_source_init(void);
int hearing_aid_volume_init(void);

/* Utils */
void print_hex(const uint8_t *ptr, size_t len);
void print_codec(const struct bt_codec *codec);
void print_qos(struct bt_codec_qos *qos);
void print_codec_capabilities(const struct bt_codec *codec);

/* Button callback structure */
struct button_cb {
	void (*on_volume_up)(void);
	void (*on_volume_down)(void);
	void (*on_play_pause)(void);
	void (*on_button_5)(void);
	void (*on_button_4)(void);
};

void button_cb_register(struct button_cb *cb);
