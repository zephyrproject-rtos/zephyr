/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/types.h>
#include <zephyr/bluetooth/audio/pacs.h>

#include "pacs.h"
#include "pacs_internal.h"

/* List of fakes used by this unit tester */
#define PACS_FFF_FAKES_LIST(FAKE) \
	FAKE(bt_pacs_cap_foreach) \
	FAKE(bt_pacs_get_available_contexts_for_conn) \

static const struct bt_audio_codec_cap lc3_codec = BT_AUDIO_CODEC_CAP_LC3(
	BT_AUDIO_CODEC_CAP_FREQ_ANY, BT_AUDIO_CODEC_CAP_DURATION_10,
	BT_AUDIO_CODEC_CAP_CHAN_COUNT_SUPPORT(1), 40u, 120u, 1u,
	(BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | BT_AUDIO_CONTEXT_TYPE_MEDIA));

DEFINE_FAKE_VOID_FUNC(bt_pacs_cap_foreach, enum bt_audio_dir, bt_pacs_cap_foreach_func_t, void *);
DEFINE_FAKE_VALUE_FUNC(enum bt_audio_context, bt_pacs_get_available_contexts_for_conn,
		       struct bt_conn *, enum bt_audio_dir);

static void pacs_cap_foreach_custom_fake(enum bt_audio_dir dir, bt_pacs_cap_foreach_func_t func,
					 void *user_data)
{
	static const struct bt_pacs_cap cap[] = {
		{
			&lc3_codec,
		},
	};

	for (size_t i = 0; i < ARRAY_SIZE(cap); i++) {
		if (func(&cap[i], user_data) == false) {
			break;
		}
	}
}

void mock_bt_pacs_init(void)
{
	PACS_FFF_FAKES_LIST(RESET_FAKE);
	bt_pacs_cap_foreach_fake.custom_fake = pacs_cap_foreach_custom_fake;
}

void mock_bt_pacs_cleanup(void)
{

}

const struct bt_audio_codec_cap *bt_pacs_get_codec_cap(enum bt_audio_dir dir,
						       const struct bt_pac_codec *codec_id)
{
	static struct bt_audio_codec_cap mock_cap;

	return &mock_cap;
}
