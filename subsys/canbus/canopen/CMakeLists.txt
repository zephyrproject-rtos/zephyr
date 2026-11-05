# SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(
  canopen_core.c
  canopen_emcy.c
  canopen_hb.c
  canopen_nmt.c
  canopen_od.c
  canopen_od_comm.c
  canopen_pdo.c
  canopen_sdo_server.c
  canopen_sync.c
)
zephyr_library_sources_ifdef(CONFIG_CANOPEN_LEDS canopen_leds.c)
zephyr_library_sources_ifdef(CONFIG_CANOPEN_MASTER canopen_master.c canopen_sdo_client.c)
zephyr_library_sources_ifdef(CONFIG_CANOPEN_NODE_GUARDING canopen_guard.c)
zephyr_library_sources_ifdef(CONFIG_CANOPEN_SDO_BLOCK canopen_sdo_block.c)
zephyr_library_sources_ifdef(CONFIG_CANOPEN_SHELL canopen_shell.c)
zephyr_library_sources_ifdef(CONFIG_CANOPEN_STORAGE canopen_storage.c)
zephyr_library_sources_ifdef(CONFIG_CANOPEN_TIME canopen_time.c)

function(canopen_od_from_eds target eds name)
  set(header ${CMAKE_CURRENT_BINARY_DIR}/canopen/${name}.h)
  get_filename_component(eds ${eds} ABSOLUTE)
  add_custom_command(
    OUTPUT ${header}
    COMMAND ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/scripts/canopen/eds2od.py
      --config ${DOTCONFIG} --name ${name} ${eds} ${header}
    DEPENDS
      ${eds}
      ${DOTCONFIG}
      ${ZEPHYR_BASE}/scripts/canopen/eds2od.py
      ${ZEPHYR_BASE}/scripts/canopen/canopen_eds.py
  )
  add_custom_target(${target}_${name} DEPENDS ${header})
  add_dependencies(${target} ${target}_${name})
  target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/canopen)
endfunction()

if(CONFIG_CANOPEN_EDS)
  function(canopen_eds_target)
    if(TARGET native_runner_executable)
      set(elf ${BYPRODUCT_KERNEL_EXE_NAME})
      set(elf_target native_runner_executable)
    else()
      set(elf ${BYPRODUCT_KERNEL_ELF_NAME})
      set(elf_target ${logical_target_for_zephyr_elf})
    endif()
    set(eds ${PROJECT_BINARY_DIR}/canopen.eds)
    add_custom_command(
      OUTPUT ${eds}
      COMMAND ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/scripts/canopen/od2eds.py
        --elf ${elf} --config ${DOTCONFIG} --node-config ${CONFIG_CANOPEN_EDS_NODE_CONFIG}
        --vendor-name=${CONFIG_CANOPEN_EDS_VENDOR_NAME}
        --bitrates=${CONFIG_CANOPEN_EDS_BITRATES} ${eds}
      DEPENDS
        ${elf_target}
        ${elf}
        ${ZEPHYR_BASE}/scripts/canopen/od2eds.py
        ${ZEPHYR_BASE}/scripts/canopen/canopen_eds.py
      VERBATIM
    )
    add_custom_target(canopen_eds ALL DEPENDS ${eds})
  endfunction()
  cmake_language(DEFER DIRECTORY ${ZEPHYR_BASE} CALL canopen_eds_target)
endif()
