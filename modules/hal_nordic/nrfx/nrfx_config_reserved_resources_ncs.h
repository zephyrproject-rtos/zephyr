/*
 * Copyright (c) 2024, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NRFX_CONFIG_RESERVED_RESOURCES_H__
#define NRFX_CONFIG_RESERVED_RESOURCES_H__

/** @brief Bitmask that defines GPIOTE130 channels reserved for use outside
 *         of the nrfx library.
 */
#define NRFX_GPIOTE130_CHANNELS_USED                                                               \
	(~NRFX_CONFIG_MASK_DT(DT_NODELABEL(gpiote130), owned_channels) |                           \
	 NRFX_CONFIG_MASK_DT(DT_NODELABEL(gpiote130), child_owned_channels))

/** @brief Bitmask that defines GPIOTE131 channels reserved for use outside
 *         of the nrfx library.
 */
#define NRFX_GPIOTE131_CHANNELS_USED                                                               \
	(~NRFX_CONFIG_MASK_DT(DT_NODELABEL(gpiote131), owned_channels) |                           \
	 NRFX_CONFIG_MASK_DT(DT_NODELABEL(gpiote131), child_owned_channels))

/** @brief Bitmask that defines EGU instances that are reserved for use outside
 *         of the nrfx library.
 */
#define NRFX_EGUS_USED 0

/** @brief Bitmask that defines TIMER instances that are reserved for use outside
 *         of the nrfx library.
 */
#define NRFX_TIMERS_USED 0

/* If the GRTC system timer driver is to be used, prepare definitions required
 * by the nrfx_grtc driver (NRFX_GRTC_CONFIG_ALLOWED_CC_CHANNELS_MASK and
 * NRFX_GRTC_CONFIG_NUM_OF_CC_CHANNELS) based on information from devicetree.
 */
#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_grtc)
#define NRFX_GRTC_CONFIG_ALLOWED_CC_CHANNELS_MASK \
	(NRFX_CONFIG_MASK_DT(DT_INST(0, nordic_nrf_grtc), owned_channels) & \
	 ~NRFX_CONFIG_MASK_DT(DT_INST(0, nordic_nrf_grtc), child_owned_channels))
#define NRFX_GRTC_CONFIG_NUM_OF_CC_CHANNELS \
	(DT_PROP_LEN_OR(DT_INST(0, nordic_nrf_grtc), owned_channels, 0) - \
	 DT_PROP_LEN_OR(DT_INST(0, nordic_nrf_grtc), child_owned_channels, 0))
#endif /* DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_grtc) */

/*
 * The enabled Bluetooth controller subsystem is responsible for providing
 * definitions of the BT_CTLR_USED_* symbols used below in a file named
 * bt_ctlr_used_resources.h and for adding its location to global include
 * paths so that the file can be included here for all Zephyr libraries that
 * are to be built.
 */
#if defined(CONFIG_BT_LL_SW_SPLIT)
#include <bt_ctlr_used_resources.h>
#if defined(CONFIG_SOC_SERIES_NRF51X) || defined(CONFIG_SOC_COMPATIBLE_NRF52X)
#define NRFX_PPI_CHANNELS_USED_BY_BT_CTLR BT_CTLR_USED_PPI_CHANNELS
#define NRFX_PPI_GROUPS_USED_BY_BT_CTLR   BT_CTLR_USED_PPI_GROUPS
#elif defined(CONFIG_SOC_COMPATIBLE_NRF53X)
#define NRFX_DPPI0_CHANNELS_USED_BY_BT_CTLR BT_CTLR_USED_PPI_CHANNELS
#define NRFX_DPPI0_GROUPS_USED_BY_BT_CTLR   BT_CTLR_USED_PPI_GROUPS
#elif defined(CONFIG_SOC_COMPATIBLE_NRF54LX)
#define NRFX_DPPI10_CHANNELS_USED_BY_BT_CTLR BT_CTLR_USED_PPI_CHANNELS
#define NRFX_DPPI10_GROUPS_USED_BY_BT_CTLR   BT_CTLR_USED_PPI_GROUPS
#endif
#endif /* defined(CONFIG_BT_LL_SW_SPLIT) */

#if defined(CONFIG_BT_LL_SOFTDEVICE)
/* Define auxiliary symbols needed for SDC device dispatch. */
#if defined(CONFIG_SOC_COMPATIBLE_NRF52X)
#define NRF52_SERIES
#elif defined(CONFIG_SOC_COMPATIBLE_NRF53X)
#define NRF53_SERIES
#elif defined(CONFIG_SOC_COMPATIBLE_NRF54LX)
#define NRF54L_SERIES
#elif defined(CONFIG_SOC_SERIES_NRF54HX)
#define NRF54H_SERIES
#endif
#include <sdc_soc.h>
#if defined(CONFIG_SOC_COMPATIBLE_NRF52X)
#define NRFX_PPI_CHANNELS_USED_BY_BT_CTLR SDC_PPI_CHANNELS_USED_MASK
#elif defined(CONFIG_SOC_COMPATIBLE_NRF53X)
#define NRFX_DPPI0_CHANNELS_USED_BY_BT_CTLR SDC_DPPI_CHANNELS_USED_MASK
#elif defined(CONFIG_SOC_COMPATIBLE_NRF54LX)
#define NRFX_DPPI10_CHANNELS_USED_BY_BT_CTLR SDC_DPPIC10_CHANNELS_USED_MASK
#define NRFX_DPPI00_CHANNELS_USED_BY_BT_CTLR SDC_DPPIC00_CHANNELS_USED_MASK
#define NRFX_PPIB_00_10_CHANNELS_USED_BY_BT_CTLR                                                   \
	(SDC_PPIB00_CHANNELS_USED_MASK | SDC_PPIB10_CHANNELS_USED_MASK)
