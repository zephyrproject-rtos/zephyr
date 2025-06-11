# Copyright 2025 NXP
# SPDX-License-Identifier: Apache-2.0

if(${BOARD} STREQUAL "mimxrt685_evk")
  set(REMOTE_BOARD "mimxrt685_evk/mimxrt685s/hifi4")
else()
  message(FATAL_ERROR "This example is not supported on the selected board.")
endif()
