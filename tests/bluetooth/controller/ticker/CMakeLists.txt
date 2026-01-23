# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr COMPONENTS unittest REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(bluetooth_ticker)

# Include ticker source file
target_sources(testbinary
  PRIVATE
    ${ZEPHYR_BASE}/subsys/bluetooth/controller/ticker/ticker.c
    src/main.c
)

# Include ticker header directories
# Note: Mock HAL headers in src/hal/ take precedence over real HAL headers
target_include_directories(testbinary
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src  # For mock HAL headers
    ${ZEPHYR_BASE}/subsys/bluetooth/controller/ticker
    ${ZEPHYR_BASE}/subsys/bluetooth/controller/include
    ${ZEPHYR_BASE}/subsys/bluetooth/controller
)

# Add mocks for HAL dependencies
target_sources(testbinary
  PRIVATE
    src/hal_mock.c
)
