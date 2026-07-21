#
# Copyright (c) 2019 Microchip Technology Inc.
#
# SPDX-License-Identifier: Apache-2.0
#

if(DEFINED ENV{EVERGLADES_SPI_GEN})
  # Grab it from environment variable if defined
  set(EVERGLADES_SPI_GEN $ENV{EVERGLADES_SPI_GEN})
else()
  # Else find the tool in PATH

  if(CMAKE_HOST_APPLE)
    # CMAKE_HOST_UNIX is also true for Apple,
    # but there is no tool for Apple. So
    # we need to skip it.
    message(NOTICE "There is no SPI image generation tool for MacOS.")
  elseif(CMAKE_HOST_UNIX)
    set(EVERGLADES_SPI_GEN_FILENAME everglades_spi_gen_lin64)
  elseif(CMAKE_HOST_WIN32)
    set(EVERGLADES_SPI_GEN_FILENAME everglades_spi_gen.exe)
  endif()

  find_file(EVERGLADES_SPI_GEN_FINDFILE ${EVERGLADES_SPI_GEN_FILENAME})
  if(EVERGLADES_SPI_GEN_FINDFILE STREQUAL EVERGLADES_SPI_GEN_FINDFILE-NOTFOUND)
    message(WARNING "Microchip SPI Image Generation tool (${EVERGLADES_SPI_GEN_FILENAME}) is not available. SPI Image will not be generated.")
  else()
    set(EVERGLADES_SPI_GEN ${EVERGLADES_SPI_GEN_FINDFILE})
  endif()

  unset(EVERGLADES_SPI_GEN_FINDFILE)
  unset(EVERGLADES_SPI_GEN_FILENAME)
endif()

if(DEFINED EVERGLADES_SPI_GEN)
  if(DEFINED ENV{EVERGLADES_SPI_CFG})
    set(EVERGLADES_SPI_CFG $ENV{EVERGLADES_SPI_CFG})
  else()
    set(EVERGLADES_SPI_CFG ${BOARD_DIR}/support/spi_cfg.txt)
  endif()

  set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
    COMMAND ${EVERGLADES_SPI_GEN}
    -i ${EVERGLADES_SPI_CFG}
    -o ${PROJECT_BINARY_DIR}/${SPI_IMAGE_NAME}
  )

  unset(EVERGLADES_SPI_GEN)
  unset(EVERGLADES_SPI_CFG)
endif()
