# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_NRF_PERIPHCONF_SECTION)
  zephyr_linker_sources(SECTIONS uicr.ld)
endif()

if(CONFIG_NRF_HALTIUM_GENERATE_UICR)
  set(uicr_hex_file ${PROJECT_BINARY_DIR}/uicr.hex)
  set(periphconf_hex_file ${PROJECT_BINARY_DIR}/periphconf.hex)

  if(CONFIG_NRF_HALTIUM_UICR_PERIPHCONF)
    set(in_periphconf_elf_arg
      --in-periphconf-elf $<TARGET_FILE:${ZEPHYR_LINK_STAGE_EXECUTABLE}>
    )
  endif()

  set(uicrgen_command
    ${CMAKE_COMMAND} -E env PYTHONPATH=${ZEPHYR_BASE}/scripts/dts/python-devicetree/src
    ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/uicrgen.py
    --in-config ${DOTCONFIG}
    --in-edt-pickle ${EDT_PICKLE}
    ${in_periphconf_elf_arg}
    --out-uicr-hex ${uicr_hex_file}
    --out-periphconf-hex ${periphconf_hex_file}
  )

  set_property(
    GLOBAL APPEND
    PROPERTY extra_post_build_commands COMMAND ${uicrgen_command}
  )
  set_property(
    GLOBAL APPEND
    PROPERTY extra_post_build_byproducts ${uicr_hex_file} ${periphconf_hex_file}
  )
endif()
