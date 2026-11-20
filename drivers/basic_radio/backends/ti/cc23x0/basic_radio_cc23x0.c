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
#include <stddef.h>
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
#include <inc/hw_systim.h>
#include <inc/hw_types.h>
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

/* RCL times are SYSTIM 250 ns steps */
#define SYSTIM_TICKS_PER_US 4U

/*
 * RCL compares start times as signed 32-bit differences of 250 ns steps,
 * a window of +-536 s. Keep a margin below it.
 */
#define SCHED_HORIZON_US 500000000U

/* Longest tone: relHardStopTime is 32 bits of 250 ns steps */
#define TONE_DURATION_MAX_US (UINT32_MAX / SYSTIM_TICKS_PER_US)

/*
 * Refresh period of the 64-bit time extension. TIME1U wraps every 71.6 min,
 * so one read per period catches every wrap.
 */
#define TIME_REFRESH_PERIOD K_MINUTES(30)

/* PLLM base is frequency / 48 MHz with 18 fractional bits */
#define SYNTH_STEP_MILLIHZ ((48000000ULL * 1000ULL) >> 18)

/*
 * Estimate of the retune time: RCL configure and arm margins before a
 * scheduled start. Synthesizer settling is not included.
 */
#define RETUNE_TIME_US 116U

/* Highest sync word the RCL generic commands accept */
#define SYNC_WORD_MAX UINT32_MAX

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

/* Visit one region of a register list. Returns true if the region holds the searched value. */
typedef bool (*region_visit_fn)(LRF_RegionOperation op, uint32_t start, const uint32_t *data, uint32_t regLen,
                                void *arg);

