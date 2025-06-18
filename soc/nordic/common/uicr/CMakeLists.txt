# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_NRF_PERIPHCONF_SECTION)
  zephyr_linker_sources(SECTIONS uicr.ld)
endif()

if(CONFIG_NRF_HALTIUM_GENERATE_UICR)
  if(CONFIG_NRF_PERIPHCONF_SECTION)
    set(in_periphconf_elf_arg
      --in-periphconf-elf $<TARGET_FILE:${ZEPHYR_LINK_STAGE_EXECUTABLE}>
    )
  endif()

  if(CONFIG_NRF_HALTIUM_UICR_PERIPHCONF)
    set(periphconf_hex_file ${PROJECT_BINARY_DIR}/periphconf.hex)
    set(out_periphconf_hex_arg
      --out-periphconf-hex ${periphconf_hex_file}
    )
    list(APPEND optional_byproducts ${periphconf_hex_file})
  endif()

  set(uicr_hex_file ${PROJECT_BINARY_DIR}/uicr.hex)
  set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
    COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${ZEPHYR_BASE}/scripts/dts/python-devicetree/src
    ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/gen_uicr.py
    --in-config ${DOTCONFIG}
    --in-edt-pickle ${EDT_PICKLE}
    ${in_periphconf_elf_arg}
    ${out_periphconf_hex_arg}
    --out-uicr-hex ${uicr_hex_file}
  )
  set_property(GLOBAL APPEND PROPERTY extra_post_build_byproducts
    ${uicr_hex_file} ${optional_byproducts}
  )
endif()
