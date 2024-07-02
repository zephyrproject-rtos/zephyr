# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(iface)

target_include_directories(app PRIVATE ${ZEPHYR_BASE}/subsys/net/ip)
FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

if(CONFIG_NET_CONFIG_USE_YAML)
  network_generate_config_file_for_target(app ${CONFIG_NET_CONFIG_YAML_FILE})
endif()
