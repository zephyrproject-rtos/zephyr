# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

if(NOT CMAKE_SCRIPT_MODE_FILE)
  find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
  project(mcuboot_image_size_check_test)
  target_sources(app PRIVATE src/main.c)
  set(ZEPHYR_BASE_DIR ${ZEPHYR_BASE})
  set(PYTHON ${PYTHON_EXECUTABLE})
else()
  if(NOT DEFINED ZEPHYR_BASE_DIR)
    set(ZEPHYR_BASE_DIR ${CMAKE_CURRENT_LIST_DIR}/../../..)
  endif()
  find_program(PYTHON python3 python REQUIRED)
endif()

set(check_script ${ZEPHYR_BASE_DIR}/cmake/mcuboot_check_image_size.cmake)
set(test_dir ${CMAKE_CURRENT_BINARY_DIR})
if(CMAKE_SCRIPT_MODE_FILE)
  set(test_dir ${CMAKE_CURRENT_LIST_DIR}/_build)
  file(MAKE_DIRECTORY ${test_dir})
endif()

# ---------------------------------------------------------------------------
# Helper: create a dummy binary of exactly <size> bytes
# ---------------------------------------------------------------------------
function(create_dummy_bin path size)
  execute_process(
    COMMAND ${PYTHON} -c
      "open(r'${path}', 'wb').write(b'\\x00' * ${size})"
    RESULT_VARIABLE rc
  )
  if(NOT rc EQUAL 0)
    message(FATAL_ERROR "Failed to create dummy binary: ${path}")
  endif()
endfunction()

# ---------------------------------------------------------------------------
# Test 1 – image fits with a small margin (should PASS)
#
#   slot_size  = 393216  (0x60000, typical nRF52 slot)
#   footer     = 8192    (0x2000, two 4 KB erase blocks)
#   max image  = 385024
#   binary     = 384000  → 1024 bytes of margin
# ---------------------------------------------------------------------------
set(test1_bin ${test_dir}/test_pass.bin)
create_dummy_bin(${test1_bin} 384000)

execute_process(
  COMMAND ${CMAKE_COMMAND}
    -DSIGNED_BIN=${test1_bin}
    -DSLOT_SIZE=393216
    -DFOOTER_SIZE=8192
    -P ${check_script}
  RESULT_VARIABLE result1
  OUTPUT_VARIABLE output1
  ERROR_VARIABLE  error1
)

if(NOT result1 EQUAL 0)
  message(FATAL_ERROR
    "Test 1 FAILED – expected size check to PASS (image within limit)\n"
    "${error1}")
endif()
message(STATUS "Test 1 PASSED – image fits with 1024-byte margin")

# ---------------------------------------------------------------------------
# Test 2 – image overflows the slot (should FAIL)
#
#   slot_size  = 393216
#   footer     = 8192
#   max image  = 385024
#   binary     = 386000  → 976-byte overflow
# ---------------------------------------------------------------------------
set(test2_bin ${test_dir}/test_fail.bin)
create_dummy_bin(${test2_bin} 386000)

execute_process(
  COMMAND ${CMAKE_COMMAND}
    -DSIGNED_BIN=${test2_bin}
    -DSLOT_SIZE=393216
    -DFOOTER_SIZE=8192
    -P ${check_script}
  RESULT_VARIABLE result2
  OUTPUT_VARIABLE output2
  ERROR_VARIABLE  error2
)

if(result2 EQUAL 0)
  message(FATAL_ERROR
    "Test 2 FAILED – expected size check to FAIL (image overflows slot)\n"
    "${output2}")
endif()
message(STATUS "Test 2 PASSED – oversized image correctly rejected")

# ---------------------------------------------------------------------------
# Test 3 – exactly at the limit (should PASS)
#
#   binary = max_image = 385024
# ---------------------------------------------------------------------------
set(test3_bin ${test_dir}/test_exact.bin)
create_dummy_bin(${test3_bin} 385024)

execute_process(
  COMMAND ${CMAKE_COMMAND}
    -DSIGNED_BIN=${test3_bin}
    -DSLOT_SIZE=393216
    -DFOOTER_SIZE=8192
    -P ${check_script}
  RESULT_VARIABLE result3
  OUTPUT_VARIABLE output3
  ERROR_VARIABLE  error3
)

if(NOT result3 EQUAL 0)
  message(FATAL_ERROR
    "Test 3 FAILED – expected size check to PASS (image exactly at limit)\n"
    "${error3}")
endif()
message(STATUS "Test 3 PASSED – image exactly at limit accepted")

# ---------------------------------------------------------------------------
# Test 4 – one byte over the limit (should FAIL)
#
#   binary = 385025 = max_image + 1
# ---------------------------------------------------------------------------
set(test4_bin ${test_dir}/test_one_over.bin)
create_dummy_bin(${test4_bin} 385025)

execute_process(
  COMMAND ${CMAKE_COMMAND}
    -DSIGNED_BIN=${test4_bin}
    -DSLOT_SIZE=393216
    -DFOOTER_SIZE=8192
    -P ${check_script}
  RESULT_VARIABLE result4
  OUTPUT_VARIABLE output4
  ERROR_VARIABLE  error4
)

if(result4 EQUAL 0)
  message(FATAL_ERROR
    "Test 4 FAILED – expected size check to FAIL (one byte over limit)\n"
    "${output4}")
endif()
message(STATUS "Test 4 PASSED – one-byte overflow correctly rejected")

# ---------------------------------------------------------------------------
# Test 5 – missing binary file (should FAIL)
# ---------------------------------------------------------------------------
execute_process(
  COMMAND ${CMAKE_COMMAND}
    -DSIGNED_BIN=${test_dir}/nonexistent.bin
    -DSLOT_SIZE=393216
    -DFOOTER_SIZE=8192
    -P ${check_script}
  RESULT_VARIABLE result5
  OUTPUT_VARIABLE output5
  ERROR_VARIABLE  error5
)

if(result5 EQUAL 0)
  message(FATAL_ERROR
    "Test 5 FAILED – expected size check to FAIL when binary is missing\n"
    "${output5}")
endif()
message(STATUS "Test 5 PASSED – missing binary correctly rejected")

# ---------------------------------------------------------------------------
# Test 6 – zero footer (overwrite-only / direct-xip, no overhead)
#
#   binary fills the entire slot → should pass
# ---------------------------------------------------------------------------
set(test6_bin ${test_dir}/test_no_footer.bin)
create_dummy_bin(${test6_bin} 393216)

execute_process(
  COMMAND ${CMAKE_COMMAND}
    -DSIGNED_BIN=${test6_bin}
    -DSLOT_SIZE=393216
    -DFOOTER_SIZE=0
    -P ${check_script}
  RESULT_VARIABLE result6
  OUTPUT_VARIABLE output6
  ERROR_VARIABLE  error6
)

if(NOT result6 EQUAL 0)
  message(FATAL_ERROR
    "Test 6 FAILED – expected size check to PASS with zero footer\n"
    "${error6}")
endif()
message(STATUS "Test 6 PASSED – zero footer allows full slot usage")
