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
#include <zephyr/sd_dev/sdio_dev.h>
#include <zephyr/sd_dev/sd_dev.h>
#include <zephyr/sd/sd_spec.h>
#include <zephyr/drivers/sdev.h>

LOG_MODULE_REGISTER(sdio_dev, LOG_LEVEL_INF);

int sdio_dev_func_init(struct sdio_dev_func *func)
{
    func->cis_addr = sdio_dev_read_cis_addr(func);

    if (func->cis_addr == 0) {
        LOG_WRN("func%d: CIS addr is 0 (check device)",
            func->fn);
    }

    k_fifo_init(&func->rx_fifo);

    return 0;
}

int sdio_dev_init(struct sdio_dev *sdio,
          const struct sdio_dev_cfg *cfg)
{
    int ret = 0;
    struct sdev_card *card = 0;

    if (!sdio || !cfg) {
        LOG_ERR("%s: sdio or cfg is NULL", __func__);
        return -EINVAL;
    }

    card = sdio->card;

    sdio->priv       = cfg->priv;
    sdio->cccr_addr  = cfg->cccr_addr;
    sdio->num_funcs  = 0;

    for (int i = 0; i < CONFIG_SDIO_DEV_MAX_FUNCS; i++) {
        if (cfg->func_bitmap & (1 << i)) {
            sdio->num_funcs++;
        }
    }

    for (int i = 0; i < CONFIG_SDIO_DEV_MAX_FUNCS; i++) {

        const struct sdio_func_cfg *fcfg = &cfg->funcs[i];

        if (fcfg->fn == 0) {
            continue;
        }

        if (fcfg->fn < 1 || fcfg->fn > 7) {
            LOG_WRN("%s: invalid fn=%d",
                __func__, fcfg->fn);
            continue;
        }

        if (!(cfg->func_bitmap &
              (1 << (fcfg->fn - 1)))) {
            continue;
        }

        struct sdio_dev_func *func =
            sdev_heap_alloc(sizeof(*func));

        if (!func) {
            LOG_ERR("%s: alloc func failed", __func__);
            return -ENOMEM;
        }

        memset(func, 0, sizeof(*func));

        sdio->funcs[fcfg->fn - 1] = func;

        func->sdio     = sdio;
        func->fn       = fcfg->fn;
        func->fbr_addr = fcfg->fbr_addr;

        func->block_size = sdio_dev_get_block_size(func);
        func->func_code  = sdio_dev_read_fn_code(func);

        LOG_INF("sdio func%d defaule value: block_size=%d func_code=%d",
            func->fn,
            func->block_size,
            func->func_code);

        ret = sdio_dev_func_init(func);
        if (ret) {
            LOG_ERR("func%d init failed (%d)",
                func->fn, ret);
            return ret;
        }
    }

    sdio_dev_cis_table_configurate(sdio);

    sdio_dev_read_capability(sdio);

    sdio->cccr_version = sdio_dev_read_cccr_version(sdio);
    sdio->spec_version = sdio_dev_get_spec_version(sdio);
    sdio->bus_speed    = sdio_dev_get_bus_speed(sdio);
    sdio->bus_width    = sdio_dev_get_bus_width(sdio);

    sdio_dev_rd_clr_intr_pending(sdio);

    LOG_INF("sdio default value: cccr=%d spec=%d speed=%d width=%d",
        sdio->cccr_version,
        sdio->spec_version,
        sdio->bus_speed,
        sdio->bus_width);

    return 0;
}

int sdio_dev_is_enumed(struct sdio_dev *sdio)
{
    if (!sdio) {
        return 0;
    }

    for (int fn = 0; fn < sdio->num_funcs; fn++) {
        struct sdio_dev_func *func = sdio->funcs[fn];

        if (!func) {
            continue;
        }

        if (sdio_dev_func_is_ready(func)) {
            return 1;
        }
    }

    return 0;
}

/************************** sdio device APIs ***********************/

int sdio_dev_cis_table_configurate(struct sdio_dev *sdio)
{
    const struct device *dev = sdio->card->dev;

    return sdev_cis_table_configurate(dev);
}

int sdio_dev_read_capability(struct sdio_dev *sdio)
{
    int ret;
    const struct device *dev = sdio->card->dev;

    uint32_t cccr_addr = sdio->cccr_addr;
    uint32_t cap_addr  = cccr_addr + SDIO_CCCR_CAPS;
    uint8_t val;

    ret = sdev_read_reg8(dev, cap_addr, &val);
    if (ret) {
        return ret;
    }

    sdio->support_multiblock = !!(val & (1 << 1));
    sdio->support_lowspeed   = !!(val & (1 << 0));

    return 0;
}

int sdio_dev_rd_clr_intr_pending(struct sdio_dev *sdio)
{
    const struct device *dev = sdio->card->dev;
    uint8_t val;
    uint32_t addr = sdio->cccr_addr + SDIO_CCCR_INT_P;
    int ret;

    ret = sdev_read_reg8(dev, addr, &val);
    if (ret) {
        return ret;
    }

    return 0;
}

int sdio_dev_read_cccr_version(struct sdio_dev *sdio)
{
    const struct device *dev = sdio->card->dev;
    uint32_t addr = sdio->cccr_addr + SDIO_CCCR_CCCR;
    uint8_t val;
    int ret;

    ret = sdev_read_reg8(dev, addr, &val);
    if (ret) {
        return ret;
    }

    return 0;
}

