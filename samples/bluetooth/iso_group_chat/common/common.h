/* Copyright (c) 2025 Tailored Technology Ltd
 * Copyright (c) 2021-2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SAMPLES_BLUETOOTH_ISO_GROUPCHAT_COMMON_COMMON_H_
#define ZEPHYR_SAMPLES_BLUETOOTH_ISO_GROUPCHAT_COMMON_COMMON_H_

#include <stddef.h>
#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/iso.h>

#ifdef __cplusplus
extern "C" {
#endif

/* If the size of the payload struct is increased, then increase the value
   of the following configs by **at least** the value it has increased by
   CONFIG_BT_ISO_TX_MTU  (in file proj.conf)
   CONFIG_BT_CTLR_ISO_TX_BUFFER_SIZE (in file overlay-bt_ll_sw_split.conf)
   CONFIG_BT_CTLR_ADV_ISO_PDU_LEN_MAX (in file overlay-bt_ll_sw_split.conf)
   */
struct app_bis_payload {
   uint32_t     src_send_count;
   uint8_t 	    src_bis_index;  /* 1..N */
   uint8_t      src_type_id;  /* 1=Primary,2=Mixer, 10..99=Secondary */
   uint8_t 	    padding[2];
};

/* number of BISes in the BIG */
#define BIS_ISO_CHAN_COUNT       (2)


/**
 * @brief Given a pointer to a BIS struct, return the BIS index
 *
 * @param chan    Pointer to the BIS structure.
 *
 * @return 0 in case the BIS index is not in the range 1 to Max Number of BIS allowed .
 */
uint8_t get_bis_idx(struct bt_iso_chan *chan);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SAMPLES_BLUETOOTH_ISO_GROUPCHAT_COMMON_COMMON_H_ */
