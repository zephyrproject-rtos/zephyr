/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SC2355A_PBUFMAP_H_
#define __SC2355A_PBUFMAP_H_

#define  MAX_MAP_BUF_NUM 20

struct buf_payload{
    struct pbuf *pbuf_map[MAX_MAP_BUF_NUM];
    uint16_t pos[MAX_MAP_BUF_NUM];
    uint16_t head,tail;

    pid_t   thread_pid_event;
    pid_t   thread_pid_data;
    uint8_t dst;
    uint8_t channel_event;
    uint8_t channel_data;
};

enum map_type{
    TX_BUF,
    RX_BUF_EVENT,
    RX_BUF_DATA
};

#define CONFIG_PBUF_THREAD_DEFPRIO 50
#define CONFIG_PBUF_THREAD_STACKSIZE 1024

void buf_payload_init(void);
int sprd_wifi_send_event(struct pbuf *pb);
int sprd_wifi_send_data(struct pbuf *pb);
#endif
