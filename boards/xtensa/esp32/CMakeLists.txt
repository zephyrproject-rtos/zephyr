# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BOOTLOADER_ESP_IDF)
  include(ExternalProject)

  ## we use hell-world project, but I think any can be used.
  set(espidf_components_dir   ${ESP_IDF_PATH}/components)
  set(espidf_prefix    ${CMAKE_BINARY_DIR}/esp-idf)
  set(espidf_build_dir ${espidf_prefix}/build)

  ExternalProject_Add(
    EspIdfBootloader
    PREFIX ${espidf_prefix}
    SOURCE_DIR ${espidf_components_dir}/bootloader/subproject
    BINARY_DIR ${espidf_build_dir}/bootloader
    CONFIGURE_COMMAND
    ${CMAKE_COMMAND} -G${CMAKE_GENERATOR}
    -S ${espidf_components_dir}/bootloader/subproject
    -B ${espidf_build_dir}/bootloader -DSDKCONFIG=${espidf_build_dir}/sdkconfig
    -DIDF_PATH=${ESP_IDF_PATH} -DIDF_TARGET=${CONFIG_BOARD}
    -DPYTHON_DEPS_CHECKED=1
    BUILD_COMMAND
    ${CMAKE_COMMAND} --build .
    INSTALL_COMMAND ""      # This particular build system has no install command
    )

  ExternalProject_Add(
    EspPartitionTable
    SOURCE_DIR ${espidf_components_dir}/partition_table
    BINARY_DIR ${espidf_build_dir}
    CONFIGURE_COMMAND ""
    BUILD_COMMAND
    python ${ESP_IDF_PATH}/components/partition_table/gen_esp32part.py -q
    --offset 0x1000 --flash-size 4MB ${ESP_IDF_PATH}/components/partition_table/partitions_singleapp.csv ${espidf_build_dir}/partitions_singleapp.bin
    INSTALL_COMMAND ""
    )

  add_dependencies(app EspIdfBootloader EspPartitionTable)

  board_finalize_runner_args(esp32 "--esp-flash-bootloader=${espidf_build_dir}/bootloader/bootloader.bin")

  board_finalize_runner_args(esp32 "--esp-flash-partition_table=${espidf_build_dir}/partitions_singleapp.bin")

endif()
