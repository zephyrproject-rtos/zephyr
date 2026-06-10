/*
 * SPDX-FileCopyrightText: 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(sd_dev, LOG_LEVEL_INF);

#include <zephyr/sd_dev/sdev_io.h>
#include <zephyr/sd_dev/sd_dev.h>
#include <zephyr/sd_dev/sdio_dev.h>
#include <zephyr/drivers/sdev.h>
#include <zephyr/sd_dev/sdev_pkt.h>

void sdev_rx_dispatch(const struct device *dev, int fn, sdev_pkt_t *pkt)
{
    struct sdev_card *card = dev->data;
    struct sdio_dev *sdio_dev = card->sdio;
    struct sdio_dev_func *func = sdio_dev->funcs[fn - 1];

    if (sdev_get_state(card) != SDEV_DEVICE_READY) {
        LOG_INF("%s, card is not ready, and can't receive data",
            __func__);
        return;
    }

    sdev_pkt_get(pkt);
    k_fifo_put(&func->rx_fifo, pkt);
}

int sdio_read(struct sdio_dev_func *func, uint8_t *data)
{
    sdev_pkt_t *rx_pkt = 0;

    if (sdev_get_state(func->sdio->card) != SDEV_DEVICE_READY) {
        LOG_INF("%s, card is not ready, and can't write data",
            __func__);
        return -ENODEV;
    }

    rx_pkt = k_fifo_get(&func->rx_fifo, K_FOREVER);

    memcpy(data, rx_pkt->data, rx_pkt->len);

    sdev_pkt_free(rx_pkt);

    return rx_pkt->len;
}

sdev_pkt_t *sdio_read_pkt(struct sdio_dev_func *func)
{
    return (sdev_pkt_t *)k_fifo_get(&func->rx_fifo, K_FOREVER);
}

int sdio_write_pkt(struct sdio_dev_func *func, sdev_pkt_t *pkt)
{
    int ret = 0;

    if (sdev_get_state(func->sdio->card) != SDEV_DEVICE_READY) {
        LOG_INF("%s, card is not ready, and can't write data",
            __func__);
        return -ENODEV;
    }

#if CONFIG_SDIO_TX_FIFO
    send_to_tx_fifo
#else
    ret = sdio_send_data(func, pkt);
    if (ret < 0) {
        LOG_INF("%s, sdio send data fail", __func__);
    }
#endif

    return 0;
}

int sdio_write(struct sdio_dev_func *func, uint8_t *data, int len)
{
    int ret = 0;
    sdev_pkt_t *pkt;

    if (sdev_get_state(func->sdio->card) != SDEV_DEVICE_READY) {
        LOG_INF("%s, card is not ready, and can't write data",
            __func__);
        return -ENODEV;
    }

    pkt = sdev_pkt_alloc(SDEV_PKT_TX);
    pkt->len  = len;
    pkt->data = data;

#if CONFIG_SDIO_TX_FIFO
    send_to_tx_fifo
#else
    ret = sdio_send_data(func, pkt);
    if (ret < 0) {
        LOG_ERR("%s, sdio send data fail", __func__);
    }
    sdev_pkt_free(pkt);
#endif

    return 0;
}

int sdio_dev_poll(struct sdio_dev_func *func,
          uint32_t events,
          uint32_t *revents,
          k_timeout_t timeout)
{
    struct k_poll_event poll_events[3];
    int n = 0;
    int ret;
    int idx = 0;

    *revents = 0;

    if (events & SDEV_POLLIN) {
        poll_events[n++] = (struct k_poll_event)
            K_POLL_EVENT_INITIALIZER(
                K_POLL_TYPE_FIFO_DATA_AVAILABLE,
                K_POLL_MODE_NOTIFY_ONLY,
                &func->rx_fifo);
    }

    ret = k_poll(poll_events, n, timeout);
    if (ret < 0) {
        return ret;
    }

    if (events & SDEV_POLLIN) {
        if (poll_events[idx].state ==
            K_POLL_STATE_FIFO_DATA_AVAILABLE) {
            *revents |= SDEV_POLLIN;
        }
        poll_events[idx++].state = K_POLL_STATE_NOT_READY;
    }

    return 0;
}