#elif defined(CONFIG_SOC_SERIES_NRF54HX)
#define NRFX_DPPI020_CHANNELS_USED_BY_BT_CTLR SDC_DPPIC020_CHANNELS_USED_MASK
#define NRFX_DPPI030_CHANNELS_USED_BY_BT_CTLR SDC_DPPIC030_CHANNELS_USED_MASK
#define NRFX_PPIB_020_030_CHANNELS_USED_BY_BT_CTLR                                                 \
	(SDC_PPIB020_CHANNELS_USED_MASK | SDC_PPIB030_CHANNELS_USED_MASK)
#else
#error Unsupported chip family
#endif
#endif /* defined(CONFIG_BT_LL_SOFTDEVICE) */

#if defined(CONFIG_NRF_802154_RADIO_DRIVER)
#if defined(CONFIG_SOC_COMPATIBLE_NRF52X)
#include <../src/nrf_802154_peripherals_nrf52.h>
#define NRFX_PPI_CHANNELS_USED_BY_802154_DRV NRF_802154_PPI_CHANNELS_USED_MASK
#define NRFX_PPI_GROUPS_USED_BY_802154_DRV   NRF_802154_PPI_GROUPS_USED_MASK
#elif defined(CONFIG_SOC_COMPATIBLE_NRF53X)
#include <../src/nrf_802154_peripherals_nrf53.h>
#define NRFX_DPPI0_CHANNELS_USED_BY_802154_DRV NRF_802154_DPPI_CHANNELS_USED_MASK
#define NRFX_DPPI0_GROUPS_USED_BY_802154_DRV   NRF_802154_DPPI_GROUPS_USED_MASK
#elif defined(CONFIG_SOC_COMPATIBLE_NRF54LX)
#include <../src/nrf_802154_peripherals_nrf54l.h>
#define NRFX_DPPI10_CHANNELS_USED_BY_802154_DRV NRF_802154_DPPI_CHANNELS_USED_MASK
#define NRFX_DPPI10_GROUPS_USED_BY_802154_DRV   NRF_802154_DPPI_GROUPS_USED_MASK
#elif defined(CONFIG_SOC_SERIES_NRF54HX)
#include <../src/nrf_802154_peripherals_nrf54h.h>
#define NRFX_DPPI020_CHANNELS_USED_BY_802154_DRV NRF_802154_DPPI_CHANNELS_USED_MASK
#define NRFX_DPPI020_GROUPS_USED_BY_802154_DRV   NRF_802154_DPPI_GROUPS_USED_MASK
#else
#error Unsupported chip family
#endif
#endif /* CONFIG_NRF_802154_RADIO_DRIVER */

#if defined(CONFIG_MPSL)
#include <mpsl.h>
#if defined(CONFIG_SOC_COMPATIBLE_NRF52X)
#define NRFX_PPI_CHANNELS_USED_BY_MPSL MPSL_PPI_CHANNELS_USED_MASK
#elif defined(CONFIG_SOC_COMPATIBLE_NRF53X)
#define NRFX_DPPI0_CHANNELS_USED_BY_MPSL MPSL_DPPIC_CHANNELS_USED_MASK
#elif defined(CONFIG_SOC_COMPATIBLE_NRF54LX)
#define NRFX_DPPI10_CHANNELS_USED_BY_MPSL MPSL_DPPIC10_CHANNELS_USED_MASK
#define NRFX_DPPI20_CHANNELS_USED_BY_MPSL MPSL_DPPIC20_CHANNELS_USED_MASK
#define NRFX_PPIB_11_21_CHANNELS_USED_BY_MPSL                                                      \
	(MPSL_PPIB11_CHANNELS_USED_MASK | MPSL_PPIB21_CHANNELS_USED_MASK)
#elif defined(CONFIG_SOC_SERIES_NRF54HX)
#define NRFX_DPPI020_CHANNELS_USED_BY_MPSL MPSL_DPPIC020_CHANNELS_USED_MASK
#else
#error Unsupported chip family
#endif
#endif

#ifndef NRFX_DPPI0_CHANNELS_USED_BY_BT_CTLR
#define NRFX_DPPI0_CHANNELS_USED_BY_BT_CTLR 0
#endif
#ifndef NRFX_DPPI0_GROUPS_USED_BY_BT_CTLR
#define NRFX_DPPI0_GROUPS_USED_BY_BT_CTLR 0
#endif
#ifndef NRFX_DPPI0_CHANNELS_USED_BY_802154_DRV
#define NRFX_DPPI0_CHANNELS_USED_BY_802154_DRV 0
#endif
#ifndef NRFX_DPPI0_GROUPS_USED_BY_802154_DRV
#define NRFX_DPPI0_GROUPS_USED_BY_802154_DRV 0
#endif
#ifndef NRFX_DPPI0_CHANNELS_USED_BY_MPSL
#define NRFX_DPPI0_CHANNELS_USED_BY_MPSL 0
#endif
#ifndef NRFX_DPPI0_GROUPS_USED_BY_MPSL
#define NRFX_DPPI0_GROUPS_USED_BY_MPSL 0
#endif

#ifndef NRFX_DPPI00_CHANNELS_USED_BY_BT_CTLR
#define NRFX_DPPI00_CHANNELS_USED_BY_BT_CTLR 0
#endif
#ifndef NRFX_DPPI00_GROUPS_USED_BY_BT_CTLR
#define NRFX_DPPI00_GROUPS_USED_BY_BT_CTLR 0
#endif
#ifndef NRFX_DPPI00_CHANNELS_USED_BY_802154_DRV
#define NRFX_DPPI00_CHANNELS_USED_BY_802154_DRV 0
#endif
#ifndef NRFX_DPPI00_GROUPS_USED_BY_802154_DRV
#define NRFX_DPPI00_GROUPS_USED_BY_802154_DRV 0
#endif
#ifndef NRFX_DPPI00_CHANNELS_USED_BY_MPSL
#define NRFX_DPPI00_CHANNELS_USED_BY_MPSL 0
#endif
#ifndef NRFX_DPPI00_GROUPS_USED_BY_MPSL
#define NRFX_DPPI00_GROUPS_USED_BY_MPSL 0
#endif

