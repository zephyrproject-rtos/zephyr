# Copyright (c) 2022 Google LLC
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_CHRE)
  zephyr_library_named(chre_asserts)
  zephyr_library_sources(src/sensor_constants_asserts.c)
endif()
