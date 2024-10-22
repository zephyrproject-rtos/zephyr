/*
 * Copyright (c) 2024, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NRFX_RESERVED_RESOURCES_NCS_H__
#define NRFX_RESERVED_RESOURCES_NCS_H__

#if defined(CONFIG_BT_LL_SOFTDEVICE)
#include <sdc_soc.h>
#if defined(CONFIG_SOC_COMPATIBLE_NRF52X)
#define NRFX_PPI_CHANNELS_USED_BY_BT_CTLR SDC_PPI_CHANNELS_USED_MASK
#elif defined(CONFIG_SOC_COMPATIBLE_NRF53X)
#define NRFX_DPPI0_CHANNELS_USED_BY_BT_CTLR SDC_DPPI_CHANNELS_USED_MASK
#elif defined(CONFIG_SOC_COMPATIBLE_NRF54LX) || defined(CONFIG_SOC_SERIES_NRF71)
#define NRFX_DPPI10_CHANNELS_USED_BY_BT_CTLR SDC_DPPIC10_CHANNELS_USED_MASK
#define NRFX_DPPI00_CHANNELS_USED_BY_BT_CTLR SDC_DPPIC00_CHANNELS_USED_MASK
#define NRFX_PPIB_00_10_CHANNELS_USED_BY_BT_CTLR                                                   \
	(SDC_PPIB00_CHANNELS_USED_MASK | SDC_PPIB10_CHANNELS_USED_MASK)
#elif defined(CONFIG_SOC_SERIES_NRF54H)
#define NRFX_DPPI020_CHANNELS_USED_BY_BT_CTLR SDC_DPPIC020_CHANNELS_USED_MASK
#define NRFX_DPPI030_CHANNELS_USED_BY_BT_CTLR SDC_DPPIC030_CHANNELS_USED_MASK
#define NRFX_PPIB_020_030_CHANNELS_USED_BY_BT_CTLR                                                 \
	(SDC_PPIB020_CHANNELS_USED_MASK | SDC_PPIB030_CHANNELS_USED_MASK)
#else
#error Unsupported chip family
#endif
#endif /* defined(CONFIG_BT_LL_SOFTDEVICE) */

#if defined(CONFIG_MPSL)
#include <mpsl_hwres.h>
#if defined(CONFIG_SOC_COMPATIBLE_NRF52X)
#define NRFX_PPI_CHANNELS_USED_BY_MPSL MPSL_PPI_CHANNELS_USED_MASK
#elif defined(CONFIG_SOC_COMPATIBLE_NRF53X)
#define NRFX_DPPI0_CHANNELS_USED_BY_MPSL MPSL_DPPIC_CHANNELS_USED_MASK
#elif defined(CONFIG_SOC_COMPATIBLE_NRF54LX) || defined(CONFIG_SOC_SERIES_NRF71)
#define NRFX_DPPI10_CHANNELS_USED_BY_MPSL MPSL_DPPIC10_CHANNELS_USED_MASK
#define NRFX_DPPI20_CHANNELS_USED_BY_MPSL MPSL_DPPIC20_CHANNELS_USED_MASK
#define NRFX_PPIB_11_21_CHANNELS_USED_BY_MPSL                                                      \
	(MPSL_PPIB11_CHANNELS_USED_MASK | MPSL_PPIB21_CHANNELS_USED_MASK)
#elif defined(CONFIG_SOC_SERIES_NRF54H)
#define NRFX_DPPI020_CHANNELS_USED_BY_MPSL MPSL_DPPIC020_CHANNELS_USED_MASK
#else
#error Unsupported chip family
#endif
#endif

#include "nrfx_reserved_resources.h"

#if defined(NRF_802154_VERIFY_PERIPHS_ALLOC_AGAINST_MPSL)

BUILD_ASSERT((NRFX_DPPI0_CHANNELS_USED_BY_802154_DRV & NRFX_DPPI0_CHANNELS_USED_BY_MPSL) == 0,
	     "PPI channels used by the IEEE802.15.4 radio driver overlap with those "
	     "assigned to the MPSL.");

