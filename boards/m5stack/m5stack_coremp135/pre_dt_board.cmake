# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

if(NOT BOARD_QUALIFIERS STREQUAL "stm32mp135dxx/fsbl")
  message(FATAL_ERROR
    "CoreMP135 supports only the m5stack_coremp135/stm32mp135dxx/fsbl target."
  )
endif()
