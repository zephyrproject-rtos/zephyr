/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mocks/smp.h"

#include <zephyr/kernel.h>

DEFINE_FAKE_VALUE_FUNC(int, bt_smp_irk_get, uint8_t *, uint8_t *);
DEFINE_FAKE_VALUE_FUNC(int, bt_smp_le_oob_set_tk, struct bt_conn *, const uint8_t *);
DEFINE_FAKE_VALUE_FUNC(int, bt_smp_le_oob_generate_sc_data, struct bt_le_oob_sc_data *);
DEFINE_FAKE_VALUE_FUNC(int, bt_smp_le_oob_set_sc_data, struct bt_conn *,
		       const struct bt_le_oob_sc_data *, const struct bt_le_oob_sc_data *);
DEFINE_FAKE_VALUE_FUNC(int, bt_smp_le_oob_get_sc_data, struct bt_conn *,
		       const struct bt_le_oob_sc_data **, const struct bt_le_oob_sc_data **);
