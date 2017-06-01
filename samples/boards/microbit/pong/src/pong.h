/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

void pong_ball_received(s8_t x_pos, s8_t y_pos, s8_t x_vel, s8_t y_vel);
void pong_conn_ready(bool initiator);
void pong_remote_disconnected(void);
void pong_remote_lost(void);

void ble_send_ball(s8_t x_pos, s8_t y_pos, s8_t x_vel, s8_t y_vel);
void ble_send_lost(void);
void ble_connect(void);
void ble_cancel_connect(void);
void ble_init(void);
