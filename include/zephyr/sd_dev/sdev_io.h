/*
 * SPDX-FileCopyrightText: 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SD_DEV_CORE_H
#define ZEPHYR_SD_DEV_CORE_H

#include <zephyr/kernel.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sd_dev/sdev_pkt.h>

struct sdev_card;
struct sdio_dev_func;

int sd_core_init(struct sdev_card *card);

void sdev_rx_dispatch(const struct device *dev,
              int fn,
              sdev_pkt_t *pkt);

int sdio_read(struct sdio_dev_func *func, uint8_t *data);

int sdio_write(struct sdio_dev_func *func,
           uint8_t *data,
           int len);

sdev_pkt_t *sdio_read_pkt(struct sdio_dev_func *func);

int sdio_write_pkt(struct sdio_dev_func *func,
           sdev_pkt_t *pkt);

int sdio_dev_poll(struct sdio_dev_func *card,
          uint32_t events,
          uint32_t *revents,
          k_timeout_t timeout);

#endif /* ZEPHYR_SD_DEV_CORE_H */