#ifndef NRFX_DPPI10_CHANNELS_USED_BY_BT_CTLR
#define NRFX_DPPI10_CHANNELS_USED_BY_BT_CTLR 0
#endif
#ifndef NRFX_DPPI10_GROUPS_USED_BY_BT_CTLR
#define NRFX_DPPI10_GROUPS_USED_BY_BT_CTLR 0
#endif
#ifndef NRFX_DPPI10_CHANNELS_USED_BY_802154_DRV
#define NRFX_DPPI10_CHANNELS_USED_BY_802154_DRV 0
#endif
#ifndef NRFX_DPPI10_GROUPS_USED_BY_802154_DRV
#define NRFX_DPPI10_GROUPS_USED_BY_802154_DRV 0
#endif
#ifndef NRFX_DPPI10_CHANNELS_USED_BY_MPSL
#define NRFX_DPPI10_CHANNELS_USED_BY_MPSL 0
#endif
#ifndef NRFX_DPPI10_GROUPS_USED_BY_MPSL
#define NRFX_DPPI10_GROUPS_USED_BY_MPSL 0
#endif

#ifndef NRFX_DPPI20_CHANNELS_USED_BY_BT_CTLR
#define NRFX_DPPI20_CHANNELS_USED_BY_BT_CTLR 0
#endif
#ifndef NRFX_DPPI20_GROUPS_USED_BY_BT_CTLR
#define NRFX_DPPI20_GROUPS_USED_BY_BT_CTLR 0
#endif
#ifndef NRFX_DPPI20_CHANNELS_USED_BY_802154_DRV
#define NRFX_DPPI20_CHANNELS_USED_BY_802154_DRV 0
#endif
#ifndef NRFX_DPPI20_GROUPS_USED_BY_802154_DRV
#define NRFX_DPPI20_GROUPS_USED_BY_802154_DRV 0
#endif
#ifndef NRFX_DPPI20_CHANNELS_USED_BY_MPSL
#define NRFX_DPPI20_CHANNELS_USED_BY_MPSL 0
#endif
#ifndef NRFX_DPPI20_GROUPS_USED_BY_MPSL
#define NRFX_DPPI20_GROUPS_USED_BY_MPSL 0
#endif

#ifndef NRFX_DPPI30_CHANNELS_USED_BY_BT_CTLR
#define NRFX_DPPI30_CHANNELS_USED_BY_BT_CTLR 0
#endif
#ifndef NRFX_DPPI30_GROUPS_USED_BY_BT_CTLR
#define NRFX_DPPI30_GROUPS_USED_BY_BT_CTLR 0
#endif
#ifndef NRFX_DPPI30_CHANNELS_USED_BY_802154_DRV
#define NRFX_DPPI30_CHANNELS_USED_BY_802154_DRV 0
#endif
#ifndef NRFX_DPPI30_GROUPS_USED_BY_802154_DRV
#define NRFX_DPPI30_GROUPS_USED_BY_802154_DRV 0
#endif
#ifndef NRFX_DPPI30_CHANNELS_USED_BY_MPSL
#define NRFX_DPPI30_CHANNELS_USED_BY_MPSL 0
#endif
#ifndef NRFX_DPPI30_GROUPS_USED_BY_MPSL
#define NRFX_DPPI30_GROUPS_USED_BY_MPSL 0
#endif

#ifndef NRFX_DPPI020_CHANNELS_USED_BY_BT_CTLR
#define NRFX_DPPI020_CHANNELS_USED_BY_BT_CTLR 0
#endif
#ifndef NRFX_DPPI020_GROUPS_USED_BY_BT_CTLR
#define NRFX_DPPI020_GROUPS_USED_BY_BT_CTLR 0
#endif
#ifndef NRFX_DPPI020_CHANNELS_USED_BY_802154_DRV
#define NRFX_DPPI020_CHANNELS_USED_BY_802154_DRV 0
#endif
#ifndef NRFX_DPPI020_GROUPS_USED_BY_802154_DRV
#define NRFX_DPPI020_GROUPS_USED_BY_802154_DRV 0
#endif
#ifndef NRFX_DPPI020_CHANNELS_USED_BY_MPSL
#define NRFX_DPPI020_CHANNELS_USED_BY_MPSL 0
#endif
#ifndef NRFX_DPPI020_GROUPS_USED_BY_MPSL
#define NRFX_DPPI020_GROUPS_USED_BY_MPSL 0
#endif

#ifndef NRFX_DPPI030_CHANNELS_USED_BY_BT_CTLR
#define NRFX_DPPI030_CHANNELS_USED_BY_BT_CTLR 0
#endif
#ifndef NRFX_DPPI030_GROUPS_USED_BY_BT_CTLR
#define NRFX_DPPI030_GROUPS_USED_BY_BT_CTLR 0
#endif
#ifndef NRFX_DPPI030_CHANNELS_USED_BY_802154_DRV
#define NRFX_DPPI030_CHANNELS_USED_BY_802154_DRV 0
#endif
#ifndef NRFX_DPPI030_GROUPS_USED_BY_802154_DRV
#define NRFX_DPPI030_GROUPS_USED_BY_802154_DRV 0
#endif
#ifndef NRFX_DPPI030_CHANNELS_USED_BY_MPSL
#define NRFX_DPPI030_CHANNELS_USED_BY_MPSL 0
#endif
#ifndef NRFX_DPPI030_GROUPS_USED_BY_MPSL
#define NRFX_DPPI030_GROUPS_USED_BY_MPSL 0
#endif

#ifndef NRFX_DPPI120_CHANNELS_USED_BY_BT_CTLR
#define NRFX_DPPI120_CHANNELS_USED_BY_BT_CTLR 0
#endif
#ifndef NRFX_DPPI120_GROUPS_USED_BY_BT_CTLR
#define NRFX_DPPI120_GROUPS_USED_BY_BT_CTLR 0
#endif
#ifndef NRFX_DPPI120_CHANNELS_USED_BY_802154_DRV
#define NRFX_DPPI120_CHANNELS_USED_BY_802154_DRV 0
#endif
#ifndef NRFX_DPPI120_GROUPS_USED_BY_802154_DRV
#define NRFX_DPPI120_GROUPS_USED_BY_802154_DRV 0
#endif
#ifndef NRFX_DPPI120_CHANNELS_USED_BY_MPSL
#define NRFX_DPPI120_CHANNELS_USED_BY_MPSL 0
#endif
#ifndef NRFX_DPPI120_GROUPS_USED_BY_MPSL
#define NRFX_DPPI120_GROUPS_USED_BY_MPSL 0
#endif

