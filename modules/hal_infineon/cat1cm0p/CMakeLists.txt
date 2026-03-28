# Copyright (c) 2022 Cypress Semiconductor Corporation.
#
# SPDX-License-Identifier: Apache-2.0

set(hal_dir               ${ZEPHYR_HAL_INFINEON_MODULE_DIR})
set(hal_cat1cm0p_dir      ${hal_dir}/cat1cm0p)
set(hal_blobs_dir         ${hal_dir}/zephyr/blobs/img/cat1cm0p/COMPONENT_CM0P_SLEEP)
set(blob_gen_dir          ${ZEPHYR_BINARY_DIR}/include/generated)
set(blob_gen_inc_file     ${blob_gen_dir}/psoc6_cm0p_sleep.bin.inc)

if(CONFIG_SOC_DIE_PSOC6_01)
    set(blob_bin_file ${hal_blobs_dir}/psoc6_01_cm0p_sleep.bin)
endif()

if(CONFIG_SOC_DIE_PSOC6_02)
    set(blob_bin_file ${hal_blobs_dir}/psoc6_02_cm0p_sleep.bin)
endif()

if(CONFIG_SOC_DIE_PSOC6_03)
    set(blob_bin_file ${hal_blobs_dir}/psoc6_03_cm0p_sleep.bin)
endif()

if(CONFIG_SOC_DIE_PSOC6_04)
    set(blob_bin_file ${hal_blobs_dir}/psoc6_04_cm0p_sleep.bin)
endif()

# Generate Include binaries and add source of psoc6_cm0p_sleep.c
if(EXISTS ${blob_bin_file})
    # Generate Include binaries
    generate_inc_file_for_target(app ${blob_bin_file} ${blob_gen_inc_file})

    # psoc6_cm0p_sleep.c - template c-array file, which include generated
    # blob include file.
    zephyr_library_sources(${hal_cat1cm0p_dir}/COMPONENT_CM0P_SLEEP/psoc6_cm0p_sleep.c)
else()
    message(WARNING "Can't find CM0+ binaries, please execute: \'west blobs fetch hal_infineon\' ")
endif()
