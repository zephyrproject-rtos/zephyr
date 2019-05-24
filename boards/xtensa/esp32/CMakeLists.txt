# SPDX-License-Identifier: Apache-2.0


if(CONFIG_BOOTLOADER_ESP_IDF)
  include(ExternalProject)

  ## we use hell-world project, but I think any can be used.
  set(espidf_src_dir   ${ESP_IDF_PATH}/examples/get-started/hello_world/)
  set(espidf_prefix    ${CMAKE_BINARY_DIR}/esp-idf)
  set(espidf_build_dir ${espidf_prefix}/build)

  ExternalProject_Add(
    EspIdfBootloader
    PREFIX ${espidf_prefix}
    SOURCE_DIR ${espidf_src_dir}
    BINARY_DIR ${espidf_src_dir}
    CONFIGURE_COMMAND ""    # Skip configuring the project, e.g. with autoconf
    BUILD_COMMAND
    PATH=$ENV{PATH}:${ESPRESSIF_TOOLCHAIN_PATH}/bin
    make
    IDF_PATH=${ESP_IDF_PATH}
    BUILD_DIR_BASE=${espidf_build_dir}
    SDKCONFIG=${espidf_build_dir}/sdkconfig
    defconfig bootloader partition_table
    INSTALL_COMMAND ""      # This particular build system has no install command
    )

  add_dependencies(app EspIdfBootloader)

  board_finalize_runner_args(esp32 "--esp-flash-bootloader=${espidf_build_dir}/bootloader/bootloader.bin")

  board_finalize_runner_args(esp32 "--esp-flash-partition_table=${espidf_build_dir}/partitions_singleapp.bin")

endif()
