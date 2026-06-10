/*
 * SPDX-FileCopyrightText: 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SD_DEV_SDIO_DEV_H
#define ZEPHYR_SD_DEV_SDIO_DEV_H

#include <zephyr/kernel.h>
#include <zephyr/sd/sd_spec.h>

struct sdev_card;

/**************** SDIO Function ****************/

enum sdio_func_state {
    SDIO_FUNC_DISABLED = 0, /* sdio function is disabled */
    SDIO_FUNC_READY,       /* sdio function is enabled and ready to transfer */
};

struct sdio_dev_func {
    enum sdio_func_num fn;
    struct sdio_dev *sdio;
    int state;

    uint32_t fbr_addr;
    uint32_t cis_addr;

    uint8_t  func_code;
    uint16_t block_size;

    struct k_fifo rx_fifo;

    void *priv;
};

/**************** SDIO Device ****************/

struct sdio_dev {
    struct sdev_card *card;
    struct sdio_dev_func *funcs[CONFIG_SDIO_DEV_MAX_FUNCS];

    uint32_t cccr_addr;
    uint8_t  num_funcs;

    uint16_t vendor_id;
    uint16_t device_id;

    uint8_t cccr_version;
    uint8_t spec_version;

    uint8_t bus_width;
    uint8_t bus_speed;
    uint8_t support_multiblock;
    uint8_t support_lowspeed;

    const void *priv;
};

struct sdio_func_cfg {
    uint8_t fn; /* function number (1~7) */
    uint32_t fbr_addr;
};

struct sdio_dev_cfg {
    uint32_t cccr_addr;
    uint8_t func_bitmap;
    struct sdio_func_cfg funcs[CONFIG_SDIO_DEV_MAX_FUNCS];
    void *priv;
};

#define SDIO_IO_READY_POLL_US     100
#define SDIO_IO_READY_TIMEOUT_US (100 * 1000) /* 100 ms */

/* SDIO Function Code（FBR[0x00]） */
typedef enum {
    SDIO_FUNC_CODE_NONE       = 0x00, /* No specific function */
    SDIO_FUNC_CODE_UART       = 0x01, /* UART */
    SDIO_FUNC_CODE_BT         = 0x02, /* Bluetooth */
    SDIO_FUNC_CODE_GPS        = 0x03, /* GPS */
    SDIO_FUNC_CODE_CAMERA     = 0x04, /* Camera */
    SDIO_FUNC_CODE_PHS        = 0x05, /* PHS */
    SDIO_FUNC_CODE_WLAN       = 0x0C, /* WLAN (WiFi) ✅ */

    /* vendor-specific */
    SDIO_FUNC_CODE_VENDOR_MIN = 0x80,
    SDIO_FUNC_CODE_VENDOR_MAX = 0xFF
} sdio_fn_code;

int sdio_dev_init(struct sdio_dev *sdio,
          const struct sdio_dev_cfg *cfg);

int sdio_dev_func_init(struct sdio_dev_func *func);

int sdio_dev_is_enumed(struct sdio_dev *sdio);

int sdio_dev_read_capability(struct sdio_dev *sdio);

int sdio_dev_rd_clr_intr_pending(struct sdio_dev *sdio);

int sdio_dev_read_cccr_version(struct sdio_dev *sdio);

int sdio_dev_get_spec_version(struct sdio_dev *sdio);

int sdio_dev_get_bus_speed(struct sdio_dev *sdio);

int sdio_dev_get_bus_width(struct sdio_dev *sdio);

int sdio_dev_cis_table_configurate(struct sdio_dev *sdio);

/************************** sdio function APIs ***********************/

bool sdio_dev_func_is_enabled(const struct sdio_dev_func *func);

bool sdio_dev_func_is_ready(const struct sdio_dev_func *func);

int sdio_dev_get_block_size(struct sdio_dev_func *func);

int sdio_dev_set_fn_code(struct sdio_dev_func *func,
             uint8_t fn_code);

uint8_t sdio_dev_read_fn_code(const struct sdio_dev_func *func);

uint32_t sdio_dev_read_cis_addr(struct sdio_dev_func *func);

#endif /* ZEPHYR_SD_DEV_SDIO_DEV_H */
