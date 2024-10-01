# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

# You can name the project however you like. Having a unique name is encouraged.
project(sample_test)

# This contains a variety of helper functions that abstract away common tasks,
# like scanning, setting up a connection, querying the peer for a given
# characteristic, etc..
add_subdirectory(${ZEPHYR_BASE}/tests/bluetooth/common/testlib testlib)
target_link_libraries(app PRIVATE testlib)

# This contains babblesim-specific helpers, e.g. device synchronization.
add_subdirectory(${ZEPHYR_BASE}/tests/bsim/babblekit babblekit)
target_link_libraries(app PRIVATE babblekit)

zephyr_include_directories(
  ${BSIM_COMPONENTS_PATH}/libUtilv1/src/
  ${BSIM_COMPONENTS_PATH}/libPhyComv1/src/
)

# List every source file in the test application. Do not use GLOB.
#
# It is a good idea to have one file per test role / entry point.
#
# Try to keep test procedures readable, that is minimizing the amount of
# boilerplate. Use the `testlib` for anything that is not the object of the test
# or that you don't need tight control over. For example, setting up a
# connection between two devices.
#
# As a general rule of thumb: All functions that use `TEST_ASSERT()` are part of
# the test and should be in the same file as the test entry point. Any other
# "helper" functions can be isolated in their own file, minimizing visual
# clutter and cognitive overhead for future readers.
#
# Common data can live in a header included by multiple roles (e.g. service or
# characteristic UUIDs, test data that should be verified by both parties,
# etc..).
#
# Ideally, main.c should only set up the test framework, and map the entry
# points to the test identifiers.
target_sources(app PRIVATE
  src/main.c
  src/dut.c
  src/peer.c
)
