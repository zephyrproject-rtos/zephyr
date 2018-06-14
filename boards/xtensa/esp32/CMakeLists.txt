
if(CONFIG_BOOTLOADER_ESP_IDF)
  add_custom_target(EspIdfBootloader
	COMMAND PATH=$ENV{PATH}:${ESPRESSIF_TOOLCHAIN_PATH}/bin make IDF_PATH=${ESP_IDF_PATH} -C ${ESP_IDF_PATH}/examples/get-started/hello_world/ BUILD_DIR_BASE=${CMAKE_BINARY_DIR}/esp-idf defconfig
	COMMAND PATH=$ENV{PATH}:${ESPRESSIF_TOOLCHAIN_PATH}/bin make IDF_PATH=${ESP_IDF_PATH} -C ${ESP_IDF_PATH}/examples/get-started/hello_world/ BUILD_DIR_BASE=${CMAKE_BINARY_DIR}/esp-idf bootloader
	COMMAND PATH=$ENV{PATH}:${ESPRESSIF_TOOLCHAIN_PATH}/bin make IDF_PATH=${ESP_IDF_PATH} -C ${ESP_IDF_PATH}/examples/get-started/hello_world/ BUILD_DIR_BASE=${CMAKE_BINARY_DIR}/esp-idf partition_table
	)

  add_dependencies(app EspIdfBootloader)

endif()
