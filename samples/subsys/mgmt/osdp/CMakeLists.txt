#
# Copyright (c) 2020 Siddharth Chandrasekaran <siddharth@embedjournal.com>
#
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.13.1)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)

project(osdp_sample)

target_sources_ifdef(CONFIG_OSDP_MODE_PD app PRIVATE src/pd_main.c)
target_sources_ifdef(CONFIG_OSDP_MODE_CP app PRIVATE src/cp_main.c)
