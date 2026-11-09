/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * TI CC23x0 Basic Radio backend.
 *
 * Wraps the TI Radio Control Layer (RCL) commands behind the
 * Zephyr basic_radio driver API.
 *
 * A PHY is a struct basic_radio_cc23x0_phy passed in basic_radio_config::phy.
 * It names the LRF_Config (SmartRF Studio C export) and the phyFeatures
 * word: certified PHYs come from basic_radio_cc23x0_phy_catalog_<soc>.c,
 * custom PHYs from the application. The boot PHY is selected with Kconfig.
 */

#define DT_DRV_COMPAT ti_cc23x0_basic_radio

#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/basic_radio.h>
#include <zephyr/drivers/basic_radio/ti/cc23x0.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>

#include <ti/devices/DeviceFamily.h>
#include <ti/drivers/rcl/RCL.h>
#include <ti/drivers/rcl/LRF.h>
#include <ti/drivers/rcl/commands/generic.h>

#include <inc/hw_memmap.h>
#include <inc/pbe_generic_ram_regs.h>
#include <rf_patches/lrf_pbe_binary_generic.h>

#include "basic_radio_cc23x0.h"

LOG_MODULE_REGISTER(basic_radio_cc23x0, CONFIG_BASIC_RADIO_LOG_LEVEL);

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) <= 1, "basic_radio_cc23x0 backend supports at most one enabled instance");

#define INST                0
#define RX_BUFFER_COUNT     CONFIG_BASIC_RADIO_RX_BUFFER_COUNT
#define RX_BUFFER_SIZE      CONFIG_BASIC_RADIO_CC23X0_RX_BUFFER_SIZE
#define TX_BUFFER_COUNT     CONFIG_BASIC_RADIO_TX_BUFFER_COUNT
#define MAX_PACKET_LEN      CONFIG_BASIC_RADIO_MAX_PACKET_LEN

/* Boot PHY selected with Kconfig, NULL if none */
#if defined(CONFIG_BASIC_RADIO_CC23X0_BOOT_PHY_BLE5_1M)
#define BOOT_PHY (&basic_radio_cc23x0_phy_ble5_1m)
#elif defined(CONFIG_BASIC_RADIO_CC23X0_BOOT_PHY_BLE5_2M)
#define BOOT_PHY (&basic_radio_cc23x0_phy_ble5_2m)
#elif defined(CONFIG_BASIC_RADIO_CC23X0_BOOT_PHY_BLE5_CODED)
#define BOOT_PHY (&basic_radio_cc23x0_phy_ble5_coded)
#elif defined(CONFIG_BASIC_RADIO_CC23X0_BOOT_PHY_IEEE_802_15_4)
#define BOOT_PHY (&basic_radio_cc23x0_phy_ieee_802_15_4)
#elif defined(CONFIG_BASIC_RADIO_CC23X0_BOOT_PHY_PROP_250KBPS_MSK_FEC)
#define BOOT_PHY (&basic_radio_cc23x0_phy_prop_250kbps_msk_fec)
#elif defined(CONFIG_BASIC_RADIO_CC23X0_BOOT_PHY_PROP_2MBPS_320KHZ_GFSK)
#define BOOT_PHY (&basic_radio_cc23x0_phy_prop_2mbps_320khz_gfsk)
#elif defined(CONFIG_BASIC_RADIO_CC23X0_BOOT_PHY_CUSTOM)
#define BOOT_PHY (&basic_radio_cc23x0_phy_custom)
#else
#define BOOT_PHY NULL
#endif

#define MAX_HDR_LEN CONFIG_BASIC_RADIO_CC23X0_MAX_HDR_LEN

/* Runtime parameters of the boot PHY until basic_radio_config() */
#define BOOT_SYNC_WORD    0x930B51DEU
#define BOOT_TX_POWER_DBM 0

/* LENCFG as addressed by SW region headers (offset from PBE_RAM_BASE_ADDR) */
#define LENCFG_OFFSET ((LRFD_BUFRAM_BASE - PBE_RAM_BASE_ADDR) + PBE_GENERIC_RAM_O_LENCFG)

/* Region types of LRF_RegionOperation that carry no data words */
#define REGION_IS_CLEAR(op) (((op) == HW_Region_Clear) || ((op) == SW_Region_Clear) || \
                             ((op) == Par_Region_Clear))

