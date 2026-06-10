/*
 * SPDX-FileCopyrightText: 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SDEV_PKT_H
#define SDEV_PKT_H

#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/slist.h>

#define SDEV_PKT_POOL_SIZE    CONFIG_SDEV_PKT_POOL_SIZE
#define SDEV_PKT_BUF_SIZE     CONFIG_SDEV_PKT_BUF_SIZE
#define SDEV_DMA_ALIGN        CONFIG_SDEV_DMA_ALIGN

/* sdio data path tx and rx */
typedef enum {
    SDEV_PKT_TX = 0,
    SDEV_PKT_RX,
} sdev_pkt_dir_t;

/* =========================================================
 * Packet Structure (wrapper only, data buffer is external)
 * =========================================================
 */
typedef struct __aligned(32) {
    sys_snode_t node;   /* Used by FIFO */

    uint8_t  *data;    /* Pointer to fixed buffer */
    uint16_t len;      /* Valid data length */
    uint8_t  dir;
    atomic_t ref;      /* Reference counter */

} sdev_pkt_t;

/* API */

sdev_pkt_t *sdev_pkt_alloc(int dir);

void sdev_pkt_free(sdev_pkt_t *pkt);

sdev_pkt_t *sdev_pkt_clone(sdev_pkt_t *pkt);

#define sdev_pkt_get(pkt) sdev_pkt_clone(pkt)

#endif /* SDEV_PKT_H */
