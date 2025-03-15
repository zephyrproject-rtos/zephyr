# Copyright (c) 2024 Basalte bv
# SPDX-License-Identifier: Apache-2.0

# WARNING: the PreLoad.cmake is an undocumented feature
# We need to create the CMake file API query before the configure step

execute_process(
  COMMAND west spdx --init -d ${CMAKE_BINARY_DIR}
  COMMAND_ERROR_IS_FATAL ANY
)