/* Segment selection, same rule as LRF_applySettings() on a full setup */
static bool segment_is_applied(LRF_ConfigWord hdr, uint16_t phyFeatures)
{
    uint16_t mask = hdr.segment.featureMask;

    if (hdr.segment.invertedFeatureMask != 0U)
    {
        return ((mask & phyFeatures) == 0U);
    }

    return ((mask == 0U) || ((mask & phyFeatures) != 0U));
}

/* Halfword written to LENCFG_OFFSET by one region, if any */
static bool region_find_lencfg(LRF_RegionOperation op, uint32_t start, const uint32_t *data, uint32_t regLen,
                               uint16_t *lencfg)
{
    bool isFound = false;

    for (uint32_t i = 0; i < regLen; i++)
    {
        switch (op)
        {
            case SW_Write_16bit:
                /* Two halfwords per word, low half first */
                if ((start + (2U * i)) == LENCFG_OFFSET)
                {
                    *lencfg = ((const LRF_ConfigWord *)&data[i / 2U])->value16[i % 2U];
                    isFound = true;
                }
                break;

            case SW_Write_32bit:
                if ((start + (4U * i)) == LENCFG_OFFSET)
                {
                    *lencfg = ((const LRF_ConfigWord *)&data[i])->value16[0];
                    isFound = true;
                }
                break;

            case SW_Write_16bit_sparse:
                if ((start + ((const LRF_ConfigWord *)&data[i])->sparse.address) == LENCFG_OFFSET)
                {
                    *lencfg = ((const LRF_ConfigWord *)&data[i])->sparse.value16;
                    isFound = true;
                }
                break;

            default:
                /* HW and parameter regions do not touch PBE RAM */
                return (false);
        }
    }

    return (isFound);
}

/*
 * Read PBE_GENERIC_RAM.LENCFG from the register list of a PHY. The radio
 * applies settings at command start, so it cannot be read back from PBE RAM
 * before the first TX header is built.
 */
static int lencfg_find(const struct basic_radio_cc23x0_phy *phy, uint16_t *lencfg)
{
    const LRF_Config *lrf = phy->lrf_config;
    bool isFound = false;

    /* LENCFG lives at this offset only in the generic PBE RAM map */
    if ((lrf == NULL) || (lrf->regConfigList == NULL) ||
        (lrf->pbeImage != (const LRF_TOPsmImage *)LRF_PBE_binary_generic))
    {
        return (-ENOTSUP);
    }

    for (uint32_t e = 0; e < lrf->regConfigList->numEntries; e++)
    {
        const uint32_t *word = &lrf->regConfigList->entries[e]->value32;
        LRF_ConfigWord entryHdr = {.value32 = *word};
        uint32_t totalLen = entryHdr.segment.length + 1U;

        /* A compound header is followed by several segments */
        if (entryHdr.segment.compoundSegment != 0U)
        {
            totalLen = entryHdr.segment.length;
            word++;
        }

        while (totalLen > 0U)
        {
            LRF_ConfigWord hdr = {.value32 = *word++};
            uint32_t segLen = hdr.segment.length;

            if ((segLen == 0U) || (segLen >= totalLen))
            {
                return (-EINVAL);
            }

            totalLen -= segLen + 1U;

            if (!segment_is_applied(hdr, phy->phy_features))
            {
                word += segLen;
                continue;
            }

            /* Walk the regions of the segment */
            while (segLen > 0U)
            {
                LRF_ConfigWord reg = {.value32 = *word++};
                LRF_RegionOperation op = (LRF_RegionOperation)reg.region.type;
                uint32_t regLen = reg.region.lengthMinus1 + 1U;
                uint32_t numWords = regLen;

                segLen--;

                if (REGION_IS_CLEAR(op))
                {
                    numWords = 0U;
                }
                else if ((op == HW_Write_16bit) || (op == SW_Write_16bit))
                {
                    numWords = DIV_ROUND_UP(regLen, 2U);
                }

                if (numWords > segLen)
                {
                    return (-EINVAL);
                }

                /* Later writes win, as on the radio */
                if (region_find_lencfg(op, reg.region.startAddress, word, regLen, lencfg))
                {
                    isFound = true;
                }

                word   += numWords;
                segLen -= numWords;
            }
        }
    }

    return (isFound ? 0 : -ENOTSUP);
}

/*
 * Frame header layout of a PHY. The driver writes the length as one byte,
 * so the field must be byte aligned and at most 8 bits.
 */
