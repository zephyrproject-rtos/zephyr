/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <zephyr/bluetooth/audio/pacs.h>

#include "pacs.h"

/* List of fakes used by this unit tester */
#define PACS_FFF_FAKES_LIST(FAKE)                                                                  \
	FAKE(bt_pacs_cap_foreach)                                                                  \

static struct bt_codec lc3_codec =
	BT_CODEC_LC3(BT_CODEC_LC3_FREQ_ANY, BT_CODEC_LC3_DURATION_10,
		     BT_CODEC_LC3_CHAN_COUNT_SUPPORT(1), 40u, 120u, 1u,
		     (BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | BT_AUDIO_CONTEXT_TYPE_MEDIA));

DEFINE_FAKE_VOID_FUNC(bt_pacs_cap_foreach, enum bt_audio_dir, bt_pacs_cap_foreach_func_t, void *);

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
