#
# Copyright (c) 2019,2020 Linaro Limited
# Copyright 2026 Arm Limited and/or its affiliates <open-source-office@arm.com>
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_library()

if(CONFIG_BUILD_WITH_TFM)
  set_property(TARGET zephyr_property_target
    APPEND PROPERTY TFM_CMAKE_OPTIONS -DCRYPTO_HW_ACCELERATOR=OFF
    )
endif()
