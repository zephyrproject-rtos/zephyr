/* board.h - Board-specific hooks */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

void board_output_number(bt_mesh_output_action_t action, uint32_t number);

void board_prov_complete(void);

int board_init(struct k_work *button_work);

void board_led_set(bool val);
