# Makefile - Atmel SAM MCU family
#
# Copyright (c) 2016 Piotr Mienkowski
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(${SOC_SERIES})
add_subdirectory_ifdef(CONFIG_ASF common)
