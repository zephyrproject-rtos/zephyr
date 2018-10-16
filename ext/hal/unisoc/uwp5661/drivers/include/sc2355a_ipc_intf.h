/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SC2355A_IPC_INTF_H_
#define __SC2355A_IPC_INTF_H_

int sprd_wifi_send(int ch,int prio,void *data,int len,int offset);
int create_wifi_channel(int ch);
int create_bt_channel(int ch);
#endif