static int phy_hdr_get(const struct basic_radio_cc23x0_phy *phy, struct basic_radio_cc23x0_hdr *hdr)
{
    uint16_t lencfg = 0;
    int ret = lencfg_find(phy, &lencfg);

    if (ret != 0)
    {
        LOG_ERR("PHY has no generic PBE LENCFG");

        return (ret);
    }

    uint32_t lenPos = (lencfg & PBE_GENERIC_RAM_LENCFG_LENPOS_M) >> PBE_GENERIC_RAM_LENCFG_LENPOS_S;
    uint32_t numLenBits = (lencfg & PBE_GENERIC_RAM_LENCFG_NUMLENBITS_M) >> PBE_GENERIC_RAM_LENCFG_NUMLENBITS_S;
    uint32_t len = DIV_ROUND_UP(lenPos + numLenBits, CHAR_BIT);

    LOG_DBG("LENCFG: LENPOS=%u NUMLENBITS=%u", lenPos, numLenBits);

    if (((lenPos % CHAR_BIT) != 0U) || (numLenBits == 0U) || (numLenBits > CHAR_BIT))
    {
        return (-ENOTSUP);
    }

    if (len > MAX_HDR_LEN)
    {
        return (-EINVAL);
    }

    hdr->lenIndex = (uint8_t)(lenPos / CHAR_BIT);
    hdr->len      = (uint8_t)len;
    hdr->lenMask  = (uint8_t)BIT_MASK(numLenBits);

    return (0);
}

/*****************************************************************************************************************/

/* RCL command templates from TI Radio Control Layer (RCL).
 * RCL_CmdGenericTx_Default()/RCL_CmdGenericRx_Default() provide zero-initialized
 * structs with default cmdId, scheduling, status, etc. Runtime fields
 * (frequency, sync word, power, buffers) are filled in by rf_params_set()
 * and send()/recv().
 */
static RCL_CmdGenericTx tx_cmd = RCL_CmdGenericTx_Default();
static RCL_CmdGenericRx rx_cmd = RCL_CmdGenericRx_Default();

static uint32_t rx_bufs[RX_BUFFER_COUNT][RCL_MultiBuffer_len_u32(RX_BUFFER_SIZE)];
static uint32_t tx_bufs[TX_BUFFER_COUNT][RCL_TxBuffer_len_u32(NUM_PAD_BYTES, MAX_HDR_LEN, MAX_PACKET_LEN)];

static const struct device *basic_radio_cc23x0_dev;

/*
 * Write the runtime parameters into the driver cache and both RCL commands.
 * Shared by init and basic_radio_config().
 */
static void rf_params_set(struct basic_radio_cc23x0_data *data, uint32_t frequencyHz, uint32_t syncWord,
                          int8_t txPowerDbm, uint8_t maxPacketLen)
{
    data->cfg_frequency_hz   = frequencyHz;
    data->cfg_sync_word      = syncWord;
    data->cfg_tx_power_dbm   = txPowerDbm;
    data->cfg_max_packet_len = MIN(maxPacketLen, MAX_PACKET_LEN);

    tx_cmd.rfFrequency      = frequencyHz;
    rx_cmd.rfFrequency      = frequencyHz;

    tx_cmd.syncWord         = syncWord;
    rx_cmd.syncWordA        = syncWord;

    tx_cmd.txPower.dBm      = txPowerDbm;
    tx_cmd.txPower.fraction = 0;

    rx_cmd.maxPktLen        = data->cfg_max_packet_len;
}

/*
 * Two possible paths:
 *   SAME LRF_Config (same radio config export, e.g., BLE 1M-2M-Coded):
 *     - Only phyFeatures differs. RCL reapplies feature-gated segments
 *       on next command setup. Fast (~μs), no RCL_close/open.
 *   DIFFERENT LRF_Config (different radio config export, e.g., BLE-IEEE):
 *     - Requires RCL_close() + RCL_open() with new LRF_Config.
 *     - Reloads PBE/MCE/RFE images and full register list. Slow (~1-2ms).
 *
 * The header layout is read from the PHY in phy_hdr_get() before this is
 * called, so PHYs with different LENCFG can be mixed.
 */
