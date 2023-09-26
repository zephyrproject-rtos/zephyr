# Copyright (c) 2023 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_cc_option(-march=pentium -mtune=i486)

zephyr_sources(soc.c)
add_subdirectory_ifdef(CONFIG_PM pm)

include(../utils/build_ish_firmware.cmake)
