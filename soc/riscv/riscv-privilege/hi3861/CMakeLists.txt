# SPDX-License-Identifier: Apache-2.0

zephyr_sources(
  soc.c
)

include(ExternalProject)

set(hi3861_build_dir ${CMAKE_BINARY_DIR}/hi3861)

ExternalProject_Add(
  Hi3861LoaderBoot
  PREFIX ${hi3861_build_dir}
  SOURCE_DIR ${ZEPHYR_HAL_HISILICON_MODULE_DIR}/hi3861/loaderboot
  BINARY_DIR ${hi3861_build_dir}/loaderboot
  CONFIGURE_COMMAND
    ${CMAKE_COMMAND}
    -G${CMAKE_GENERATOR}
    -S${ZEPHYR_HAL_HISILICON_MODULE_DIR}/hi3861/loaderboot
    -B${hi3861_build_dir}/loaderboot
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    -DCMAKE_TOOLCHAIN_FILE=${CMAKE_CURRENT_SOURCE_DIR}/bootable.cmake
    -DCROSS_COMPILE=${CROSS_COMPILE}
    -DPYTHON_EXECUTABLE=${PYTHON_EXECUTABLE}
  BUILD_COMMAND
    ${CMAKE_COMMAND}
    --build .
  INSTALL_COMMAND ""
)

ExternalProject_Add(
  Hi3861FlashBoot
  PREFIX ${hi3861_build_dir}
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/flashboot
  BINARY_DIR ${hi3861_build_dir}/flashboot
  CONFIGURE_COMMAND
    ${CMAKE_COMMAND}
    -G${CMAKE_GENERATOR}
    -S${CMAKE_CURRENT_SOURCE_DIR}/flashboot
    -B${hi3861_build_dir}/flashboot
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    -DCMAKE_TOOLCHAIN_FILE=${CMAKE_CURRENT_SOURCE_DIR}/bootable.cmake
    -DCROSS_COMPILE=${CROSS_COMPILE}
    -DPYTHON_EXECUTABLE=${PYTHON_EXECUTABLE}
    -DCONFIG_FLASH_BASE_ADDRESS=${CONFIG_FLASH_BASE_ADDRESS}
    -DCONFIG_FLASH_LOAD_OFFSET=${CONFIG_FLASH_LOAD_OFFSET}
  BUILD_COMMAND
    ${CMAKE_COMMAND}
    --build .
  INSTALL_COMMAND ""
)

add_dependencies(app Hi3861LoaderBoot Hi3861FlashBoot)

board_finalize_runner_args(hiburn "--loaderboot-bin=${hi3861_build_dir}/loaderboot/loaderboot.bin")
board_finalize_runner_args(hiburn "--flashboot-bin=${hi3861_build_dir}/flashboot/flashboot.bin")
board_finalize_runner_args(hiburn "--app-address=${CONFIG_FLASH_LOAD_OFFSET}")