static int phy_select(struct basic_radio_cc23x0_data *data, const struct basic_radio_cc23x0_phy *phy,
                      const struct basic_radio_cc23x0_hdr *hdr)
{
    if ((data->handle == NULL) || (data->phy == NULL) || (phy->lrf_config != data->phy->lrf_config))
    {
        if (data->handle != NULL)
        {
            RCL_close(data->handle);
            data->handle = NULL;
        }

        data->handle = RCL_open(&data->rcl_client, phy->lrf_config);

        if (data->handle == NULL)
        {
            LOG_ERR("RCL_open failed");
            data->phy = NULL;

            return (-EIO);
        }
    }

    tx_cmd.common.phyFeatures = phy->phy_features;
    rx_cmd.common.phyFeatures = phy->phy_features;
    data->phy = phy;
    data->hdr = *hdr;

    return (0);
}

/*****************************************************************************************************************/
/* RCL Rx Callback */
static void rx_callback(RCL_Command *cmd, LRF_Events lrfEvt, RCL_Events rclEvt)
{
    ARG_UNUSED(cmd);
    struct basic_radio_cc23x0_data *data = basic_radio_cc23x0_dev->data;

    if (lrfEvt.rxOk)
    {
        data->rx_ok_count++;
    }

    if (lrfEvt.rxNok)
    {
        data->rx_crc_err_count++;
    }

    if (rclEvt.cmdStarted)
    {
        data->cmd_started_count++;
    }

    if (rclEvt.startRejected || rclEvt.startDelayed)
    {
        data->sched_reject_count++;
    }

    if (rclEvt.rxEntryAvail || rclEvt.lastCmdDone)
    {
        k_sem_give(&data->rx_sem);
    }
}

/* RCL Tx Callback */
static void tx_callback(RCL_Command *cmd, LRF_Events lrfEvt, RCL_Events rclEvt)
{
    ARG_UNUSED(lrfEvt);
    struct basic_radio_cc23x0_data *data = basic_radio_cc23x0_dev->data;

    if (rclEvt.cmdStarted)
    {
        data->cmd_started_count++;
    }

    if (rclEvt.startRejected || rclEvt.startDelayed)
    {
        data->sched_reject_count++;
    }

    if (!rclEvt.lastCmdDone)
    {
        return;
    }

    if (cmd->status >= RCL_CommandStatus_Error)
    {
        LOG_ERR("TX error: status=%d", cmd->status);
    }

    k_sem_give(&data->tx_sync_done);
}

/*****************************************************************************************************************/
/* Driver API: Configuration */
/*
 * cfg->phy is a const struct basic_radio_cc23x0_phy *; NULL keeps the
 * active PHY. The runtime parameters are applied on top.
 */
static int basic_radio_cc23x0_config_impl(const struct device *dev, const struct basic_radio_config *cfg)
{
    struct basic_radio_cc23x0_data *dd = dev->data;
    const struct basic_radio_cc23x0_phy *phy = (cfg->phy != NULL) ? cfg->phy : dd->phy;
    struct basic_radio_cc23x0_hdr hdr = dd->hdr;
    int ret = 0;

    if (phy == NULL)
    {
        LOG_ERR("no active PHY");

        return (-EINVAL);
    }

    /* Evaluate the frequency */
    if ((cfg->frequency < phy->frequency_min) || (cfg->frequency > phy->frequency_max))
    {
        LOG_ERR("frequency %u outside the PHY's characterized range", cfg->frequency);

        return (-EINVAL);
    }

    /* Header layout of a new PHY, from its register list */
    if (phy != dd->phy)
    {
        ret = phy_hdr_get(phy, &hdr);

        if (ret != 0)
        {
            return (ret);
        }
    }

    /* Own the radio so no send()/recv() runs while RCL is reconfigured */
    if (!atomic_cas(&dd->state, RADIO_STATE_IDLE, RADIO_STATE_TX_ACTIVE))
    {
        return (-EBUSY);
    }

    if (phy != dd->phy)
    {
        ret = phy_select(dd, phy, &hdr);
    }

    /* After changing the PHY, modify the runtime parameters */
    if (ret == 0)
    {
        rf_params_set(dd, cfg->frequency, cfg->sync_word, cfg->tx_power, cfg->max_packet_len);
    }

    atomic_set(&dd->state, RADIO_STATE_IDLE);

    return (ret);
}

/*****************************************************************************************************************/
/* Driver API: Get Caps */
static uint32_t basic_radio_cc23x0_get_caps(const struct device *dev)
{
    ARG_UNUSED(dev);

    return (0);
}

