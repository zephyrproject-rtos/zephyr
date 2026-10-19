/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Private types for the TI CC23x0 BASIC_RADIO backend.
 * Not part of the public driver API.
 */

#ifndef DRIVERS_BASIC_RADIO_BACKENDS_TI_CC23X0_BASIC_RADIO_CC23X0_H_
#define DRIVERS_BASIC_RADIO_BACKENDS_TI_CC23X0_BASIC_RADIO_CC23X0_H_

#include <stdint.h>

#include <zephyr/drivers/basic_radio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>

#include <ti/drivers/rcl/RCL.h>
#include <ti/drivers/rcl/commands/generic.h>

/* Padding bytes in front of the TX header for the PBE generic image */
#define NUM_PAD_BYTES 3

/*
 * One application PHY, built at compile time from a ti,cc23x0-prop-rf-phy
 * child node and the operating mode it references (ti,radio-mode).
 *
 * lrf_config and phy_feature select the waveform. frequency_min/max is the
 * mode's characterized range. frequency_hz, sync_word, tx_power_dbm and
 * max_packet_len are the DT defaults. dt_id identifies the DT node
 * (BASIC_RADIO_PHY_DT_ID) for BASIC_RADIO_config().
 */
struct phy_profile
{
    const LRF_Config *lrf_config;
    uint32_t          frequency_hz;
    uint32_t          frequency_min;
    uint32_t          frequency_max;
    uint32_t          sync_word;
    uint16_t          dt_id;
    uint16_t          phy_feature;
    int8_t            tx_power_dbm;
    uint8_t           max_packet_len;
};

struct basic_radio_cc23x0_config
{
    const struct phy_profile *phy_table;
    uint8_t                   phy_count;
};

/*
 * Radio state machine. Radio returns to IDLE between every send() and
 * every recv(); no continuous background RX.
 */
enum radio_state
{
    RADIO_STATE_IDLE = 0,
    RADIO_STATE_TX_ACTIVE,
    RADIO_STATE_RX_ACTIVE,
};

struct basic_radio_cc23x0_data
{
    RCL_Client       rcl_client;
    RCL_Handle       handle;
    RCL_StatsGeneric stats;

    atomic_t state;

    struct k_sem rx_sem;
    struct k_sem tx_sync_done;

    uint8_t tx_pool_idx;
    uint8_t active_phy_idx;

    /* Layer 1 config cache */
    uint32_t cfg_frequency_hz;
    uint32_t cfg_sync_word;
    int8_t   cfg_tx_power_dbm;
    uint8_t  cfg_max_packet_len;

    /* Observability counters, written from HWI (single writer) */
    uint32_t rx_ok_count;
    uint32_t rx_crc_err_count;
    uint32_t sched_reject_count;
    uint32_t cmd_started_count;
};

#endif /* DRIVERS_BASIC_RADIO_BACKENDS_TI_CC23X0_BASIC_RADIO_CC23X0_H_ */