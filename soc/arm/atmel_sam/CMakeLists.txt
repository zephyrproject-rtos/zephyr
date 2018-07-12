# Makefile - Atmel SAM MCU family
#
# Copyright (c) 2016 Piotr Mienkowski
# SPDX-License-Identifier: Apache-2.0
#

include_relative(${SOC_SERIES}/CMakeLists.txt)
include_relative_ifdef(CONFIG_ASF common/CMakeLists.txt)