/*****************************************************************************************************************/
/* Driver API: Send */
static int basic_radio_cc23x0_send(const struct device *dev, const uint8_t *data, uint16_t len, const struct basic_radio_tx_params *params, k_timeout_t timeout)
{
    struct basic_radio_cc23x0_data *dd = dev->data;

    LOG_INF("TI-cc23xx");

    if (len == 0 || len > MAX_PACKET_LEN)
    {
        return (-EINVAL);
    }

    static const uint32_t supported_tx_flags = BASIC_RADIO_TX_FLAG_SCHED_ABS | BASIC_RADIO_TX_FLAG_CHANNEL | BASIC_RADIO_TX_FLAG_SYNC_WORD | BASIC_RADIO_TX_FLAG_POWER;

    if (params && (params->flags & ~supported_tx_flags))
    {
        return (-ENOTSUP);
    }

    if (dd->phy == NULL)
    {
        return (-ENODEV);
    }

    /* Length must fit the length field of the active PHY */
    if (len > dd->hdr.lenMask)
    {
        return (-EINVAL);
    }

    if (!atomic_cas(&dd->state, RADIO_STATE_IDLE, RADIO_STATE_TX_ACTIVE))
    {
        return (-EBUSY);
    }

    uint8_t slot = dd->tx_pool_idx;
    dd->tx_pool_idx = (slot + 1) % TX_BUFFER_COUNT;
    RCL_Buffer_TxBuffer *txBuf = (RCL_Buffer_TxBuffer *)tx_bufs[slot];
    uint8_t *txData = RCL_TxBuffer_init(txBuf, NUM_PAD_BYTES, dd->hdr.len, len);
    memset(txData, 0, dd->hdr.lenIndex);
    txData[dd->hdr.lenIndex] = (uint8_t)len;
    memcpy(&txData[dd->hdr.len], data, len);
    tx_cmd.common.status = RCL_CommandStatus_Idle;
    memset(&tx_cmd.txBuffers, 0, sizeof(tx_cmd.txBuffers));
    RCL_TxBuffer_put(&tx_cmd.txBuffers, txBuf);

    tx_cmd.rfFrequency = (params && (params->flags & BASIC_RADIO_TX_FLAG_CHANNEL)) ? params->frequency_hz : dd->cfg_frequency_hz;
    tx_cmd.syncWord = (params && (params->flags & BASIC_RADIO_TX_FLAG_SYNC_WORD)) ? params->sync_word : dd->cfg_sync_word;
    tx_cmd.txPower.dBm = (params && (params->flags & BASIC_RADIO_TX_FLAG_POWER)) ? params->tx_power_dbm : dd->cfg_tx_power_dbm;

    if (params && (params->flags & BASIC_RADIO_TX_FLAG_SCHED_ABS))
    {
        tx_cmd.common.scheduling = RCL_Schedule_AbsTime;
        tx_cmd.common.timing.absStartTime = params->abs_start_time;
    }
    else
    {
        tx_cmd.common.scheduling = RCL_Schedule_Now;
    }

    k_sem_reset(&dd->tx_sync_done);

    RCL_CommandStatus st = RCL_Command_submit(dd->handle, &tx_cmd.common);

    if (st >= RCL_CommandStatus_Error)
    {
        atomic_set(&dd->state, RADIO_STATE_IDLE);

        return (-EIO);
    }

    if (k_sem_take(&dd->tx_sync_done, timeout) != 0)
    {
        RCL_Command_stop(&tx_cmd.common, RCL_StopType_Hard);
        RCL_Command_pend(&tx_cmd.common);
        atomic_set(&dd->state, RADIO_STATE_IDLE);

        return (-ETIMEDOUT);
    }

    atomic_set(&dd->state, RADIO_STATE_IDLE);

    if (tx_cmd.common.status >= RCL_CommandStatus_Error)
    {
        return (-EIO);
    }

    return (0);
}

