/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

extern struct bt_le_ext_adv *g_adv;
extern struct bt_le_adv_param g_param;
extern uint8_t g_cte_len;

/**
 * @brief The test's global state.
 *
 * This struct is used to script the test. Some suites can only run under specific conditions (such
 * as being setup or keep track of the global state of the test
 */
struct bt_test_state {
	/** Whether or not ut_bt_setup() was called */
	bool is_setup;
	/** Whether or not bt_le_ext_adv_create() was called (not deleted) */
	bool is_adv_set_created;
};
extern struct bt_test_state test_state;

void common_create_adv_set(void);
void common_delete_adv_set(void);
void common_delete_adv_set(void);
void common_set_cte_params(void);
void common_set_cl_cte_tx_params(void);
void common_set_adv_params(void);
void common_per_adv_enable(void);
void common_per_adv_disable(void);
