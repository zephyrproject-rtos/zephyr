/* @file
 * @brief Internal APIs for Audio Channel handling

 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_BT_BAP)
#define SNK_SIZE CONFIG_BT_BAP_ASE_SNK_COUNT
#define SRC_SIZE CONFIG_BT_BAP_ASE_SRC_COUNT
#else
#define SNK_SIZE 0
#define SRC_SIZE 0
#endif

/** Life-span states of ASE. Used only by internal APIs
 *  dealing with setting ASE to proper state depending on operational
 *  context.
 */
enum bt_audio_chan_state {
	/** Audio Stream Endpoint idle state */
	BT_AUDIO_CHAN_IDLE,
	/** Audio Stream Endpoint configured state */
	BT_AUDIO_CHAN_CONFIGURED,
	/** Audio Stream Endpoint streaming state */
	BT_AUDIO_CHAN_STREAMING,
};

#if defined(CONFIG_BT_AUDIO_DEBUG_CHAN)
void bt_audio_chan_set_state_debug(struct bt_audio_chan *chan, uint8_t state,
				   const char *func, int line);
#define bt_audio_chan_set_state(_chan, _state) \
	bt_audio_chan_set_state_debug(_chan, _state, __func__, __LINE__)
#else
void bt_audio_chan_set_state(struct bt_audio_chan *chan, uint8_t state);
#endif /* CONFIG_BT_AUDIO_DEBUG_CHAN */

/* Bind ISO channel */
struct bt_conn_iso *bt_audio_cig_create(struct bt_audio_chan *chan,
					struct bt_codec_qos *qos);

/* Unbind ISO channel */
int bt_audio_cig_terminate(struct bt_audio_chan *chan);

/* Connect ISO channel */
int bt_audio_chan_connect(struct bt_audio_chan *chan);

/* Disconnect ISO channel */
int bt_audio_chan_disconnect(struct bt_audio_chan *chan);

void bt_audio_chan_reset(struct bt_audio_chan *chan);