#ifndef NRFX_DPPI130_CHANNELS_USED_BY_BT_CTLR
#define NRFX_DPPI130_CHANNELS_USED_BY_BT_CTLR 0
#endif
#ifndef NRFX_DPPI130_GROUPS_USED_BY_BT_CTLR
#define NRFX_DPPI130_GROUPS_USED_BY_BT_CTLR 0
#endif
#ifndef NRFX_DPPI130_CHANNELS_USED_BY_802154_DRV
#define NRFX_DPPI130_CHANNELS_USED_BY_802154_DRV 0
#endif
#ifndef NRFX_DPPI130_GROUPS_USED_BY_802154_DRV
#define NRFX_DPPI130_GROUPS_USED_BY_802154_DRV 0
#endif
#ifndef NRFX_DPPI130_CHANNELS_USED_BY_MPSL
#define NRFX_DPPI130_CHANNELS_USED_BY_MPSL 0
#endif
#ifndef NRFX_DPPI130_GROUPS_USED_BY_MPSL
#define NRFX_DPPI130_GROUPS_USED_BY_MPSL 0
#endif

#ifndef NRFX_DPPI131_CHANNELS_USED_BY_BT_CTLR
#define NRFX_DPPI131_CHANNELS_USED_BY_BT_CTLR 0
#endif
#ifndef NRFX_DPPI131_GROUPS_USED_BY_BT_CTLR
#define NRFX_DPPI131_GROUPS_USED_BY_BT_CTLR 0
#endif
#ifndef NRFX_DPPI131_CHANNELS_USED_BY_802154_DRV
#define NRFX_DPPI131_CHANNELS_USED_BY_802154_DRV 0
#endif
#ifndef NRFX_DPPI131_GROUPS_USED_BY_802154_DRV
#define NRFX_DPPI131_GROUPS_USED_BY_802154_DRV 0
#endif
#ifndef NRFX_DPPI131_CHANNELS_USED_BY_MPSL
#define NRFX_DPPI131_CHANNELS_USED_BY_MPSL 0
#endif
#ifndef NRFX_DPPI131_GROUPS_USED_BY_MPSL
#define NRFX_DPPI131_GROUPS_USED_BY_MPSL 0
#endif

#ifndef NRFX_DPPI132_CHANNELS_USED_BY_BT_CTLR
#define NRFX_DPPI132_CHANNELS_USED_BY_BT_CTLR 0
#endif
#ifndef NRFX_DPPI132_GROUPS_USED_BY_BT_CTLR
#define NRFX_DPPI132_GROUPS_USED_BY_BT_CTLR 0
#endif
#ifndef NRFX_DPPI132_CHANNELS_USED_BY_802154_DRV
#define NRFX_DPPI132_CHANNELS_USED_BY_802154_DRV 0
#endif
#ifndef NRFX_DPPI132_GROUPS_USED_BY_802154_DRV
#define NRFX_DPPI132_GROUPS_USED_BY_802154_DRV 0
#endif
#ifndef NRFX_DPPI132_CHANNELS_USED_BY_MPSL
#define NRFX_DPPI132_CHANNELS_USED_BY_MPSL 0
#endif
#ifndef NRFX_DPPI132_GROUPS_USED_BY_MPSL
#define NRFX_DPPI132_GROUPS_USED_BY_MPSL 0
#endif

#ifndef NRFX_DPPI133_CHANNELS_USED_BY_BT_CTLR
#define NRFX_DPPI133_CHANNELS_USED_BY_BT_CTLR 0
#endif
#ifndef NRFX_DPPI133_GROUPS_USED_BY_BT_CTLR
#define NRFX_DPPI133_GROUPS_USED_BY_BT_CTLR 0
#endif
#ifndef NRFX_DPPI133_CHANNELS_USED_BY_802154_DRV
#define NRFX_DPPI133_CHANNELS_USED_BY_802154_DRV 0
#endif
#ifndef NRFX_DPPI133_GROUPS_USED_BY_802154_DRV
#define NRFX_DPPI133_GROUPS_USED_BY_802154_DRV 0
#endif
#ifndef NRFX_DPPI133_CHANNELS_USED_BY_MPSL
#define NRFX_DPPI133_CHANNELS_USED_BY_MPSL 0
#endif
#ifndef NRFX_DPPI133_GROUPS_USED_BY_MPSL
#define NRFX_DPPI133_GROUPS_USED_BY_MPSL 0
#endif

#ifndef NRFX_DPPI134_CHANNELS_USED_BY_BT_CTLR
#define NRFX_DPPI134_CHANNELS_USED_BY_BT_CTLR 0
#endif
#ifndef NRFX_DPPI134_GROUPS_USED_BY_BT_CTLR
#define NRFX_DPPI134_GROUPS_USED_BY_BT_CTLR 0
#endif
#ifndef NRFX_DPPI134_CHANNELS_USED_BY_802154_DRV
#define NRFX_DPPI134_CHANNELS_USED_BY_802154_DRV 0
#endif
#ifndef NRFX_DPPI134_GROUPS_USED_BY_802154_DRV
#define NRFX_DPPI134_GROUPS_USED_BY_802154_DRV 0
#endif
#ifndef NRFX_DPPI134_CHANNELS_USED_BY_MPSL
#define NRFX_DPPI134_CHANNELS_USED_BY_MPSL 0
#endif
#ifndef NRFX_DPPI134_GROUPS_USED_BY_MPSL
#define NRFX_DPPI134_GROUPS_USED_BY_MPSL 0
#endif

