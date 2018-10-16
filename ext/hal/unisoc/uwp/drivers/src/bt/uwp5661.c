/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */
 
#include <zephyr.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <atomic.h>
#include <misc/util.h>
#include <misc/slist.h>
#include <misc/byteorder.h>
#include <misc/stack.h>
#include <misc/__assert.h>
#include <soc.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/crypto.h>
#include <bluetooth/conn.h>

#include "uwp5661.h"
#include "uki_utlis.h"
#include "uki_config.h"
#include "host/hci_core.h"
#include "bt_configure.h"

extern int uwp_mcu_init(void);

int get_disable_buf(void *buf)
{
    uint8_t *p, msg_req[HCI_CMD_MAX_LEN];
    int size;

    p = msg_req;

    UINT16_TO_STREAM(p, DUAL_MODE);
    UINT8_TO_STREAM(p, DISABLE_BT);
    size = p - msg_req;
    memcpy(buf, msg_req, size);
    return size;
}

int get_enable_buf(void *buf)
{
    uint8_t *p, msg_req[HCI_CMD_MAX_LEN];
    int size;

    p = msg_req;

    UINT16_TO_STREAM(p, DUAL_MODE);
    UINT8_TO_STREAM(p, ENABLE_BT);
    size = p - msg_req;
    memcpy(buf, msg_req, size);
    return size;
}

int marlin3_rf_preload(void *buf)
{
    uint8_t *p, msg_req[HCI_CMD_MAX_LEN];
    int i, size;

    //BTD("%s", __FUNCTION__);
    p = msg_req;

    for (i = 0; i < 6; i++) {
        UINT16_TO_STREAM(p, bt_configure_rf.g_GainValue_A[i]);
    }

    for (i = 0; i < 10; i++) {
        UINT16_TO_STREAM(p, bt_configure_rf.g_ClassicPowerValue_A[i]);
    }

    for (i = 0; i < 16; i++) {
        UINT16_TO_STREAM(p, bt_configure_rf.g_LEPowerValue_A[i]);
    }

    for (i = 0; i < 8; i++) {
        UINT16_TO_STREAM(p, bt_configure_rf.g_BRChannelpwrvalue_A[i]);
    }

    for (i = 0; i < 8; i++) {
        UINT16_TO_STREAM(p, bt_configure_rf.g_EDRChannelpwrvalue_A[i]);
    }

    for (i = 0; i < 8; i++) {
        UINT16_TO_STREAM(p, bt_configure_rf.g_LEChannelpwrvalue_A[i]);
    }

    for (i = 0; i < 6; i++) {
        UINT16_TO_STREAM(p, bt_configure_rf.g_GainValue_B[i]);
    }

    for (i = 0; i < 10; i++) {
        UINT16_TO_STREAM(p, bt_configure_rf.g_ClassicPowerValue_B[i]);
    }


    for (i = 0; i < 16; i++) {
        UINT16_TO_STREAM(p, bt_configure_rf.g_LEPowerValue_B[i]);
    }

    for (i = 0; i < 8; i++) {
        UINT16_TO_STREAM(p, bt_configure_rf.g_BRChannelpwrvalue_B[i]);
    }

    for (i = 0; i < 8; i++) {
        UINT16_TO_STREAM(p, bt_configure_rf.g_EDRChannelpwrvalue_B[i]);
    }

    for (i = 0; i < 8; i++) {
        UINT16_TO_STREAM(p, bt_configure_rf.g_LEChannelpwrvalue_B[i]);
    }

    UINT16_TO_STREAM(p, bt_configure_rf.LE_fix_powerword);

    UINT8_TO_STREAM(p, bt_configure_rf.Classic_pc_by_channel);
    UINT8_TO_STREAM(p, bt_configure_rf.LE_pc_by_channel);
    UINT8_TO_STREAM(p, bt_configure_rf.RF_switch_mode);
    UINT8_TO_STREAM(p, bt_configure_rf.Data_Capture_Mode);
    UINT8_TO_STREAM(p, bt_configure_rf.Analog_IQ_Debug_Mode);
    UINT8_TO_STREAM(p, bt_configure_rf.RF_common_rfu_b3);

    for (i = 0; i < 5; i++) {
        UINT32_TO_STREAM(p, bt_configure_rf.RF_common_rfu_w[i]);
    }

    size = p - msg_req;
    memcpy(buf, msg_req, size);
    return size;
}


