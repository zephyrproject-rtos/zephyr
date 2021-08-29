# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(smf)

target_sources(app PRIVATE src/main.c)

target_sources_ifdef(
	CONFIG_SMF_FLAT
	app PRIVATE src/test_lib_flat_smf.c
	)

target_sources_ifndef(
	CONFIG_SMF_FLAT
	app PRIVATE src/test_lib_hierarchical_smf.c
	)
