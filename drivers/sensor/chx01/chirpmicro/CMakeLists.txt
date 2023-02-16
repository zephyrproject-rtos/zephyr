# Copyright (c) 2023 Google LLC
# SPDX-License-Identifier: Apache-2.0

zephyr_library_include_directories(inc)
zephyr_library_sources(
    src/ch_api.c
    src/ch_common.c
    src/ch_driver.c
    src/ch_math_utils.c
)

zephyr_library_sources_ifdef(CONFIG_CH101_GPR_FW
    src/ch101_gpr.c
    src/ch101_gpr_fw.c
)

zephyr_library_sources_ifdef(CONFIG_CH101_GPR_SR_FW
    src/ch101_gpr_sr.c
    src/ch101_gpr_sr_fw.c
)
