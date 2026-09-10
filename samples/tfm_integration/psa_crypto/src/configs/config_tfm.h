/*
 * Copyright (c) 2024 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* In TF-M the default value of CRYPTO_ENGINE_BUF_SIZE is 0x2080.
 * It causes insufficient memory failure while verifying signature.
 */
#define CRYPTO_ENGINE_BUF_SIZE 0x2400
