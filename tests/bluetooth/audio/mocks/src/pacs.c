/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/audio/pacs.h>

#include "pacs.h"

DEFINE_FAKE_VOID_FUNC(bt_pacs_cap_foreach, enum bt_audio_dir, bt_pacs_cap_foreach_func_t, void *);
