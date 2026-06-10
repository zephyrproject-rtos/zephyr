/*
 * SPDX-FileCopyrightText: 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_rw6xx_sdio_device

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/drivers/sdev.h>
#include <zephyr/drivers/pinctrl.h>

#include <zephyr/sd_dev/sdio_dev.h>
#include <zephyr/sd_dev/sd_dev.h>
#include <zephyr/sd_dev/sdev_io.h>
#include <zephyr/sd_dev/sdev_pkt.h>

#include "fsl_adapter_sdu.h"

LOG_MODULE_REGISTER(rw6xx_sdio_device, LOG_LEVEL_INF);

/*
 * ============================================================
 * RW6xx SDIO Device-side driver
 *
 * - Implements Zephyr driver model (config / data / api)
 * - Device-side SDIO (slave)
 * - No dependency on subsys headers
 * - Interaction only via drivers/sdev.h
 * ============================================================
 */

/* ------------------------------------------------------------
 * Configuration structure (read-only, from Devicetree)
 * ------------------------------------------------------------ */
struct rw6xx_sdio_config {
    const struct pinctrl_dev_config *pcfg;

    int irq;

    uint8_t num_funcs;
    uint16_t vendor_id;
    uint16_t device_id;

    uint8_t fun_num;
    uint8_t cpu_num;
    uint8_t used_port_num;

    uint8_t cmd_tx_format;
    uint8_t cmd_rd_format;
    uint8_t data_tx_format;
    uint8_t data_rd_format;

    struct sdev_cfg sdev_cfg;
};

/* ------------------------------------------------------------
 * IRQ handler
 * ------------------------------------------------------------ */
static void rw6xx_sdio_isr(const struct device *dev)
{
    ARG_UNUSED(dev);

    SDU_DriverIRQHandler();
}

/* ------------------------------------------------------------
 * SDIO register read8
 * ------------------------------------------------------------ */
/* rw612 sdu addr is not identical to standard sdio, so need offset */
static uint32_t sdio_to_rw612_addr(const struct device *dev, uint32_t sdio_addr)
{
    uint32_t offset = sdio_addr & 0xFF;
    const struct rw6xx_sdio_config *cfg = dev->config;
    const struct sdio_dev_cfg *sdio_cfg = &cfg->sdev_cfg.sdio_cfg;

    uint32_t phy_off = 0;

    if ((sdio_addr >= sdio_cfg->funcs[0].fbr_addr) &&
        (sdio_addr <= (sdio_cfg->funcs[0].fbr_addr + 0x12))) {

        switch (offset) {
        case 0x20:
        case 0x21:
        case 0x22:
            phy_off = offset - 0x20;   /* -> 0x00~0x02 */
            break;

        case 0x29:
            phy_off = 0x05;
            break;

        case 0x2A:
            phy_off = 0x06;
            break;

        case 0x2B:
            phy_off = 0x07;
            break;

        case 0x30:
            phy_off = 0x08;
            break;

        case 0x31:
            phy_off = 0x09;
            break;

        default:
            return -1;
        }

        return sdio_cfg->funcs[0].fbr_addr + phy_off;

    } else {
        return sdio_addr;
    }
}

static inline int rw6xx_sdio_read_reg8(const struct device *dev,
                                       uint32_t addr,
                                       uint8_t *val)
{
    struct sdev_card *card = dev->data;
    k_spinlock_key_t key;
    volatile uint8_t *reg;

    if (val == NULL) {
        return -EINVAL;
    }

    key = k_spin_lock(&card->lock);

    reg = (volatile uint8_t *)sdio_to_rw612_addr(dev, addr);
    *val = *reg;

    k_spin_unlock(&card->lock, key);

    return 0;
}

/* ------------------------------------------------------------
 * SDIO register read32
 * ------------------------------------------------------------ */
