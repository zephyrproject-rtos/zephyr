#
# Copyright 2026 NXP
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BOARD_NXP_SPSDK_IMAGE OR (DEFINED ENV{USE_NXP_SPSDK_IMAGE}
  AND "$ENV{USE_NXP_SPSDK_IMAGE}" STREQUAL "y"))
  find_program(7Z_EXECUTABLE 7z REQUIRED)
  set(FIRMWARE_RELEASE "imx91evk-boot-firmware-6.12.34-2.1.0")
  # Parse SPSDK version
  execute_process(
    COMMAND spsdk --version
    OUTPUT_VARIABLE SPSDK_VERSION_OUTPUT
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  string(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+" SPSDK_VERSION "${SPSDK_VERSION_OUTPUT}")
  message(STATUS "SPSDK version is ${SPSDK_VERSION}")

  file(WRITE ${CMAKE_BINARY_DIR}/zephyr/imx91_evk_mimx9131_a55_ahab_primary.yaml
    "
    family: mimx9131
    revision: a0
    target_memory: serial_downloader
    output:                   ${CMAKE_BINARY_DIR}/zephyr/primary_ahab.bin
    containers:
      - binary_container:
          path:               ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/mx91a0-ahab-container.img
      - container:
          srk_set: none
          images:
            - lpddr_imem_1d:     ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/lpddr4_imem_1d_v202201.bin
              lpddr_imem_2d:     ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/lpddr4_imem_2d_v202201.bin
              lpddr_dmem_1d:     ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/lpddr4_dmem_1d_v202201.bin
              lpddr_dmem_2d:     ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/lpddr4_dmem_2d_v202201.bin
              spl_ddr:           ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/u-boot-spl.bin-imx91evk-sd
  ")
  file(WRITE ${CMAKE_BINARY_DIR}/zephyr/imx91_evk_mimx9131_a55_ahab_secondary.yaml
    "
    family: mimx9131
    revision: a0
    target_memory: serial_downloader
    output:                   ${CMAKE_BINARY_DIR}/zephyr/secondary_ahab.bin
    containers:
      - container:
          srk_set: none
          images:
            - atf:            ${CMAKE_BINARY_DIR}/zephyr/${FIRMWARE_RELEASE}/bl31-imx91-zephyr.bin
            - image_path:     ${CMAKE_BINARY_DIR}/zephyr/${CONFIG_KERNEL_BIN_NAME}.bin
              load_address:   '${CONFIG_SRAM_BASE_ADDRESS}'
              entry_point:    '${CONFIG_SRAM_BASE_ADDRESS}'
              image_type:     executable
              core_id:        cortex-a55
              is_encrypted:   false
  ")
  file(WRITE ${CMAKE_BINARY_DIR}/zephyr/imx91_evk_mimx9131_a55_ahab_flash_template.yaml
    "
    family: mimx9131
    revision: a0
    memory_type: serial_downloader
    primary_image_container_set: ${CMAKE_BINARY_DIR}/zephyr/imx91_evk_mimx9131_a55_ahab_primary.yaml
    secondary_image_container_set: ${CMAKE_BINARY_DIR}/zephyr/imx91_evk_mimx9131_a55_ahab_secondary.yaml
 ")
  set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
    COMMAND ${7Z_EXECUTABLE} x ${ZEPHYR_HAL_NXP_MODULE_DIR}/zephyr/blobs/imx-boot-firmware/${FIRMWARE_RELEASE}.bin -o./imx-boot-firmware -aos -bso0 -bse1
    COMMAND ${7Z_EXECUTABLE} x imx-boot-firmware/${FIRMWARE_RELEASE} -aos -bso0 -bse1
  )
  if(SPSDK_VERSION VERSION_GREATER_EQUAL "3.0.0")
    set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
      COMMAND nxpimage bootable-image export -c ${CMAKE_BINARY_DIR}/zephyr/imx91_evk_mimx9131_a55_ahab_flash_template.yaml -o flash.bin
    )
  else()
    set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
      COMMAND nxpimage bootable-image merge -c ${CMAKE_BINARY_DIR}/zephyr/imx91_evk_mimx9131_a55_ahab_flash_template.yaml -o flash.bin
    )
  endif()

  zephyr_blobs_verify(FILES
    ${ZEPHYR_HAL_NXP_MODULE_DIR}/zephyr/blobs/imx-boot-firmware/${FIRMWARE_RELEASE}.bin
    REQUIRED)
endif()
