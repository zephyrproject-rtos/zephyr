# Copyright (c) 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_NRFS)
    if(CONFIG_NRFS_LOCAL_DOMAIN)
        zephyr_sources_ifdef(CONFIG_NRFS_BACKEND_IPC_SERVICE_EXTENDED
                            backends/nrfs_backend_ipc_service.c)
        zephyr_sources_ifdef(CONFIG_NRFS_BACKEND_IPC_SERVICE_LITE
                            backends/nrfs_backend_ipc_service_lite.c)
    endif()
    zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/backends)
    zephyr_include_directories_ifdef(CONFIG_NRFS_DVFS_LOCAL_DOMAIN ${CMAKE_CURRENT_SOURCE_DIR}/dvfs)
    add_subdirectory_ifdef(CONFIG_NRFS_DVFS_LOCAL_DOMAIN dvfs)
endif()