static inline int rw6xx_sdio_read_reg32(const struct device *dev,
                                        uint32_t addr,
                                        uint32_t *val)
{
    struct sdev_card *card = dev->data;
    k_spinlock_key_t key;
    volatile const uint32_t *reg;
    uint32_t raw;

    if (val == NULL) {
        return -EINVAL;
    }

    if ((addr & 0x3U) != 0U) {
        return -EINVAL;
    }

    key = k_spin_lock(&card->lock);

    reg = (volatile const uint32_t *)sdio_to_rw612_addr(dev, addr);
    raw = *reg;

    *val = sys_le32_to_cpu(raw);

    k_spin_unlock(&card->lock, key);

    return 0;
}

/* ------------------------------------------------------------
 * SDIO register write8
 * ------------------------------------------------------------ */
static inline int rw6xx_sdio_write_reg8(const struct device *dev,
                                        uint32_t addr,
                                        uint8_t val)
{
    struct sdev_card *card = dev->data;
    k_spinlock_key_t key;
    volatile uint8_t *reg;

    key = k_spin_lock(&card->lock);

    reg = (volatile uint8_t *)sdio_to_rw612_addr(dev, addr);
    *reg = val;

    k_spin_unlock(&card->lock, key);

    return 0;
}

/* ------------------------------------------------------------
 * SDIO register write32
 * ------------------------------------------------------------ */
static inline int rw6xx_sdio_write_reg32(const struct device *dev,
                                         uint32_t addr,
                                         uint32_t val)
{
    struct sdev_card *card = dev->data;
    k_spinlock_key_t key;
    volatile uint32_t *reg;
    uint32_t raw;

    if ((addr & 0x3U) != 0U) {
        return -EINVAL;
    }

    key = k_spin_lock(&card->lock);

    raw = sys_cpu_to_le32(val);

    reg = (volatile uint32_t *)sdio_to_rw612_addr(dev, addr);
    *reg = raw;

    k_spin_unlock(&card->lock, key);

    return 0;
}

/* ------------------------------------------------------------
 * CIS table configuration
 * ------------------------------------------------------------ */
static int rw6xx_cis_tuple_configurate(const struct device *dev)
{
    struct sdev_card *card = dev->data;
    struct sdio_dev *sdio = card->sdio;

    SDU_GetDefaultCISTable(sdio->cccr_addr);

    return 0;
}

int rw6xx_set_dev_ready(const struct device *dev)
{
    ARG_UNUSED(dev);

    return (int)SDU_SetFwReady();
}

int rw6xx_send_data(const struct sdio_dev_func *func, sdev_pkt_t *pkt)
{
    int ret;

    ARG_UNUSED(func);

    sdev_pkt_get(pkt);

    ret = SDU_Send(*(uint32_t *)pkt->data, pkt->data, pkt->len);
    if (ret != 0) {
        LOG_DBG("%s: sdev send data fail", __func__);
    }

    sdev_pkt_free(pkt);

    return ret;
}

void rw6xx_rx_pkt_dispatch(void *data, size_t len)
{
    const struct device *dev = DEVICE_DT_INST_GET(0);
    sdev_pkt_t *pkt;

    pkt = sdev_pkt_alloc(SDEV_PKT_RX);
    memcpy(pkt->data, data, len);
    pkt->len = len;

    sdev_rx_dispatch(dev, 1, pkt);
    sdev_pkt_free(pkt);

    LOG_DBG("%s: data=%p len=%zu", __func__, data, len);
}

/* ------------------------------------------------------------
 * Driver API table
 * ------------------------------------------------------------ */
static const struct sdev_driver_api rw6xx_sdio_api = {
    .read_reg8 = rw6xx_sdio_read_reg8,
    .write_reg8 = rw6xx_sdio_write_reg8,
    .read_reg32 = rw6xx_sdio_read_reg32,
    .write_reg32 = rw6xx_sdio_write_reg32,
    .cis_tuple_configurate = rw6xx_cis_tuple_configurate,
    .set_dev_ready = rw6xx_set_dev_ready,
    .send_data = rw6xx_send_data,
};

/* ------------------------------------------------------------
 * Device initialization
 * ------------------------------------------------------------ */