#ifndef NRFX_DPPI135_CHANNELS_USED_BY_BT_CTLR
#define NRFX_DPPI135_CHANNELS_USED_BY_BT_CTLR 0
#endif
#ifndef NRFX_DPPI135_GROUPS_USED_BY_BT_CTLR
#define NRFX_DPPI135_GROUPS_USED_BY_BT_CTLR 0
#endif
#ifndef NRFX_DPPI135_CHANNELS_USED_BY_802154_DRV
#define NRFX_DPPI135_CHANNELS_USED_BY_802154_DRV 0
#endif
#ifndef NRFX_DPPI135_GROUPS_USED_BY_802154_DRV
#define NRFX_DPPI135_GROUPS_USED_BY_802154_DRV 0
#endif
#ifndef NRFX_DPPI135_CHANNELS_USED_BY_MPSL
#define NRFX_DPPI135_CHANNELS_USED_BY_MPSL 0
#endif
#ifndef NRFX_DPPI135_GROUPS_USED_BY_MPSL
#define NRFX_DPPI135_GROUPS_USED_BY_MPSL 0
#endif

#ifndef NRFX_DPPI136_CHANNELS_USED_BY_BT_CTLR
#define NRFX_DPPI136_CHANNELS_USED_BY_BT_CTLR 0
#endif
#ifndef NRFX_DPPI136_GROUPS_USED_BY_BT_CTLR
#define NRFX_DPPI136_GROUPS_USED_BY_BT_CTLR 0
#endif
#ifndef NRFX_DPPI136_CHANNELS_USED_BY_802154_DRV
#define NRFX_DPPI136_CHANNELS_USED_BY_802154_DRV 0
#endif
#ifndef NRFX_DPPI136_GROUPS_USED_BY_802154_DRV
#define NRFX_DPPI136_GROUPS_USED_BY_802154_DRV 0
#endif
#ifndef NRFX_DPPI136_CHANNELS_USED_BY_MPSL
#define NRFX_DPPI136_CHANNELS_USED_BY_MPSL 0
#endif
#ifndef NRFX_DPPI136_GROUPS_USED_BY_MPSL
#define NRFX_DPPI136_GROUPS_USED_BY_MPSL 0
#endif

#ifndef NRFX_PPI_CHANNELS_USED_BY_BT_CTLR
#define NRFX_PPI_CHANNELS_USED_BY_BT_CTLR 0
#endif
#ifndef NRFX_PPI_GROUPS_USED_BY_BT_CTLR
#define NRFX_PPI_GROUPS_USED_BY_BT_CTLR 0
#endif

#ifndef NRFX_PPI_CHANNELS_USED_BY_802154_DRV
#define NRFX_PPI_CHANNELS_USED_BY_802154_DRV 0
#endif
#ifndef NRFX_PPI_GROUPS_USED_BY_802154_DRV
#define NRFX_PPI_GROUPS_USED_BY_802154_DRV 0
#endif

#ifndef NRFX_PPI_CHANNELS_USED_BY_MPSL
#define NRFX_PPI_CHANNELS_USED_BY_MPSL 0
#endif
#ifndef NRFX_PPI_GROUPS_USED_BY_MPSL
#define NRFX_PPI_GROUPS_USED_BY_MPSL 0
#endif

#ifndef NRFX_PPIB_00_10_CHANNELS_USED_BY_BT_CTLR
#define NRFX_PPIB_00_10_CHANNELS_USED_BY_BT_CTLR 0
#endif
#ifndef NRFX_PPIB_00_10_CHANNELS_USED_BY_802154_DRV
#define NRFX_PPIB_00_10_CHANNELS_USED_BY_802154_DRV 0
#endif
#ifndef NRFX_PPIB_00_10_CHANNELS_USED_BY_MPSL
#define NRFX_PPIB_00_10_CHANNELS_USED_BY_MPSL 0
#endif

#ifndef NRFX_PPIB_01_20_CHANNELS_USED_BY_BT_CTLR
#define NRFX_PPIB_01_20_CHANNELS_USED_BY_BT_CTLR 0
#endif
#ifndef NRFX_PPIB_01_20_CHANNELS_USED_BY_802154_DRV
#define NRFX_PPIB_01_20_CHANNELS_USED_BY_802154_DRV 0
#endif
#ifndef NRFX_PPIB_01_20_CHANNELS_USED_BY_MPSL
#define NRFX_PPIB_01_20_CHANNELS_USED_BY_MPSL 0
#endif

#ifndef NRFX_PPIB_11_21_CHANNELS_USED_BY_BT_CTLR
#define NRFX_PPIB_11_21_CHANNELS_USED_BY_BT_CTLR 0
#endif
#ifndef NRFX_PPIB_11_21_CHANNELS_USED_BY_802154_DRV
#define NRFX_PPIB_11_21_CHANNELS_USED_BY_802154_DRV 0
#endif
#ifndef NRFX_PPIB_11_21_CHANNELS_USED_BY_MPSL
#define NRFX_PPIB_11_21_CHANNELS_USED_BY_MPSL 0
#endif

#ifndef NRFX_PPIB_22_30_CHANNELS_USED_BY_BT_CTLR
#define NRFX_PPIB_22_30_CHANNELS_USED_BY_BT_CTLR 0
#endif
#ifndef NRFX_PPIB_22_30_CHANNELS_USED_BY_802154_DRV
#define NRFX_PPIB_22_30_CHANNELS_USED_BY_802154_DRV 0
#endif
#ifndef NRFX_PPIB_22_30_CHANNELS_USED_BY_MPSL
#define NRFX_PPIB_22_30_CHANNELS_USED_BY_MPSL 0
#endif

#ifndef NRFX_PPIB_02_03_CHANNELS_USED_BY_BT_CTLR
#define NRFX_PPIB_02_03_CHANNELS_USED_BY_BT_CTLR 0
#endif
#ifndef NRFX_PPIB_02_03_CHANNELS_USED_BY_802154_DRV
#define NRFX_PPIB_02_03_CHANNELS_USED_BY_802154_DRV 0
#endif
#ifndef NRFX_PPIB_02_03_CHANNELS_USED_BY_MPSL
#define NRFX_PPIB_02_03_CHANNELS_USED_BY_MPSL 0
#endif

