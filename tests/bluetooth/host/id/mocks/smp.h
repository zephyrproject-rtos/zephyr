/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>

#include <host/smp.h>

/* List of fakes used by this unit tester */
#define SMP_FFF_FAKES_LIST(FAKE)                                                                   \
	FAKE(bt_smp_irk_get)                                                                       \
	FAKE(bt_smp_le_oob_set_tk)                                                                 \
	FAKE(bt_smp_le_oob_generate_sc_data)                                                       \
	FAKE(bt_smp_le_oob_set_sc_data)                                                            \
	FAKE(bt_smp_le_oob_get_sc_data)

DECLARE_FAKE_VALUE_FUNC(int, bt_smp_irk_get, uint8_t *, uint8_t *);
DECLARE_FAKE_VALUE_FUNC(int, bt_smp_le_oob_set_tk, struct bt_conn *, const uint8_t *);
DECLARE_FAKE_VALUE_FUNC(int, bt_smp_le_oob_generate_sc_data, struct bt_le_oob_sc_data *);
DECLARE_FAKE_VALUE_FUNC(int, bt_smp_le_oob_set_sc_data, struct bt_conn *,
			const struct bt_le_oob_sc_data *, const struct bt_le_oob_sc_data *);
DECLARE_FAKE_VALUE_FUNC(int, bt_smp_le_oob_get_sc_data, struct bt_conn *,
			const struct bt_le_oob_sc_data **, const struct bt_le_oob_sc_data **);
