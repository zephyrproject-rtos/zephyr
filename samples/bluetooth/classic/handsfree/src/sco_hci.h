/*
 * Copyright (c) 2025 SiFli Technologies(Nanjing) Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SCO_HCI_H_
#define SCO_HCI_H_

#include <stdint.h>

#include <zephyr/bluetooth/conn.h>

int sco_hci_init(uint8_t air_mode);
int sco_hci_start(struct bt_conn *sco_conn);
int sco_hci_stop(void);

#endif /* SCO_HCI_H_ */