int get_pskey_buf(void *buf)
{
    uint8_t *p, msg_req[HCI_CMD_MAX_LEN];
    int i, size;

    p = msg_req;
    UINT32_TO_STREAM(p, bt_configure_pskey.device_class);

    for (i = 0; i < 16; i++) {
        UINT8_TO_STREAM(p, bt_configure_pskey.feature_set[i]);
    }

    for (i = 0; i < 6; i++) {
        UINT8_TO_STREAM(p, bt_configure_pskey.device_addr[i]);
    }

    UINT16_TO_STREAM(p, bt_configure_pskey.comp_id);
    UINT8_TO_STREAM(p, bt_configure_pskey.g_sys_uart0_communication_supported);
    UINT8_TO_STREAM(p, bt_configure_pskey.cp2_log_mode);
    UINT8_TO_STREAM(p, bt_configure_pskey.LogLevel);
    UINT8_TO_STREAM(p, bt_configure_pskey.g_central_or_perpheral);

    UINT16_TO_STREAM(p, bt_configure_pskey.Log_BitMask);
    UINT8_TO_STREAM(p, bt_configure_pskey.super_ssp_enable);
    UINT8_TO_STREAM(p, bt_configure_pskey.common_rfu_b3);

    for (i = 0; i < 2; i++) {
        UINT32_TO_STREAM(p, bt_configure_pskey.common_rfu_w[i]);
    }

    for (i = 0; i < 2; i++) {
        UINT32_TO_STREAM(p, bt_configure_pskey.le_rfu_w[i]);
    }

    for (i = 0; i < 2; i++) {
        UINT32_TO_STREAM(p, bt_configure_pskey.lmp_rfu_w[i]);
    }

    for (i = 0; i < 2; i++) {
        UINT32_TO_STREAM(p, bt_configure_pskey.lc_rfu_w[i]);
    }

    UINT16_TO_STREAM(p, bt_configure_pskey.g_wbs_nv_117);
    UINT16_TO_STREAM(p, bt_configure_pskey.g_wbs_nv_118);
    UINT16_TO_STREAM(p, bt_configure_pskey.g_nbv_nv_117);
    UINT16_TO_STREAM(p, bt_configure_pskey.g_nbv_nv_118);

    UINT8_TO_STREAM(p, bt_configure_pskey.g_sys_sco_transmit_mode);
    UINT8_TO_STREAM(p, bt_configure_pskey.audio_rfu_b1);
    UINT8_TO_STREAM(p, bt_configure_pskey.audio_rfu_b2);
    UINT8_TO_STREAM(p, bt_configure_pskey.audio_rfu_b3);

    for (i = 0; i < 2; i++) {
        UINT32_TO_STREAM(p, bt_configure_pskey.audio_rfu_w[i]);
    }

    UINT8_TO_STREAM(p, bt_configure_pskey.g_sys_sleep_in_standby_supported);
    UINT8_TO_STREAM(p, bt_configure_pskey.g_sys_sleep_master_supported);
    UINT8_TO_STREAM(p, bt_configure_pskey.g_sys_sleep_slave_supported);
    UINT8_TO_STREAM(p, bt_configure_pskey.power_rfu_b1);

    for (i = 0; i < 2; i++) {
        UINT32_TO_STREAM(p, bt_configure_pskey.power_rfu_w[i]);
    }

    UINT32_TO_STREAM(p, bt_configure_pskey.win_ext);

    UINT8_TO_STREAM(p, bt_configure_pskey.edr_tx_edr_delay);
    UINT8_TO_STREAM(p, bt_configure_pskey.edr_rx_edr_delay);
    UINT8_TO_STREAM(p, bt_configure_pskey.tx_delay);
    UINT8_TO_STREAM(p, bt_configure_pskey.rx_delay);

    for (i = 0; i < 2; i++) {
        UINT32_TO_STREAM(p, bt_configure_pskey.bb_rfu_w[i]);
    }

    UINT8_TO_STREAM(p, bt_configure_pskey.agc_mode);
    UINT8_TO_STREAM(p, bt_configure_pskey.diff_or_eq);
    UINT8_TO_STREAM(p, bt_configure_pskey.ramp_mode);
    UINT8_TO_STREAM(p, bt_configure_pskey.modem_rfu_b1);

    for (i = 0; i < 2; i++) {
        UINT32_TO_STREAM(p, bt_configure_pskey.modem_rfu_w[i]);
    }

    UINT32_TO_STREAM(p, bt_configure_pskey.BQB_BitMask_1);
    UINT32_TO_STREAM(p, bt_configure_pskey.BQB_BitMask_2);

    for (i = 0; i < 8; i++) {
        UINT16_TO_STREAM(p, bt_configure_pskey.bt_coex_threshold[i]);
    }

    for (i = 0; i < 2; i++) {
        UINT32_TO_STREAM(p, bt_configure_pskey.other_rfu_w[i]);
    }

    size = p - msg_req;
    memcpy(buf, msg_req, size);
    return size;
}

void set_mac_address(uint8_t *addr)
{
    uint8_t default_addr[6] = {0x00, 0x00, 0x00, 0xDA, 0x45, 0x40};
    u32_t random;

    if(0 == memcmp(bt_info.address, default_addr,sizeof(default_addr))) {
        random =sys_rand32_get();
        memcpy(default_addr, &random, 3);
        memcpy(addr, default_addr, sizeof(default_addr));
    } else {
        memcpy(addr, bt_info.address, sizeof(bt_info.address));
    }
}

void get_mac_address(char *addr)
{
    memcpy(addr, bt_configure_pskey.device_addr, sizeof(bt_configure_pskey.device_addr));
}

int marlin3_init(void) {
    int ret;

    BTD("%s", __func__);
    set_mac_address(bt_configure_pskey.device_addr);

    ret = uwp_mcu_init();
    if (ret) {
        BTD("%s firmware download failed", __func__);
    }

    return ret;
}

void uwp5661_vendor_init(void)
{
    struct net_buf *rsp, *buf;
    int size;
    char data[256] = {0};
    marlin3_init();

    BTD("send pskey");
    size = get_pskey_buf(data);
    buf = bt_hci_cmd_create(BT_HCI_OP_PSKEY, size);
    net_buf_add_mem(buf, data, size);
    bt_hci_cmd_send_sync(BT_HCI_OP_PSKEY, buf, &rsp);
    net_buf_unref(rsp);

    BTD("send rfkey");
    size = marlin3_rf_preload(data);
    buf = bt_hci_cmd_create(BT_HCI_OP_RF, size);
    net_buf_add_mem(buf, data, size);
    bt_hci_cmd_send_sync(BT_HCI_OP_RF, buf, &rsp);
    net_buf_unref(rsp);

    BTD("send enable");
    size = get_enable_buf(data);
    buf = bt_hci_cmd_create(BT_HCI_OP_ENABLE_CMD, size);
    net_buf_add_mem(buf, data, size);
    bt_hci_cmd_send_sync(BT_HCI_OP_ENABLE_CMD, buf, &rsp);
    net_buf_unref(rsp);

}

