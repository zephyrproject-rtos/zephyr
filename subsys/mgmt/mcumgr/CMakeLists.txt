# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(buf.c)
zephyr_library_sources(smp.c)

add_subdirectory(transport)
zephyr_library_sources_ifdef(CONFIG_MCUMGR_SMP_REASSEMBLY smp_reassembly.c)

add_subdirectory(lib)
zephyr_library_link_libraries(MCUMGR)
