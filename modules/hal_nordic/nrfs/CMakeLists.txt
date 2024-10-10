# Copyright (c) 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0


if(CONFIG_NRFS)
    zephyr_library()
    if(NOT DEFINED NRFS_DIR)
        set(NRFS_DIR ${ZEPHYR_CURRENT_MODULE_DIR}/nrfs CACHE PATH "nrfs directory")
    endif()

    set(INC_DIR ${NRFS_DIR}/include)
    set(SRC_DIR ${NRFS_DIR}/src)
    set(HELPERS_DIR ${NRFS_DIR}/helpers)

    zephyr_include_directories(${INC_DIR})
    zephyr_include_directories(${INC_DIR}/services)
    zephyr_include_directories(${HELPERS_DIR})
    zephyr_include_directories(.)
    zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/backends)
    zephyr_include_directories_ifdef(CONFIG_NRFS_DVFS_LOCAL_DOMAIN ${CMAKE_CURRENT_SOURCE_DIR}/dvfs)

    zephyr_library_sources(${HELPERS_DIR}/dvfs_oppoint.c)

    if(CONFIG_NRFS_LOCAL_DOMAIN)
        zephyr_library_sources_ifdef(CONFIG_NRFS_CLOCK_SERVICE_ENABLED ${SRC_DIR}/services/nrfs_clock.c)
        zephyr_library_sources_ifdef(CONFIG_NRFS_DIAG_SERVICE_ENABLED ${SRC_DIR}/services/nrfs_diag.c)
        zephyr_library_sources_ifdef(CONFIG_NRFS_DVFS_SERVICE_ENABLED ${SRC_DIR}/services/nrfs_dvfs.c)
        zephyr_library_sources_ifdef(CONFIG_NRFS_GDPWR_SERVICE_ENABLED ${SRC_DIR}/services/nrfs_gdpwr.c)
        zephyr_library_sources_ifdef(CONFIG_NRFS_MRAM_SERVICE_ENABLED ${SRC_DIR}/services/nrfs_mram.c)
        zephyr_library_sources_ifdef(CONFIG_NRFS_PMIC_SERVICE_ENABLED ${SRC_DIR}/services/nrfs_pmic.c)
        zephyr_library_sources_ifdef(CONFIG_NRFS_RESET_SERVICE_ENABLED ${SRC_DIR}/services/nrfs_reset.c)
        zephyr_library_sources_ifdef(CONFIG_NRFS_TEMP_SERVICE_ENABLED ${SRC_DIR}/services/nrfs_temp.c)
        zephyr_library_sources_ifdef(CONFIG_NRFS_VBUS_DETECTOR_SERVICE_ENABLED ${SRC_DIR}/services/nrfs_usb.c)
        zephyr_library_sources(${SRC_DIR}/internal/nrfs_dispatcher.c)
        add_subdirectory_ifdef(CONFIG_NRFS_DVFS_LOCAL_DOMAIN dvfs)

        if(CONFIG_NRFS_DIAG_SERVICE_ENABLED)
            message(WARNING "This service is for Nordic Semiconductor INTERNAL purposes ONLY. Use it with caution due to risk of hardware damage!")
        endif()
    endif()
    zephyr_library_sources_ifdef(CONFIG_NRFS_LOCAL_DOMAIN backends/nrfs_backend_ipc_service.c)
endif()
