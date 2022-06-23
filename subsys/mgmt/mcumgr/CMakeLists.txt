#
# Copyright (c) 2018-2021 mcumgr authors
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

# The mgmt_mcumgr is interface library for MCUmgr services.
zephyr_interface_library_named(mgmt_mcumgr)

add_subdirectory(mgmt)
add_subdirectory(smp)
add_subdirectory(util)
add_subdirectory(grp)
add_subdirectory(transport)
add_subdirectory_ifdef(CONFIG_SMP_CLIENT    smp_client)

zephyr_library_link_libraries(mgmt_mcumgr)

if (CONFIG_BOOT_IMAGE_ACCESS_HOOKS)
    zephyr_include_directories(
        ${ZEPHYR_MCUBOOT_MODULE_DIR}/boot/bootutil/include
        ${ZEPHYR_MCUBOOT_MODULE_DIR}/boot/zephyr/include
    )
    zephyr_library_sources(bootutil_hooks/nrf53_hooks.c)
endif()
