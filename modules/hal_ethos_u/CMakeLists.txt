# SPDX-FileCopyrightText: <text>Copyright 2022, 2024 Arm Limited and/or its
# affiliates <open-source-office@arm.com></text>
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_ETHOS_U AND CONFIG_MULTITHREADING)
    # Due to CMP0126 not being NEW, ETHOSU_TARGET_NPU_CONFIG originally
    # as directory variable will fail to override that in ethos-u-core-driver
    # as cache variable. Fix by passing as cache variable. See:
    # https://cmake.org/cmake/help/latest/policy/CMP0126.html#policy:CMP0126
    set(ETHOSU_TARGET_NPU_CONFIG ${CONFIG_ETHOS_U_NPU_NAME} CACHE STRING "NPU configuration")

    # Mapping log level from Zephyr (none=0, err=1, wrn=2, inf=3, dbg=4) to
    # Ethos-U driver (err=0, warn=1, info=2, debug=3)
    set(ETHOSU_LOG_SEVERITY_MAP err err warning info debug)
    list(LENGTH ETHOSU_LOG_SEVERITY_MAP ETHOSU_LOG_SEVERITY_MAP_LENGTH)

    if(${CONFIG_ETHOS_U_LOG_LEVEL} EQUAL 0)
        # The Ethos-U driver does not have a corresponding "none" log level. Disable logging instead.
        set(ETHOSU_LOG_ENABLE OFF CACHE BOOL "")
    elseif(${CONFIG_ETHOS_U_LOG_LEVEL} LESS "${ETHOSU_LOG_SEVERITY_MAP_LENGTH}")
        list(GET ETHOSU_LOG_SEVERITY_MAP ${CONFIG_ETHOS_U_LOG_LEVEL} ETHOSU_LOG_SEVERITY)
        set(ETHOSU_LOG_SEVERITY ${ETHOSU_LOG_SEVERITY} CACHE STRING "")
    else()
        set(ETHOSU_LOG_SEVERITY debug CACHE STRING "")
    endif()

    add_subdirectory(${ZEPHYR_CURRENT_MODULE_DIR} ethos-u-core-driver)

    target_link_libraries(ethosu_core_driver PUBLIC
       zephyr_interface)

    zephyr_link_libraries(ethosu_core_driver)
endif()
