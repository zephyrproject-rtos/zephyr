# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

include(./ironside_support_dir.cmake)

zephyr_include_directories(include)
zephyr_include_directories(${IRONSIDE_SUPPORT_DIR}/se/include)

zephyr_library()
zephyr_library_property(ALLOW_EMPTY TRUE)
zephyr_library_sources_ifdef(CONFIG_IRONSIDE_SE_CALL
  ${IRONSIDE_SUPPORT_DIR}/se/src/ironside_se_api.c
  call.c
  glue.c
)
zephyr_library_sources_ifdef(CONFIG_IRONSIDE_SE_DVFS dvfs.c)

if(CONFIG_NRF_PERIPHCONF_SECTION)
  zephyr_linker_sources(SECTIONS uicr.ld)
endif()

if(CONFIG_NRF_PERIPHCONF_GENERATE_ENTRIES)
  set(optional_args)
  if(CONFIG_NRF_PERIPHCONF_GENERATE_ENTRIES_LOCK)
    list(APPEND optional_args --lock)
  else()
    list(APPEND optional_args --no-lock)
  endif()
  set(periphconf_entries_c_file ${PROJECT_BINARY_DIR}/periphconf_entries_generated.c)
  execute_process(
    COMMAND
      ${CMAKE_COMMAND} -E env ZEPHYR_BASE=${ZEPHYR_BASE}
      ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/scripts/gen_periphconf_entries.py
      --soc ${CONFIG_SOC}
      --in-edt-pickle ${EDT_PICKLE}
      --out-periphconf-source ${periphconf_entries_c_file}
      ${optional_args}
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    COMMAND_ERROR_IS_FATAL ANY
  )
  zephyr_sources(${periphconf_entries_c_file})
  message(STATUS "Generated ${periphconf_entries_c_file} from ${EDT_PICKLE}")
endif()

if(CONFIG_NRF_PERIPHCONF_SECTION AND NOT SYSBUILD)
  message(WARNING "CONFIG_NRF_PERIPHCONF_SECTION is enabled, but Sysbuild is not being used. "
                  "The global peripheral configuration will not be applied unless artifacts "
                  "are generated manually/externally. To enable automatic generation, build with "
                  "Sysbuild and ensure that SB_CONFIG_NRF_HALTIUM_GENERATE_UICR=y."
  )
endif()