#ifndef NRFX_PPIB_04_12_CHANNELS_USED_BY_BT_CTLR
#define NRFX_PPIB_04_12_CHANNELS_USED_BY_BT_CTLR 0
#endif
#ifndef NRFX_PPIB_04_12_CHANNELS_USED_BY_802154_DRV
#define NRFX_PPIB_04_12_CHANNELS_USED_BY_802154_DRV 0
#endif
#ifndef NRFX_PPIB_04_12_CHANNELS_USED_BY_MPSL
#define NRFX_PPIB_04_12_CHANNELS_USED_BY_MPSL 0
#endif

#ifndef NRFX_PPIB_020_030_CHANNELS_USED_BY_BT_CTLR
#define NRFX_PPIB_020_030_CHANNELS_USED_BY_BT_CTLR 0
#endif
#ifndef NRFX_PPIB_020_030_CHANNELS_USED_BY_802154_DRV
#define NRFX_PPIB_020_030_CHANNELS_USED_BY_802154_DRV 0
#endif
#ifndef NRFX_PPIB_020_030_CHANNELS_USED_BY_MPSL
#define NRFX_PPIB_020_030_CHANNELS_USED_BY_MPSL 0
#endif

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

/** @brief Bitmask that defines DPPI channels that are reserved for use outside
 *         of the nrfx library.
 */
#define NRFX_DPPI0_CHANNELS_USED                                                                   \
	(NRFX_DPPI0_CHANNELS_USED_BY_BT_CTLR | NRFX_DPPI0_CHANNELS_USED_BY_802154_DRV |            \
	 NRFX_DPPI0_CHANNELS_USED_BY_MPSL)

/** @brief Bitmask that defines DPPI groups that are reserved for use outside
 *         of the nrfx library.
 */
#define NRFX_DPPI0_GROUPS_USED                                                                     \
	(NRFX_DPPI0_GROUPS_USED_BY_BT_CTLR | NRFX_DPPI0_GROUPS_USED_BY_802154_DRV |                \
	 NRFX_DPPI0_GROUPS_USED_BY_MPSL)

/** @brief Bitmask that defines DPPI channels that are reserved for use outside
 *         of the nrfx library.
 */
#define NRFX_DPPI00_CHANNELS_USED                                                                  \
	(NRFX_DPPI00_CHANNELS_USED_BY_BT_CTLR | NRFX_DPPI00_CHANNELS_USED_BY_802154_DRV |          \
	 NRFX_DPPI00_CHANNELS_USED_BY_MPSL)

/** @brief Bitmask that defines DPPI groups that are reserved for use outside
 *         of the nrfx library.
 */
#define NRFX_DPPI00_GROUPS_USED                                                                    \
	(NRFX_DPPI00_GROUPS_USED_BY_BT_CTLR | NRFX_DPPI00_GROUPS_USED_BY_802154_DRV |              \
	 NRFX_DPPI00_GROUPS_USED_BY_MPSL)

/** @brief Bitmask that defines DPPI channels that are reserved for use outside
 *         of the nrfx library.
 */
#define NRFX_DPPI10_CHANNELS_USED                                                                  \
	(NRFX_DPPI10_CHANNELS_USED_BY_BT_CTLR | NRFX_DPPI10_CHANNELS_USED_BY_802154_DRV |          \
	 NRFX_DPPI10_CHANNELS_USED_BY_MPSL)

/** @brief Bitmask that defines DPPI groups that are reserved for use outside
 *         of the nrfx library.
 */
#define NRFX_DPPI10_GROUPS_USED                                                                    \
	(NRFX_DPPI10_GROUPS_USED_BY_BT_CTLR | NRFX_DPPI10_GROUPS_USED_BY_802154_DRV |              \
	 NRFX_DPPI10_GROUPS_USED_BY_MPSL)

/** @brief Bitmask that defines DPPI channels that are reserved for use outside
 *         of the nrfx library.
 */
#define NRFX_DPPI20_CHANNELS_USED                                                                  \
	(NRFX_DPPI20_CHANNELS_USED_BY_BT_CTLR | NRFX_DPPI20_CHANNELS_USED_BY_802154_DRV |          \
	 NRFX_DPPI20_CHANNELS_USED_BY_MPSL)

/** @brief Bitmask that defines DPPI groups that are reserved for use outside
 *         of the nrfx library.
 */
#define NRFX_DPPI20_GROUPS_USED                                                                    \
	(NRFX_DPPI20_GROUPS_USED_BY_BT_CTLR | NRFX_DPPI20_GROUPS_USED_BY_802154_DRV |              \
	 NRFX_DPPI20_GROUPS_USED_BY_MPSL)

/** @brief Bitmask that defines DPPI channels that are reserved for use outside
 *         of the nrfx library.
 */
#define NRFX_DPPI30_CHANNELS_USED                                                                  \
	(NRFX_DPPI30_CHANNELS_USED_BY_BT_CTLR | NRFX_DPPI30_CHANNELS_USED_BY_802154_DRV |          \
	 NRFX_DPPI30_CHANNELS_USED_BY_MPSL)

/** @brief Bitmask that defines DPPI groups that are reserved for use outside
 *         of the nrfx library.
 */
#define NRFX_DPPI30_GROUPS_USED                                                                    \
	(NRFX_DPPI30_GROUPS_USED_BY_BT_CTLR | NRFX_DPPI30_GROUPS_USED_BY_802154_DRV |              \
	 NRFX_DPPI30_GROUPS_USED_BY_MPSL)

/** @brief Bitmask that defines DPPI channels that are reserved for use outside
 *         of the nrfx library.
 */
