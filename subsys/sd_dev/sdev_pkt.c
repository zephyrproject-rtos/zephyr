/*
 * SPDX-FileCopyrightText: 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sd_dev/sdev_pkt.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(sd_dev, LOG_LEVEL_INF);

/* =========================================================
 * Memory Pools
 * =========================================================
 */
#define SDEV_PKT_OBJ_SIZE \
    ROUND_UP(sizeof(sdev_pkt_t), CONFIG_SDEV_DMA_ALIGN)

/* TX Packet structure pool */
K_MEM_SLAB_DEFINE(sdev_tx_pkt_pool,
                  SDEV_PKT_OBJ_SIZE,
                  SDEV_PKT_POOL_SIZE,
                  SDEV_DMA_ALIGN);


/* RX Packet structure pool */
K_MEM_SLAB_DEFINE(sdev_rx_pkt_pool,
                  SDEV_PKT_OBJ_SIZE,
                  SDEV_PKT_POOL_SIZE,
                  SDEV_DMA_ALIGN);

/* Data buffer pool */
K_MEM_SLAB_DEFINE(sdev_rx_data_pool,
                  SDEV_PKT_BUF_SIZE,
                  SDEV_PKT_POOL_SIZE,
                  SDEV_DMA_ALIGN);


/* =========================================================
 * Packet APIs
 * ========================================================= */
sdev_pkt_t *sdev_pkt_alloc(int dir)
{
    sdev_pkt_t *pkt = NULL;
    uint8_t *buf = NULL;
    uint32_t cycle = k_cycle_get_32();
    uint32_t time_us_start = k_cyc_to_us_near32(cycle);
    uint32_t time_us_end, time_dis;

    /* Validate direction */
    if (dir != SDEV_PKT_RX && dir != SDEV_PKT_TX) {
        return NULL;
    }

    if (dir == SDEV_PKT_RX) {

        /* Allocate packet structure */
        if (k_mem_slab_alloc(&sdev_rx_pkt_pool,
                             (void **)&pkt,
                             K_NO_WAIT) != 0) {
            return NULL;
        }

        /* Allocate RX data buffer */
        if (k_mem_slab_alloc(&sdev_rx_data_pool,
                             (void **)&buf,
                             K_NO_WAIT) != 0) {
            k_mem_slab_free(&sdev_rx_pkt_pool, pkt);
            return NULL;
        }

        pkt->data = buf;

    } else { /* TX packet */

        /* Allocate packet structure */
        if (k_mem_slab_alloc(&sdev_tx_pkt_pool,
                             (void **)&pkt,
                             K_NO_WAIT) != 0) {
            return NULL;
        }

        /* TX buffer is managed by upper layer */
        pkt->data = NULL;
    }

    pkt->dir = dir;

    /* Initialize reference count to 1 */
    atomic_set(&pkt->ref, 1);
    cycle = k_cycle_get_32();
    time_us_end = k_cyc_to_us_near32(cycle);
    time_dis = time_us_end - time_us_start;

    return pkt;
}

/* =========================================================
 * Packet Free
 * ========================================================= */
void sdev_pkt_free(sdev_pkt_t *pkt)
{
    if (!pkt) {
        return;
    }

    /*
     * atomic_dec() returns the value BEFORE decrement.
     * If it returns 1, this is the last reference.
     */
    if (atomic_dec(&pkt->ref) == 1) {
        if (pkt->dir == SDEV_PKT_RX) {
            if (pkt->data) {
                k_mem_slab_free(&sdev_rx_data_pool, pkt->data);
            }
            k_mem_slab_free(&sdev_rx_pkt_pool, pkt);
        } else if (pkt->dir == SDEV_PKT_TX) {
            k_mem_slab_free(&sdev_tx_pkt_pool, pkt);
        } else {
            LOG_ERR("Unknown packet direction: %d", pkt->dir);
            k_mem_slab_free(&sdev_rx_pkt_pool, pkt);
        }
    }
}

/* =========================================================
 * Packet Clone (Increase Reference Count)
 * ========================================================= */
sdev_pkt_t *sdev_pkt_clone(sdev_pkt_t *pkt)
{
    if (!pkt) {
        return NULL;
    }

    /* Optional: protect against overflow */
    __ASSERT(atomic_get(&pkt->ref) < INT32_MAX,
             "Reference counter overflow");

    atomic_inc(&pkt->ref);

    return pkt;
}


