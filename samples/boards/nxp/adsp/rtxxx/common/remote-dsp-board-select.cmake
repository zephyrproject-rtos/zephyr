# Copyright 2025 NXP
# SPDX-License-Identifier: Apache-2.0

set(TARGET "${BOARD}${BOARD_QUALIFIERS}")

if(${TARGET} STREQUAL "mimxrt685_evk/mimxrt685s/cm33")
  set(REMOTE_BOARD "mimxrt685_evk/mimxrt685s/hifi4")
elseif(${TARGET} STREQUAL "mimxrt700_evk/mimxrt798s/cm33_cpu0")
  set(REMOTE_BOARD "mimxrt700_evk/mimxrt798s/hifi4")
else()
  message(FATAL_ERROR "This example is not supported on the selected board.")
endif()