#define NRFX_DPPI020_CHANNELS_USED                                                                 \
	(NRFX_DPPI020_CHANNELS_USED_BY_BT_CTLR | NRFX_DPPI020_CHANNELS_USED_BY_802154_DRV |        \
	 NRFX_DPPI020_CHANNELS_USED_BY_MPSL)

/** @brief Bitmask that defines DPPI groups that are reserved for use outside
 *         of the nrfx library.
 */
#define NRFX_DPPI020_GROUPS_USED                                                                   \
	(NRFX_DPPI020_GROUPS_USED_BY_BT_CTLR | NRFX_DPPI020_GROUPS_USED_BY_802154_DRV |            \
	 NRFX_DPPI020_GROUPS_USED_BY_MPSL)

/** @brief Bitmask that defines DPPI channels that are reserved for use outside
 *         of the nrfx library.
 */
#define NRFX_DPPI030_CHANNELS_USED                                                                 \
	(NRFX_DPPI030_CHANNELS_USED_BY_BT_CTLR | NRFX_DPPI030_CHANNELS_USED_BY_802154_DRV |        \
	 NRFX_DPPI030_CHANNELS_USED_BY_MPSL)

/** @brief Bitmask that defines DPPI groups that are reserved for use outside
 *         of the nrfx library.
 */
#define NRFX_DPPI030_GROUPS_USED                                                                   \
	(NRFX_DPPI030_GROUPS_USED_BY_BT_CTLR | NRFX_DPPI030_GROUPS_USED_BY_802154_DRV |            \
	 NRFX_DPPI030_GROUPS_USED_BY_MPSL)

/** @brief Bitmask that defines DPPI channels that are reserved for use outside
 *         of the nrfx library.
 */
#define NRFX_DPPI120_CHANNELS_USED                                                                 \
	(NRFX_DPPI120_CHANNELS_USED_BY_BT_CTLR | NRFX_DPPI120_CHANNELS_USED_BY_802154_DRV |        \
	 NRFX_DPPI120_CHANNELS_USED_BY_MPSL)

/** @brief Bitmask that defines DPPI groups that are reserved for use outside
 *         of the nrfx library.
 */
#define NRFX_DPPI120_GROUPS_USED                                                                   \
	(NRFX_DPPI120_GROUPS_USED_BY_BT_CTLR | NRFX_DPPI120_GROUPS_USED_BY_802154_DRV |            \
	 NRFX_DPPI120_GROUPS_USED_BY_MPSL)

/** @brief Bitmask that defines DPPI channels that are reserved for use outside
 *         of the nrfx library.
 */
#define NRFX_DPPI130_CHANNELS_USED                                                                 \
	(NRFX_DPPI130_CHANNELS_USED_BY_BT_CTLR | NRFX_DPPI130_CHANNELS_USED_BY_802154_DRV |        \
	 NRFX_DPPI130_CHANNELS_USED_BY_MPSL)

/** @brief Bitmask that defines DPPI groups that are reserved for use outside
 *         of the nrfx library.
 */
#define NRFX_DPPI130_GROUPS_USED                                                                   \
	(NRFX_DPPI130_GROUPS_USED_BY_BT_CTLR | NRFX_DPPI130_GROUPS_USED_BY_802154_DRV |            \
	 NRFX_DPPI130_GROUPS_USED_BY_MPSL)

/** @brief Bitmask that defines DPPI channels that are reserved for use outside
 *         of the nrfx library.
 */
#define NRFX_DPPI131_CHANNELS_USED                                                                 \
	(NRFX_DPPI131_CHANNELS_USED_BY_BT_CTLR | NRFX_DPPI131_CHANNELS_USED_BY_802154_DRV |        \
	 NRFX_DPPI131_CHANNELS_USED_BY_MPSL)

/** @brief Bitmask that defines DPPI groups that are reserved for use outside
 *         of the nrfx library.
 */
#define NRFX_DPPI131_GROUPS_USED                                                                   \
	(NRFX_DPPI131_GROUPS_USED_BY_BT_CTLR | NRFX_DPPI131_GROUPS_USED_BY_802154_DRV |            \
	 NRFX_DPPI131_GROUPS_USED_BY_MPSL)

/** @brief Bitmask that defines DPPI channels that are reserved for use outside
 *         of the nrfx library.
 */
#define NRFX_DPPI132_CHANNELS_USED                                                                 \
	(NRFX_DPPI132_CHANNELS_USED_BY_BT_CTLR | NRFX_DPPI132_CHANNELS_USED_BY_802154_DRV |        \
	 NRFX_DPPI132_CHANNELS_USED_BY_MPSL)

/** @brief Bitmask that defines DPPI groups that are reserved for use outside
 *         of the nrfx library.
 */
#define NRFX_DPPI132_GROUPS_USED                                                                   \
	(NRFX_DPPI132_GROUPS_USED_BY_BT_CTLR | NRFX_DPPI132_GROUPS_USED_BY_802154_DRV |            \
	 NRFX_DPPI132_GROUPS_USED_BY_MPSL)

/** @brief Bitmask that defines DPPI channels that are reserved for use outside
 *         of the nrfx library.
 */
#define NRFX_DPPI133_CHANNELS_USED                                                                 \
	(NRFX_DPPI133_CHANNELS_USED_BY_BT_CTLR | NRFX_DPPI133_CHANNELS_USED_BY_802154_DRV |        \
	 NRFX_DPPI133_CHANNELS_USED_BY_MPSL)

/** @brief Bitmask that defines DPPI groups that are reserved for use outside
 *         of the nrfx library.
 */
#define NRFX_DPPI133_GROUPS_USED                                                                   \
	(NRFX_DPPI133_GROUPS_USED_BY_BT_CTLR | NRFX_DPPI133_GROUPS_USED_BY_802154_DRV |            \
	 NRFX_DPPI133_GROUPS_USED_BY_MPSL)

/** @brief Bitmask that defines DPPI channels that are reserved for use outside
 *         of the nrfx library.
 */
#define NRFX_DPPI134_CHANNELS_USED                                                                 \
	(NRFX_DPPI134_CHANNELS_USED_BY_BT_CTLR | NRFX_DPPI134_CHANNELS_USED_BY_802154_DRV |        \
	 NRFX_DPPI134_CHANNELS_USED_BY_MPSL)

