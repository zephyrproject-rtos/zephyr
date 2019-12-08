# Copyright (c) 2019 Stephanos Ioannidis <root@stephanos.io>
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(Include)

if(CONFIG_CMSIS_DSP_MVEF OR CONFIG_CMSIS_DSP_SUPPORT)
  include_directories(PrivateInclude)
endif()

add_subdirectory(Source)