static int sdio_hw_init(const struct device *dev)
{
    const struct rw6xx_sdio_config *cfg;
    struct sdev_card *card;
    int ret;

    if (dev == NULL) {
        LOG_ERR("dev is NULL");
        return -EINVAL;
    }

    card = dev->data;
    cfg = dev->config;

    if ((card == NULL) || (cfg == NULL)) {
        LOG_ERR("invalid card or cfg");
        return -EINVAL;
    }

    if (cfg->pcfg != NULL) {
        ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
        if (ret < 0) {
            LOG_ERR("RW6xx SDIO pinctrl init failed (%d)", ret);
            return ret;
        }
    }

    IRQ_CONNECT(DT_IRQN(DT_DRV_INST(0)),
                DT_IRQ(DT_DRV_INST(0), priority),
                rw6xx_sdio_isr,
                DEVICE_DT_INST_GET(0),
                0);

    irq_enable(cfg->irq);

    SDU_InitPhase1();
    SDU_InitPhase2();
    SDU_InitPhase3();

    LOG_INF("SDU init done");

    SDU_InstallCallback(SDU_TYPE_FOR_WRITE_CMD, rw6xx_rx_pkt_dispatch);
    SDU_InstallCallback(SDU_TYPE_FOR_WRITE_DATA, rw6xx_rx_pkt_dispatch);

    LOG_INF("SDU callback installed");

    return 0;
}

static int sdio_sw_init(const struct device *dev)
{
    const struct rw6xx_sdio_config *cfg;
    struct sdev_card *card;
    int ret;

    if (dev == NULL) {
        LOG_ERR("sdio_sw_init: dev is NULL");
        return -EINVAL;
    }

    cfg = dev->config;
    card = dev->data;

    if ((card == NULL) || (cfg == NULL)) {
        LOG_ERR("sdio_sw_init: invalid data or config");
        return -EINVAL;
    }

    card->dev = dev;
    card->card_type = SDIO_DEVICE_CARD;

    if (card->sdio != NULL) {
        LOG_WRN("sdio already initialized");
        return 0;
    }

    ret = sdev_init(card, &cfg->sdev_cfg);
    if (ret < 0) {
        LOG_ERR("%s: sdev_init failed (%d)", dev->name, ret);
        return ret;
    }

    if (card->sdio == NULL) {
        LOG_ERR("%s: sdio is NULL after init", dev->name);
        return -EFAULT;
    }

    LOG_INF("%s: SDIO software init done", dev->name);

    sdev_notify_host_ready(card);

    return 0;
}

static int rw6xx_sdio_init(const struct device *dev)
{
    struct sdev_card *card;
    int ret;

    if (dev == NULL) {
        LOG_ERR("dev is NULL");
        return -EINVAL;
    }

    card = dev->data;

    LOG_INF("%s: SDIO init start", dev->name);

    sdev_set_state(card, SDEV_DEVICE_INIT);

    ret = sdio_hw_init(dev);
    if (ret < 0) {
        LOG_ERR("%s: sdio_hw_init failed (%d)", dev->name, ret);
        return ret;
    }

    ret = sdio_sw_init(dev);
    if (ret < 0) {
        LOG_ERR("%s: sdio_sw_init failed (%d)", dev->name, ret);
        return ret;
    }

    sdev_set_state(card, SDEV_DEVICE_READY);

    return 0;
}

#ifdef CONFIG_PM_DEVICE
static int rw6xx_sdio_suspend(const struct device *dev)
{
    struct sdev_card *card;

    if (dev == NULL) {
        LOG_ERR("sdio_suspend: dev is NULL");
        return -EINVAL;
    }

    card = dev->data;

    sdev_set_state(card, SDEV_DEVICE_SUSPEND);
    SDU_EnterSuspend();

    POWER_EnableWakeup(DT_IRQN(DT_NODELABEL(sdio_device0)));

    return 0;
}

static int rw6xx_sdio_resume(const struct device *dev)
{
    struct sdev_card *card;

    if (dev == NULL) {
        LOG_ERR("sdio_resume: dev is NULL");
        return -EINVAL;
    }

    card = dev->data;

    SDU_ExitSuspend();
    sdev_set_state(card, SDEV_DEVICE_READY);

    return 0;
}

