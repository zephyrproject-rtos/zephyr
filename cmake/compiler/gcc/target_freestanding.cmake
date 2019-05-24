# Copyright (c) 2019 Intel Corporation.
# SPDX-License-Identifier: Apache-2.0

macro(toolchain_cc_no_freestanding_options)

  zephyr_cc_option(-fno-freestanding)

endmacro()
