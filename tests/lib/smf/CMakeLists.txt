# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(smf)

target_sources(app PRIVATE src/main.c)

if(CONFIG_SMF_ANCESTOR_SUPPORT)
  target_sources(app PRIVATE src/test_lib_hierarchical_smf.c
    src/test_lib_hierarchical_5_ancestor_smf.c)
else()
  target_sources(app PRIVATE src/test_lib_flat_smf.c)
endif()