/*****************************************************************************************************************/
/* Driver API: Receive */
static int basic_radio_cc23x0_recv(const struct device *dev, uint8_t *buf, uint16_t buf_len, const struct basic_radio_rx_params *params, k_timeout_t timeout, struct basic_radio_rx_info *info)
{
    struct basic_radio_cc23x0_data *data = dev->data;

    LOG_INF("TI-cc23xx");

    static const uint32_t supported_rx_flags = BASIC_RADIO_RX_FLAG_SCHED_ABS | BASIC_RADIO_RX_FLAG_ONE_SHOT | BASIC_RADIO_RX_FLAG_DISCARD_PAYL | BASIC_RADIO_RX_FLAG_CHANNEL | BASIC_RADIO_RX_FLAG_SYNC_WORD | BASIC_RADIO_RX_FLAG_MAX_LEN;

    if (params && (params->flags & ~supported_rx_flags))
    {
        return (-ENOTSUP);
    }

    /* Own the radio so no send()/config() touches RCL while RX is pending */
    if (data->phy == NULL)
    {
        return (-ENODEV);
    }

    if (!atomic_cas(&data->state, RADIO_STATE_IDLE, RADIO_STATE_RX_ACTIVE))
    {
        return (-EBUSY);
    }

    rx_cmd.config.repeated = (params && (params->flags & BASIC_RADIO_RX_FLAG_ONE_SHOT)) ? 0 : 1;
    rx_cmd.config.discardRxPackets = (params && (params->flags & BASIC_RADIO_RX_FLAG_DISCARD_PAYL)) ? 1 : 0;
    rx_cmd.rfFrequency = (params && (params->flags & BASIC_RADIO_RX_FLAG_CHANNEL)) ? params->frequency_hz : data->cfg_frequency_hz;
    rx_cmd.syncWordA = (params && (params->flags & BASIC_RADIO_RX_FLAG_SYNC_WORD)) ? params->sync_word : data->cfg_sync_word;
    rx_cmd.maxPktLen = (params && (params->flags & BASIC_RADIO_RX_FLAG_MAX_LEN)) ? params->max_packet_len : data->cfg_max_packet_len;

    if (params && (params->flags & BASIC_RADIO_RX_FLAG_SCHED_ABS))
    {
        rx_cmd.common.scheduling = RCL_Schedule_AbsTime;
        rx_cmd.common.timing.absStartTime = params->abs_start_time;
    }
    else
    {
        rx_cmd.common.scheduling = RCL_Schedule_Now;
    }

    for (uint8_t i = 0; i < RX_BUFFER_COUNT; i++)
    {
        RCL_MultiBuffer *mb = (RCL_MultiBuffer *)rx_bufs[i];
        RCL_MultiBuffer_clear(mb);
        RCL_MultiBuffer_put(&rx_cmd.rxBuffers, mb);
    }

    rx_cmd.common.status = RCL_CommandStatus_Idle;
    k_sem_reset(&data->rx_sem);

    RCL_CommandStatus st = RCL_Command_submit(data->handle, &rx_cmd.common);

    if (st >= RCL_CommandStatus_Error)
    {
        atomic_set(&data->state, RADIO_STATE_IDLE);

        return (-EIO);
    }

    if (k_sem_take(&data->rx_sem, timeout) != 0)
    {
        RCL_Command_stop(&rx_cmd.common, RCL_StopType_Hard);
        RCL_Command_pend(&rx_cmd.common);
        atomic_set(&data->state, RADIO_STATE_IDLE);

        return (-EAGAIN);
    }

    List_List consumed = {0};
    RCL_Buffer_DataEntry *entry = RCL_MultiBuffer_RxEntry_get(&rx_cmd.rxBuffers, &consumed);

    if (entry == NULL)
    {
        atomic_set(&data->state, RADIO_STATE_IDLE);

        return (-EIO);
    }

    uint8_t *pkt     = &entry->pad0 + entry->numPad;
    uint8_t  datalen = (uint8_t)(pkt[data->hdr.lenIndex] & data->hdr.lenMask);
    uint8_t *payload = pkt + data->hdr.len;
    uint16_t copy    = MIN((uint16_t)datalen, buf_len);

    if (!(params && (params->flags & BASIC_RADIO_RX_FLAG_DISCARD_PAYL)))
    {
        memcpy(buf, payload, copy);
    }

    if (info)
    {
        info->rssi = rx_cmd.stats->lastRssi;
        info->timestamp = 0;
        info->crc_ok = true;
    }

    RCL_MultiBuffer *mb;

    while ((mb = RCL_MultiBuffer_get(&consumed)) != NULL)
    {
        RCL_MultiBuffer_clear(mb);
    }

    /* Release only after rx_bufs is drained: the next recv() reuses them */
    atomic_set(&data->state, RADIO_STATE_IDLE);

    return ((int)copy);
}

