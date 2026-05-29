# Copyright (c) 2026 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

set(SUPPORTED_EMU_PLATFORMS whisper)

set(
  BOARD_LINKER_SCRIPT ${BOARD_DIR}/${NORMALIZED_BOARD_TARGET}.ld
  CACHE INTERNAL "Board linker script, ${BOARD}"
)
