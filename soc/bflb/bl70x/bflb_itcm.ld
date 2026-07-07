/*
 * Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * Place the timing-critical lmac154 / RF blob code (.tcm_code / .tcm_data /
 * .pds_code) into ITCM, matching the vendor SDK. Injected via
 * zephyr_linker_sources(ITCM_SECTION) for IEEE 802.15.4 builds.
 */

*(.tcm_code .tcm_code.*)
*(.tcm_data .tcm_data.*)
*(.pds_code .pds_code.*)