int sdio_dev_get_spec_version(struct sdio_dev *sdio)
{
    const struct device *dev = sdio->card->dev;
    uint32_t addr = sdio->cccr_addr + SDIO_CCCR_SD;
    uint8_t val;
    int ret;

    ret = sdev_read_reg8(dev, addr, &val);
    if (ret) {
        return ret;
    }

    return val & 0x0F;
}

int sdio_dev_get_bus_speed(struct sdio_dev *sdio)
{
    const struct device *dev = sdio->card->dev;
    uint32_t addr = sdio->cccr_addr + SDIO_CCCR_SPEED;

    uint8_t val;
    uint8_t bss;

    if (sdev_read_reg8(dev, addr, &val)) {
        return SDIO_CCCR_SPEED_SHS;
    }

    if (!(val & SDIO_CCCR_SPEED_SHS)) {
        return SDIO_CCCR_SPEED_SHS;
    }

    bss = (val & SDIO_CCCR_SPEED_MASK) >>
          SDIO_CCCR_SPEED_SHIFT;

    return bss;
}

int sdio_dev_get_bus_width(struct sdio_dev *sdio)
{
    const struct device *dev = sdio->card->dev;
    uint32_t cccr_addr = sdio->cccr_addr;

    uint32_t read_addr = cccr_addr + SDIO_CCCR_BUS_IF;
    uint8_t read_value = 0;

    sdev_read_reg8(dev, read_addr, &read_value);

    switch (read_value & SDIO_CCCR_BUS_IF_WIDTH_MASK) {
    case 0x00:
        return SDIO_CCCR_BUS_IF_WIDTH_1_BIT;
    case 0x02:
        return SDIO_CCCR_BUS_IF_WIDTH_4_BIT;
    default:
        return SDIO_CCCR_BUS_IF_WIDTH_8_BIT;
    }

    return -1;
}

/************************** sdio function APIs ***********************/

bool sdio_dev_func_is_enabled(const struct sdio_dev_func *func)
{
    const struct device *dev = func->sdio->card->dev;
    uint8_t fn = func->fn;
    uint32_t cccr_addr = func->sdio->cccr_addr;
    uint32_t enable_addr = cccr_addr + SDIO_CCCR_IO_EN;

    uint8_t val;
    int ret;

    ret = sdev_read_reg8(dev, enable_addr, &val);
    if (ret) {
        return false;
    }

    if (val & (1U << fn)) {
        return true;
    }

    return false;
}

bool sdio_dev_func_is_ready(const struct sdio_dev_func *func)
{
    const struct device *dev = func->sdio->card->dev;
    uint8_t fn = func->fn;
    uint32_t cccr_addr = func->sdio->cccr_addr;
    uint32_t ready_addr = cccr_addr + SDIO_CCCR_IO_RD;

    uint8_t val;
    int ret;

    ret = sdev_read_reg8(dev, ready_addr, &val);
    if (ret) {
        return false;
    }

    if (val & (1U << fn)) {
        return true;
    }

    return false;
}

int sdio_dev_get_block_size(struct sdio_dev_func *func)
{
    const struct device *dev = func->sdio->card->dev;
    uint32_t fbr_addr = func->fbr_addr;
    uint32_t addr = fbr_addr + SDIO_FBR_BLK_SIZE;

    uint8_t lsb;
    uint8_t msb;
    int ret = 0;
    int block_size = 0;

    if (!dev) {
        return -EINVAL;
    }

    ret = sdev_read_reg8(dev, addr, &lsb);
    if (ret) {
        return ret;
    }

    ret = sdev_read_reg8(dev, addr + 1, &msb);
    if (ret) {
        return ret;
    }

    block_size = ((uint16_t)msb << 8) | lsb;

    return block_size;
}

int sdio_dev_set_fn_code(struct sdio_dev_func *func, uint8_t fn_code)
{
    const struct device *dev = func->sdio->card->dev;
    uint32_t addr = func->fbr_addr + SDIO_FBR_CODE;
    int ret;

    if (func->fn == 0) {
        return -EINVAL;
    }

    ret = sdev_write_reg8(dev, addr, fn_code);
    if (ret) {
        return ret;
    }

    return 0;
}

uint8_t sdio_dev_read_fn_code(const struct sdio_dev_func *func)
{
    const struct device *dev = func->sdio->card->dev;
    uint32_t addr = func->fbr_addr + SDIO_FBR_CODE;
    uint8_t val;

    if (sdev_read_reg8(dev, addr, &val)) {
        return 0;
    }

    return val;
}

uint32_t sdio_dev_read_cis_addr(struct sdio_dev_func *func)
{
    const struct device *dev = func->sdio->card->dev;
    uint32_t base = func->fbr_addr;

    uint8_t lsb, mid, msb;
    int ret;
    uint32_t cis_addr = 0;

    ret = sdev_read_reg8(dev, base + SDIO_FBR_CIS, &lsb);
    if (ret) {
        return ret;
    }

    ret = sdev_read_reg8(dev, base + SDIO_FBR_CIS + 1, &mid);
    if (ret) {
        return ret;
    }

    ret = sdev_read_reg8(dev, base + SDIO_FBR_CIS + 2, &msb);
    if (ret) {
        return ret;
    }

    cis_addr = ((uint32_t)msb << 16) |
           ((uint32_t)mid << 8)  |
           (uint32_t)lsb;

    return cis_addr;
}
