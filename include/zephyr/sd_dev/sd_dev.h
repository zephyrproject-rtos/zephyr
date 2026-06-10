/*
 * SPDX-FileCopyrightText: 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SD_DEV_SD_DEV_H
#define ZEPHYR_SD_DEV_SD_DEV_H

#include <zephyr/device.h>
#include <zephyr/kernel.h>

#ifdef CONFIG_SDIO_DEV
#include <zephyr/sd_dev/sdio_dev.h>
#endif

#ifdef CONFIG_SDMMC_DEV
#include <zephyr/sd_dev/sd_dev_mmc.h>
#endif

#include <zephyr/sd_dev/sdev_io.h>

typedef enum {
    SD_DEVICE_CARD = 0,
    SDIO_DEVICE_CARD,
    MMC_DEVICE_CARD,
} sdev_card_type_t;

/**
 * @brief SDIO Device state
 *
 */
enum sdio_dev_state {
    SDEV_DEVICE_RESET = 0,
    SDEV_DEVICE_INIT,       /* initializing */
    SDEV_DEVICE_READY,
    SDEV_DEVICE_SUSPEND,
    SDEV_DEVICE_ERROR,
};

struct sdev_card {
    const struct device *dev;
    sdev_card_type_t card_type;
    atomic_t state;
    bool is_enum;
    atomic_t user_cnt;
    struct k_poll_signal state_sig;
    struct k_spinlock lock;

#ifdef CONFIG_SDIO_DEV
    struct sdio_dev *sdio;
#endif

#ifdef CONFIG_SDMMC_DEV
    struct mmc_dev mmc;
#endif

    void *vendor_priv;
};

struct sdev_cfg {
    sdev_card_type_t card_type;

#ifdef CONFIG_SDIO_DEV
    struct sdio_dev_cfg sdio_cfg;
#endif

#ifdef CONFIG_SDMMC_DEV
    struct sdio_dev_cfg mmc_cfg;
#endif
};

/**************** Framework API ****************/

int sdev_init(struct sdev_card *card,
          const struct sdev_cfg *cfg);

void sdev_set_state(struct sdev_card *card, int state);

int sdev_get_state(struct sdev_card *card);

bool sdev_card_is_enumed(struct sdev_card *card);

void sdev_notify_host_ready(struct sdev_card *card);

/**************** SD device heap ****************/

#define SD_DEV_HEAP_SIZE    (4096)

void *sdev_heap_alloc(size_t size);
void sdev_heap_free(void *ptr);

#define SDEV_POLLIN        BIT(0)
#define SDEV_POLLOUT    BIT(1)
#define SDEV_POLLERR    BIT(2)
#define SDEV_POLLHUP    BIT(3)

#endif /* ZEPHYR_SD_DEV_SD_DEV_H */
