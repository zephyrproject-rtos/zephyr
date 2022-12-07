#
# Copyright (c) 2021 Microchip Technology Inc.
#
# SPDX-License-Identifier: Apache-2.0
#

if(DEFINED ENV{MEC172X_SPI_GEN})
  # Grab it from environment variable if defined
  set(MEC172X_SPI_GEN $ENV{MEC172X_SPI_GEN})
else()
  # Else find the tool in PATH

  if(CMAKE_HOST_APPLE)
    # CMAKE_HOST_UNIX is also true for Apple,
    # but there is no tool for Apple. So
    # we need to skip it.
    message(NOTICE "There is no SPI image generation tool for MacOS.")
  elseif(CMAKE_HOST_UNIX)
    set(MEC172X_SPI_GEN_FILENAME mec172x_spi_gen_linux_x86_64)
  elseif(CMAKE_HOST_WIN32)
    set(MEC172X_SPI_GEN_FILENAME mec172x_spi_gen.exe)
  endif()

  find_file(MEC172X_SPI_GEN_FINDFILE ${MEC172X_SPI_GEN_FILENAME})
  if(MEC172X_SPI_GEN_FINDFILE STREQUAL MEC172X_SPI_GEN_FINDFILE-NOTFOUND)
    message(WARNING "Microchip SPI Image Generation tool (${MEC172X_SPI_GEN_FILENAME}) is not available. SPI Image will not be generated.")
  else()
    set(MEC172X_SPI_GEN ${MEC172X_SPI_GEN_FINDFILE})
  endif()

  unset(MEC172X_SPI_GEN_FINDFILE)
  unset(MEC172X_SPI_GEN_FILENAME)
endif()

if(DEFINED MEC172X_SPI_GEN)
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