BUILD_ASSERT((NRFX_DPPI0_GROUPS_USED_BY_802154_DRV & NRFX_DPPI0_GROUPS_USED_BY_MPSL) == 0,
	     "PPI groups used by the IEEE802.15.4 radio driver overlap with those "
	     "assigned to the MPSL.");

BUILD_ASSERT((NRFX_DPPI00_CHANNELS_USED_BY_802154_DRV & NRFX_DPPI00_CHANNELS_USED_BY_MPSL) == 0,
	     "PPI channels used by the IEEE802.15.4 radio driver overlap with those "
	     "assigned to the MPSL.");

BUILD_ASSERT((NRFX_DPPI00_GROUPS_USED_BY_802154_DRV & NRFX_DPPI00_GROUPS_USED_BY_MPSL) == 0,
	     "PPI groups used by the IEEE802.15.4 radio driver overlap with those "
	     "assigned to the MPSL.");

BUILD_ASSERT((NRFX_DPPI10_CHANNELS_USED_BY_802154_DRV & NRFX_DPPI10_CHANNELS_USED_BY_MPSL) == 0,
	     "PPI channels used by the IEEE802.15.4 radio driver overlap with those "
	     "assigned to the MPSL.");

BUILD_ASSERT((NRFX_DPPI10_GROUPS_USED_BY_802154_DRV & NRFX_DPPI10_GROUPS_USED_BY_MPSL) == 0,
	     "PPI groups used by the IEEE802.15.4 radio driver overlap with those "
	     "assigned to the MPSL.");

BUILD_ASSERT((NRFX_DPPI20_CHANNELS_USED_BY_802154_DRV & NRFX_DPPI20_CHANNELS_USED_BY_MPSL) == 0,
	     "PPI channels used by the IEEE802.15.4 radio driver overlap with those "
	     "assigned to the MPSL.");

BUILD_ASSERT((NRFX_DPPI20_GROUPS_USED_BY_802154_DRV & NRFX_DPPI20_GROUPS_USED_BY_MPSL) == 0,
	     "PPI groups used by the IEEE802.15.4 radio driver overlap with those "
	     "assigned to the MPSL.");

BUILD_ASSERT((NRFX_DPPI30_CHANNELS_USED_BY_802154_DRV & NRFX_DPPI30_CHANNELS_USED_BY_MPSL) == 0,
	     "PPI channels used by the IEEE802.15.4 radio driver overlap with those "
	     "assigned to the MPSL.");

BUILD_ASSERT((NRFX_DPPI30_GROUPS_USED_BY_802154_DRV & NRFX_DPPI30_GROUPS_USED_BY_MPSL) == 0,
	     "PPI groups used by the IEEE802.15.4 radio driver overlap with those "
	     "assigned to the MPSL.");

BUILD_ASSERT((NRFX_DPPI020_CHANNELS_USED_BY_802154_DRV & NRFX_DPPI020_CHANNELS_USED_BY_MPSL) == 0,
	     "PPI channels used by the IEEE802.15.4 radio driver overlap with those "
	     "assigned to the MPSL.");

BUILD_ASSERT((NRFX_DPPI020_GROUPS_USED_BY_802154_DRV & NRFX_DPPI020_GROUPS_USED_BY_MPSL) == 0,
	     "PPI groups used by the IEEE802.15.4 radio driver overlap with those "
	     "assigned to the MPSL.");

BUILD_ASSERT((NRFX_DPPI030_CHANNELS_USED_BY_802154_DRV & NRFX_DPPI030_CHANNELS_USED_BY_MPSL) == 0,
	     "PPI channels used by the IEEE802.15.4 radio driver overlap with those "
	     "assigned to the MPSL.");

BUILD_ASSERT((NRFX_DPPI030_GROUPS_USED_BY_802154_DRV & NRFX_DPPI030_GROUPS_USED_BY_MPSL) == 0,
	     "PPI groups used by the IEEE802.15.4 radio driver overlap with those "
	     "assigned to the MPSL.");