static int rw6xx_sdio_turn_off(const struct device *dev)
{
    struct sdev_card *card;

    if (dev == NULL) {
        LOG_ERR("sdio_turn_off: dev is NULL");
        return -EINVAL;
    }

    card = dev->data;

    sdev_set_state(card, SDEV_DEVICE_RESET);

    return SDU_EnterPowerDown();
}

static int rw6xx_sdio_turn_on(const struct device *dev)
{
    SDU_ExitPowerDown();
    return rw6xx_sdio_init(dev);
}

static int rw6xx_sdio_pm_action(const struct device *dev,
                                enum pm_device_action action)
{
    switch (action) {
    case PM_DEVICE_ACTION_SUSPEND:
        return rw6xx_sdio_suspend(dev);

    case PM_DEVICE_ACTION_RESUME:
        return rw6xx_sdio_resume(dev);

    case PM_DEVICE_ACTION_TURN_OFF:
        return rw6xx_sdio_turn_off(dev);

    case PM_DEVICE_ACTION_TURN_ON:
        return rw6xx_sdio_turn_on(dev);

    default:
        return -ENOTSUP;
    }
}
#endif

/* ------------------------------------------------------------
 * Device instance (generated from DTS)
 * ------------------------------------------------------------ */
#define RW6XX_SDIO_DEVICE_DEFINE(inst)                                 \
    PINCTRL_DT_INST_DEFINE(inst);                                      \
                                                                       \
    static struct sdev_card rw6xx_sdev_card_##inst;                    \
                                                                       \
    static const struct rw6xx_sdio_config rw6xx_sdio_cfg_##inst = {    \
        .pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                  \
                                                                       \
        .irq        = DT_INST_IRQN(inst),                              \
        .num_funcs  = DT_INST_PROP(inst, num_funcs),                   \
        .vendor_id  = DT_INST_PROP(inst, vendor_id),                   \
        .device_id  = DT_INST_PROP(inst, device_id),                   \
                                                                       \
        .cpu_num        = 4,                                           \
        .used_port_num  = 1,                                           \
        .cmd_tx_format  = 1,                                           \
        .cmd_rd_format  = 1,                                           \
        .data_tx_format = 1,                                           \
        .data_rd_format = 1,                                           \
                                                                       \
        .sdev_cfg = {                                                  \
            .card_type = SDIO_DEVICE_CARD,                             \
            .sdio_cfg = {                                              \
                .cccr_addr = DT_INST_REG_ADDR(inst),                   \
                .func_bitmap =                                         \
                    BIT_MASK(DT_INST_PROP(inst, num_funcs)),           \
                .funcs = {                                             \
                    [0] = {                                            \
                        .fn = 1,                                       \
                        .fbr_addr =                                    \
                            DT_INST_REG_ADDR(inst) + 0x20,             \
                    },                                                 \
                },                                                     \
                .priv = NULL,                                          \
            },                                                         \
        },                                                             \
    };                                                                 \
                                                                       \
    /* PM support (optional) */                                        \
    IF_ENABLED(CONFIG_PM_DEVICE,                                      \
        (PM_DEVICE_DT_INST_DEFINE(inst, rw6xx_sdio_pm_action);))       \
                                                                       \
    DEVICE_DT_INST_DEFINE(inst,                                        \
                          rw6xx_sdio_init,                             \
                          COND_CODE_1(CONFIG_PM_DEVICE,                \
                                      (PM_DEVICE_DT_INST_GET(inst)),   \
                                      (NULL)),                         \
                          &rw6xx_sdev_card_##inst,                     \
                          &rw6xx_sdio_cfg_##inst,                      \
                          POST_KERNEL,                                 \
                          CONFIG_KERNEL_INIT_PRIORITY_DEVICE,          \
                          &rw6xx_sdio_api);


DT_INST_FOREACH_STATUS_OKAY(RW6XX_SDIO_DEVICE_DEFINE)

