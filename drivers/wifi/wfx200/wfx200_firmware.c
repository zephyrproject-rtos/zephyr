/**
 * Copyright (c) 2023 grandcentrix GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#if defined(__APPLE__) && defined(__clang__)
__attribute__ ((__section__("__SL_WFX_FIRMWARE,__sl_wfx_firmware"), used))
#elif defined(__GNUC__) || defined(__ARMCC_VERSION)
__attribute__ ((__section__(".sl_wfx_firmware"), used))
#elif defined(__ICCARM__)
#pragma  location=".sl_wfx_firmware"
#else
#error "An unsupported toolchain"
#endif
const uint8_t sl_wfx_firmware[] = {
#include <wfm_wf200_C0.sec.inc>
};
