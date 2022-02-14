# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(buf.c)
zephyr_library_sources(smp.c)
zephyr_library_sources_ifdef(CONFIG_MCUMGR_SMP_BT smp_bt.c)
zephyr_library_sources_ifdef(CONFIG_MCUMGR_SMP_SHELL smp_shell.c)
zephyr_library_sources_ifdef(CONFIG_MCUMGR_SMP_UART smp_uart.c)
zephyr_library_sources_ifdef(CONFIG_MCUMGR_SMP_UDP smp_udp.c)
zephyr_library_sources_ifdef(CONFIG_MCUMGR_SMP_REASSEMBLY smp_reassembly.c)
add_subdirectory_ifdef(CONFIG_MCUMGR_GRP_ZEPHYR_BASIC zephyr_grp)

add_subdirectory(lib)
zephyr_library_link_libraries(MCUMGR)

if (CONFIG_MCUMGR_SMP_SHELL OR CONFIG_MCUMGR_SMP_UART)
    zephyr_library_sources(serial_util.c)
endif()

if (CONFIG_FS_MGMT_CHECKSUM_HASH AND CONFIG_FS_MGMT_HASH_SHA256)
    if (NOT CONFIG_TINYCRYPT)
        zephyr_link_libraries(mbedTLS)
    endif()
endif()
