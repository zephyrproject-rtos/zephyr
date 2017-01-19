/* hci_ecc.h - HCI ECC emulation */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_BLUETOOTH_TINYCRYPT_ECC)
void bt_hci_ecc_init(void);
#else
#define bt_hci_ecc_init()
#endif /* CONFIG_BLUETOOTH_TINYCRYPT_ECC */
