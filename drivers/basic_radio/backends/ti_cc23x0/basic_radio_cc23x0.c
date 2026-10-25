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
 * PHY selection comes from devicetree. Each basic-radio-base PHY
 * child of the radio node selects one operating mode of a
 * ti,cc23x0-radio-phy-config node through phy-mode. That node names
 * the LRF_Config (SmartRF Studio C export) the driver links against:
 * certified PHYs come from hal_ti, custom PHYs from the application.
 */

#define DT_DRV_COMPAT basic_radio_base

#include <errno.h>
#include <limits.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/basic_radio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>

#include <ti/devices/DeviceFamily.h>
#include <ti/drivers/rcl/RCL.h>
#include <ti/drivers/rcl/LRF.h>
#include <ti/drivers/rcl/commands/generic.h>

#include "basic_radio_cc23x0.h"

LOG_MODULE_REGISTER(basic_radio_cc23x0, CONFIG_BASIC_RADIO_LOG_LEVEL);

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) <= 1, "basic_radio_cc23x0 backend supports at most one enabled instance");

#define INST                0
#define RX_BUFFER_COUNT     CONFIG_BASIC_RADIO_RX_BUFFER_COUNT
#define RX_BUFFER_SIZE      CONFIG_BASIC_RADIO_CC23X0_RX_BUFFER_SIZE
#define TX_BUFFER_COUNT     CONFIG_BASIC_RADIO_TX_BUFFER_COUNT
#define MAX_PACKET_LEN      CONFIG_BASIC_RADIO_MAX_PACKET_LEN

/*****************************************************************************************************************/
/*
 * PHY table: one entry per basic-radio-base PHY child.
 *
 * Hierarchy: instance → phy child (phy-mode → mode node → ti,cc23x0-radio-phy-config)
 *
 * Runtime (child): frequency_hz, sync_word, tx_power_dbm, max_packet_length
 * Caps (mode):     frequency_min_hz, frequency_max_hz, ti,phy-feature
 */
#define PHY_MODE(child)    DT_PHANDLE(child, phy_mode)
#define PHY_LRF_CFG(child) DT_PARENT(PHY_MODE(child))
#define PHY_LRF_SYM(child) DT_STRING_TOKEN(PHY_LRF_CFG(child), ti_lrf_config)

/*
 * Declare only the LRF_Configs the enabled phy children reference, so
 * unused PHYs are dropped at link time. Used to generate external declarations 
 * for configuration structures
 */
#define PHY_LRF_EXTERN(child) extern const LRF_Config PHY_LRF_SYM(child);

DT_INST_FOREACH_CHILD_STATUS_OKAY(INST, PHY_LRF_EXTERN)

/*
 * TODO: check again if needed when hal_ti RCL driver is updated in upstream.
 * RCL_AdcNoise (used by the entropy driver) opens this config. The HAL
 * normally provides it with its pre-generated settings, which basic_radio
 * disables (CONFIG_RCL_RF_SETTING_PREGEN=n). It is not a PHY.

extern const uint32_t LRF_RFE_binary_ble5[];

static const LRF_RegConfigList adc_noise_reg_config_list = {
    .numEntries = 0,
};

const LRF_Config LRF_configAdcNoise = {
    .pbeImage      = NULL,
    .mceImage      = NULL,
    .rfeImage      = (const LRF_TOPsmImage *)LRF_RFE_binary_ble5,
    .regConfigList = &adc_noise_reg_config_list,
};
*/

#define CHILD_PHY_INIT(child)                                                                           \
{                                                                                                       \
    .lrf_config     = &PHY_LRF_SYM(child),                                  /* SmartRF LRF_Config */    \
    .dt_id          = BASIC_RADIO_PHY_DT_ID(child),                         /* phy index for API */     \
    .frequency_hz   = DT_PROP(child, frequency_hz),                         /* center frequency */      \
    .sync_word      = DT_PROP(child, sync_word),                            /* sync word */             \
    .tx_power_dbm   = DT_PROP(child, tx_power_dbm),                         /* TX power dBm */          \
    .max_packet_len = DT_PROP(child, max_packet_length),                    /* max packet length */     \
    .frequency_min  = DT_PROP_OR(PHY_MODE(child), frequency_min_hz, 0),     /* min freq allowed */      \
    .frequency_max  = DT_PROP_OR(PHY_MODE(child), frequency_max_hz, 0),     /* max freq allowed */      \
    .phy_feature    = DT_PROP_OR(PHY_MODE(child), ti_phy_feature, 0),       /* phyFeatures */           \
}

