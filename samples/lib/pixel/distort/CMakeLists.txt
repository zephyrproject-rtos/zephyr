# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(pixel_distort)

# Copy the input file to the build directory
add_custom_command(PRE_BUILD
  DEPENDS ${CMAKE_CURRENT_LIST_DIR}/data/checkerboard.jpg
  OUTPUT checkerboard.jpg
  COMMAND cp ${CMAKE_CURRENT_LIST_DIR}/data/checkerboard.jpg checkerboard.jpg
)

# Generate a binary 8-bit grayscale image from a JPEG input.
add_custom_command(PRE_BUILD
  OUTPUT checkerboard.raw
  DEPENDS checkerboard.jpg
  COMMAND ffmpeg -y -i checkerboard.jpg -f rawvideo -pix_fmt gray checkerboard.raw
)

# Generate a C source from this raw 8-bit grayscale image
add_custom_command(PRE_BUILD
  OUTPUT checkerboard.c
  DEPENDS checkerboard.h
  DEPENDS checkerboard.raw
  COMMAND xxd -i -n distorted_raw8 checkerboard.raw | sed "1s/^/const /" >checkerboard.c
)

# Generate calibration ${DISTORT_DATA_DIR} out of this checkerboard pattern (see the README file)
add_custom_command(PRE_BUILD
  OUTPUT checkerboard.matrix.txt
  OUTPUT checkerboard.dist.txt
  OUTPUT checkerboard.annotated.jpg
  DEPENDS checkerboard.jpg
  DEPENDS ${CMAKE_CURRENT_LIST_DIR}/checkerboard_to_calibration.py
  COMMAND python ${CMAKE_CURRENT_LIST_DIR}/checkerboard_to_calibration.py checkerboard.jpg
)

# Generate a C array that describes grid coordinates of the deformation. (see the README file)
add_custom_command(PRE_BUILD
  OUTPUT checkerboard.plot.png
  OUTPUT checkerboard.h
  DEPENDS checkerboard.matrix.txt
  DEPENDS checkerboard.dist.txt
  DEPENDS checkerboard.jpg
  DEPENDS ${CMAKE_CURRENT_LIST_DIR}/calibration_to_grid.py
  COMMAND python ${CMAKE_CURRENT_LIST_DIR}/calibration_to_grid.py checkerboard.jpg >checkerboard.h
)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

target_sources(app PRIVATE checkerboard.c)
target_sources(app PRIVATE src/main.c)
