# Copyright (c) 2025 Conclusive Engineering Sp. z o.o.
#
# SPDX-License-Identifier: Apache-2.0

function(zephyr_simplelink_tasks)
  set(SOC_DIR ${CMAKE_CURRENT_LIST_DIR})

  string(CONFIGURE "${CONFIG_CC35XXE_SIGN_PUBLIC_KEY_FILE}" pubkey)
  string(CONFIGURE "${CONFIG_CC35XXE_SIGN_PRIVATE_KEY_FILE}" privkey)
  string(CONFIGURE "${CONFIG_CC35XXE_FLASH_DISCOVERY_CONFIG_OTFDE}" otfde)
  string(CONFIGURE "${CONFIG_CC35XXE_FLASH_DISCOVERY_CONFIG_EXT_MEM}" ext_mem)
  string(CONFIGURE "${CONFIG_CC35XXE_FLASH_DISCOVERY_CONFIG_XSPI}" xspi)
  string(CONFIGURE "${CONFIG_CC35XXE_FLASH_PROFILE}" flash_profile)
  string(CONFIGURE "${CONFIG_CC35XXE_FUSE_CONFIG}" fuse_config)
  string(CONFIGURE "${CONFIG_CC35XXE_ACTION_PARAMS_FILE}" action_params)
  string(CONFIGURE "${CONFIG_CC35XXE_TOOL_SETTINGS_FILE}" tool_settings)

  function(cc35xx_to_hex output expression)
    math(EXPR value "${expression}" OUTPUT_FORMAT HEXADECIMAL)
    set(${output} ${value} PARENT_SCOPE)
  endfunction()

  # Use default keys if not set
  if("${pubkey}" STREQUAL "")
    set(pubkey "${SOC_DIR}/keys/cc35x1e_lp_em_pub_key.pem")
  endif()

  if(NOT EXISTS "${pubkey}")
    message(FATAL_ERROR "Public key file not found: ${pubkey}")
  endif()

  if("${privkey}" STREQUAL "")
    set(privkey "${SOC_DIR}/keys/cc35x1e_lp_em_prv_key.pem")
  endif()

  if(NOT EXISTS "${privkey}")
    message(FATAL_ERROR "Private key file not found: ${privkey}")
  endif()

  # Use default config jsons if not set
  if("${flash_profile}" STREQUAL "")
    set(flash_profile "is25wj032f")
  endif()

  set(flash_profile_dir "${BOARD_DIR}/config/flash/${flash_profile}")
  if(CONFIG_CC35XXE_FWU)
    if(EXISTS "${flash_profile_dir}/ota")
      set(flash_profile_dir "${flash_profile_dir}/ota")
    elseif("${otfde}" STREQUAL "" AND
           "${ext_mem}" STREQUAL "" AND
           "${xspi}" STREQUAL "")
      message(FATAL_ERROR
              "FWU requires an OTA flash profile for '${flash_profile}'. "
              "Provide explicit flash discovery JSONs or select a profile with an ota/ layout.")
    endif()
  endif()

  if("${otfde}" STREQUAL "")
    set(otfde "${flash_profile_dir}/flash_disc_param_otfde.json")
  endif()

  if(NOT EXISTS "${otfde}")
    message(FATAL_ERROR "OTFDE config file not found: ${otfde}")
  endif()

  if("${ext_mem}" STREQUAL "")
    set(ext_mem "${flash_profile_dir}/flash_disc_param_ext_mem.json")
  endif()

  if(NOT EXISTS "${ext_mem}")
    message(FATAL_ERROR "External memory config file not found: ${ext_mem}")
  endif()

  if("${xspi}" STREQUAL "")
    set(xspi "${flash_profile_dir}/flash_disc_param_xspi.json")
  endif()

  if(NOT EXISTS "${xspi}")
    message(FATAL_ERROR "xSPI config file not found: ${xspi}")
  endif()

  if(CONFIG_CC35XXE_GENERATE_FLASH_MAP_CONFIG)
    string(CONFIGURE "${CONFIG_CC35XXE_MEMORY_CONFIG_JSON}" memory_config)
    if("${memory_config}" STREQUAL "")
      set(memory_config "${flash_profile_dir}/external_memory_configurator.json")
    endif()

    if(NOT EXISTS "${memory_config}")
      message(FATAL_ERROR "Memory configuration file not found: ${memory_config}")
    endif()

    set(flash_map_config "${ZEPHYR_BINARY_DIR}/ti_flash_map_config.c")
    file(READ "${memory_config}" memory_config_json)
    file(READ "${ext_mem}" flash_discovery_json)

    string(JSON flash_sector_size GET "${memory_config_json}" flash_type flash_sector_size)
    string(JSON nvs_size_kib GET "${memory_config_json}" inputs nvs_data_max_size)
    string(JSON key_storage_size_kib GET "${memory_config_json}" inputs key_storage_data_max_size)
    string(JSON vendor_specific_size_kib GET "${memory_config_json}" inputs vendor_specific_max_size)
    string(JSON region0_end GET "${flash_discovery_json}" region_0 mem_region_0_end_phy_address)
    string(JSON region2_start GET "${flash_discovery_json}" region_2 mem_region_2_start_phy_address)
    string(JSON region2_end GET "${flash_discovery_json}" region_2 mem_region_2_end_phy_address)
    string(JSON bl2_slot1_start GET "${flash_discovery_json}" boot_region_conf boot_region_primary_ti_bl_start_phy_address)
    string(JSON bl2_slot2_start GET "${flash_discovery_json}" boot_region_conf boot_region_sec_ti_bl_start_phy_address)
    string(JSON vendor_slot1_start GET "${flash_discovery_json}" main_code_region_conf main_code_region_vendor_image_start_phy_address)
    string(JSON vendor_slot2_start GET "${flash_discovery_json}" alt_code_region_conf alt_code_region_vendor_image_start_phy_address)
    string(JSON protected_storage_start GET "${flash_discovery_json}" nvs_code_region_conf nvs_start_phy_address)
    string(JSON wsoc_slot1_start GET "${flash_discovery_json}" nvs_code_region_conf nvs_region_primary_wsoc_start_phy_address)
    string(JSON wsoc_slot2_start GET "${flash_discovery_json}" nvs_code_region_conf nvs_region_sec_wsoc_start_phy_address)

    set(gpe_header_size 4092)

    math(EXPR bl2_slot1_base "${bl2_slot1_start} - ${gpe_header_size}")
    math(EXPR bl2_slot2_base "${bl2_slot2_start} - ${gpe_header_size}")
    math(EXPR wsoc_slot1_base "${wsoc_slot1_start} - ${gpe_header_size}")
    math(EXPR wsoc_slot2_base "${wsoc_slot2_start} - ${gpe_header_size}")
    math(EXPR vendor_slot1_base "${vendor_slot1_start} - ${gpe_header_size}")
    math(EXPR bl2_slot1_size "${bl2_slot2_base} - ${bl2_slot1_base}")
    math(EXPR bl2_slot2_size "${wsoc_slot1_base} - ${bl2_slot2_base}")
    math(EXPR wsoc_slot1_size "${wsoc_slot2_base} - ${wsoc_slot1_base}")
    math(EXPR protected_storage_header_size "${flash_sector_size}")
    math(EXPR nvs_size "${nvs_size_kib} * 1024")
    math(EXPR key_storage_size "${key_storage_size_kib} * 1024")
    math(EXPR vendor_specific_size "${vendor_specific_size_kib} * 1024")
    math(EXPR vendor_specific_base
         "${protected_storage_start} + ${protected_storage_header_size}")
    math(EXPR nvs_start "${vendor_specific_base} + ${vendor_specific_size}")
    math(EXPR key_storage_start "${nvs_start} + ${nvs_size}")
    math(EXPR generated_vendor_slot1_base "${key_storage_start} + ${key_storage_size}")
    math(EXPR wsoc_slot2_size "${protected_storage_start} - ${wsoc_slot2_base}")
    math(EXPR vendor_slot1_size "${region0_end} + 1 - ${vendor_slot1_base}")

    if(NOT generated_vendor_slot1_base EQUAL vendor_slot1_base)
      message(FATAL_ERROR
              "MemoryConfigurator layout does not match flash profile: generated vendor base "
              "${generated_vendor_slot1_base}, flash profile base ${vendor_slot1_base}")
    endif()

    set(vendor_slot2_base 0)
    set(vendor_slot2_size 0)
    set(vendor_slot2_logical 0)

    if("${region2_end}" GREATER "${region2_start}" AND
       "${vendor_slot2_start}" GREATER "${gpe_header_size}")
      math(EXPR vendor_slot2_base "${vendor_slot2_start} - ${gpe_header_size}")
      math(EXPR vendor_slot2_size "${region2_end} + 1 - ${vendor_slot2_base}")
      set(vendor_slot2_logical 0xA4000000)
    endif()

    cc35xx_to_hex(wsoc_slot1_base_hex "${wsoc_slot1_base}")
    cc35xx_to_hex(wsoc_slot1_logical_hex "0xA0000000 + ${wsoc_slot1_base}")
    cc35xx_to_hex(wsoc_slot1_size_hex "${wsoc_slot1_size}")
    cc35xx_to_hex(wsoc_slot2_base_hex "${wsoc_slot2_base}")
    cc35xx_to_hex(wsoc_slot2_logical_hex "0xA0000000 + ${wsoc_slot2_base}")
    cc35xx_to_hex(wsoc_slot2_size_hex "${wsoc_slot2_size}")
    cc35xx_to_hex(vendor_slot1_base_hex "${vendor_slot1_base}")
    cc35xx_to_hex(vendor_slot1_size_hex "${vendor_slot1_size}")
    cc35xx_to_hex(vendor_slot2_base_hex "${vendor_slot2_base}")
    cc35xx_to_hex(vendor_slot2_size_hex "${vendor_slot2_size}")
    cc35xx_to_hex(bl2_slot1_base_hex "${bl2_slot1_base}")
    cc35xx_to_hex(bl2_slot1_logical_hex "0xA0000000 + ${bl2_slot1_base}")
    cc35xx_to_hex(bl2_slot1_size_hex "${bl2_slot1_size}")
    cc35xx_to_hex(bl2_slot2_base_hex "${bl2_slot2_base}")
    cc35xx_to_hex(bl2_slot2_logical_hex "0xA0000000 + ${bl2_slot2_base}")
    cc35xx_to_hex(bl2_slot2_size_hex "${bl2_slot2_size}")
    cc35xx_to_hex(nvs_start_hex "${nvs_start}")
    cc35xx_to_hex(nvs_logical_hex "0xA0000000 + ${nvs_start}")
    cc35xx_to_hex(nvs_size_hex "${nvs_size}")
    cc35xx_to_hex(key_storage_start_hex "${key_storage_start}")
    cc35xx_to_hex(key_storage_logical_hex "0xA0000000 + ${key_storage_start}")
    cc35xx_to_hex(key_storage_size_hex "${key_storage_size}")

    file(WRITE "${flash_map_config}" "#include <stdint.h>\n\n")
    file(APPEND "${flash_map_config}" "uint32_t wifi_connectivity_physical_slot_1_address = ${wsoc_slot1_base_hex};\n")
    file(APPEND "${flash_map_config}" "uint32_t wifi_connectivity_logical_slot_1_address = ${wsoc_slot1_logical_hex};\n")
    file(APPEND "${flash_map_config}" "uint32_t wifi_connectivity_slot_1_region_size = ${wsoc_slot1_size_hex};\n")
    file(APPEND "${flash_map_config}" "uint32_t wifi_connectivity_physical_slot_2_address = ${wsoc_slot2_base_hex};\n")
    file(APPEND "${flash_map_config}" "uint32_t wifi_connectivity_logical_slot_2_address = ${wsoc_slot2_logical_hex};\n")
    file(APPEND "${flash_map_config}" "uint32_t wifi_connectivity_slot_2_region_size = ${wsoc_slot2_size_hex};\n")
    file(APPEND "${flash_map_config}" "uint32_t vendor_image_physical_slot_1_address = ${vendor_slot1_base_hex};\n")
    file(APPEND "${flash_map_config}" "uint32_t vendor_image_logical_slot_1_address = 0x14000000;\n")
    file(APPEND "${flash_map_config}" "uint32_t vendor_image_slot_1_region_size = ${vendor_slot1_size_hex};\n")
    file(APPEND "${flash_map_config}" "uint32_t vendor_image_physical_slot_2_address = ${vendor_slot2_base_hex};\n")
    file(APPEND "${flash_map_config}" "uint32_t vendor_image_logical_slot_2_address = ${vendor_slot2_logical};\n")
    file(APPEND "${flash_map_config}" "uint32_t vendor_image_slot_2_region_size = ${vendor_slot2_size_hex};\n")
    file(APPEND "${flash_map_config}" "uint32_t bl2_physical_slot_1_address = ${bl2_slot1_base_hex};\n")
    file(APPEND "${flash_map_config}" "uint32_t bl2_logical_slot_1_address = ${bl2_slot1_logical_hex};\n")
    file(APPEND "${flash_map_config}" "uint32_t bl2_slot_1_region_size = ${bl2_slot1_size_hex};\n")
    file(APPEND "${flash_map_config}" "uint32_t bl2_physical_slot_2_address = ${bl2_slot2_base_hex};\n")
    file(APPEND "${flash_map_config}" "uint32_t bl2_logical_slot_2_address = ${bl2_slot2_logical_hex};\n")
    file(APPEND "${flash_map_config}" "uint32_t bl2_slot_2_region_size = ${bl2_slot2_size_hex};\n")
    file(APPEND "${flash_map_config}" "uint32_t nvocmp_physical_slot_address = ${nvs_start_hex};\n")
    file(APPEND "${flash_map_config}" "uint32_t nvocmp_logical_slot_address = ${nvs_logical_hex};\n")
    file(APPEND "${flash_map_config}" "uint32_t nvocmp_region_size = ${nvs_size_hex};\n")
    file(APPEND "${flash_map_config}" "uint32_t key_storage_physical_slot_address = ${key_storage_start_hex};\n")
    file(APPEND "${flash_map_config}" "uint32_t key_storage_logical_slot_address = ${key_storage_logical_hex};\n")
    file(APPEND "${flash_map_config}" "uint32_t key_storage_region_size = ${key_storage_size_hex};\n")

    set_property(GLOBAL PROPERTY CC35XXE_FLASH_MAP_CONFIG_SOURCE "${flash_map_config}")
    set_property(GLOBAL PROPERTY CC35XXE_FLASH_MAP_CONFIG_TARGET "")
  endif()

  if("${fuse_config}" STREQUAL "")
    set(fuse_config "${BOARD_DIR}/config/fuse_prog_inst_param.json")
  endif()

  if(NOT EXISTS "${fuse_config}")
    message(FATAL_ERROR "Fuse config file not found: ${fuse_config}")
  endif()

  if("${action_params}" STREQUAL "")
    set(action_params "${BOARD_DIR}/config/action_params.json")
  endif()

  if(NOT EXISTS "${action_params}")
    message(FATAL_ERROR "Action parameters file not found: ${action_params}")
  endif()

  if("${tool_settings}" STREQUAL "")
    set(tool_settings "${BOARD_DIR}/config/tool_settings.json")
  endif()

  if(NOT EXISTS "${tool_settings}")
    message(FATAL_ERROR "Tool settings file not found: ${tool_settings}")
  endif()

  set(outdir ${ZEPHYR_BINARY_DIR}/flash)
  set(output ${ZEPHYR_BINARY_DIR}/${KERNEL_NAME})

  set_property(GLOBAL APPEND PROPERTY extra_post_build_commands COMMAND
               ${CMAKE_COMMAND} -E make_directory ${outdir})

  set_property(GLOBAL APPEND PROPERTY extra_post_build_commands COMMAND
               ${CMAKE_COMMAND} -E copy ${tool_settings} ${outdir}/tool_settings.json)

  set_property(GLOBAL APPEND PROPERTY extra_post_build_commands COMMAND
               simplelink-wifi-toolbox flash-images-builder build programming_image
               --flash_discovery_config_otfde ${otfde}
               --flash_discovery_config_ext_mem ${ext_mem}
               --flash_discovery_config_xspi ${xspi}
               --fuses_programming_instructions ${fuse_config}
               --dir_out_path ${outdir})

  set_property(GLOBAL APPEND PROPERTY extra_post_build_commands COMMAND
               simplelink-wifi-toolbox flash-images-builder sign programming_image
               --unsign_image ${outdir}/programming_instructions_image.unsign.bin
               --private_key ${privkey} --public_key ${pubkey}
               --dir_out_path ${outdir})

  set_property(GLOBAL APPEND PROPERTY extra_post_build_commands COMMAND
               simplelink-wifi-toolbox flash-images-builder build action_request
               --type programming
               --params_json ${action_params}
               --dir_out_path ${outdir})

  set_property(GLOBAL APPEND PROPERTY extra_post_build_commands COMMAND
               simplelink-wifi-toolbox flash-images-builder sign action_request
               --unsign_request ${outdir}/programming_action_request.unsign.bin
               --private_key ${privkey} --public_key ${pubkey}
               --dir_out_path ${outdir})

  set_property(GLOBAL APPEND PROPERTY extra_post_build_commands COMMAND
               simplelink-wifi-toolbox flash-images-builder build action_request
               --type debug
               --params_json ${action_params}
               --dir_out_path ${outdir})

  set_property(GLOBAL APPEND PROPERTY extra_post_build_commands COMMAND
               simplelink-wifi-toolbox flash-images-builder sign action_request
               --unsign_request ${outdir}/debug_action_request.unsign.bin
               --private_key ${privkey} --public_key ${pubkey}
               --dir_out_path ${outdir})

  set(conf_default_dir "${SOC_DIR}/conf")

  string(CONFIGURE "${CONFIG_CC35XXE_CONF_INI_FILE}" conf_ini)
  if("${conf_ini}" STREQUAL "")
    set(conf_ini "${conf_default_dir}/cc35xx-conf.ini")
  endif()

  set_property(GLOBAL APPEND PROPERTY extra_post_build_commands COMMAND
               simplelink-wifi-toolbox ini-composer generate bin_from_files
               --device_family CC35X1E
               --dict "${conf_default_dir}/dictionary_cc35xx.txt"
               --default_conf "${conf_default_dir}/default_cc35xx.conf"
               --header "${conf_default_dir}/conf_cc35xx.h"
               --ini "${conf_ini}"
               --conf_key "${conf_default_dir}/conf.key"
               --output "${outdir}/cc35xx-conf.bin")

  set_property(GLOBAL APPEND PROPERTY extra_post_build_commands COMMAND
               simplelink-wifi-toolbox flash-images-builder build vendor_image
               --version ${CONFIG_CC35XXE_VENDOR_IMAGE_VERSION}
               --vendor_out_file ${output}.elf
               --conf_bin_file ${outdir}/cc35xx-conf.bin
               --dir_out_path ${outdir})

  set_property(GLOBAL APPEND PROPERTY extra_post_build_commands COMMAND
               simplelink-wifi-toolbox flash-images-builder sign vendor_image
               --unsign_image ${outdir}/vendor_image.unsign.bin
               --private_key ${privkey} --public_key ${pubkey}
               --dir_out_path ${outdir})

  set_property(GLOBAL APPEND PROPERTY extra_post_build_byproducts
               "${outdir}/programming_instructions_image.sign.bin"
               "${outdir}/programming_action_request.sign.bin"
               "${outdir}/debug_action_request.sign.bin"
               "${outdir}/vendor_image.sign.bin"
               "${outdir}/tool_settings.json")
endfunction()

zephyr_simplelink_tasks()
