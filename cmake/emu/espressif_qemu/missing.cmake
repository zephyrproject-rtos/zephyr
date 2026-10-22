# Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
# SPDX-License-Identifier: Apache-2.0
#
# Failure path when no Espressif QEMU binary with the required -machine was
# found. Kept in a separate file so scripts/cmake/cmake_style.py does not
# segfault on the combined if/else tree.

string(CONCAT _qemu_missing_msg
  "Espressif QEMU not found: need ${ESPRESSIF_QEMU_BIN_NAME} "
  "with -machine ${ESPRESSIF_QEMU_MACHINE}. "
  "Set ESPRESSIF_QEMU_PATH and re-run CMake.")
# Echo the message, then fail. Do not pass the string as an argv to
# `cmake -E false`: ninja reprints that recipe with shell-escaped spaces,
# which looks like part of the error.
add_custom_target(run_espressif_qemu
  COMMAND ${CMAKE_COMMAND} -E echo "${_qemu_missing_msg}"
  COMMAND ${CMAKE_COMMAND} -E false
  COMMENT "Espressif QEMU not available"
)
add_custom_target(debugserver_espressif_qemu
  COMMAND ${CMAKE_COMMAND} -E echo "${_qemu_missing_msg}"
  COMMAND ${CMAKE_COMMAND} -E false
  COMMENT "Espressif QEMU not available"
)
