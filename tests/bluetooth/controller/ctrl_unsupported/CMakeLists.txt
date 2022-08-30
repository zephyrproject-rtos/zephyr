# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

FILE(GLOB SOURCES
  src/*.c
)

# Propagate NO_ENC to compile
if(NO_ENC)
  add_compile_definitions(NO_ENC)
endif()

# Propagate NO_PER_FEAT_EXCH to compile
if(NO_PER_FEAT_EXCH)
  add_compile_definitions(NO_PER_FEAT_EXCH)
endif()

# Propagate NO_CPR to compile
if(NO_CPR)
  add_compile_definitions(NO_CPR)
endif()

# Propagate NO_PHY to compile
if(NO_PHY)
  add_compile_definitions(NO_PHY)
endif()

project(bluetooth_ull_llcp_unsupported)
find_package(Zephyr COMPONENTS unittest HINTS $ENV{ZEPHYR_BASE})
include(${ZEPHYR_BASE}/tests/bluetooth/controller/common/defaults_cmake.txt)

# Remove ull_llcp_enc.c from compile
if(NO_ENC)
  list(REMOVE_ITEM ll_sw_sources ${ZEPHYR_BASE}/subsys/bluetooth/controller/ll_sw/ull_llcp_enc.c)
endif()

# Remove ull_llcp_phy.c from compile
if(NO_PHY)
  list(REMOVE_ITEM ll_sw_sources ${ZEPHYR_BASE}/subsys/bluetooth/controller/ll_sw/ull_llcp_phy.c)
endif()

target_sources(testbinary PRIVATE ${ll_sw_sources} ${mock_sources} ${common_sources})
