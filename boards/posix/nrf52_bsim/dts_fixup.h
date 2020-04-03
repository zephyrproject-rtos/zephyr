/* Copyright (c) 2020 Nordic Semiconductor ASA */
/* SPDX-License-Identifier: Apache-2.0 */

/* Board level DTS fixup file */

#ifndef CONFIG_ENTROPY_NAME
#define CONFIG_ENTROPY_NAME	DT_LABEL(DT_INST(0, nordic_nrf_rng))
#endif
