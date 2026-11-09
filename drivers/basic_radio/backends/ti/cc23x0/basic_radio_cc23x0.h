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

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/drivers/basic_radio.h>
#include <zephyr/drivers/basic_radio/ti/cc23x0.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>

#include <ti/drivers/rcl/RCL.h>
#include <ti/drivers/rcl/commands/generic.h>

/* Padding bytes in front of the TX header for the PBE generic image */
#define NUM_PAD_BYTES 3

/* Frame header layout of a PHY, from its LENCFG */
struct basic_radio_cc23x0_hdr
{
    uint8_t lenIndex; /* Byte holding the length field */
    uint8_t len;      /* Header bytes up to and including the length field */
    uint8_t lenMask;  /* Length field bits within that byte */
};

struct basic_radio_cc23x0_config
{
    /* PHY applied at init, NULL if none */
    const struct basic_radio_cc23x0_phy *boot_phy;
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

    /* Active PHY, NULL until the first PHY is applied */
    const struct basic_radio_cc23x0_phy *phy;

    uint8_t tx_pool_idx;

    /* Frame header layout of the active PHY */
    struct basic_radio_cc23x0_hdr hdr;

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
