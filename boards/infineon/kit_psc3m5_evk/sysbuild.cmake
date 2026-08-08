# SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
# SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0

if(SB_CONFIG_BOOTLOADER_MCUBOOT)
    list(APPEND ${DEFAULT_IMAGE}_EXTRA_DTC_OVERLAY_FILE
         "${CMAKE_CURRENT_LIST_DIR}/kit_psc3m5_evk_mcuboot.overlay"
    )
    set(${DEFAULT_IMAGE}_EXTRA_DTC_OVERLAY_FILE
        "${${DEFAULT_IMAGE}_EXTRA_DTC_OVERLAY_FILE}"
        CACHE STRING "Board-specific MCUboot DTS overlay" FORCE
    )
endif()
