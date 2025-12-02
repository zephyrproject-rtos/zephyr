# SPDX-FileCopyrightText: Copyright (c) 2024 Carl Zeiss Meditec AG
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_property(ALLOW_EMPTY TRUE)

zephyr_library_sources_ifdef(CONFIG_STEPPER_ADI_TMC2209 tmc22xx.c)
zephyr_library_sources_ifdef(CONFIG_STEPPER_ADI_TMC50XX tmc50xx.c)
add_subdirectory_ifdef(CONFIG_STEPPER_ADI_TMC51XX tmc51xx)

add_subdirectory(bus)
add_subdirectory(common)
