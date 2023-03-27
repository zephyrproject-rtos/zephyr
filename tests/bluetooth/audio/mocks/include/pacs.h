/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MOCKS_PACS_H_
#define MOCKS_PACS_H_

#include <zephyr/fff.h>
#include <zephyr/bluetooth/audio/pacs.h>

/* List of fakes used by this unit tester */
#define PACS_FFF_FAKES_LIST(FAKE)                                                                  \
	FAKE(bt_pacs_cap_foreach)                                                                  \

DECLARE_FAKE_VOID_FUNC(bt_pacs_cap_foreach, enum bt_audio_dir, bt_pacs_cap_foreach_func_t, void *);

#endif /* MOCKS_PACS_H_ */
