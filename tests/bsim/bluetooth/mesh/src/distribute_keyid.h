/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TESTS_BSIM_BLUETOOTH_MESH_SRC_DISTRIBUTE_KEYID_H_
#define TESTS_BSIM_BLUETOOTH_MESH_SRC_DISTRIBUTE_KEYID_H_

#if defined CONFIG_BT_MESH_USES_MBEDTLS_PSA
void stored_keys_clear(void);
#else
static inline void stored_keys_clear(void)
{}
#endif

#endif /* TESTS_BSIM_BLUETOOTH_MESH_SRC_DISTRIBUTE_KEYID_H_ */
