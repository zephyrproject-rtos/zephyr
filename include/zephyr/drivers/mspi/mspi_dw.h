/*
 * Copyright (c) 2025 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MSPI_DW_H_
#define ZEPHYR_INCLUDE_DRIVERS_MSPI_DW_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Configure RX_SAMPLE_DLY register for MSPI DW SSI */
#define MSPI_DW_RX_TIMING_CFG BIT(0)

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MSPI_DW_H_ */
