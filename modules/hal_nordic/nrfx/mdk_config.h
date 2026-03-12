/*
 * Copyright (c) 2026, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MDK_CONFIG_H__
#define MDK_CONFIG_H__

/*
 * These are mappings of Kconfig options enabling MDK-based anomaly workarounds
   to the corresponding symbols used inside of nrfx-based MDK.
 */

#ifdef CONFIG_NRF91_ANOMALY_36_WORKAROUND
#define NRF91_ERRATA_36_ENABLE_WORKAROUND 1
#endif

#endif /* MDK_CONFIG_H__ */