/** @brief Bitmask that defines DPPI groups that are reserved for use outside
 *         of the nrfx library.
 */
#define NRFX_DPPI134_GROUPS_USED                                                                   \
	(NRFX_DPPI134_GROUPS_USED_BY_BT_CTLR | NRFX_DPPI134_GROUPS_USED_BY_802154_DRV |            \
	 NRFX_DPPI134_GROUPS_USED_BY_MPSL)

/** @brief Bitmask that defines DPPI channels that are reserved for use outside
 *         of the nrfx library.
 */
#define NRFX_DPPI135_CHANNELS_USED                                                                 \
	(NRFX_DPPI135_CHANNELS_USED_BY_BT_CTLR | NRFX_DPPI135_CHANNELS_USED_BY_802154_DRV |        \
	 NRFX_DPPI135_CHANNELS_USED_BY_MPSL)

/** @brief Bitmask that defines DPPI groups that are reserved for use outside
 *         of the nrfx library.
 */
#define NRFX_DPPI135_GROUPS_USED                                                                   \
	(NRFX_DPPI135_GROUPS_USED_BY_BT_CTLR | NRFX_DPPI135_GROUPS_USED_BY_802154_DRV |            \
	 NRFX_DPPI135_GROUPS_USED_BY_MPSL)

/** @brief Bitmask that defines DPPI channels that are reserved for use outside
 *         of the nrfx library.
 */
#define NRFX_DPPI136_CHANNELS_USED                                                                 \
	(NRFX_DPPI136_CHANNELS_USED_BY_BT_CTLR | NRFX_DPPI136_CHANNELS_USED_BY_802154_DRV |        \
	 NRFX_DPPI136_CHANNELS_USED_BY_MPSL)

/** @brief Bitmask that defines DPPI groups that are reserved for use outside
 *         of the nrfx library.
 */
#define NRFX_DPPI136_GROUPS_USED                                                                   \
	(NRFX_DPPI136_GROUPS_USED_BY_BT_CTLR | NRFX_DPPI136_GROUPS_USED_BY_802154_DRV |            \
	 NRFX_DPPI136_GROUPS_USED_BY_MPSL)

/** @brief Bitmask that defines PPI channels that are reserved for use outside
 *         of the nrfx library.
 */
#define NRFX_PPI_CHANNELS_USED                                                                     \
	(NRFX_PPI_CHANNELS_USED_BY_BT_CTLR | NRFX_PPI_CHANNELS_USED_BY_802154_DRV |                \
	 NRFX_PPI_CHANNELS_USED_BY_MPSL)

#define NRFX_DPPI_CHANNELS_USED NRFX_DPPI0_CHANNELS_USED
#define NRFX_DPPI_GROUPS_USED   NRFX_DPPI0_GROUPS_USED

/** @brief Bitmask that defines PPI groups that are reserved for use outside
 *         of the nrfx library.
 */
#define NRFX_PPI_GROUPS_USED                                                                       \
	(NRFX_PPI_GROUPS_USED_BY_BT_CTLR | NRFX_PPI_GROUPS_USED_BY_802154_DRV |                    \
	 NRFX_PPI_GROUPS_USED_BY_MPSL)

#define NRFX_PPIB_INTERCONNECT_00_10_CHANNELS_USED                                                 \
	(NRFX_PPIB_00_10_CHANNELS_USED_BY_BT_CTLR | NRFX_PPIB_00_10_CHANNELS_USED_BY_802154_DRV |  \
	 NRFX_PPIB_00_10_CHANNELS_USED_BY_MPSL)

#define NRFX_PPIB_INTERCONNECT_01_20_CHANNELS_USED                                                 \
	(NRFX_PPIB_01_20_CHANNELS_USED_BY_BT_CTLR | NRFX_PPIB_01_20_CHANNELS_USED_BY_802154_DRV |  \
	 NRFX_PPIB_01_20_CHANNELS_USED_BY_MPSL)

#define NRFX_PPIB_INTERCONNECT_11_21_CHANNELS_USED                                                 \
	(NRFX_PPIB_11_21_CHANNELS_USED_BY_BT_CTLR | NRFX_PPIB_11_21_CHANNELS_USED_BY_802154_DRV |  \
	 NRFX_PPIB_11_21_CHANNELS_USED_BY_MPSL)

#define NRFX_PPIB_INTERCONNECT_22_30_CHANNELS_USED                                                 \
	(NRFX_PPIB_22_30_CHANNELS_USED_BY_BT_CTLR | NRFX_PPIB_22_30_CHANNELS_USED_BY_802154_DRV |  \
	 NRFX_PPIB_22_30_CHANNELS_USED_BY_MPSL)

#define NRFX_PPIB_INTERCONNECT_02_03_CHANNELS_USED                                                 \
	(NRFX_PPIB_02_03_CHANNELS_USED_BY_BT_CTLR | NRFX_PPIB_02_03_CHANNELS_USED_BY_802154_DRV |  \
	 NRFX_PPIB_02_03_CHANNELS_USED_BY_MPSL)

#define NRFX_PPIB_INTERCONNECT_04_12_CHANNELS_USED                                                 \
	(NRFX_PPIB_04_12_CHANNELS_USED_BY_BT_CTLR | NRFX_PPIB_04_12_CHANNELS_USED_BY_802154_DRV |  \
	 NRFX_PPIB_04_12_CHANNELS_USED_BY_MPSL)

#define NRFX_PPIB_INTERCONNECT_020_030_CHANNELS_USED                                               \
	(NRFX_PPIB_020_030_CHANNELS_USED_BY_BT_CTLR |                                              \
	 NRFX_PPIB_020_030_CHANNELS_USED_BY_802154_DRV | NRFX_PPIB_020_030_CHANNELS_USED_BY_MPSL)

#endif /* NRFX_CONFIG_RESERVED_RESOURCES_H__ */