/*****************************************************************************************************************/
/* Driver API: Test CW */
static int basic_radio_cc23x0_test_cw(const struct device *dev, uint32_t frequency, int8_t tx_power, uint16_t duration_ms)
{
    ARG_UNUSED(dev);
    ARG_UNUSED(frequency);
    ARG_UNUSED(tx_power);
    ARG_UNUSED(duration_ms);

    return (-ENOTSUP);
}

/*****************************************************************************************************************/
/* Driver API: External Function */
static int basic_radio_cc23x0_ext_ctl(const struct device *dev, uint32_t cmd, void *arg, size_t arg_sz)
{
    ARG_UNUSED(dev);
    ARG_UNUSED(cmd);
    ARG_UNUSED(arg);
    ARG_UNUSED(arg_sz);

    return (-ENOTSUP);
}

/*****************************************************************************************************************/
/* Driver API: Init */
static int basic_radio_cc23x0_init(const struct device *dev)
{
    struct basic_radio_cc23x0_data *data = dev->data;
    const struct basic_radio_cc23x0_config *cfg = dev->config;

    k_sem_init(&data->rx_sem, 0, RX_BUFFER_COUNT);
    k_sem_init(&data->tx_sync_done, 0, 1);
    atomic_set(&data->state, RADIO_STATE_IDLE);

    data->phy = NULL;
    data->handle = NULL;

    RCL_init();

    /* Boot PHY at the center of its range, until basic_radio_config() */
    if (cfg->boot_phy != NULL)
    {
        struct basic_radio_cc23x0_hdr hdr;
        int ret = phy_hdr_get(cfg->boot_phy, &hdr);

        if (ret == 0)
        {
            ret = phy_select(data, cfg->boot_phy, &hdr);
        }

        if (ret != 0)
        {
            return (ret);
        }

        rf_params_set(data,
                      cfg->boot_phy->frequency_min / 2U + cfg->boot_phy->frequency_max / 2U,
                      BOOT_SYNC_WORD, BOOT_TX_POWER_DBM, MAX_PACKET_LEN);
    }

    tx_cmd.common.runtime.callback = tx_callback;
    tx_cmd.common.runtime.rclCallbackMask.value = RCL_EventLastCmdDone.value | RCL_EventCmdStarted.value |
                                                  RCL_EventStartRejected.value | RCL_EventStartDelayed.value;

    rx_cmd.common.runtime.callback = rx_callback;
    rx_cmd.common.runtime.rclCallbackMask.value = RCL_EventRxEntryAvail.value | RCL_EventLastCmdDone.value |
                                                  RCL_EventCmdStarted.value | RCL_EventStartRejected.value | 
                                                  RCL_EventStartDelayed.value;
                                                  
    rx_cmd.common.runtime.lrfCallbackMask.value = LRF_EventRxOk.value | LRF_EventRxNok.value;

    data->stats = RCL_StatsGeneric_DefaultRuntime();
    rx_cmd.stats = &data->stats;
    rx_cmd.stats->config.activeUpdate = 1;

    for (uint8_t i = 0; i < RX_BUFFER_COUNT; i++)
    {
        RCL_MultiBuffer *mb = (RCL_MultiBuffer *)rx_bufs[i];
        RCL_MultiBuffer_init(mb, RCL_MultiBuffer_len(RX_BUFFER_SIZE));
    }

    basic_radio_cc23x0_dev = dev;

    return (0);
}

/*****************************************************************************************************************/
static const struct basic_radio_driver_api basic_radio_cc23x0_api = {
    /* Main Functions */
    .config   = basic_radio_cc23x0_config_impl,
    .send     = basic_radio_cc23x0_send,
    .recv     = basic_radio_cc23x0_recv,
    .test_cw  = basic_radio_cc23x0_test_cw,
    /* Optional Functions */
    .get_caps = basic_radio_cc23x0_get_caps,
	/* Vendor-Specific */
    .ext_ctl  = basic_radio_cc23x0_ext_ctl,
};

static const struct basic_radio_cc23x0_config basic_radio_cc23x0_cfg_0 = {
    .boot_phy = BOOT_PHY,
};

static struct basic_radio_cc23x0_data basic_radio_cc23x0_data_0;

DEVICE_DT_INST_DEFINE(INST,
                      basic_radio_cc23x0_init,
                      NULL,
                      &basic_radio_cc23x0_data_0,
                      &basic_radio_cc23x0_cfg_0,
                      POST_KERNEL,
                      CONFIG_BASIC_RADIO_INIT_PRIORITY,
                      &basic_radio_cc23x0_api);
                      