static const struct phy_profile phy_table[] = {
    DT_INST_FOREACH_CHILD_STATUS_OKAY_SEP(INST, CHILD_PHY_INIT, (,))
};

BUILD_ASSERT(ARRAY_SIZE(phy_table) >= 1, "basic-radio-base node needs at least one PHY child");

/*
 * TX header layout comes from the FIRST PHY's PBE_GENERIC_RAM.LENCFG.
 * ALL PHYs UNDER THIS INSTANCE MUST USE THE SAME LENCFG because TX/RX
 * buffers are allocated ONCE at compile time using HDR_LEN/LEN_INDEX.
 *
 * LENCFG exists only in generic PBE exports (ti,pbe-image = "generic").
 * BLE5/IEEE PBE images hardcode length position in firmware — no LENCFG.
 * This driver uses RCL generic commands (RCL_CmdGenericTx/Rx) which require
 * generic PBE. If a PHY uses ble5/ieee PBE, LENCFG is absent and the build
 * fails here.
 *
 * Mixing PHYs with different LENCFG (e.g., BLE LENPOS=8 + IEEE LENPOS=0)
 * in one instance is NOT supported — would corrupt TX/RX parsing.
 *
 * CURRENT ASSERT ONLY CHECKS PROPERTIES EXIST, NOT THAT VALUES MATCH.
 * PHY0: LENPOS=8, NUMLENBITS=8  and  PHY1: LENPOS=0, NUMLENBITS=7
 * will BUILD OK but CORRUPT RX/TX at runtime. No cross-PHY check exists yet.
 */
#define PHY_ASSERT_GENERIC_PBE(child)                                              \
    BUILD_ASSERT(DT_NODE_HAS_PROP(PHY_LRF_CFG(child), ti_len_pos) &&               \
                 DT_NODE_HAS_PROP(PHY_LRF_CFG(child), ti_num_len_bits),            \
                 "phy-mode needs a generic PBE radio config "                 \
                 "(ti,len-pos and ti,num-len-bits missing)");

DT_INST_FOREACH_CHILD_STATUS_OKAY(INST, PHY_ASSERT_GENERIC_PBE)

#define PHY_LEN_POS(child)      DT_PROP_OR(PHY_LRF_CFG(child), ti_len_pos, 0)
#define PHY_NUM_LEN_BITS(child) DT_PROP_OR(PHY_LRF_CFG(child), ti_num_len_bits, 0)
#define BOOT_LEN_POS            GET_ARG_N(1, DT_INST_FOREACH_CHILD_STATUS_OKAY_SEP(INST, PHY_LEN_POS, (,)))
#define BOOT_NUM_LEN_BITS       GET_ARG_N(1, DT_INST_FOREACH_CHILD_STATUS_OKAY_SEP(INST, PHY_NUM_LEN_BITS, (,)))
#define LEN_INDEX               (BOOT_LEN_POS / CHAR_BIT)
#define HDR_LEN                 (LEN_INDEX + (BOOT_NUM_LEN_BITS / CHAR_BIT))

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
static uint32_t tx_bufs[TX_BUFFER_COUNT][RCL_TxBuffer_len_u32(NUM_PAD_BYTES, HDR_LEN, MAX_PACKET_LEN)];

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
 * All PHYs in one instance MUST share LENCFG (same HDR_LEN), enforced
 * by design, not checked at runtime. Mixing mismatched LENCFG corrupts TX/RX.
 */
static int phy_select(struct basic_radio_cc23x0_data *data, const struct basic_radio_cc23x0_config *cfg, uint8_t idx)
{
    const struct phy_profile *phy = &cfg->phy_table[idx];
    const struct phy_profile *current = &cfg->phy_table[data->active_phy_idx];

    if ((data->handle == NULL) || (phy->lrf_config != current->lrf_config))
    {
        if (data->handle != NULL)
        {
            RCL_close(data->handle);
        }

        data->handle = RCL_open(&data->rcl_client, phy->lrf_config);

        if (data->handle == NULL)
        {
            LOG_ERR("RCL_open failed for PHY %u", idx);

            return (-EIO);
        }
    }

    tx_cmd.common.phyFeatures = phy->phy_feature;
    rx_cmd.common.phyFeatures = phy->phy_feature;
    data->active_phy_idx      = idx;

    return (0);
}

