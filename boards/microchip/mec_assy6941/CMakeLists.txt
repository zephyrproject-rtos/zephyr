#
# Copyright (c) 2025 Microchip Technology Inc.
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_SOC_SERIES_MEC174X)
  set(PART_PREFIX mec174x)
elseif(CONFIG_SOC_SERIES_MEC175X)
  set(PART_PREFIX mec175x)
else()
  message(NOTICE "Unknown SoC series! No SPI image will be generated")
endif()

if(DEFINED ENV{MEC5_SPI_GEN})
  # Grab it from environment variable if defined
  set(MEC5_SPI_GEN $ENV{MEC5_SPI_GEN})
else()
  # Else find the tool in PATH
  if(CMAKE_HOST_APPLE)
    # CMAKE_HOST_UNIX is also true for Apple,
    # but there is no tool for Apple. So
    # we need to skip it.
    message(NOTICE "There is no SPI image generation tool for MacOS.")
  elseif(CMAKE_HOST_UNIX)
    set(MEC5_SPI_GEN_FILENAME ${PART_PREFIX}_spi_gen_linux_x86_64)
  elseif(CMAKE_HOST_WIN32)
    set(MEC5_SPI_GEN_FILENAME ${PART_PREFIX}_spi_gen.exe)
  endif()

  find_file(MEC5_SPI_GEN_FINDFILE ${MEC5_SPI_GEN_FILENAME})
  if(MEC5_SPI_GEN_FINDFILE STREQUAL MEC5_SPI_GEN_FINDFILE-NOTFOUND)
    message(WARNING "Microchip SPI Image Generation tool (${MEC5_SPI_GEN_FILENAME}) is not available. SPI Image will not be generated.")
  else()
    set(MEC5_SPI_GEN ${MEC5_SPI_GEN_FINDFILE})
  endif()

  unset(MEC5_SPI_GEN_FINDFILE)
  unset(MEC5_SPI_GEN_FILENAME)
endif()

if(DEFINED MEC5_SPI_GEN)
  if(DEFINED ENV{MEC5_SPI_CFG})
    set(MEC5_SPI_CFG $ENV{MEC5_SPI_CFG})
  else()
    set(MEC5_SPI_CFG ${BOARD_DIR}/support/${PART_PREFIX}_spi_cfg.txt)
  endif()

  set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
    COMMAND ${MEC5_SPI_GEN}
    -i ${MEC5_SPI_CFG}
    -o ${PROJECT_BINARY_DIR}/${SPI_IMAGE_NAME}
  )

  unset(MEC5_SPI_GEN)
  unset(MEC5_SPI_CFG)
endif()

unset(PART_PREFIX)
