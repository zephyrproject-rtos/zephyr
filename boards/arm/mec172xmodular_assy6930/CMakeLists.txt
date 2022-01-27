#
# Copyright (c) 2021 Microchip Technology Inc.
#
# SPDX-License-Identifier: Apache-2.0
#

if(NOT DEFINED MEC172X_SPI_GEN)
  set(MEC172X_SPI_GEN $ENV{MEC172X_SPI_GEN})
endif()
find_program(MEC172X_SPI_GEN NAMES mec172x_spi_gen_linux_x86_64 mec172x_spi_gen)

if(MEC172X_SPI_GEN)
  if(DEFINED ENV{MEC172X_SPI_CFG})
    set(MEC172X_SPI_CFG $ENV{MEC172X_SPI_CFG})
  else()
    set(MEC172X_SPI_CFG ${BOARD_DIR}/support/spi_cfg.txt)
  endif()

  set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
    COMMAND ${MEC172X_SPI_GEN}
    -i ${MEC172X_SPI_CFG}
    -o ${PROJECT_BINARY_DIR}/${SPI_IMAGE_NAME}
  )

  unset(MEC172X_SPI_GEN)
  unset(MEC172X_SPI_CFG)
endif()