/* Halfword written to LENCFG_OFFSET by one region, if any */
static bool region_find_lencfg(LRF_RegionOperation op, uint32_t start, const uint32_t *data, uint32_t regLen,
                               void *arg)
{
    uint16_t *lencfg = arg;
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

/* TX power table referenced by one parameter region, if any */
static bool region_find_tx_power_table(LRF_RegionOperation op, uint32_t start, const uint32_t *data, uint32_t regLen,
                                       void *arg)
{
    const LRF_TxPowerTable **table = arg;
    uint32_t offset = offsetof(LRF_SwParam, txPowerTable);

    /* An embedded table: the parameter points to the region data */
    if (op == Par_Reference_32bit)
    {
        if (start != offset)
        {
            return (false);
        }

        *table = (const LRF_TxPowerTable *)data;

        return (true);
    }

    if ((op != Par_Write_32bit) || (offset < start) || (((offset - start) % sizeof(uint32_t)) != 0U) ||
        (((offset - start) / sizeof(uint32_t)) >= regLen))
    {
        return (false);
    }

    *table = (const LRF_TxPowerTable *)data[(offset - start) / sizeof(uint32_t)];

    return (true);
}

/*
 * Walk the regions of the segments the radio applies for a PHY.
 * Returns 0 if a visit found the value, -ENOTSUP if none did.
 */
static int phy_regions_walk(const struct basic_radio_cc23x0_phy *phy, region_visit_fn visit, void *arg)
{
    const LRF_Config *lrf = phy->lrf_config;
    bool isFound = false;

    if ((lrf == NULL) || (lrf->regConfigList == NULL))
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
                if (visit(op, reg.region.startAddress, word, regLen, arg))
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
 * Read PBE_GENERIC_RAM.LENCFG from the register list of a PHY. The radio
 * applies settings at command start, so it cannot be read back from PBE RAM
 * before the first TX header is built.
 */
static int lencfg_find(const struct basic_radio_cc23x0_phy *phy, uint16_t *lencfg)
{
    /* LENCFG lives at this offset only in the generic PBE RAM map */
    if (phy->lrf_config->pbeImage != (const LRF_TOPsmImage *)LRF_PBE_binary_generic)
    {
        return (-ENOTSUP);
    }

    return (phy_regions_walk(phy, region_find_lencfg, lencfg));
}

/*
 * TX power range of a PHY, from the first and last entry of its sorted TX
 * power table. Returns false if the register list names no table.
 */
static bool phy_tx_power_get(const struct basic_radio_cc23x0_phy *phy, int8_t *minDbm, int8_t *maxDbm)
{
    const LRF_TxPowerTable *table = NULL;

    if ((phy_regions_walk(phy, region_find_tx_power_table, &table) != 0) || (table == NULL) ||
        (table->numEntries == 0U))
    {
        return (false);
    }

    *minDbm = table->powerTable[0].power.dBm;
    *maxDbm = table->powerTable[table->numEntries - 1U].power.dBm;

    return (true);
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
static RCL_CmdGenericTxTest tone_cmd = RCL_CmdGenericTxTest_Default();

static uint32_t rx_bufs[RX_BUFFER_COUNT][RCL_MultiBuffer_len_u32(RX_BUFFER_SIZE)];
static uint32_t tx_bufs[TX_BUFFER_COUNT][RCL_TxBuffer_len_u32(NUM_PAD_BYTES, MAX_HDR_LEN, MAX_PACKET_LEN)];

static const struct device *basic_radio_cc23x0_dev;

/* 64-bit extension of SYSTIM TIME1U: wrap count and last read value */
static uint32_t time_hi;
static uint32_t time_last;
static struct k_timer time_refresh_timer;

/*****************************************************************************************************************/
/* Driver API: Time Get */
static uint64_t basic_radio_cc23x0_time_get(const struct device *dev)
{
    ARG_UNUSED(dev);

    /* Read and extend in one step, so concurrent callers see one wrap */
    unsigned int key = irq_lock();
    uint32_t now = HWREG(SYSTIM_BASE + SYSTIM_O_TIME1U);

    if (now < time_last)
    {
        time_hi++;
    }

    time_last = now;

    uint64_t time = ((uint64_t)time_hi << 32) | now;

    irq_unlock(key);

    return (time);
}

/* Read the time often enough to catch every wrap of TIME1U */
static void time_refresh(struct k_timer *timer)
{
    ARG_UNUSED(timer);

    (void)basic_radio_cc23x0_time_get(basic_radio_cc23x0_dev);
}

/*
 * Program the start of a command. An absolute start must be in the future
 * and within the RCL compare window.
 */
static int start_time_set(RCL_Command *cmd, bool isScheduled, uint64_t startUs)
{
    if (!isScheduled)
    {
        cmd->scheduling = RCL_Schedule_Now;

        return (0);
    }

    uint64_t now = basic_radio_cc23x0_time_get(basic_radio_cc23x0_dev);

    if (startUs <= now)
    {
        return (-ETIME);
    }

    if ((startUs - now) > SCHED_HORIZON_US)
    {
        return (-EINVAL);
    }

    /* Low 32 bits of the 250 ns SYSTIM count */
    cmd->scheduling = RCL_Schedule_AbsTime;
    cmd->timing.absStartTime = (uint32_t)(startUs * SYSTIM_TICKS_PER_US);

    return (0);
}

/* RX timestamp on the 64-bit time base, 0 if the radio has none */
static uint64_t rx_timestamp_get(void)
{
    if (!rx_cmd.stats->timestampValid)
    {
        return (0);
    }

    /* The packet is in the past: age in 250 ns steps, modulo 2^32 */
    uint64_t now = basic_radio_cc23x0_time_get(basic_radio_cc23x0_dev);
    uint32_t age = (uint32_t)(now * SYSTIM_TICKS_PER_US) - rx_cmd.stats->lastTimestamp;

    return (now - (age / SYSTIM_TICKS_PER_US));
}

/* Carrier frequency inside the characterized range of a PHY */
static bool frequency_is_valid(const struct basic_radio_cc23x0_phy *phy, uint32_t frequency)
{
    return ((frequency >= phy->frequency_min) && (frequency <= phy->frequency_max));
}

/* TX power inside the TX power table of the active PHY, if it has one */
static bool tx_power_is_valid(const struct basic_radio_cc23x0_data *data, int8_t txPowerDbm)
{
    if (!data->isTxPowerKnown)
    {
        return (true);
    }

    return ((txPowerDbm >= data->tx_power_min_dbm) && (txPowerDbm <= data->tx_power_max_dbm));
}

/*
 * Write the runtime parameters into the driver cache and both RCL commands.
 * Shared by init and basic_radio_config().
 */
static void rf_params_set(struct basic_radio_cc23x0_data *data, uint32_t frequency, uint32_t syncWord,
                          int8_t txPowerDbm, uint8_t maxPacketLen)
{
    data->cfg_frequency      = frequency;
    data->cfg_sync_word      = syncWord;
    data->cfg_tx_power_dbm   = txPowerDbm;
    data->cfg_max_packet_len = MIN(maxPacketLen, MAX_PACKET_LEN);

    tx_cmd.rfFrequency      = frequency * BASIC_RADIO_FREQ_UNIT_HZ;
    rx_cmd.rfFrequency      = frequency * BASIC_RADIO_FREQ_UNIT_HZ;

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

    tx_cmd.common.phyFeatures   = phy->phy_features;
    rx_cmd.common.phyFeatures   = phy->phy_features;
    tone_cmd.common.phyFeatures = phy->phy_features;
    data->phy = phy;
    data->hdr = *hdr;

    /* Without a TX power table, RCL rejects an invalid power at command start */
    data->isTxPowerKnown = phy_tx_power_get(phy, &data->tx_power_min_dbm, &data->tx_power_max_dbm);

    if (!data->isTxPowerKnown)
    {
        LOG_WRN("PHY has no TX power table");
    }

    return (0);
}

/*****************************************************************************************************************/
/* Final status of an RCL command as reported in basic_radio_evt::status */
static int status_to_errno(RCL_CommandStatus status)
{
    /* A timeout stop is the programmed end of a tone */
    if ((status == RCL_CommandStatus_Finished) || RCL_CommandStatus_isAnyTimeoutStop(status))
    {
        return (0);
    }

    if (RCL_CommandStatus_isAnyApiStop(status))
    {
        return (-ECANCELED);
    }

    if ((status == RCL_CommandStatus_RejectedStart) || (status == RCL_CommandStatus_Error_StartTooLate))
    {
        return (-ETIME);
    }

    return (-EIO);
}

/*
 * Report the final event of the active operation. The radio is idle before
 * the callback runs, so the callback can start the next operation.
 */
static void op_done(struct basic_radio_cc23x0_data *data, enum basic_radio_evt_type type, RCL_CommandStatus status)
{
    basic_radio_callback_t cb = data->cb;
    void *userData = data->user_data;
    struct basic_radio_evt evt = {
        .type   = type,
        .status = status_to_errno(status),
    };

    atomic_set(&data->state, RADIO_STATE_IDLE);
    cb(basic_radio_cc23x0_dev, &evt, userData);
}

/* Report every stored packet, then hand the drained buffers back to RX */
static void rx_entries_report(struct basic_radio_cc23x0_data *data)
{
    List_List consumed = {0};
    RCL_Buffer_DataEntry *entry;
    RCL_MultiBuffer *mb;

    while ((entry = RCL_MultiBuffer_RxEntry_get(&rx_cmd.rxBuffers, &consumed)) != NULL)
    {
        uint8_t *pkt = &entry->pad0 + entry->numPad;
        struct basic_radio_evt evt = {
            .type = BASIC_RADIO_EVT_RX_PKT,
            .data = pkt + data->hdr.len,
            .len  = (uint16_t)(pkt[data->hdr.lenIndex] & data->hdr.lenMask),
            .info = {.timestamp_us = rx_timestamp_get(), .rssi = rx_cmd.stats->lastRssi, .crc_ok = true},
        };

        data->cb(basic_radio_cc23x0_dev, &evt, data->user_data);
    }

    while ((mb = RCL_MultiBuffer_get(&consumed)) != NULL)
    {
        RCL_MultiBuffer_clear(mb);
        RCL_MultiBuffer_put(&rx_cmd.rxBuffers, mb);
    }
}

/* RCL Rx Callback */
static void rx_callback(RCL_Command *cmd, LRF_Events lrfEvt, RCL_Events rclEvt)
{
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

    /* Discarded packets are not stored, report them from the LRF event */
    if (rx_cmd.config.discardRxPackets && lrfEvt.rxOk)
    {
        struct basic_radio_evt evt = {
            .type = BASIC_RADIO_EVT_RX_PKT,
            .info = {.timestamp_us = rx_timestamp_get(), .rssi = rx_cmd.stats->lastRssi, .crc_ok = true},
        };

        data->cb(basic_radio_cc23x0_dev, &evt, data->user_data);
    }

    /* Packets stored before the end are reported before RX_DONE */
    if (rclEvt.rxEntryAvail || rclEvt.lastCmdDone)
    {
        rx_entries_report(data);
    }

    if (rclEvt.lastCmdDone)
    {
        op_done(data, BASIC_RADIO_EVT_RX_DONE, cmd->status);
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

    op_done(data, BASIC_RADIO_EVT_TX_DONE, cmd->status);
}

/* RCL Tone Callback */
static void tone_callback(RCL_Command *cmd, LRF_Events lrfEvt, RCL_Events rclEvt)
{
    ARG_UNUSED(lrfEvt);
    struct basic_radio_cc23x0_data *data = basic_radio_cc23x0_dev->data;

    if (rclEvt.lastCmdDone)
    {
        op_done(data, BASIC_RADIO_EVT_TONE_DONE, cmd->status);
    }
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
    if (!frequency_is_valid(phy, cfg->frequency))
    {
        LOG_ERR("frequency %u outside the PHY's characterized range", cfg->frequency);

        return (-EINVAL);
    }

    if (cfg->sync_word > SYNC_WORD_MAX)
    {
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

    /* The TX power range is known once the PHY is applied */
    if ((ret == 0) && !tx_power_is_valid(dd, cfg->tx_power))
    {
        ret = -EINVAL;
    }

    /* After changing the PHY, modify the runtime parameters */
    if (ret == 0)
    {
        rf_params_set(dd, cfg->frequency, (uint32_t)cfg->sync_word, cfg->tx_power, cfg->max_packet_len);
    }

    atomic_set(&dd->state, RADIO_STATE_IDLE);

    return (ret);
}

/*****************************************************************************************************************/
/* Driver API: Get Caps */
static int basic_radio_cc23x0_get_caps(const struct device *dev, struct basic_radio_caps *caps)
{
    const struct basic_radio_cc23x0_data *data = dev->data;

    if (data->phy == NULL)
    {
        return (-ENODEV);
    }

    *caps = (struct basic_radio_caps){
        .flags                   = BASIC_RADIO_CAP_SCHED | BASIC_RADIO_CAP_TONE | BASIC_RADIO_CAP_STOP_GRACEFUL,
        .freq_min                = data->phy->frequency_min,
        .freq_max                = data->phy->frequency_max,
        .freq_resolution_millihz = (uint32_t)SYNTH_STEP_MILLIHZ,
        .retune_time_us          = RETUNE_TIME_US,
        .sched_horizon_us        = SCHED_HORIZON_US,
        .tx_power_min_dbm        = data->tx_power_min_dbm,
        .tx_power_max_dbm        = data->tx_power_max_dbm,
    };

    return (0);
}

/*****************************************************************************************************************/
/* Driver API: Send */
static int basic_radio_cc23x0_send_async(const struct device *dev, const uint8_t *data, uint16_t len,
                                         const struct basic_radio_tx_params *params, basic_radio_callback_t cb,
                                         void *user_data)
{
    struct basic_radio_cc23x0_data *dd = dev->data;

    LOG_INF("TI-cc23xx");

    if (len == 0 || len > MAX_PACKET_LEN || cb == NULL)
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

    uint32_t frequency = (params && (params->flags & BASIC_RADIO_TX_FLAG_CHANNEL)) ? params->frequency : dd->cfg_frequency;
    uint64_t syncWord = (params && (params->flags & BASIC_RADIO_TX_FLAG_SYNC_WORD)) ? params->sync_word : dd->cfg_sync_word;
    int8_t txPowerDbm = (params && (params->flags & BASIC_RADIO_TX_FLAG_POWER)) ? params->tx_power_dbm : dd->cfg_tx_power_dbm;

    if (!frequency_is_valid(dd->phy, frequency) || (syncWord > SYNC_WORD_MAX) || !tx_power_is_valid(dd, txPowerDbm))
    {
        return (-EINVAL);
    }

    if (!atomic_cas(&dd->state, RADIO_STATE_IDLE, RADIO_STATE_TX_ACTIVE))
    {
        return (-EBUSY);
    }

    dd->cb = cb;
    dd->user_data = user_data;

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

    tx_cmd.rfFrequency = frequency * BASIC_RADIO_FREQ_UNIT_HZ;
    tx_cmd.syncWord = (uint32_t)syncWord;
    tx_cmd.txPower.dBm = txPowerDbm;

    int ret = start_time_set(&tx_cmd.common, (params && (params->flags & BASIC_RADIO_TX_FLAG_SCHED_ABS)),
                             (params != NULL) ? params->abs_start_time_us : 0U);

    if (ret != 0)
    {
        atomic_set(&dd->state, RADIO_STATE_IDLE);

        return (ret);
    }

    /* A rejected submit reports no event. Otherwise tx_callback ends the operation */
    RCL_CommandStatus st = RCL_Command_submit(dd->handle, &tx_cmd.common);

    if (st >= RCL_CommandStatus_Error)
    {
        atomic_set(&dd->state, RADIO_STATE_IDLE);

        return (-EIO);
    }

    return (0);
}

/*****************************************************************************************************************/
/* Driver API: Receive */
static int basic_radio_cc23x0_recv_async(const struct device *dev, const struct basic_radio_rx_params *params,
                                         basic_radio_callback_t cb, void *user_data)
{
    struct basic_radio_cc23x0_data *data = dev->data;

    LOG_INF("TI-cc23xx");

    static const uint32_t supported_rx_flags = BASIC_RADIO_RX_FLAG_SCHED_ABS | BASIC_RADIO_RX_FLAG_ONE_SHOT | BASIC_RADIO_RX_FLAG_DISCARD_PAYL | BASIC_RADIO_RX_FLAG_CHANNEL | BASIC_RADIO_RX_FLAG_SYNC_WORD | BASIC_RADIO_RX_FLAG_MAX_LEN;

    if (cb == NULL)
    {
        return (-EINVAL);
    }

    if (params && (params->flags & ~supported_rx_flags))
    {
        return (-ENOTSUP);
    }

    if (data->phy == NULL)
    {
        return (-ENODEV);
    }

    uint32_t frequency = (params && (params->flags & BASIC_RADIO_RX_FLAG_CHANNEL)) ? params->frequency : data->cfg_frequency;
    uint64_t syncWord = (params && (params->flags & BASIC_RADIO_RX_FLAG_SYNC_WORD)) ? params->sync_word : data->cfg_sync_word;

    if (!frequency_is_valid(data->phy, frequency) || (syncWord > SYNC_WORD_MAX))
    {
        return (-EINVAL);
    }

    /* Own the radio so no send()/config() touches RCL while RX is pending */
    if (!atomic_cas(&data->state, RADIO_STATE_IDLE, RADIO_STATE_RX_ACTIVE))
    {
        return (-EBUSY);
    }

    data->cb = cb;
    data->user_data = user_data;

    rx_cmd.config.repeated = (params && (params->flags & BASIC_RADIO_RX_FLAG_ONE_SHOT)) ? 0 : 1;
    rx_cmd.config.discardRxPackets = (params && (params->flags & BASIC_RADIO_RX_FLAG_DISCARD_PAYL)) ? 1 : 0;
    rx_cmd.rfFrequency = frequency * BASIC_RADIO_FREQ_UNIT_HZ;
    rx_cmd.syncWordA = (uint32_t)syncWord;
    rx_cmd.maxPktLen = (params && (params->flags & BASIC_RADIO_RX_FLAG_MAX_LEN)) ? params->max_packet_len : data->cfg_max_packet_len;

    int ret = start_time_set(&rx_cmd.common, (params && (params->flags & BASIC_RADIO_RX_FLAG_SCHED_ABS)),
                             (params != NULL) ? params->abs_start_time_us : 0U);

    if (ret != 0)
    {
        atomic_set(&data->state, RADIO_STATE_IDLE);

        return (ret);
    }

    /* Rebuild the list: the last RX may have ended with buffers still in it */
    List_clearList(&rx_cmd.rxBuffers);

    for (uint8_t i = 0; i < RX_BUFFER_COUNT; i++)
    {
        RCL_MultiBuffer *mb = (RCL_MultiBuffer *)rx_bufs[i];
        RCL_MultiBuffer_clear(mb);
        RCL_MultiBuffer_put(&rx_cmd.rxBuffers, mb);
    }

    rx_cmd.common.status = RCL_CommandStatus_Idle;

    /* A rejected submit reports no event. Otherwise rx_callback ends the operation */
    RCL_CommandStatus st = RCL_Command_submit(data->handle, &rx_cmd.common);

    if (st >= RCL_CommandStatus_Error)
    {
        atomic_set(&data->state, RADIO_STATE_IDLE);

        return (-EIO);
    }

    return (0);
}

/*****************************************************************************************************************/
/* Driver API: Stop */
static int basic_radio_cc23x0_stop(const struct device *dev, enum basic_radio_stop_mode mode)
{
    struct basic_radio_cc23x0_data *data = dev->data;
    RCL_Command *cmd;
    RCL_StopType stopType;

    switch (mode)
    {
        case BASIC_RADIO_STOP_DESCHEDULE:
            stopType = RCL_StopType_DescheduleOnly;
            break;

        case BASIC_RADIO_STOP_GRACEFUL:
            stopType = RCL_StopType_Graceful;
            break;

        case BASIC_RADIO_STOP_ABORT:
            stopType = RCL_StopType_Hard;
            break;

        default:
            return (-EINVAL);
    }

    /* The operation ends in the RCL interrupt; keep it from ending between state and stop */
    unsigned int key = irq_lock();

    switch (atomic_get(&data->state))
    {
        case RADIO_STATE_TX_ACTIVE:
            cmd = &tx_cmd.common;
            break;

        case RADIO_STATE_RX_ACTIVE:
            cmd = &rx_cmd.common;
            break;

        case RADIO_STATE_TONE_ACTIVE:
            cmd = &tone_cmd.common;
            break;

        default:
            cmd = NULL;
            break;
    }

    /* The final event follows once the stop takes effect */
    RCL_CommandStatus st = (cmd != NULL) ? RCL_Command_stop(cmd, stopType) : RCL_CommandStatus_Idle;

    irq_unlock(key);

    if ((st < RCL_CommandStatus_Scheduled) || (st >= RCL_CommandStatus_Finished))
    {
        return (-EALREADY);
    }

    if ((stopType == RCL_StopType_DescheduleOnly) && (st != RCL_CommandStatus_Scheduled))
    {
        return (-EBUSY);
    }

    return (0);
}

/*****************************************************************************************************************/
/* Driver API: Tone */
static int basic_radio_cc23x0_tone_async(const struct device *dev, const struct basic_radio_tone_params *params,
                                         basic_radio_callback_t cb, void *user_data)
{
    struct basic_radio_cc23x0_data *data = dev->data;

    if ((params == NULL) || (cb == NULL))
    {
        return (-EINVAL);
    }

    if ((params->flags & ~BASIC_RADIO_TONE_FLAG_SCHED_ABS) != 0U)
    {
        return (-ENOTSUP);
    }

    /* RCL needs an open PHY to drive the synthesizer */
    if (data->phy == NULL)
    {
        return (-ENODEV);
    }

    if (!frequency_is_valid(data->phy, params->frequency) || (params->duration_us > TONE_DURATION_MAX_US) ||
        !tx_power_is_valid(data, params->tx_power_dbm))
    {
        return (-EINVAL);
    }

    if (!atomic_cas(&data->state, RADIO_STATE_IDLE, RADIO_STATE_TONE_ACTIVE))
    {
        return (-EBUSY);
    }

    data->cb = cb;
    data->user_data = user_data;

    tone_cmd.rfFrequency = params->frequency * BASIC_RADIO_FREQ_UNIT_HZ;
    tone_cmd.txPower.dBm = params->tx_power_dbm;
    tone_cmd.txPower.fraction = 0;
    tone_cmd.config.sendCw = 1;
    /* The hard stop ends the tone; 0 runs it until basic_radio_stop() */
    tone_cmd.common.timing.relHardStopTime = params->duration_us * SYSTIM_TICKS_PER_US;

    int ret = start_time_set(&tone_cmd.common, ((params->flags & BASIC_RADIO_TONE_FLAG_SCHED_ABS) != 0U),
                             params->abs_start_time_us);

    if (ret != 0)
    {
        atomic_set(&data->state, RADIO_STATE_IDLE);

        return (ret);
    }

    tone_cmd.common.status = RCL_CommandStatus_Idle;

    /* A rejected submit reports no event. Otherwise tone_callback ends the operation */
    RCL_CommandStatus st = RCL_Command_submit(data->handle, &tone_cmd.common);

    if (st >= RCL_CommandStatus_Error)
    {
        atomic_set(&data->state, RADIO_STATE_IDLE);

        return (-EIO);
    }

    return (0);
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

    tone_cmd.common.runtime.callback = tone_callback;
    tone_cmd.common.runtime.rclCallbackMask.value = RCL_EventLastCmdDone.value;

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

    k_timer_init(&time_refresh_timer, time_refresh, NULL);
    k_timer_start(&time_refresh_timer, TIME_REFRESH_PERIOD, TIME_REFRESH_PERIOD);

    return (0);
}

/*****************************************************************************************************************/
static const struct basic_radio_driver_api basic_radio_cc23x0_api = {
    /* Main Functions */
    .config     = basic_radio_cc23x0_config_impl,
    .send_async = basic_radio_cc23x0_send_async,
    .recv_async = basic_radio_cc23x0_recv_async,
    .tone_async = basic_radio_cc23x0_tone_async,
    .stop       = basic_radio_cc23x0_stop,
    .time_get   = basic_radio_cc23x0_time_get,
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
                      