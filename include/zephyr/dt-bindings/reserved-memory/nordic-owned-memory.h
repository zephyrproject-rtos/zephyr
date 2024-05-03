/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_RESERVED_MEMORY_NORDIC_OWNED_MEMORY_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_RESERVED_MEMORY_NORDIC_OWNED_MEMORY_H_

#include <zephyr/dt-bindings/dt-util.h>

/**
 * @name Basic memory permission flags.
 * @{
 */

/** Readable. */
#define NRF_PERM_R BIT(0)
/** Writable. */
#define NRF_PERM_W BIT(1)
/** Executable. */
#define NRF_PERM_X BIT(2)
/** Secure-only. */
#define NRF_PERM_S BIT(3)
/** Non-secure-callable. */
#define NRF_PERM_NSC BIT(4)

/**
 * @}
 */

/**
 * @name Memory permission flag combinations.
 * @note NRF_PERM_NSC overrides all other flags, so it is not included here.
 * @{
 */

#define NRF_PERM_RW   (NRF_PERM_R | NRF_PERM_W)
#define NRF_PERM_RX   (NRF_PERM_R | NRF_PERM_X)
#define NRF_PERM_RS   (NRF_PERM_R | NRF_PERM_S)
#define NRF_PERM_WX   (NRF_PERM_W | NRF_PERM_X)
#define NRF_PERM_WS   (NRF_PERM_W | NRF_PERM_S)
#define NRF_PERM_XS   (NRF_PERM_X | NRF_PERM_S)
#define NRF_PERM_RWX  (NRF_PERM_R | NRF_PERM_W | NRF_PERM_X)
#define NRF_PERM_RWS  (NRF_PERM_R | NRF_PERM_W | NRF_PERM_S)
#define NRF_PERM_RXS  (NRF_PERM_R | NRF_PERM_X | NRF_PERM_S)
#define NRF_PERM_WXS  (NRF_PERM_W | NRF_PERM_X | NRF_PERM_S)
#define NRF_PERM_RWXS (NRF_PERM_R | NRF_PERM_W | NRF_PERM_X | NRF_PERM_S)

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_RESERVED_MEMORY_NORDIC_OWNED_MEMORY_H_ */