BUILD_ASSERT((NRFX_DPPI120_CHANNELS_USED_BY_802154_DRV & NRFX_DPPI120_CHANNELS_USED_BY_MPSL) == 0,
	     "PPI channels used by the IEEE802.15.4 radio driver overlap with those "
	     "assigned to the MPSL.");

BUILD_ASSERT((NRFX_DPPI120_GROUPS_USED_BY_802154_DRV & NRFX_DPPI120_GROUPS_USED_BY_MPSL) == 0,
	     "PPI groups used by the IEEE802.15.4 radio driver overlap with those "
	     "assigned to the MPSL.");

BUILD_ASSERT((NRFX_DPPI130_CHANNELS_USED_BY_802154_DRV & NRFX_DPPI130_CHANNELS_USED_BY_MPSL) == 0,
	     "PPI channels used by the IEEE802.15.4 radio driver overlap with those "
	     "assigned to the MPSL.");

BUILD_ASSERT((NRFX_DPPI130_GROUPS_USED_BY_802154_DRV & NRFX_DPPI130_GROUPS_USED_BY_MPSL) == 0,
	     "PPI groups used by the IEEE802.15.4 radio driver overlap with those "
	     "assigned to the MPSL.");

BUILD_ASSERT((NRFX_DPPI131_CHANNELS_USED_BY_802154_DRV & NRFX_DPPI131_CHANNELS_USED_BY_MPSL) == 0,
	     "PPI channels used by the IEEE802.15.4 radio driver overlap with those "
	     "assigned to the MPSL.");

BUILD_ASSERT((NRFX_DPPI131_GROUPS_USED_BY_802154_DRV & NRFX_DPPI131_GROUPS_USED_BY_MPSL) == 0,
	     "PPI groups used by the IEEE802.15.4 radio driver overlap with those "
	     "assigned to the MPSL.");

BUILD_ASSERT((NRFX_DPPI132_CHANNELS_USED_BY_802154_DRV & NRFX_DPPI132_CHANNELS_USED_BY_MPSL) == 0,
	     "PPI channels used by the IEEE802.15.4 radio driver overlap with those "
	     "assigned to the MPSL.");

BUILD_ASSERT((NRFX_DPPI132_GROUPS_USED_BY_802154_DRV & NRFX_DPPI132_GROUPS_USED_BY_MPSL) == 0,
	     "PPI groups used by the IEEE802.15.4 radio driver overlap with those "
	     "assigned to the MPSL.");

BUILD_ASSERT((NRFX_DPPI133_CHANNELS_USED_BY_802154_DRV & NRFX_DPPI133_CHANNELS_USED_BY_MPSL) == 0,
	     "PPI channels used by the IEEE802.15.4 radio driver overlap with those "
	     "assigned to the MPSL.");

BUILD_ASSERT((NRFX_DPPI133_GROUPS_USED_BY_802154_DRV & NRFX_DPPI133_GROUPS_USED_BY_MPSL) == 0,
	     "PPI groups used by the IEEE802.15.4 radio driver overlap with those "
	     "assigned to the MPSL.");

BUILD_ASSERT((NRFX_DPPI134_CHANNELS_USED_BY_802154_DRV & NRFX_DPPI134_CHANNELS_USED_BY_MPSL) == 0,
	     "PPI channels used by the IEEE802.15.4 radio driver overlap with those "
	     "assigned to the MPSL.");

BUILD_ASSERT((NRFX_DPPI134_GROUPS_USED_BY_802154_DRV & NRFX_DPPI134_GROUPS_USED_BY_MPSL) == 0,
	     "PPI groups used by the IEEE802.15.4 radio driver overlap with those "
	     "assigned to the MPSL.");

BUILD_ASSERT((NRFX_DPPI135_CHANNELS_USED_BY_802154_DRV & NRFX_DPPI135_CHANNELS_USED_BY_MPSL) == 0,
	     "PPI channels used by the IEEE802.15.4 radio driver overlap with those "
	     "assigned to the MPSL.");

