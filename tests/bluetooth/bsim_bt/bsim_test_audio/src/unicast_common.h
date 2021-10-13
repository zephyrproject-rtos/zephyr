/**
 * Common functions and helpers for unicast audio BSIM audio tests
 *
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TEST_BSIM_BT_AUDIO_TEST_UNICAST_COMMON_
#define ZEPHYR_TEST_BSIM_BT_AUDIO_TEST_UNICAST_COMMON_

#include <bluetooth/bluetooth.h>
#include <bluetooth/audio.h>

void print_codec(const struct bt_codec *codec);
void print_qos(const struct bt_codec_qos *qos);

#endif /* ZEPHYR_TEST_BSIM_BT_AUDIO_TEST_UNICAST_COMMON_ */