/* Index of the phy_table entry built from DT node id, or -1 */
static int phy_find(const struct basic_radio_cc23x0_config *cfg, uint16_t dtId)
{
    for (uint8_t i = 0; i < cfg->phy_count; i++)
    {
        if (cfg->phy_table[i].dt_id == dtId)
        {
            return (i);
        }
    }

    return (-1);
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
 * cfg->phy selects the waveform (BASIC_RADIO_CONFIG_DT() fills it from the PHY
 * node); BASIC_RADIO_PHY_CURRENT keeps the active one. The runtime parameters
 * are then applied on top. Waveform fields of cfg are ignored: on CC23x0 the
 * waveform is defined by the PHY's phy-mode.
 */
static int basic_radio_cc23x0_config_impl(const struct device *dev, const struct basic_radio_config *cfg)
{
    const struct basic_radio_cc23x0_config *drvCfg = dev->config;
    struct basic_radio_cc23x0_data *dd = dev->data;
    int idx = dd->active_phy_idx;
    int ret = 0;

    /* If phy is not the phy0 default one, then search for others in the phy table (phy childs in the overlay) */
    if (cfg->phy != BASIC_RADIO_PHY_CURRENT)
    {
        idx = phy_find(drvCfg, cfg->phy);

        if (idx < 0)
        {
            return (-EINVAL);
        }
    }

    const struct phy_profile *phy = &drvCfg->phy_table[idx];

    /* Evaluate the frequency */
    if ((cfg->frequency < phy->frequency_min) || (cfg->frequency > phy->frequency_max))
    {
        LOG_ERR("frequency %u outside the PHY's characterized range", cfg->frequency);

        return (-EINVAL);
    }

    /* Own the radio so no send()/recv() runs while RCL is reconfigured */
    if (!atomic_cas(&dd->state, RADIO_STATE_IDLE, RADIO_STATE_TX_ACTIVE))
    {
        return (-EBUSY);
    }

    /* If a new phy node is detected, then change the RCL settings */
    if (idx != dd->active_phy_idx)
    {
        ret = phy_select(dd, drvCfg, (uint8_t)idx);
    }

    /* After chaning the main phy settings, modify the runtime parameters */
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

    if (!atomic_cas(&dd->state, RADIO_STATE_IDLE, RADIO_STATE_TX_ACTIVE))
    {
        return (-EBUSY);
    }

    uint8_t slot = dd->tx_pool_idx;
    dd->tx_pool_idx = (slot + 1) % TX_BUFFER_COUNT;
    RCL_Buffer_TxBuffer *txBuf = (RCL_Buffer_TxBuffer *)tx_bufs[slot];
    uint8_t *txData = RCL_TxBuffer_init(txBuf, NUM_PAD_BYTES, HDR_LEN, len);
    memset(txData, 0, LEN_INDEX);
    txData[LEN_INDEX] = (uint8_t)len;
    memcpy(&txData[HDR_LEN], data, len);
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
    uint8_t  datalen = pkt[LEN_INDEX];
    uint8_t *payload = pkt + HDR_LEN;
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
    const struct phy_profile *boot = &cfg->phy_table[0];

    k_sem_init(&data->rx_sem, 0, RX_BUFFER_COUNT);
    k_sem_init(&data->tx_sync_done, 0, 1);
    atomic_set(&data->state, RADIO_STATE_IDLE);

    data->active_phy_idx = 0;
    data->handle = NULL;

    RCL_init();

    int ret = phy_select(data, cfg, 0);

    if (ret != 0)
    {
        return (ret);
    }

    rf_params_set(data, boot->frequency_hz, boot->sync_word, boot->tx_power_dbm, boot->max_packet_len);

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
    .phy_table = phy_table,
    .phy_count = ARRAY_SIZE(phy_table),
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
                      