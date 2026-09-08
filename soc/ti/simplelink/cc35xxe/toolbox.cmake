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
