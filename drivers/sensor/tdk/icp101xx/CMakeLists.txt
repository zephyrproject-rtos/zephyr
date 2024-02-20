# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(icp101xx.c Devices/Drivers/ICP101xx/Icp101xx.c)
zephyr_library_include_directories(. ./Devices/Drivers/ICP101xx/ ./EmbUtils)