BUILD_ASSERT((NRFX_DPPI135_GROUPS_USED_BY_802154_DRV & NRFX_DPPI135_GROUPS_USED_BY_MPSL) == 0,
	     "PPI groups used by the IEEE802.15.4 radio driver overlap with those "
	     "assigned to the MPSL.");

BUILD_ASSERT((NRFX_DPPI136_CHANNELS_USED_BY_802154_DRV & NRFX_DPPI136_CHANNELS_USED_BY_MPSL) == 0,
	     "PPI channels used by the IEEE802.15.4 radio driver overlap with those "
	     "assigned to the MPSL.");

BUILD_ASSERT((NRFX_DPPI136_GROUPS_USED_BY_802154_DRV & NRFX_DPPI136_GROUPS_USED_BY_MPSL) == 0,
	     "PPI groups used by the IEEE802.15.4 radio driver overlap with those "
	     "assigned to the MPSL.");

BUILD_ASSERT((NRFX_PPI_CHANNELS_USED_BY_802154_DRV & NRFX_PPI_CHANNELS_USED_BY_MPSL) == 0,
	     "PPI channels used by the IEEE802.15.4 radio driver overlap with those "
	     "assigned to the MPSL.");

BUILD_ASSERT((NRFX_PPI_GROUPS_USED_BY_802154_DRV & NRFX_PPI_GROUPS_USED_BY_MPSL) == 0,
	     "PPI groups used by the IEEE802.15.4 radio driver overlap with those "
	     "assigned to the MPSL.");

BUILD_ASSERT((NRFX_PPIB_00_10_CHANNELS_USED_BY_802154_DRV &
	      NRFX_PPIB_00_10_CHANNELS_USED_BY_MPSL) == 0,
	     "PPI channels used by the IEEE802.15.4 radio driver overlap with those "
	     "assigned to the MPSL.");

BUILD_ASSERT((NRFX_PPIB_01_20_CHANNELS_USED_BY_802154_DRV &
	      NRFX_PPIB_01_20_CHANNELS_USED_BY_MPSL) == 0,
	     "PPI channels used by the IEEE802.15.4 radio driver overlap with those "
	     "assigned to the MPSL.");

BUILD_ASSERT((NRFX_PPIB_11_21_CHANNELS_USED_BY_802154_DRV &
	      NRFX_PPIB_11_21_CHANNELS_USED_BY_MPSL) == 0,
	     "PPI channels used by the IEEE802.15.4 radio driver overlap with those "
	     "assigned to the MPSL.");

BUILD_ASSERT((NRFX_PPIB_22_30_CHANNELS_USED_BY_802154_DRV &
	      NRFX_PPIB_22_30_CHANNELS_USED_BY_MPSL) == 0,
	     "PPI channels used by the IEEE802.15.4 radio driver overlap with those "
	     "assigned to the MPSL.");

BUILD_ASSERT((NRFX_PPIB_02_03_CHANNELS_USED_BY_802154_DRV &
	      NRFX_PPIB_02_03_CHANNELS_USED_BY_MPSL) == 0,
	     "PPI channels used by the IEEE802.15.4 radio driver overlap with those "
	     "assigned to the MPSL.");

BUILD_ASSERT((NRFX_PPIB_04_12_CHANNELS_USED_BY_802154_DRV &
	      NRFX_PPIB_04_12_CHANNELS_USED_BY_MPSL) == 0,
	     "PPI channels used by the IEEE802.15.4 radio driver overlap with those "
	     "assigned to the MPSL.");

BUILD_ASSERT((NRFX_PPIB_020_030_CHANNELS_USED_BY_802154_DRV &
	      NRFX_PPIB_020_030_CHANNELS_USED_BY_MPSL) == 0,
	     "PPI channels used by the IEEE802.15.4 radio driver overlap with those "
	     "assigned to the MPSL.");

#endif /* NRF_802154_VERIFY_PERIPHS_ALLOC_AGAINST_MPSL */
#endif /* NRFX_RESERVED_RESOURCES_NCS_H__ */
