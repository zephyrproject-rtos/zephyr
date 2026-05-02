# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: Apache-2.0

# This sysbuild CMake file sets the sysbuild controlled settings as properties
# on all images.

if(SB_CONFIG_COMPILER_WARNINGS_AS_ERRORS)
  set_config_bool(${ZCMAKE_APPLICATION} CONFIG_COMPILER_WARNINGS_AS_ERRORS y)
endif()
