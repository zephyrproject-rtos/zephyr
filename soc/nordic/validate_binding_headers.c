/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * This file validates definitions found in dt-bindings headers against their
 * expected values from MDK, which may be provided in the form of C types.
 *
 * Note: all dt-bindings headers which have been included by DTS in this build
 * are automagically included in this file as well. See CMakeLists.txt.
 */

#include <zephyr/sys/util_macro.h>
#include <zephyr/toolchain.h>

#include <nrf.h>

/**
 * Domain IDs. See:
 *   - dt-bindings/misc/nordic-domain-id-nrf54h20.h
 */
#if defined(NRF_DOMAIN_ID_APPLICATION)
BUILD_ASSERT(NRF_DOMAIN_ID_APPLICATION == NRF_DOMAIN_APPLICATION);
#endif
#if defined(NRF_DOMAIN_ID_RADIOCORE)
BUILD_ASSERT(NRF_DOMAIN_ID_RADIOCORE == NRF_DOMAIN_RADIOCORE);
#endif
#if defined(NRF_DOMAIN_ID_GLOBALFAST)
BUILD_ASSERT(NRF_DOMAIN_ID_GLOBALFAST == NRF_DOMAIN_GLOBALFAST);
#endif
#if defined(NRF_DOMAIN_ID_GLOBALSLOW)
BUILD_ASSERT(NRF_DOMAIN_ID_GLOBALSLOW == NRF_DOMAIN_GLOBALSLOW);
#endif

/**
 * Owner IDs. See:
 *   - dt-bindings/misc/nordic-owner-id-nrf54h20.h
 */
#if defined(NRF_OWNER_ID_NONE)
BUILD_ASSERT(NRF_OWNER_ID_NONE == NRF_OWNER_NONE);
#endif
#if defined(NRF_OWNER_ID_APPLICATION)
BUILD_ASSERT(NRF_OWNER_ID_APPLICATION == NRF_OWNER_APPLICATION);
#endif
#if defined(NRF_OWNER_ID_RADIOCORE)
BUILD_ASSERT(NRF_OWNER_ID_RADIOCORE == NRF_OWNER_RADIOCORE);
#endif
