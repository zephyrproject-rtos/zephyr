#
# Copyright (c) 2025 Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: Apache-2.0
#

if(EXISTS "${BOARD_DIR}/${BOARD}.overlay")
  list(APPEND EXTRA_DTC_OVERLAY_FILE "${BOARD_DIR}/${BOARD}.overlay")
endif()
