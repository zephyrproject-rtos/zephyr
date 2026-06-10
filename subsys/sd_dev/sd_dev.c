/*
 * SPDX-FileCopyrightText: 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <stdio.h>
#include <zephyr/sd_dev/sd_dev.h>
#include <zephyr/drivers/sdev.h>
#include <zephyr/sys/time_units.h>

LOG_MODULE_REGISTER(sd_dev, LOG_LEVEL_INF);

/**************** SD device heap ****************/

static uint8_t sdev_heap_buf[SD_DEV_HEAP_SIZE];
static struct k_heap sdev_heap;

static inline void sdev_heap_init(void)
{
    k_heap_init(&sdev_heap, sdev_heap_buf, sizeof(sdev_heap_buf));
}

void *sdev_heap_alloc(size_t size)
{
    return k_heap_alloc(&sdev_heap, size, K_NO_WAIT);
}

void sdev_heap_free(void *ptr)
{
    k_heap_free(&sdev_heap, ptr);
}

void sdev_notify_host_ready(struct sdev_card *card)
{
    const struct device *dev = card->dev;

    sdev_set_dev_ready(dev);
}

bool sdev_card_is_enumed(struct sdev_card *card)
{
    if (card->card_type == SDIO_DEVICE_CARD) {
        return sdio_dev_is_enumed(card->sdio);
    }

    return 0;
}

void sdev_set_state(struct sdev_card *card, int state)
{
    atomic_set(&card->state, state);
}

int sdev_get_state(struct sdev_card *card)
{
    return atomic_get(&card->state);
}

int sdev_init(struct sdev_card *card,
          const struct sdev_cfg *cfg)
{
    int ret = 0;

    if (!card || !cfg) {
        LOG_ERR("Invalid param: card=%p cfg=%p", card, cfg);
        return -EINVAL;
    }

    sdev_heap_init();

    card->is_enum = false;

    switch (cfg->card_type) {
    case SD_DEVICE_CARD:
        LOG_INF("SD card init not implemented");
        break;

    case SDIO_DEVICE_CARD: {
        struct sdio_dev *sdio;
        const struct sdio_dev_cfg *sdio_cfg = &cfg->sdio_cfg;
#ifdef CONFIG_SDIO_DEV
        if (!sdio_cfg) {
            LOG_ERR("sdio_cfg is NULL");
            return -EINVAL;
        }

        sdio = sdev_heap_alloc(sizeof(struct sdio_dev));
        if (!sdio) {
            LOG_ERR("sdio alloc failed");
            return -ENOMEM;
        }

        memset(sdio, 0, sizeof(*sdio));

        card->sdio = sdio;
        sdio->card = card;

        ret = sdio_dev_init(sdio, sdio_cfg);
        if (ret) {
            LOG_ERR("sdio_dev_init failed (%d)", ret);
            sdev_heap_free(sdio);
            card->sdio = NULL;
            return ret;
        }
#else
        LOG_ERR("SDIO stack not enabled");
        return -ENOTSUP;
#endif
        break;
    }

    case MMC_DEVICE_CARD:
        LOG_ERR("MMC stack not enabled");
        return -ENOTSUP;
        break;

    default:
        LOG_ERR("Unknown SD card type: %d", cfg->card_type);
        return -EINVAL;
    }

    return 0;
}
